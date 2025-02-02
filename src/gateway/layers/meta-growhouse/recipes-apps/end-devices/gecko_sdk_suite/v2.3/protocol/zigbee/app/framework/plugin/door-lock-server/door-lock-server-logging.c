// Copyright 2017 Silicon Laboratories, Inc.

#include "af.h"
#include "door-lock-server.h"

static EmberAfPluginDoorLockServerLogEntry entries[EMBER_AF_PLUGIN_DOOR_LOCK_SERVER_MAX_LOG_ENTRIES];
static uint8_t nextEntryId = 1;

#define ENTRY_ID_TO_INDEX(entryId) ((entryId) - 1)
#define ENTRY_ID_IS_VALID(entryId) ((entryId) > 0 && (entryId) < nextEntryId)
#define MOST_RECENT_ENTRY_ID() (nextEntryId - 1)
#define LOG_IS_EMPTY() (nextEntryId == 1)

static bool loggingIsEnabled(void)
{
  // This is hardcoded to endpoint 1 because...we need to add endpoint support...
  uint8_t endpoint = 1;
  bool logging = false;
  EmberAfStatus status
    = emberAfReadServerAttribute(endpoint,
                                 ZCL_DOOR_LOCK_CLUSTER_ID,
                                 ZCL_ENABLE_LOGGING_ATTRIBUTE_ID,
                                 (uint8_t *)&logging,
                                 sizeof(logging));
  if (status != EMBER_ZCL_STATUS_SUCCESS) {
    emberAfDoorLockClusterPrintln("Could not read EnableLogging attribute: 0x%X",
                                  status);
  }
  return logging;
}

bool emberAfPluginDoorLockServerAddLogEntry(EmberAfDoorLockEventType eventType,
                                            EmberAfDoorLockEventSource source,
                                            uint8_t eventId,
                                            uint16_t userId,
                                            uint8_t pinLength,
                                            uint8_t *pin)
{
  if (!loggingIsEnabled()
      || ENTRY_ID_TO_INDEX(nextEntryId) >= COUNTOF(entries)) {
    return false;
  }

  EmberAfPluginDoorLockServerLogEntry *nextEntry
    = &entries[ENTRY_ID_TO_INDEX(nextEntryId)];
  nextEntry->logEntryId = nextEntryId;
  nextEntry->timestamp = emberAfGetCurrentTimeCallback();
  nextEntry->eventType = eventType;
  nextEntry->source = source;
  nextEntry->eventId = eventId;
  nextEntry->userId = userId;
  nextEntry->pin[0] = pinLength;
  MEMMOVE(nextEntry->pin + 1, pin, pinLength);

  nextEntryId++;

  return true;
}

bool emberAfPluginDoorLockServerGetLogEntry(uint16_t *entryId,
                                            EmberAfPluginDoorLockServerLogEntry *entry)
{
  if (LOG_IS_EMPTY()) {
    return false;
  }

  if (!ENTRY_ID_IS_VALID(*entryId)) {
    *entryId = MOST_RECENT_ENTRY_ID();
  }
  assert(ENTRY_ID_IS_VALID(*entryId));

  *entry = entries[ENTRY_ID_TO_INDEX(*entryId)];

  return true;
}

bool emberAfDoorLockClusterGetLogRecordCallback(uint16_t entryId)
{
  EmberStatus status;
  EmberAfPluginDoorLockServerLogEntry entry;
  if (LOG_IS_EMPTY() || !emberAfPluginDoorLockServerGetLogEntry(&entryId, &entry)) {
    status = emberAfSendImmediateDefaultResponse(EMBER_ZCL_STATUS_INVALID_VALUE);
    if (status != EMBER_ZCL_STATUS_SUCCESS) {
      emberAfDoorLockClusterPrintln("Failed to send default response: 0x%X",
                                    status);
    }
  } else {
    emberAfFillCommandDoorLockClusterGetLogRecordResponse(entry.logEntryId,
                                                          entry.timestamp,
                                                          entry.eventType,
                                                          entry.source,
                                                          entry.eventId,
                                                          entry.userId,
                                                          entry.pin);
    status = emberAfSendResponse();
    if (status != EMBER_SUCCESS) {
      emberAfDoorLockClusterPrintln("Failed to send GetLogRecordResponse: 0x%X",
                                    status);
    }
  }
  return true;
}
