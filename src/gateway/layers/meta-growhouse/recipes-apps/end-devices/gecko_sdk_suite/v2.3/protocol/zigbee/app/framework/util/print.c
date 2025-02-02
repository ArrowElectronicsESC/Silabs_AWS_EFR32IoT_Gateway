// *******************************************************************
// * print.c
// *
// * Utilities and command line interface for printing, and enabling/disabling
// * printing to different areas.
// *
// * Copyright 2007 by Ember Corporation. All rights reserved.              *80*
// *******************************************************************

#include "app/framework/include/af.h"
#include <syslog.h>

//------------------------------------------------------------------------------
// Globals

// Enable this if you want area names printed. It proved annoying
// so we're disabling it here.
//#define EMBER_AF_PRINT_AREA_NAME

#ifdef EMBER_AF_PRINT_AREA_NAME
static void reallyPrintAreaName(uint16_t area);
  #define printAreaName(area) reallyPrintAreaName(area)
#else
  #define printAreaName(area)
#endif //EMBER_AF_PRINT_AREA_NAME

#ifdef EMBER_AF_PRINT_NAMES
static PGM_P areaNames[] = EMBER_AF_PRINT_NAMES;
#endif

#ifdef EMBER_AF_PRINT_BITS
static uint8_t enablementBytes[] = EMBER_AF_PRINT_BITS;
#endif

uint16_t emberAfPrintActiveArea = 0;

//------------------------------------------------------------------------------

// Returns true if the area print is enabled
bool emberAfPrintEnabled(uint16_t area)
{
#ifndef EMBER_AF_PRINT_BITS
  return false;
#else
  uint8_t index = (uint8_t)(area >> 8);
  if ( area == 0xFFFF ) {
    return true;
  }
  if ( index >= sizeof(enablementBytes) ) {
    return false;
  } else {
    uint8_t byte = enablementBytes[index];
    return ( (byte & (uint8_t)(area & 0xFF) ) != 0x00);
  }
#endif // EMBER_AF_PRINT_BITS
}

static void printEnable(uint16_t area, bool on)
{
#ifdef EMBER_AF_PRINT_BITS
  uint8_t index = (uint8_t)(area >> 8);
  if ( index < sizeof(enablementBytes) ) {
    if ( on ) {
      enablementBytes[index] |= (uint8_t)(area & 0xFF);
    } else {
      enablementBytes[index] &= ~(uint8_t)(area & 0xFF);
    }
  }
#endif // EMBER_AF_PRINT_BITS
}

static uint16_t convertUserNumberAreaToInternalArea(uint16_t userNumberedArea)
{
  uint16_t index = userNumberedArea / 8;
  index = (uint16_t)(index << 8)
          + (uint16_t)( ((uint16_t)0x0001) << (userNumberedArea % 8) );
  return index;
}

#if defined EMBER_AF_PRINT_AREA_NAME
static void reallyPrintAreaName(uint16_t area)
{
#ifdef EMBER_AF_PRINT_NAMES
  uint8_t hi, lo, count;
  uint16_t index;

  hi = (uint8_t)(area >> 8);
  lo = (uint8_t)(area & 0xFF);
  count = 0;

  if ( lo != 0 ) {
    while ( !(lo & 0x01) ) {
      lo >>= 1;
      count++;
    }
  }
  index = ((8 * hi) + count);

  if (area != 0xFFFF
      && index < EMBER_AF_PRINT_NAME_NUMBER) {
    emberSerialPrintf(EMBER_AF_PRINT_OUTPUT, "%p:", areaNames[index]);
  }
#endif // EMBER_AF_PRINT_NAMES
}
#endif //EMBER_AF_PRINT_AREA_NAME

// Prints the trace if trace is enabled
static void emAfPrintInternalVarArg(uint16_t area,
                                    bool newLine,
                                    PGM_P formatString,
                                    va_list ap)
{
  if ( !emberAfPrintEnabled(area) ) {
    return;
  }
  printAreaName(area);

  emberSerialPrintfVarArg(EMBER_AF_PRINT_OUTPUT, formatString, ap);

  if (newLine) {
    emberSerialPrintf(EMBER_AF_PRINT_OUTPUT, "\r\n");
  }
  emberAfPrintActiveArea = area;
}

void emberAfFlush(uint16_t area)
{
  if ( emberAfPrintEnabled(area) ) {
    emberSerialWaitSend(EMBER_AF_PRINT_OUTPUT);
  }
}

void emberAfPrintln(uint16_t area, PGM_P formatString, ...)
{
  va_list ap = { 0 };
  va_start(ap, formatString);
#if defined CLI_OPTION
  emAfPrintInternalVarArg(area, true, formatString, ap);
#else
  //Redirect log in /var/log/syslog
  vsyslog(LOG_INFO, formatString, ap);
#endif
  va_end(ap);
}

void emberAfPrint(uint16_t area, PGM_P formatString, ...)
{
  va_list ap = { 0 };
  va_start(ap, formatString);
#if defined CLI_OPTION
  emAfPrintInternalVarArg(area, false, formatString, ap);
#else
  //Redirect log in /var/log/syslog
  vsyslog(LOG_INFO, formatString, ap);
#endif
  va_end(ap);
}

void emberAfPrintStatus(void)
{
#ifdef EMBER_AF_PRINT_NAMES
  uint8_t i;
  for (i = 0; i < EMBER_AF_PRINT_NAME_NUMBER; i++) {
    emberSerialPrintfLine(EMBER_AF_PRINT_OUTPUT, "[%d] %p : %p",
                          i,
                          areaNames[i],
                          (emberAfPrintEnabled(
                             convertUserNumberAreaToInternalArea(i))
                           ? "YES"
                           : "no"));
    emberSerialWaitSend(EMBER_AF_PRINT_OUTPUT);
  }
#endif // EMBER_AF_PRINT_NAMES
}

void emberAfPrintAllOn(void)
{
#ifdef EMBER_AF_PRINT_BITS
  MEMSET(enablementBytes, 0xFF, sizeof(enablementBytes));
#endif
}

void emberAfPrintAllOff(void)
{
#ifdef EMBER_AF_PRINT_BITS
  MEMSET(enablementBytes, 0x00, sizeof(enablementBytes));
#endif
}

// These are CLI functions where a user will supply a sequential numbered
// area; as opposed to the bitmask area number that we keep track
// of internally.

void emberAfPrintOn(uint16_t userArea)
{
  printEnable(convertUserNumberAreaToInternalArea(userArea),
              true);   // enable?
}

void emberAfPrintOff(uint16_t userArea)
{
  printEnable(convertUserNumberAreaToInternalArea(userArea),
              false);  // enable?
}
