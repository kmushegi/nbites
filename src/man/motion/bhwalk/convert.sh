#!/bin/bash

types="Module Vector3 Matrix2x2 Matrix3x3 Matrix3x2 Matrix2x3 RotationMatrix Pose3D Vector2 Range Pose2D RingBufferWithSum RingBuffer sgn sqr sec cosec normalize
ButtonInterface
JointData
JointRequest
SensorData
KeyStates
LEDRequest
Image
CameraInfo
FrameInfo
CognitionFrameInfo
RobotInfo
OwnTeamInfo
OpponentTeamInfo
GameInfo
TeamMateData
MotionRobotHealth
RobotHealth
TeamDataSenderOutput
USRequest
Thumbnail
CameraSettings
FieldDimensions
RobotDimensions
JointCalibration
SensorCalibration
CameraCalibration
MassCalibration
HardnessSettings
DamageConfiguration
HeadLimits
CameraMatrix
RobotCameraMatrix
ImageCoordinateSystem
BallSpots
LineSpots
PossibleObstacleSpots
BallPercept
LinePercept
RegionPercept
GoalPercept
GroundContactState
BodyContour
ColorReference
FieldBoundary
ObstacleSpots
ArmContactModel
FallDownState
BallModel
CombinedWorldModel
GroundTruthBallModel
ObstacleModel
USObstacleModel
RobotPose
FilteredRobotPose
FootContactModel
GroundTruthRobotPose
RobotsModel
GroundTruthRobotsModel
FreePartOfOpponentGoalModel
FieldCoverage
GlobalFieldCoverage
SideConfidence
Odometer
OwnSideModel
ObstacleWheel
ObstacleClusters
ActivationGraph
BehaviorControlOutput
BehaviorLEDRequest
Path
FilteredJointData
FilteredSensorData
InertiaSensorData
OrientationData
GroundTruthOrientationData
TorsoMatrix
RobotModel
JointDynamics
FutureJointDynamics
RobotBalance
FsrData
FsrZmp
ArmMotionEngineOutput
ArmMotionRequest
OdometryData
GroundTruthOdometryData
GetUpEngineOutput
MotionRequest
HeadMotionRequest
HeadAngleRequest
HeadJointRequest
MotionSelection
SpecialActionsOutput
WalkingEngineOutput
WalkingEngineStandOutput
BikeEngineOutput
MotionInfo
BallTakingOutput
IndykickEngineOutput
theButtonInterface
theJointData
theJointRequest
theSensorData
theKeyStates
theLEDRequest
theImage
theCameraInfo
theFrameInfo
theCognitionFrameInfo
theRobotInfo
theOwnTeamInfo
theOpponentTeamInfo
theGameInfo
theTeamMateData
theMotionRobotHealth
theRobotHealth
theTeamDataSenderOutput
theUSRequest
theThumbnail
theCameraSettings
theFieldDimensions
theRobotDimensions
theJointCalibration
theSensorCalibration
theCameraCalibration
theMassCalibration
theHardnessSettings
theDamageConfiguration
theHeadLimits
theCameraMatrix
theRobotCameraMatrix
theImageCoordinateSystem
theBallSpots
theLineSpots
thePossibleObstacleSpots
theBallPercept
theLinePercept
theRegionPercept
theGoalPercept
theGroundContactState
theBodyContour
theColorReference
theFieldBoundary
theObstacleSpots
theArmContactModel
theFallDownState
theBallModel
theCombinedWorldModel
theGroundTruthBallModel
theObstacleModel
theUSObstacleModel
theRobotPose
theFilteredRobotPose
theFootContactModel
theGroundTruthRobotPose
theRobotsModel
theGroundTruthRobotsModel
theFreePartOfOpponentGoalModel
theFieldCoverage
theGlobalFieldCoverage
theSideConfidence
theOdometer
theOwnSideModel
theObstacleWheel
theObstacleClusters
theActivationGraph
theBehaviorControlOutput
theBehaviorLEDRequest
thePath
theFilteredJointData
theFilteredSensorData
theInertiaSensorData
theOrientationData
theGroundTruthOrientationData
theTorsoMatrix
theRobotModel
theJointDynamics
theFutureJointDynamics
theRobotBalance
theFsrData
theFsrZmp
theArmMotionEngineOutput
theArmMotionRequest
theOdometryData
theGroundTruthOdometryData
theGetUpEngineOutput
theMotionRequest
theHeadMotionRequest
theHeadAngleRequest
theHeadJointRequest
theMotionSelection
theSpecialActionsOutput
theWalkingEngineOutput
theWalkingEngineStandOutput
theBikeEngineOutput
theMotionInfo
theBallTakingOutput
theIndykickEngineOutput"

for type in $types; do
  echo $type
  cp BHWalkModule.cpp BHWalkModule.cpp.bak.tmp
  cp BHWalkModule.h BHWalkModule.h.bak.tmp
  perl -pi -w -e "s/\b${type}\b/${type}BH/g;" *.h *.cpp */*.h */*.cpp */*/*.h */*/*.cpp
  perl -pi -w -e "s/${type}BH\.h/${type}\.h/g;" *.h *.cpp */*.h */*.cpp */*/*.h */*/*.cpp
  cp BHWalkModule.cpp.bak.tmp BHWalkModule.cpp
  cp BHWalkModule.h.bak.tmp BHWalkModule.h
done
