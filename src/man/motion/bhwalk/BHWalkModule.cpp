#include "BHWalkModule.h"

#include <math/Geometry.h>

// BH
#include "WalkingEngine.h"
#include "WalkingEngineKicks.h"
#include "Tools/Module/Module.h"
#include "Representations/Configuration/MassCalibration.h"

#include <memory/FrameInfoBlock.h>
#include <memory/JointBlock.h>
#include <memory/JointCommandBlock.h>
#include <memory/KickRequestBlock.h>
#include <memory/OdometryBlock.h>
#include <memory/RobotInfoBlock.h>
#include <memory/SensorBlock.h>
#include <memory/WalkInfoBlock.h>
#include <memory/WalkParamBlock.h>
#include <memory/WalkRequestBlock.h>

#include "Modules/Sensing/JointFilter.h"
#include "Modules/Sensing/RobotModelProvider.h"
#include "Modules/Sensing/InertiaSensorCalibrator.h"
#include "Modules/Sensing/InertiaSensorFilter.h"
#include "Modules/Sensing/SensorFilter.h"
#include "Modules/Sensing/FallDownStateDetector.h"
#include "Modules/Sensing/TorsoMatrixProvider.h"
#include "Modules/Infrastructure/NaoProvider.h"


float BHWalkModule::sumFsrs(FootSensorRegion r) 
{
	float sum = 0;
	for(int i = 0; i < 2; ++i) {
		sum += sensors_->values_[foot_contacts[(int) r][i]];
	}
	return sum;
}

BHWalkModule::BHWalkModule():
	slow_stand_start(-1),
	slow_stand_end(-1),
	walk_requested_start_time(-1),
	prev_kick_active_(false),
	arms_close_to_targets_(false),
	arm_state_(-1),
	arm_state_change_(-1),
	last_walk_or_stand_(-1),
	step_into_kick_state_(NONE),
	time_step_into_kick_finished_(0)
{
	utJointToBHJoint[HeadYaw] = JointDataBH::HeadYaw;
	utJointToBHJoint[HeadPitch] = JointDataBH::HeadPitch;

	utJointToBHJoint[LShoulderPitch] = JointDataBH::LShoulderPitch;
	utJointToBHJoint[LShoulderRoll] = JointDataBH::LShoulderRoll;
	utJointToBHJoint[LElbowYaw] = JointDataBH::LElbowYaw;
	utJointToBHJoint[LElbowRoll] = JointDataBH::LElbowRoll;

	utJointToBHJoint[RShoulderPitch] = JointDataBH::RShoulderPitch;
	utJointToBHJoint[RShoulderRoll] = JointDataBH::RShoulderRoll;
	utJointToBHJoint[RElbowYaw] = JointDataBH::RElbowYaw;
	utJointToBHJoint[RElbowRoll] = JointDataBH::RElbowRoll;

	utJointToBHJoint[LHipYawPitch] = JointDataBH::LHipYawPitch;
	utJointToBHJoint[LHipRoll] = JointDataBH::LHipRoll;
	utJointToBHJoint[LHipPitch] = JointDataBH::LHipPitch;
	utJointToBHJoint[LKneePitch] = JointDataBH::LKneePitch;
	utJointToBHJoint[LAnklePitch] = JointDataBH::LAnklePitch;
	utJointToBHJoint[LAnkleRoll] = JointDataBH::LAnkleRoll;

	utJointToBHJoint[RHipYawPitch] = JointDataBH::RHipYawPitch;
	utJointToBHJoint[RHipRoll] = JointDataBH::RHipRoll;
	utJointToBHJoint[RHipPitch] = JointDataBH::RHipPitch;
	utJointToBHJoint[RKneePitch] = JointDataBH::RKneePitch;
	utJointToBHJoint[RAnklePitch] = JointDataBH::RAnklePitch;
	utJointToBHJoint[RAnkleRoll] = JointDataBH::RAnkleRoll;
}

BHWalkModule::~BHWalkModule() {
}

void BHWalkModule::specifyMemoryDependency() {
	requiresMemoryBlock("frame_info");
	requiresMemoryBlock("processed_joint_angles");
	requiresMemoryBlock("raw_joint_angles");
	requiresMemoryBlock("processed_joint_commands");
	requiresMemoryBlock("kick_request");
	requiresMemoryBlock("odometry");
	requiresMemoryBlock("robot_info");
	requiresMemoryBlock("raw_sensors");
	requiresMemoryBlock("walk_info");
	requiresMemoryBlock("walk_param");
	requiresMemoryBlock("walk_request");
}

void BHWalkModule::specifyMemoryBlocks() {
	getOrAddMemoryBlock(frame_info_,"frame_info");
	getOrAddMemoryBlock(joints_,"processed_joint_angles");
	getOrAddMemoryBlock(raw_joints_,"raw_joint_angles");
	getOrAddMemoryBlock(commands_,"processed_joint_commands");
	getOrAddMemoryBlock(kick_request_,"kick_request");
	getOrAddMemoryBlock(odometry_,"odometry");
	getOrAddMemoryBlock(robot_info_,"robot_info");
	getOrAddMemoryBlock(sensors_,"raw_sensors");
	getOrAddMemoryBlock(walk_info_,"walk_info");
	getOrAddMemoryBlock(walk_params_,"walk_param");
	getOrAddMemoryBlock(walk_request_,"walk_request");
}

void BHWalkModule::initSpecificModule() {
	// Setup Walk Engine Configuation Parameters
	std::string config_path = memory_->data_path_;
	config_path += "config/";
	ModuleBase::config_path = config_path;

	// Setup Static Variables
	Blackboard::theInstance = new Blackboard();
	MotionSelector::theInstance = new MotionSelector();
	InertiaSensorCalibrator::theInstance = new InertiaSensorCalibrator();
	JointFilter::theInstance = new JointFilter();
	RobotModelProvider::theInstance = new RobotModelProvider();
	InertiaSensorFilter::theInstance = new InertiaSensorFilter();
	SensorFilter::theInstance = new SensorFilter();
	FallDownStateDetector::theInstance = new FallDownStateDetector();
	TorsoMatrixProvider::theInstance = new TorsoMatrixProvider();
	NaoProvider::theInstance = new NaoProvider();

	fsr_stability = new RingBufferWithSumBH<float, 100>();
	fsr_mean = new RingBufferWithSumBH<float, 30>();
	fsr_stddev = new RingBufferWithSumBH<float, 30>();

	walk_engine_ = new WalkingEngine();
	walk_engine_->theFrameInfoBH.cycleTime = 0.01f;
	walk_engine_->currentMotionType = WalkingEngine::stand;
	walk_engine_->theMotionRequestBH.motion = MotionRequestBH::specialAction;
	for (int i = 0; i < MotionRequestBH::numOfMotions; i++)
		walk_engine_->theMotionSelectionBH.ratios[i] = 0;
	walk_engine_->theMotionSelectionBH.ratios[MotionRequestBH::specialAction] = 1.0;
}

void BHWalkModule::processWalkParams() {
	if (!walk_params_->send_params_)
		return;
	walk_params_->send_params_ = false;
	stable_walk_threshold_prior_mean = walk_params_->stable_walk_threshold_prior_mean;
	stable_walk_threshold_prior_stddev = walk_params_->stable_walk_threshold_prior_stddev;
	stable_walk_threshold_mean = walk_params_->stable_walk_threshold_prior_mean;
	stable_walk_threshold_stddev = walk_params_->stable_walk_threshold_prior_stddev;
	BHWalkParameters &params = walk_params_->bh_params_;
	walk_engine_->init(); // reloads parameters from files
}

void BHWalkModule::processFrame() {
	// times
	unsigned int time = 1000 * frame_info_->seconds_since_start + 1000; // + 1000 so that it's never 0, because bhuman says they don't like that
	walk_engine_->theFrameInfoBH.time = walk_engine_->theFilteredJointDataBH.timeStamp = walk_engine_->theFilteredSensorDataBH.timeStamp = time;

	processWalkParams();
	processWalkRequest();

	assert((int)JointDataBH::numOfJoints == NUM_JOINTS);

	JointDataBH& joint_data_BH = walk_engine_->theJointDataBH;
	SensorDataBH& sensors_BH = walk_engine_->theSensorDataBH;

	// inputs to walk joints
	for (int ut_ind = 0; ut_ind < NUM_JOINTS; ut_ind++) 
	{
		int bh_ind = utJointToBHJoint[ut_ind];
		joint_data_BH.angles[bh_ind] = raw_joints_->values_[ut_ind];
	}

	// other sensors
	sensors_BH.data[SensorDataBH::gyroX] = sensors_->values_[gyroX];
	sensors_BH.data[SensorDataBH::gyroY] = sensors_->values_[gyroY];
	sensors_BH.data[SensorDataBH::accX] = sensors_->values_[accelX];
	sensors_BH.data[SensorDataBH::accY] = sensors_->values_[accelY];
	sensors_BH.data[SensorDataBH::accZ] = sensors_->values_[accelZ];
	sensors_BH.data[SensorDataBH::angleX] = sensors_->values_[angleX];
	sensors_BH.data[SensorDataBH::angleY] = sensors_->values_[angleY];
	sensors_BH.data[SensorDataBH::fsrLFL] = sensors_->values_[fsrLFL];
	sensors_BH.data[SensorDataBH::fsrLFR] = sensors_->values_[fsrLFR];
	sensors_BH.data[SensorDataBH::fsrLBL] = sensors_->values_[fsrLRL];
	sensors_BH.data[SensorDataBH::fsrLBR] = sensors_->values_[fsrLRR];
	sensors_BH.data[SensorDataBH::fsrRFL] = sensors_->values_[fsrRFL];
	sensors_BH.data[SensorDataBH::fsrRFR] = sensors_->values_[fsrRFR];
	sensors_BH.data[SensorDataBH::fsrRBL] = sensors_->values_[fsrRRL];
	sensors_BH.data[SensorDataBH::fsrRBR] = sensors_->values_[fsrRRR];

	// check ground contact state
	bool contact = true;
	float fsrs[8];
	float front = 0;
	float back = 0;
	switch(walk_engine_->pendulumPlayer.phase.type)
	{
		case WalkingEngine::leftSupportPhase:
			{
				front = fsrs[0] = sumFsrs(left_front);
				back = fsrs[1] = sumFsrs(left_back);
				fsrs[2] = sumFsrs(left_left);
				fsrs[3] = sumFsrs(left_right);
				fsrs[4] = sumFsrs(right_front);
				fsrs[5] = sumFsrs(right_back);
				fsrs[6] = sumFsrs(right_left);
				fsrs[7] = sumFsrs(right_right);
			}
			break;
		case WalkingEngine::rightSupportPhase:
			{
				front = fsrs[0] = sumFsrs(right_front);
				back = fsrs[1] = sumFsrs(right_back);
				fsrs[2] = sumFsrs(right_left);
				fsrs[3] = sumFsrs(right_right);
				fsrs[4] = sumFsrs(left_front);
				fsrs[5] = sumFsrs(left_back);
				fsrs[6] = sumFsrs(left_left);
				fsrs[7] = sumFsrs(left_right);
			}
			break;
		case WalkingEngine::standPhase:
			{
				fsrs[0] = sumFsrs(right_front);
				fsrs[1] = sumFsrs(right_back);
				fsrs[2] = sumFsrs(right_left);
				fsrs[3] = sumFsrs(right_right);
				fsrs[4] = sumFsrs(left_front);
				fsrs[5] = sumFsrs(left_back);
				fsrs[6] = sumFsrs(left_left);
				fsrs[7] = sumFsrs(left_right);
			}
			break;
	}

	if (walk_request_->motion_ == WalkRequestBlock::WALK) 
	{
		// Detect if the Nao is flying in the air
		float force = 0;
		for(int i = 0; i < FootSensorRegion::none; ++i)
		{
			force += fsrs[i];
		}

		if(force < FLYING_DETECT)
		{
			//std::cout << "I'm Flying: " << force << std::endl;
			// Recalibrate FSR Stable Walk Thresholds
			fsr_stability->init();
			fsr_mean->init();
			fsr_stddev->init();
			stable_walk_threshold_prior_mean = walk_params_->stable_walk_threshold_prior_mean;
			stable_walk_threshold_prior_stddev = walk_params_->stable_walk_threshold_prior_stddev;
			stable_walk_ready = false;

			// Turn off Walk Engine while flying
			contact = false;

			// Restart Walk Engine
			walk_engine_->reset();
		}
		else if(walk_engine_->pendulumPlayer.phase.type != WalkingEngine::standPhase)
		{
			// Detect if the Nao is experiencing instability along the sagital plane
			// Activate Stand Balance Controller if both feet are touching the ground
			float sagital_instability = abs(front - back);
			fsr_stability->add(sagital_instability);
			if(fsr_stability->isFilled())
			{
				fsr_mean->add(fsr_stability->getAverage());
				fsr_stddev->add(fsr_stability->getStdDev());
				fsr_stability->init();

				if(fsr_mean->isFilled() && fsr_stddev->isFilled())
				{
					// Update Stable Walk Threshold using Normal Posterior Distribution		
					// Posterior Mean = (Prior Precision * Prior Mean + Likelihood Precision * Likelihood Mean) / (Prior + Likelihood Precision)
					// Posterior Precision = Prior + Likelihood Precision
					float likelihood_precision = 1 / pow(fsr_stddev->getAverage(), 2);
					float prior_precision = 1 / pow(stable_walk_threshold_prior_stddev, 2);
					float posterior_precision = likelihood_precision + prior_precision;

					cout << "prior mean: " << stable_walk_threshold_prior_mean << endl;
					cout << "prior precision: " << prior_precision << endl;

					// Use current stable walk threshold as the prior for the next update
					stable_walk_threshold_prior_mean = stable_walk_threshold_mean = (likelihood_precision * fsr_mean->getAverage() + prior_precision * stable_walk_threshold_prior_mean) / posterior_precision;
					stable_walk_threshold_prior_stddev = stable_walk_threshold_stddev = 1 / pow(posterior_precision, 0.5);

					cout << "likelihood mean: " << fsr_mean->getAverage() << endl;
					cout << "likelihood precision: " << likelihood_precision << endl;
					std::cout << "sw mean: " << stable_walk_threshold_mean << std::endl;
					std::cout << "sw stddev: " << stable_walk_threshold_stddev << std::endl;

					stable_walk_ready = true;
					fsr_mean->init();
					fsr_stddev->init();
				}
			}

			// The FSR Slow Walk Threshold is set using the 85 and 90 percent confidence interval for Normal Posterior Distribution
			if (stable_walk_ready && sagital_instability > (stable_walk_threshold_mean + stable_walk_threshold_stddev))
			{
				float zscore = (sagital_instability - stable_walk_threshold_mean) / stable_walk_threshold_stddev;
				int size = sizeof(stability_limit)/sizeof(stability_limit[0]);
				for(int i = 0; i < size; ++i)
				{
					if(zscore > stability_limit[i][0])
					{
						//cout << "Slowing Down: " << sagital_instability << endl;
						walk_engine_->theMotionRequestBH.walkRequest.speed.rotation *= stability_limit[i][1];
						walk_engine_->theMotionRequestBH.walkRequest.speed.translation.x *= stability_limit[i][1];
						walk_engine_->theMotionRequestBH.walkRequest.speed.translation.y *= stability_limit[i][1];
					}
				}
			}
			//std::cout << sagital_instability << std::endl;
		}
	}	

	walk_engine_->theGroundContactStateBH.contact = contact;
	walk_engine_->update(walk_engine_->theWalkingEngineOutputBH);

	MotionRequestBH::Motion& motion = walk_engine_->theMotionRequestBH.motion;
	float *motion_ratios = walk_engine_->theMotionSelectionBH.ratios;

	// slow stand
	static bool prev_slow_stand = false;
	if (doingSlowStand()) 
	{
		//std::cout << frame_info_->frame_id << " doing slow stand" << std::endl;
		doSlowStand();
		last_walk_or_stand_ = frame_info_->seconds_since_start;
		prev_slow_stand = true;
	} 
	else if (prev_slow_stand) 
	{
		std::cout << frame_info_->frame_id << " done slow stand" << std::endl;
		prev_slow_stand = false;
	}

	if ((frame_info_->seconds_since_start - last_walk_or_stand_) > 0.3) 
	{
		arm_state_ = -1;
	}

	// odometry
	odometry_->standing = (walk_engine_->currentMotionType != WalkingEngine::stepping);
	Pose2DBH &odom = walk_engine_->theWalkingEngineOutputBH.odometryOffset;
	Pose2D delta(odom.rotation,odom.translation.x,odom.translation.y);
	odometry_->displacement += delta;
	bool kick_active = walk_engine_->predictedPendulumPlayer.kickPlayer.isActive();
	if (!kick_active && prev_kick_active_) { 
		odometry_->didKick = true;
		odometry_->kickVelocity = kick_distance_ / 1.2;
		odometry_->kickHeading = kick_angle_;
	}
	prev_kick_active_ = kick_active;

	//walk_info_->stabilizer_on_threshold_ = walk_engine_->p.stabilizerOnThreshold;
	//walk_info_->stabilizer_off_threshold_ = walk_engine_->p.stabilizerOffThreshold;
	walk_info_->finished_with_target_ = walk_engine_->finishedWithTarget;
	walk_info_->instable_ = !contact;
	walk_info_->walk_is_active_ = (walk_engine_->currentMotionType == WalkingEngine::stepping);
	walk_info_->instability_ = walk_engine_->instability.getAverage();
	setPose2D(walk_info_->robot_velocity_,walk_engine_->theWalkingEngineOutputBH.speed);
	setPose2D(walk_info_->robot_relative_next_position_,walk_engine_->theWalkingEngineOutputBH.upcomingOdometryOffset);
	walk_info_->is_stance_left_ = (walk_engine_->pendulumPlayer.phase.type == WalkingEngine::leftSupportPhase);
	walk_info_->frac_of_step_completed_ = ((walk_engine_->predictedPendulumPlayer.phase.tu) / (walk_engine_->predictedPendulumPlayer.phase.td + walk_engine_->predictedPendulumPlayer.phase.tu));
	walk_info_->time_remaining_in_step_ = walk_engine_->predictedPendulumPlayer.phase.td;

	// conitinue if standing, walking, or transitioning between standing and walking
	if ((motion_ratios[MotionRequestBH::walk] < 0.01f) && (motion_ratios[MotionRequestBH::stand] < 0.01f)) 
	{
		return;
	}
	last_walk_or_stand_ = frame_info_->seconds_since_start;

	// outputs from walk
	for (int ut_ind = BODY_JOINT_OFFSET; ut_ind < NUM_JOINTS; ut_ind++) 
	{
		int bh_ind = utJointToBHJoint[ut_ind];
		commands_->angles_[ut_ind] = robot_joint_signs[ut_ind] * (walk_engine_->theWalkingEngineOutputBH.angles[bh_ind] + walk_engine_->theJointCalibrationBH.joints[bh_ind].offset) * float(walk_engine_->theJointCalibrationBH.joints[bh_ind].sign);
		commands_->stiffness_[ut_ind] = float(walk_engine_->theWalkingEngineOutputBH.jointHardness.hardness[bh_ind]) / 100.f;
		//std::cout << "angle - " << JointNames[ut_ind] << ": " << commands_->angles_[ut_ind] << std::endl;
		//std::cout << "stiffness: " << commands_->stiffness_[ut_ind] << std::endl;
	}

	commands_->stiffness_[HeadPitch] = 1.0;
	commands_->stiffness_[HeadYaw] = 1.0;
	selectivelySendStiffness();

	setArms(commands_->angles_,0.01);
	commands_->send_body_angles_ = true;
	commands_->body_angle_time_ = 10;
}

void BHWalkModule::setKickStepParams() {
	int type = walk_request_->step_kick_type_;
	bool mirrored = (type - 1) % 2 != 0;
	assert(!mirrored);
	WalkingEngineKick& kick = walk_engine_->kicks.kicks[mirrored ? (type - 2) / 2 : (type - 1) / 2];

	kick.preStepSizeXValue = new WalkingEngineKick::ConstantValue(walk_request_->pre_kick_step_.translation.x, kick);
	kick.preStepSizeYValue = new WalkingEngineKick::ConstantValue(walk_request_->pre_kick_step_.translation.y, kick);
	kick.preStepSizeRValue = new WalkingEngineKick::ConstantValue(walk_request_->pre_kick_step_.rotation, kick);
	kick.stepSizeXValue = new WalkingEngineKick::ConstantValue(walk_request_->kick_step_.translation.x, kick);
	kick.stepSizeYValue = new WalkingEngineKick::ConstantValue(walk_request_->kick_step_.translation.y, kick);
	kick.stepSizeRValue = new WalkingEngineKick::ConstantValue(walk_request_->kick_step_.rotation, kick);
	kick.refXValue = new WalkingEngineKick::ConstantValue(walk_request_->kick_step_ref_x_, kick);
}

void BHWalkModule::processWalkRequest() {
	if (walk_request_->set_kick_step_params_)
		setKickStepParams();

	if ((!walk_request_->new_command_) || (step_into_kick_state_ == PERFORMING))
		return;

	WalkRequest& walk_request_BH = walk_engine_->theMotionRequestBH.walkRequest;
	MotionRequestBH::Motion& motion = walk_engine_->theMotionRequestBH.motion;

	if (walk_request_->motion_ == WalkRequestBlock::STAND) 
	{
		motion = MotionRequestBH::stand;
		walk_request_BH = WalkRequest();
		walk_requested_start_time = -1;
		//std::cout << frame_info_->frame_id << " STAND" << std::endl;
	} 
	else if (walk_request_->motion_ == WalkRequestBlock::WALK) 
	{
		if (walk_requested_start_time < 0) {
			walk_requested_start_time = frame_info_->seconds_since_start;
		}
		// don't walk until we're calibrated
		if (!walk_engine_->theInertiaSensorDataBH.calibrated)
		{
			//std::cout << frame_info_->frame_id << " STAND - Waiting Calibration" << std::endl;
			motion = MotionRequestBH::stand;
		}
		else
		{
			//std::cout << frame_info_->frame_id << " WALK" << std::endl;
			motion = MotionRequestBH::walk;
		}
	} 
	else 
	{
		walk_engine_->reset();
		motion = MotionRequestBH::specialAction;
		walk_requested_start_time = -1;
	}

	if (walk_request_->walk_to_target_) 
	{
		walk_request_BH.mode = WalkRequest::targetMode;
	} 
	else if (walk_request_->percentage_speed_)
	{
		walk_request_BH.mode = WalkRequest::percentageSpeedMode;
	}
	else
	{
		walk_request_BH.mode = WalkRequest::speedMode;
	}

	walk_request_BH.speed.rotation = walk_request_->speed_.rotation;
	walk_request_BH.speed.translation.x = walk_request_->speed_.translation.x;
	walk_request_BH.speed.translation.y = walk_request_->speed_.translation.y;

	walk_request_BH.target.rotation = walk_request_->target_point_.rotation;
	walk_request_BH.target.translation.x = walk_request_->target_point_.translation.x;
	walk_request_BH.target.translation.y = walk_request_->target_point_.translation.y;
	if (walk_request_->walk_to_target_) 
	{
		walk_request_BH.speed.translation.x = 1.0;
		walk_request_BH.speed.translation.y = 1.0;
		walk_request_BH.speed.rotation = 1.0;
	}

	// kicks
	if (walk_request_->perform_kick_) 
	{
		if (fabs(walk_request_->kick_heading_) < DEG_T_RAD * 15) 
		{
			if (walk_request_->kick_with_left_)
			{
				walk_request_BH.kickType = WalkRequest::left;
			}
			else
			{
				walk_request_BH.kickType = WalkRequest::right;
			}
		} else {
			if (walk_request_->kick_heading_ > DEG_T_RAD*70)
			{
				walk_request_BH.kickType = WalkRequest::sidewardsRight;
			}
			else if (walk_request_->kick_heading_ > 0)
			{
				walk_request_BH.kickType = WalkRequest::angleRight;
			}
			else if (walk_request_->kick_heading_ < -DEG_T_RAD*50)
			{
				walk_request_BH.kickType = WalkRequest::sidewardsLeft;
			}
			else
			{
				walk_request_BH.kickType = WalkRequest::angleLeft;
			}
		}
		//std::cout << "PERFORM KICK: " << WalkRequest::getName(walk_request_BH.kickType) << std::endl;
		kick_distance_ = walk_request_->kick_distance_;
		kick_angle_ = walk_request_->kick_heading_;
	} 
	else 
	{
		walk_request_BH.kickType = WalkRequest::none;
	}

	if (!doingSlowStand() && shouldStartSlowStand()) 
	{
		startSlowStand();
	}

	if (doingSlowStand()) 
	{
		motion = MotionRequestBH::specialAction; // to make sure bhuman doesn't recalibrate during this time
	} 

	//walk_engine_->walk_decides_finished_with_target_ = walk_request_->walk_decides_finished_with_target_;
	//walk_engine_->finished_with_target_max_x_error_ = walk_request_->finished_with_target_max_x_error_;
	//walk_engine_->finished_with_target_max_y_error_ = walk_request_->finished_with_target_max_y_error_;
	//walk_engine_->finished_with_target_min_y_error_ = walk_request_->finished_with_target_min_y_error_;
}

void BHWalkModule::getArmsForState(int state, Joints angles) {
	if (state <= 1) 
	{
		angles[LShoulderPitch] = DEG_T_RAD * -116;
		angles[LShoulderRoll] = DEG_T_RAD * 12;
		angles[LElbowYaw] = DEG_T_RAD * -85;
		angles[LElbowRoll] = DEG_T_RAD * -0;
		angles[RShoulderPitch] = DEG_T_RAD * -116;
		angles[RShoulderRoll] = DEG_T_RAD * 12;
		angles[RElbowYaw] = DEG_T_RAD * -85;
		angles[RElbowRoll] = DEG_T_RAD * -0;
		if (state == 1) 
		{
			angles[LElbowYaw] = DEG_T_RAD * 25;
			angles[RElbowYaw] = DEG_T_RAD * 25;
		}
	} 
	else 
	{
		angles[LShoulderPitch] = DEG_T_RAD * -116;
		angles[LShoulderRoll] = DEG_T_RAD * 8;
		angles[LElbowYaw] = DEG_T_RAD * 25;
		angles[LElbowRoll] = DEG_T_RAD * -53;
		angles[RShoulderPitch] = DEG_T_RAD * -116;
		angles[RShoulderRoll] = DEG_T_RAD * 8;
		angles[RElbowYaw] = DEG_T_RAD * 25;
		angles[RElbowRoll] = DEG_T_RAD * -53;
	}
}

void BHWalkModule::determineStartingArmState() {
	// start from the current joints
	for (int i = ARM_JOINT_FIRST; i <= ARM_JOINT_LAST; i++) 
	{
		armStart[i] = joints_->values_[i];
	}

	for (int state = 2; state >= 1; state--) {
		Joints temp;
		getArmsForState(state,temp);
		bool acceptable = true;
		for (int i = ARM_JOINT_FIRST; i <= ARM_JOINT_LAST; i++) {
			if (fabs(joints_->values_[i] - temp[i]) > DEG_T_RAD * 10) {
				//std::cout << JointNames[i] << " is too far for state " << state << " sensed: " << RAD_T_DEG * joints_->values_[i] << " " << " desired: " << RAD_T_DEG * temp[i] << std::endl;
				acceptable = false;
				break;
			}
		}
		if (acceptable) {
			arm_state_ = state;
			//std::cout << "selected: " << arm_state_ << std::endl;
			return;
		}
	}
	// default to 0 if everything else has been bad
	arm_state_ = 0;
}

void BHWalkModule::setArms(Joints angles, float timeInSeconds) {
	float armStateTimes[3] = {1.0,0.5,0.5};

	if (timeInSeconds < 0.01)
		timeInSeconds = 0.01;

	float timePassed = frame_info_->seconds_since_start - arm_state_change_;

	int prevState = arm_state_;
	if (arm_state_ < 0) {
		determineStartingArmState();
	} else if (arm_state_ >= 2)
		arm_state_ = 2;
	else if (timePassed > armStateTimes[arm_state_]) {
		arm_state_ += 1;
	}

	// goal keeper only does state 0 ever
	if (walk_request_->keep_arms_out_){
		arm_state_ = 0;
	}

	if (arm_state_ != prevState) {
		//std::cout << frame_info_->frame_id << " changing state from " << prevState << " to " << arm_state_ << " after " << timePassed << " seconds" << std::endl;
		arm_state_change_ = frame_info_->seconds_since_start;
		timePassed = 0;
		// save previous commands as start
		if (prevState >= 0) {
			getArmsForState(prevState,armStart);
		}
	}


	// calculate the fraction we're into this state
	float frac = (timePassed + timeInSeconds) / armStateTimes[arm_state_];
	frac = crop(frac,0,1.0);

	// get desired angles
	getArmsForState(arm_state_,angles);

	// set the values
	for (int i = ARM_JOINT_FIRST; i <= ARM_JOINT_LAST; i++) {
		float des = angles[i];
		float orig = armStart[i];
		float val = frac * (des - orig) + orig;
		angles[i] = val;
	}

	// see if the arms are stable
	float maxDeltaDesired = 0;
	float maxDeltaDetected = 0;
	for (int i = LShoulderPitch; i <= RElbowRoll; i++) {
		float delta = angles[i] - joints_->values_[i];
		maxDeltaDesired = max(fabs(delta),maxDeltaDesired);
		maxDeltaDetected = max(fabs(joints_->changes_[i]),maxDeltaDetected);
	}
	arms_close_to_targets_ = (maxDeltaDesired < DEG_T_RAD * 20) || (maxDeltaDetected < DEG_T_RAD * 0.35);
}

const float BHWalkModule::STAND_ANGLES[NUM_JOINTS] = 
{
	0,
	-0.366519,
	0,
	0.00669175,
	-0.548284,
	1.04734,
	-0.499061,
	-0.00669175,
	0,
	-0.00669175,
	-0.548284,
	1.04734,
	-0.499061,
	0.00669175,
	-1.5708,
	0.2,
	-1.5708,
	-0.2,
	-1.5708,
	0.2,
	-1.5708,
	-0.2
};

void BHWalkModule::setMassCalibration() {
	int bhuman_inds[MassCalibrationBH::numOfLimbs] = {
		MassCalibrationBH::neck,
		MassCalibrationBH::head,
		MassCalibrationBH::shoulderLeft,
		MassCalibrationBH::bicepsLeft,
		MassCalibrationBH::elbowLeft,
		MassCalibrationBH::foreArmLeft,
		MassCalibrationBH::shoulderRight,
		MassCalibrationBH::bicepsRight,
		MassCalibrationBH::elbowRight,
		MassCalibrationBH::foreArmRight,
		MassCalibrationBH::pelvisLeft,
		MassCalibrationBH::hipLeft,
		MassCalibrationBH::thighLeft,
		MassCalibrationBH::tibiaLeft,
		MassCalibrationBH::ankleLeft,
		MassCalibrationBH::footLeft,
		MassCalibrationBH::pelvisRight,
		MassCalibrationBH::hipRight,
		MassCalibrationBH::thighRight,
		MassCalibrationBH::tibiaRight,
		MassCalibrationBH::ankleRight,
		MassCalibrationBH::footRight,
		MassCalibrationBH::torso
	};

	int ut_inds[MassCalibrationBH::numOfLimbs] = {
		BodyPart::neck,
		BodyPart::head,
		BodyPart::left_shoulder,
		BodyPart::left_bicep,
		BodyPart::left_elbow,
		BodyPart::left_forearm,
		BodyPart::right_shoulder,
		BodyPart::right_bicep,
		BodyPart::right_elbow,
		BodyPart::right_forearm,
		BodyPart::left_pelvis,
		BodyPart::left_hip,
		BodyPart::left_thigh,
		BodyPart::left_tibia,
		BodyPart::left_ankle,
		BodyPart::left_foot,
		BodyPart::right_pelvis,
		BodyPart::right_hip,
		BodyPart::right_thigh,
		BodyPart::right_tibia,
		BodyPart::right_ankle,
		BodyPart::right_foot,
		BodyPart::torso
	};

	for (int i = 0; i < MassCalibrationBH::numOfLimbs; i++) 
	{
		walk_engine_->theMassCalibrationBH.masses[bhuman_inds[i]].mass = robot_info_->mass_calibration_.masses[ut_inds[i]].mass;
		for (int j = 0; j < 3; j++)
		{
			walk_engine_->theMassCalibrationBH.masses[bhuman_inds[i]].offset[j] = robot_info_->mass_calibration_.masses[ut_inds[i]].offset[j];
		}
	}
}

void BHWalkModule::setRobotDimensions() {
	RobotDimensionsBH &bh = walk_engine_->theRobotDimensionsBH;
	RobotDimensions &ut = robot_info_->dimensions_;

	bh.xHeadTiltToCamera = ut.values_[RobotDimensions::xHeadTiltToBottomCamera];
	bh.zHeadTiltToCamera = ut.values_[RobotDimensions::zHeadTiltToBottomCamera];
	bh.headTiltToCameraTilt = ut.values_[RobotDimensions::tiltOffsetToBottomCamera];
	bh.xHeadTiltToUpperCamera = ut.values_[RobotDimensions::xHeadTiltToTopCamera];
	bh.zHeadTiltToUpperCamera = ut.values_[RobotDimensions::zHeadTiltToTopCamera];
	bh.headTiltToUpperCameraTilt = ut.values_[RobotDimensions::tiltOffsetToTopCamera];

	bh.lengthBetweenLegs = ut.values_[RobotDimensions::lengthBetweenLegs];
	bh.upperLegLength = ut.values_[RobotDimensions::upperLegLength];
	bh.lowerLegLength = ut.values_[RobotDimensions::lowerLegLength];
	bh.heightLeg5Joint = ut.values_[RobotDimensions::footHeight];
	bh.zLegJoint1ToHeadPan = ut.values_[RobotDimensions::zLegJoint1ToHeadPan];
	bh.armOffset[0] = ut.values_[RobotDimensions::armOffset1];
	bh.armOffset[1] = ut.values_[RobotDimensions::armOffset2];
	bh.armOffset[2] = ut.values_[RobotDimensions::armOffset3];
	bh.yElbowShoulder = ut.values_[RobotDimensions::elbowOffsetY];
	bh.upperArmLength = ut.values_[RobotDimensions::upperArmLength];
	bh.lowerArmLength = ut.values_[RobotDimensions::lowerArmLength];
}

void BHWalkModule::selectivelySendStiffness() {
	for (int i = 0; i < NUM_JOINTS; i++) {
		if (fabs(joints_->stiffness_[i] - commands_->stiffness_[i]) > 0.01) {
			commands_->send_stiffness_ = true;
			commands_->stiffness_time_ = 10;
			return;
		}
	}
}

void BHWalkModule::setPose2D(Pose2D &dest, const Pose2DBH &src) {
	dest.translation.x = src.translation.x;
	dest.translation.y = src.translation.y;
	dest.rotation = src.rotation;
}

bool BHWalkModule::doingSlowStand() {
	return frame_info_->seconds_since_start < slow_stand_end;
}

void BHWalkModule::doSlowStand() {
	float dt = slow_stand_end - frame_info_->seconds_since_start;
	for (int i = BODY_JOINT_OFFSET; i < NUM_JOINTS; i++) 
	{
		commands_->angles_[i] = STAND_ANGLES[i];
		commands_->stiffness_[i] = 1.0;
	}
	setArms(commands_->angles_,dt);
	commands_->send_body_angles_ = true;
	commands_->body_angle_time_ = 1000 * dt;
	selectivelySendStiffness();
}

void BHWalkModule::setWalkHeight(float z) {
	float maxDec = 10.0 / 100.0; // 1 cm every second (i.e. 100 frames)
	float maxInc = 30.0 / 100.0; // 3 cm every second (i.e. 100 frames)

	float delta = (z - walk_engine_->standComPosition.z);
	delta = crop(delta,-maxDec,maxDec);
	z = walk_engine_->standComPosition.z + delta;

	walk_engine_->standComPosition.z = z;
	walk_engine_->walkHeight.x = z;
	if (frame_info_->frame_id % 100 == 0)
	{
		std::cout << "setting walk height to " << z << std::endl;
	}
}

bool BHWalkModule::shouldStartSlowStand() {
	//std::cout << frame_info_->frame_id << " shouldStartSlowStand: " << walkNames[walk_request_->motion_] << " " << WalkingEngine::getName(walk_engine_->currentMotionType) << " " << frame_info_->seconds_since_start << " " << slow_stand_end << " "  << RAD_T_DEG * standJointErr(LKneePitch) << " " << RAD_T_DEG * standJointErr(RKneePitch) << std::endl;
	static float kneeErrAllowed = DEG_T_RAD * 30;
	if (walk_request_->slow_stand_) // if it's requested
	{
		return true;
	}
	else if ((walk_request_->motion_ != WalkRequestBlock::STAND)
			&&  (walk_request_->motion_ != WalkRequestBlock::WALK))
	{
		return false;
	}
	else if (walk_engine_->currentMotionType == WalkingEngine::stepping) // we're walking
	{
		return false;
	}
	else if (frame_info_->seconds_since_start - slow_stand_end < 5.0) // recently did a slow stand
	{
		return false;
	}
	else if ((standJointErr(LKneePitch) > kneeErrAllowed)
			||  (standJointErr(RKneePitch) > kneeErrAllowed)) // knees far from what we want
	{
		return true;
	}
	else
	{
		return false;
	}
}

void BHWalkModule::startSlowStand() {
	std::cout << frame_info_->frame_id << " starting slow stand" << std::endl;
	float err = 0;
	for (int i = LHipYawPitch; i <= RAnkleRoll; i++)
	{
		err = max(err,standJointErr(i));
	}
	float maxJointSpeed = DEG_T_RAD * 45;
	float duration = err / maxJointSpeed;
	if (duration < 0.5)
	{
		duration = 0.5;
	}
	if (walk_request_->slow_stand_)
	{
		duration = 0.5;
	}

	slow_stand_start = frame_info_->seconds_since_start;
	slow_stand_end = slow_stand_start + duration;
}

float BHWalkModule::standJointErr(int joint) {
	return fabs(STAND_ANGLES[joint] - joints_->values_[joint]);
}

bool BHWalkModule::readyToStartKickAfterStep() {
	// at stepForLeftKick or stepForRightKick and we're walking
	return ((walk_engine_->predictedPendulumPlayer.phase.kickType == WalkRequest::left) || (walk_engine_->predictedPendulumPlayer.phase.kickType == WalkRequest::right)) && (walk_engine_->currentMotionType == WalkingEngine::stepping) && (step_into_kick_state_ == PERFORMING);
}

void BHWalkModule::handleStepIntoKick() {
	if ((walk_request_->new_command_) && (walk_request_->perform_kick_) && (walk_request_->step_into_kick_) && (step_into_kick_state_ == NONE)) {
		//std::cout << frame_info_->frame_id << " RECEIVED STEP_INTO_KICK" << std::endl;
		WalkRequest &walk_request_BH = walk_engine_->theMotionRequestBH.walkRequest;
		MotionRequestBH::Motion &motion = walk_engine_->theMotionRequestBH.motion;
		kick_request_->finished_with_step_ = false;
		step_into_kick_state_ = PERFORMING;
		motion = MotionRequestBH::walk;
		walk_request_BH.mode = WalkRequest::percentageSpeedMode;
		walk_request_BH.speed = Pose2DBH(0,0,0);
		if (walk_request_->kick_with_left_)
			walk_request_BH.kickType = WalkRequest::left;
		else
			walk_request_BH.kickType = WalkRequest::right;
	} 

	if (readyToStartKickAfterStep()) {
		step_into_kick_state_ = FINISHED_WITH_STEP;
		walk_request_->noWalk();
		kick_request_->finished_with_step_ = true;
		time_step_into_kick_finished_ = frame_info_->seconds_since_start;
		//std::cout << frame_info_->frame_id << " finished with step into kick" << std::endl;
		return;
	}

	if (step_into_kick_state_ == FINISHED_WITH_STEP) {
		if (kick_request_->kick_running_ || kick_request_->vision_kick_running_ || (frame_info_->seconds_since_start - time_step_into_kick_finished_ < 0.1)) {
			//std::cout << frame_info_->frame_id << " kick is running: " << kick_request_->kick_running_ << " " << kick_request_->vision_kick_running_ << std::endl;
		} else {
			//std::cout << frame_info_->frame_id << " done" << std::endl;
			step_into_kick_state_ = NONE;
		}
		walk_request_->noWalk();
		walk_engine_->reset();
	}

	if (step_into_kick_state_ == PERFORMING) {
		kick_request_->setNoKick();
	}
}
