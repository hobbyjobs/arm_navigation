/*********************************************************************
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2008, Willow Garage, Inc.
 *  All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 * 
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 * 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *
 *********************************************************************/

/* \author: Ioan Sucan, Sachin Chitta */

#include <ros/ros.h>
#include <actionlib/client/simple_action_client.h>
#include <move_arm_msgs/MoveArmAction.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <boost/thread.hpp>
#include <ros/ros.h>
#include <gtest/gtest.h>

#include <geometric_shapes_msgs/Shape.h>

typedef actionlib::SimpleActionClient<move_arm_msgs::MoveArmAction> MoveArmClient;

void spinThread()
{
  ros::spin();
}

TEST(MoveArm, goToPoseGoal)
{
  ros::NodeHandle nh;
  ros::NodeHandle private_handle("~");
  actionlib::SimpleActionClient<move_arm_msgs::MoveArmAction> move_arm(nh, "move_right_arm");
  boost::thread spin_thread(&spinThread);

  move_arm.waitForServer();
  ROS_INFO("Connected to server");
  move_arm_msgs::MoveArmGoal goalA;

  goalA.group_name = "right_arm";
  goalA.num_planning_attempts = 1;
  private_handle.param<std::string>("planner_id",goalA.planner_id,std::string("chomp_planner_longrange"));
  private_handle.param<std::string>("planner_service_name",goalA.planner_service_name,std::string("/chomp_planner_longrange/plan_path"));

  goalA.allowed_planning_time = 5.0;
    
  goalA.goal_constraints.set_position_constraints_size(1);
  goalA.goal_constraints.position_constraints[0].header.stamp = ros::Time::now();
  goalA.goal_constraints.position_constraints[0].header.frame_id = "torso_lift_link";
    
  goalA.goal_constraints.position_constraints[0].link_name = "r_wrist_roll_link";
  goalA.goal_constraints.position_constraints[0].position.x = 0.75;
  goalA.goal_constraints.position_constraints[0].position.y = -0.188;
  goalA.goal_constraints.position_constraints[0].position.z = 0;
    
  goalA.goal_constraints.position_constraints[0].constraint_region_shape.type = geometric_shapes_msgs::Shape::BOX;
  goalA.goal_constraints.position_constraints[0].constraint_region_shape.dimensions.push_back(0.02);
  goalA.goal_constraints.position_constraints[0].constraint_region_shape.dimensions.push_back(0.02);
  goalA.goal_constraints.position_constraints[0].constraint_region_shape.dimensions.push_back(0.02);

  goalA.goal_constraints.position_constraints[0].constraint_region_orientation.w = 1.0;
  goalA.goal_constraints.position_constraints[0].weight = 1.0;

  goalA.goal_constraints.set_orientation_constraints_size(1);
  goalA.goal_constraints.orientation_constraints[0].header.stamp = ros::Time::now();
  goalA.goal_constraints.orientation_constraints[0].header.frame_id = "torso_lift_link";    
  goalA.goal_constraints.orientation_constraints[0].link_name = "r_wrist_roll_link";
  goalA.goal_constraints.orientation_constraints[0].orientation.x = 0.0;
  goalA.goal_constraints.orientation_constraints[0].orientation.y = 0.0;
  goalA.goal_constraints.orientation_constraints[0].orientation.z = 0.0;
  goalA.goal_constraints.orientation_constraints[0].orientation.w = 1.0;
    
  goalA.goal_constraints.orientation_constraints[0].absolute_roll_tolerance = 0.04;
  goalA.goal_constraints.orientation_constraints[0].absolute_pitch_tolerance = 0.04;
  goalA.goal_constraints.orientation_constraints[0].absolute_yaw_tolerance = 0.04;

  goalA.goal_constraints.orientation_constraints[0].weight = 1.0;

  /*   
  // These are in here just for reference
  std::vector<std::string> names(7);
  names[0] = "r_shoulder_pan_joint";
  names[1] = "r_shoulder_lift_joint";
  names[2] = "r_upper_arm_roll_joint";
  names[3] = "r_elbow_flex_joint";
  names[4] = "r_forearm_roll_joint";
  names[5] = "r_wrist_flex_joint";
  names[6] = "r_wrist_roll_joint";
  */

  int num_test_attempts = 0;
  int max_attempts = 5;
  bool success = false;
  while (nh.ok())
  {
    bool finished_within_time = false;
    move_arm.sendGoal(goalA);
    finished_within_time = move_arm.waitForResult(ros::Duration(100.0));
    actionlib::SimpleClientGoalState state = move_arm.getState();
    success = (state == actionlib::SimpleClientGoalState::SUCCEEDED);
    if ((!finished_within_time || !success) && num_test_attempts < max_attempts)
    {
      move_arm.cancelGoal();
      ROS_INFO("Timed out achieving goal A, trying again. Trying again, attempt: %d",num_test_attempts);
      num_test_attempts++;
    }
    else
    {
      if(!success)
      {
        ROS_INFO("Action finished: %s",state.toString().c_str());
        move_arm.cancelGoal();
      }
      ROS_INFO("Action finished: %s",state.toString().c_str());
      break;
    }
  }
  EXPECT_TRUE(success);
  ros::shutdown();
  spin_thread.join();
}

int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  ros::init (argc, argv, "move_arm_regression_test");
  return RUN_ALL_TESTS();
}
