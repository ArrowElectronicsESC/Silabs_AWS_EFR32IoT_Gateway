// GENERATED FILE DO NOT MODIFY
// Generated by: generate-zigbee-logger-files.py -y zigbee_logger.yaml

#ifndef _ZIGBEE_EVENT_LOGGER_GEN_H_

#define ZIGBEE_EVENT_LOGGER_VERSION 1
#define LOGGER_AREA_BIT_SHIFT 12
#define EMBER_ZIGBEE_LOGGER_AREA (0 << LOGGER_AREA_BIT_SHIFT)

typedef enum {
  EMBER_LOGGER_ZIGBEE_BOOT_EVENT_ID = 0,
  EMBER_LOGGER_ZIGBEE_JOIN_NETWORK_ID = 1,
  EMBER_LOGGER_ZIGBEE_TRUST_CENTER_REJOIN_ID = 2,
  EMBER_LOGGER_ZIGBEE_SECURE_REJOIN_ID = 3,
  EMBER_LOGGER_ZIGBEE_LEAVE_WITHOUT_REJOIN_ID = 4,
  EMBER_LOGGER_ZIGBEE_LEAVE_WITH_REJOIN_ID = 5,
  EMBER_LOGGER_ZIGBEE_TRUST_CENTER_LINK_KEY_CHANGE_ID = 6,
  EMBER_LOGGER_ZIGBEE_NETWORK_KEY_SEQUENCE_NUMBER_CHANGE_ID = 7,
  EMBER_LOGGER_ZIGBEE_RESET_ID = 8,
  EMBER_LOGGER_ZIGBEE_CHANNEL_CHANGE_ID = 9,
  EMBER_LOGGER_ZIGBEE_PARENT_CHANGE_ID = 10,
  EMBER_LOGGER_ZIGBEE_DUTY_CYCLE_CHANGE_ID = 11,
  EMBER_LOGGER_ZIGBEE_CHILD_ADDED_ID = 12,
  EMBER_LOGGER_ZIGBEE_CHILD_REMOVED_ID = 13,
  EMBER_LOGGER_ZIGBEE_CHILD_TIMED_OUT_ID = 14,
  EMBER_LOGGER_ZIGBEE_STACK_STATUS_ID = 15,
  EMBER_LOGGER_ZIGBEE_TIME_SYNC_ID = 16,
  // Last value. All event IDs should be less than this
  EMBER_LOGGER_ZIGBEE_NULL_ID = 0xFFFF,
} EmberZigbeeLoggerEventTypes;

typedef enum {
  EMBER_LOGGER_ZIGBEE_FACILITY_BASE = 0,
  EMBER_LOGGER_ZIGBEE_FACILITY_MAC = 1,
  EMBER_LOGGER_ZIGBEE_FACILITY_NETWORK = 2,
  EMBER_LOGGER_ZIGBEE_FACILITY_APS = 3,
  EMBER_LOGGER_ZIGBEE_FACILITY_SECURITY = 4,
  EMBER_LOGGER_ZIGBEE_FACILITY_ZDO = 5,
  EMBER_LOGGER_ZIGBEE_FACILITY_ZCL = 6,
  EMBER_LOGGER_ZIGBEE_FACILITY_BDB = 7,
  EMBER_LOGGER_ZIGBEE_FACILITY_SMART_ENERGY = 8,
} EmberZigbeeLoggerFacilityTypes;

#define EMBER_LOGGER_ZIGBEE_BOOT_EVENT_LENGTH 6
#define EMBER_LOGGER_ZIGBEE_JOIN_NETWORK_LENGTH 12
#define EMBER_LOGGER_ZIGBEE_TRUST_CENTER_REJOIN_LENGTH 5
#define EMBER_LOGGER_ZIGBEE_SECURE_REJOIN_LENGTH 5
#define EMBER_LOGGER_ZIGBEE_LEAVE_WITHOUT_REJOIN_LENGTH 3
#define EMBER_LOGGER_ZIGBEE_LEAVE_WITH_REJOIN_LENGTH 3
#define EMBER_LOGGER_ZIGBEE_TRUST_CENTER_LINK_KEY_CHANGE_LENGTH 0
#define EMBER_LOGGER_ZIGBEE_NETWORK_KEY_SEQUENCE_NUMBER_CHANGE_LENGTH 1
#define EMBER_LOGGER_ZIGBEE_RESET_LENGTH 2
#define EMBER_LOGGER_ZIGBEE_CHANNEL_CHANGE_LENGTH 2
#define EMBER_LOGGER_ZIGBEE_PARENT_CHANGE_LENGTH 10
#define EMBER_LOGGER_ZIGBEE_DUTY_CYCLE_CHANGE_LENGTH 7
#define EMBER_LOGGER_ZIGBEE_CHILD_ADDED_LENGTH 10
#define EMBER_LOGGER_ZIGBEE_CHILD_REMOVED_LENGTH 10
#define EMBER_LOGGER_ZIGBEE_CHILD_TIMED_OUT_LENGTH 10
#define EMBER_LOGGER_ZIGBEE_STACK_STATUS_LENGTH 1
#define EMBER_LOGGER_ZIGBEE_TIME_SYNC_LENGTH 8

// Add event function prototypes
void emberAfPluginZigBeeEventLoggerAddBootEvent(uint32_t bootCount, uint16_t resetType);
void emberAfPluginZigBeeEventLoggerAddJoinNetwork(EmberPanId panId, uint8_t channelNumber, uint8_t page, uint8_t *extendedPanId);
void emberAfPluginZigBeeEventLoggerAddTrustCenterRejoin(uint32_t channelMask, EmberRejoinReason reason);
void emberAfPluginZigBeeEventLoggerAddSecureRejoin(uint32_t channelMask, EmberRejoinReason reason);
void emberAfPluginZigBeeEventLoggerAddLeaveWithoutRejoin(EmberNodeId sourceNode, EmberLeaveReason reason);
void emberAfPluginZigBeeEventLoggerAddLeaveWithRejoin(EmberNodeId sourceNode, EmberLeaveReason reason);
void emberAfPluginZigBeeEventLoggerAddTrustCenterLinkKeyChange(void);
void emberAfPluginZigBeeEventLoggerAddNetworkKeySequenceChange(uint8_t sequence);
void emberAfPluginZigBeeEventLoggerAddReset(uint8_t resetCode, uint8_t extendedReason);
void emberAfPluginZigBeeEventLoggerAddChannelChange(uint8_t page, uint8_t channel);
void emberAfPluginZigBeeEventLoggerAddParentChange(EmberNodeId parentNodeId, EmberEUI64 parentEui64);
void emberAfPluginZigBeeEventLoggerAddDutyCycleChange(EmberDutyCycleState state, EmberDutyCycleHectoPct limitThreshold, EmberDutyCycleHectoPct criticalThreshold, EmberDutyCycleHectoPct suspendedLimit);
void emberAfPluginZigBeeEventLoggerAddChildAdded(EmberNodeId childNodeId, EmberEUI64 childEui64);
void emberAfPluginZigBeeEventLoggerAddChildRemoved(EmberNodeId childNodeId, EmberEUI64 childEui64);
void emberAfPluginZigBeeEventLoggerAddChildTimedOut(EmberNodeId childNodeId, EmberEUI64 childEui64);
void emberAfPluginZigBeeEventLoggerAddStackStatus(EmberStatus status);
void emberAfPluginZigBeeEventLoggerAddTimeSync(uint32_t utcTime, uint32_t secondsSinceBoot);

#endif  // #ifndef _ZIGBEE_EVENT_LOGGER_GEN_H_
