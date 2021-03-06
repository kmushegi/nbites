// Generated by the protocol buffer compiler.  DO NOT EDIT!
// source: VisionField.proto

package messages;

public interface VisualCornerOrBuilder extends
    // @@protoc_insertion_point(interface_extends:messages.VisualCorner)
    com.google.protobuf.MessageOrBuilder {

  /**
   * <code>optional .messages.VisualDetection visual_detection = 1;</code>
   */
  boolean hasVisualDetection();
  /**
   * <code>optional .messages.VisualDetection visual_detection = 1;</code>
   */
  messages.VisualDetection getVisualDetection();
  /**
   * <code>optional .messages.VisualDetection visual_detection = 1;</code>
   */
  messages.VisualDetectionOrBuilder getVisualDetectionOrBuilder();

  /**
   * <code>optional float orientation = 2;</code>
   */
  boolean hasOrientation();
  /**
   * <code>optional float orientation = 2;</code>
   */
  float getOrientation();

  /**
   * <code>optional .messages.VisualCorner.shape corner_type = 3;</code>
   */
  boolean hasCornerType();
  /**
   * <code>optional .messages.VisualCorner.shape corner_type = 3;</code>
   */
  messages.VisualCorner.shape getCornerType();

  /**
   * <code>optional float physical_orientation = 4;</code>
   */
  boolean hasPhysicalOrientation();
  /**
   * <code>optional float physical_orientation = 4;</code>
   */
  float getPhysicalOrientation();

  /**
   * <code>repeated .messages.VisualCorner.corner_id poss_id = 5;</code>
   */
  java.util.List<messages.VisualCorner.corner_id> getPossIdList();
  /**
   * <code>repeated .messages.VisualCorner.corner_id poss_id = 5;</code>
   */
  int getPossIdCount();
  /**
   * <code>repeated .messages.VisualCorner.corner_id poss_id = 5;</code>
   */
  messages.VisualCorner.corner_id getPossId(int index);

  /**
   * <code>optional sint32 x = 6;</code>
   */
  boolean hasX();
  /**
   * <code>optional sint32 x = 6;</code>
   */
  int getX();

  /**
   * <code>optional sint32 y = 7;</code>
   */
  boolean hasY();
  /**
   * <code>optional sint32 y = 7;</code>
   */
  int getY();
}
