// *****************************************************************************
// * occupancy-sensor-server.c
// *
// * Generic occupancy sensor server implementation.
// * Requires a HAL plugin that implements the occupancy API to be included
// * in the project to provide APIs and callbacks for initialization and device
// * management.
// * Populates the Occupancy and Occupancy Sensor Type attributes of the
// * occupancy sensor cluster.
// *
// * Copyright 2015 Silicon Laboratories, Inc.                              *80*
// *****************************************************************************

#include "app/framework/include/af.h"
#include EMBER_AF_API_OCCUPANCY

#ifdef EMBER_AF_PLUGIN_REPORTING
#include "app/framework/plugin/reporting/reporting.h"
#endif

//------------------------------------------------------------------------------
// Plugin private function prototypes
static void writeServerAttributeForAllEndpoints(EmberAfAttributeId attributeID,
                                                uint8_t* dataPtr);
static void checkForReportingConfig(void);

//------------------------------------------------------------------------------
// Global variables

// The default configuration to be used if no reporting has been set up
static EmberAfPluginReportingEntry defaultConfiguration = {
  EMBER_ZCL_REPORTING_DIRECTION_REPORTED, //direction
  0, //endpoint, which will be set on a per-use basis
  ZCL_OCCUPANCY_SENSING_CLUSTER_ID, //clusterId
  ZCL_OCCUPANCY_ATTRIBUTE_ID, //attributeId
  CLUSTER_MASK_SERVER, //mask
  EMBER_AF_NULL_MANUFACTURER_CODE, //manufacturerCode
  .data.reported = {
    1, //minInterval
    EMBER_AF_PLUGIN_OCCUPANCY_SENSOR_SERVER_DEFAULT_OCCUPANCY_MAX_REPORT_PERIOD_S, //maxInterval
    1 //reportableChange
  }
};

//------------------------------------------------------------------------------
// Plugin consumed callback implementations

//******************************************************************************
// Plugin init function
//******************************************************************************
void emberAfPluginOccupancySensorServerInitCallback(void)
{
  HalOccupancySensorType deviceType;

  checkForReportingConfig();
  deviceType = halOccupancyGetSensorType();
  writeServerAttributeForAllEndpoints(ZCL_OCCUPANCY_SENSOR_TYPE_ATTRIBUTE_ID,
                                      (uint8_t *) &deviceType);
}

//******************************************************************************
// Notification callback from the HAL plugin
//******************************************************************************
void halOccupancyStateChangedCallback(HalOccupancyState occupancyState)
{
  if (occupancyState == HAL_OCCUPANCY_STATE_OCCUPIED) {
    emberAfOccupancySensingClusterPrintln("Occupancy detected");
  } else {
    emberAfOccupancySensingClusterPrintln("Occupancy no longer detected");
  }

  writeServerAttributeForAllEndpoints(ZCL_OCCUPANCY_ATTRIBUTE_ID,
                                      (uint8_t *) &occupancyState);
  emberAfPluginOccupancySensorServerOccupancyStateChangedCallback(
    occupancyState);
}

void emberAfPluginOccupancySensorServerStackStatusCallback(
  EmberStatus status)
{
  // On network connect, verify that a reporting entry is set up for the
  // occupancy sensor
  if (status == EMBER_NETWORK_UP) {
    checkForReportingConfig();
  }
}

//------------------------------------------------------------------------------
// Plugin private functions

static void checkForReportingConfig(void)
{
#ifdef EMBER_AF_PLUGIN_REPORTING
  uint8_t i;
  EmberAfPluginReportingEntry entry;
  uint8_t endpoint;
  bool existingEntry = false;

  // On initialization, cycle through the reporting table to determine if a
  // reporting table entry has been created for the occupancy state attribute
  for (i = 0; i < REPORT_TABLE_SIZE; i++) {
    emAfPluginReportingGetEntry(i, &entry);
    if ((entry.clusterId == ZCL_OCCUPANCY_SENSING_CLUSTER_ID)
        && (entry.attributeId == ZCL_OCCUPANCY_ATTRIBUTE_ID)
        && (entry.direction == EMBER_ZCL_REPORTING_DIRECTION_REPORTED)
        && (entry.endpoint != EMBER_AF_PLUGIN_REPORTING_UNUSED_ENDPOINT_ID)
        && (entry.manufacturerCode == EMBER_AF_NULL_MANUFACTURER_CODE)) {
      existingEntry = true;
    }
  }

  // If no entry is found for the occupancy sensor, a default reporting
  // configuration should be created using the plugin defined options.  This
  // needs to be done for all endpoints that support an occupancy sensor server.
  if (!existingEntry) {
    for (i = 0; i < emberAfEndpointCount(); i++) {
      endpoint = emberAfEndpointFromIndex(i);
      defaultConfiguration.endpoint = endpoint;
      if (emberAfContainsServer(endpoint, ZCL_OCCUPANCY_SENSING_CLUSTER_ID)) {
        emAfPluginReportingAddEntry(&defaultConfiguration);
      }
    }
  }
#endif
}

//******************************************************************************
// Cycle through the list of all endpoints in the system and write the given
// attribute of the occupancy sensing cluster for all endpoints in the system
// are servers of that cluster.
//******************************************************************************
static void writeServerAttributeForAllEndpoints(EmberAfAttributeId attributeID,
                                                uint8_t* dataPtr)
{
  uint8_t i;
  uint8_t endpoint;

  for (i = 0; i < emberAfEndpointCount(); i++) {
    endpoint = emberAfEndpointFromIndex(i);
    if (emberAfContainsServer(endpoint, ZCL_OCCUPANCY_SENSING_CLUSTER_ID)) {
      emberAfWriteServerAttribute(endpoint,
                                  ZCL_OCCUPANCY_SENSING_CLUSTER_ID,
                                  attributeID,
                                  dataPtr,
                                  ZCL_BITMAP8_ATTRIBUTE_TYPE);
    }
  }
}
