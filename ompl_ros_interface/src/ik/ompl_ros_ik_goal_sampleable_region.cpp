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
*   * Neither the name of Willow Garage, Inc. nor the names of its
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
*********************************************************************/

/** \author Sachin Chitta, Ioan Sucan */

#include <ompl_ros_interface/ik/ompl_ros_ik_goal_sampleable_region.h>

namespace ompl_ros_interface
{
bool OmplRosIKSampleableRegion::initialize(const ompl::base::StateManifoldPtr &state_manifold,
                                           const std::string &kinematics_solver_name,
                                           const std::string &group_name,
                                           const std::string &end_effector_name,
                                           const planning_environment::PlanningMonitor *planning_monitor)
{
  planning_monitor_ = planning_monitor;
  state_manifold_ = state_manifold;
  group_name_ = group_name;
  end_effector_name_ = end_effector_name;
  if(!kinematics_loader_.isClassAvailable(kinematics_solver_name))
  {
    ROS_ERROR("pluginlib does not have the class %s",kinematics_solver_name.c_str());
    return false;
  }
  
  try
  {
    kinematics_solver_ = kinematics_loader_.createClassInstance(kinematics_solver_name);
    return false;
  }
  catch(pluginlib::PluginlibException& ex)    //handle the class failing to load
  {
    ROS_ERROR("The plugin failed to load. Error: %s", ex.what());
    return false;
  }
  if(!kinematics_solver_->initialize(group_name))
  {
    ROS_ERROR("Could not initialize kinematics solver for group %s",group_name.c_str());
    return false;
  }
  //  scoped_state_.reset(new ompl::base::ScopedState<ompl::base::CompoundStateManifold>(state_manifold_));
  seed_state_.joint_state.name = kinematics_solver_->getJointNames();
  seed_state_.joint_state.position.resize(kinematics_solver_->getJointNames().size());
  solution_state_.joint_state.name = kinematics_solver_->getJointNames();
  solution_state_.joint_state.position.resize(kinematics_solver_->getJointNames().size());
  if(!ompl_ros_interface::getRobotStateToOmplStateMapping(seed_state_,scoped_state_,robot_state_to_ompl_state_mapping_))
    return false;
  if(!ompl_ros_interface::getOmplStateToRobotStateMapping(scoped_state_,seed_state_,ompl_state_to_robot_state_mapping_))
    return false;
}  

bool OmplRosIKSampleableRegion::configureOnRequest(const motion_planning_msgs::GetMotionPlan::Request &request,
                                                   motion_planning_msgs::GetMotionPlan::Response &response,
                                                   const unsigned int &max_sample_count)
{
  max_sample_count_ = max_sample_count;
  ik_poses_.clear();
  motion_planning_msgs::Constraints goal_constraints = request.motion_plan_request.goal_constraints;

  if(!planning_monitor_->transformConstraintsToFrame(goal_constraints,
                                                     kinematics_solver_->getBaseFrame(),
                                                     response.error_code))
    return false;
  
  if(!motion_planning_msgs::constraintsToPoseStampedVector(goal_constraints, ik_poses_))
  {
    ROS_ERROR("Could not get poses from constraints");
    return false;
  }       
  if(ik_poses_.empty())
  {
    ROS_INFO("Could not setup goals for inverse kinematics sampling");
    return false;
  }
  for(unsigned int i=0; i < ik_poses_.size(); i++)
  {
    if(ik_poses_[i].header.frame_id != kinematics_solver_->getBaseFrame())
    {
      ROS_ERROR("Goals for inverse kinematic sampling in %s frame are not in kinematics frame: %s",
                ik_poses_[i].header.frame_id.c_str(),
                kinematics_solver_->getBaseFrame().c_str());
      return false;
    }
  }
  return true;
}

unsigned int OmplRosIKSampleableRegion::maxSampleCount(void) const 
{
  return max_sample_count_;
}

void OmplRosIKSampleableRegion::sampleGoal(ompl::base::State *state) const

{
  std::vector<motion_planning_msgs::RobotState> sampled_states_vector;
  sampleGoals(1,sampled_states_vector);
  if(!sampled_states_vector.empty())
  {
    ompl_ros_interface::robotStateToOmplState(sampled_states_vector.front(),
                                              robot_state_to_ompl_state_mapping_,
                                              state);    
  }
}

void OmplRosIKSampleableRegion::sampleGoals(const unsigned int &number_goals,
                                            std::vector<motion_planning_msgs::RobotState> &sampled_states_vector) const
{
  motion_planning_msgs::RobotState seed_state,solution_state;
  seed_state = seed_state_;
  solution_state = solution_state_; 
  ompl::base::ScopedState<ompl::base::CompoundStateManifold> scoped_state(state_manifold_);
  unsigned int ik_poses_counter = 0;
  for(unsigned int i=0; i < number_goals; i++)
  {    
    //sample a state at random
    scoped_state.random();
    ompl_ros_interface::omplStateToRobotState(scoped_state,
                                              ompl_state_to_robot_state_mapping_,
                                              seed_state);    
    if(kinematics_solver_->getPositionIK(ik_poses_[ik_poses_counter].pose,
                                         seed_state.joint_state.position,
                                         solution_state.joint_state.position))
    {
      sampled_states_vector.push_back(solution_state);
      ik_poses_counter++;
      ik_poses_counter = ik_poses_counter%ik_poses_.size();
    }
  }
}
}