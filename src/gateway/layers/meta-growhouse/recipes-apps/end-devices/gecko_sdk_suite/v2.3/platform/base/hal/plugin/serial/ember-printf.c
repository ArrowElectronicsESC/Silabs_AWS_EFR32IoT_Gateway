/**
 * File: ember-printf.c
 * Description: Minimalistic implementation of printf().
 *
 *
 * Copyright 2014 by Silicon Labs.  All rights reserved.                   *80*
 */

#include PLATFORM_HEADER
#include "stack/include/ember-types.h"
#include "stack/include/error.h"

//Host processors do not use Ember Message Buffers.
#ifndef EZSP_HOST
  #include "stack/include/packet-buffer.h"
#endif

#include "hal/hal.h"
#include "serial.h"

#if CORTEXM3_EFM32_MICRO
#include "em_device.h"
#include "com.h"
#endif
#include "ember-printf.h"

#include <stdarg.h>

#ifdef EMBER_SERIAL_USE_STDIO
#include <stdio.h>
#endif //EMBER_SERIAL_USE_STDIO

#ifdef EMBER_SERIAL_CUSTOM_STDIO
#include EMBER_SERIAL_CUSTOM_STDIO
#define EMBER_SERIAL_USE_STDIO
#endif // EMBER_SERIAL_CUSTOM_STDIO

//=============================================================================
// Globals

// --------------------------------
// A simple printf() implementation
// Supported format specifiers are:
//  %% - percent sign
//  %c - single byte character
//  %s - ram string
//  %p - flash string  (non-standard)
//  %u - 2-byte unsigned decimal
//  %d - 2-byte signed decimal
//  %x %2x %4x - 1, 2, 4 BYTE hex value (always 0 padded) (non-standard)
//    Non-standard behavior: Normally a number after a % is interpreted to be
//    a minimum character width, and the value is not zero padded unless
//    there is a zero before the minimum width value.
//    i.e. '%2x' for the uint16_t value 0xb prints " b", while '%02x' would print
//    "0b".
//    Ember assumes the number after the % and before the 'x' to be the number
//    of BYTES, and all hex values are left-justified zero padded.
//
// A few macros and a function help make this readable:
//   - flush the local buffer to the output
//   - ensure that there is some room in the local buffer
//   - add a single byte to the local buffer
//   - convert a nibble to its ascii hex character
//   - convert an uint16_t to a decimal string
// Most of these only work within the emPrintfInternal() function.

// Current champion is %4x which writes 8 bytes.  (%s and %p can write
// more, but they do their own overflow checks).
#if CORTEXM3_EFM32_MICRO
#define LOCAL_BUFFER_SIZE 64
#else
#define LOCAL_BUFFER_SIZE 16
#endif
#define MAX_SINGLE_COMMAND_BYTES 8

static PGM uint32_t powers10[9] = {
  1000000000,
  100000000,
  10000000,
  1000000,
  100000,
  10000,
  1000,
  100,
  10
};

#define flushBuffer()                                                 \
  do { count = localBufferPointer - localBuffer;                      \
       if (flushHandler(port, localBuffer, count) != EMBER_SUCCESS) { \
         goto fail; }                                                 \
       total += count;                                                \
       localBufferPointer = localBuffer;                              \
       (void)localBufferPointer;                                      \
  } while (false)

#define addByte(byte) \
  do { *(localBufferPointer ++) = (byte); } while (false)

//=============================================================================

uint8_t *emWriteHexInternal(uint8_t *charBuffer, uint16_t value, uint8_t charCount)
{
  uint8_t c = charCount;
  charBuffer += charCount;
  for (; c != 0U; c--) {
    uint8_t n = value & 0x0F;
    value = value >> 4;
    *(--charBuffer) = n + (n < 10
                           ? '0'
                           : 'A' - 10);
  }
  return charBuffer + charCount;
}

// This function will write a decimal ASCII string to buffer
// containing the passed 'value'.  Includes negative sign, if applicable.
// Returns the number of bytes written.

uint8_t emDecimalStringWrite(uint32_t value,
                             bool signedValue,
                             uint8_t* buffer)
{
  uint8_t length = 0;
  if (signedValue && (value & 0x80000000UL)) {
    buffer[length++] = '-';
    // Take the absolute value.  We have already determined
    // whether the value is signed.  So we don't care
    // about the high bits anymore.
    // The range of 16-bit signed value is -32,768 <-> 32,767,
    // and for 32-bit it is -2147483648 <-> 2147483647,
    // so we must make an exception for -32,768 and -2147483648
    // which don't have a signed equivalent.
    if (value != 0x80000000UL) {
      value = (uint32_t) (-((int32_t) value));
    }
  }

  {
    uint8_t i;
    bool printedLeadingNonZeroValue = false;
    // To prevent using 32-bit divide or modulus,
    // since those operations are expensive on a 16-bit processor,
    // we use subtraction and a constant array with powers of 10.
    for (i = 0; i < 9; i++) {
      uint8_t digit = 0;
      while (value >= powers10[i]) {
        value -= powers10[i];
        digit++;
      }
      if (digit != 0 || printedLeadingNonZeroValue) {
        buffer[length++] = '0' + digit;
        printedLeadingNonZeroValue = true;
      }
    }
    buffer[length++] = '0' + value;
  }

  return length;
}

// Returns number of characters written
uint8_t emPrintfInternal(emPrintfFlushHandler flushHandler,
                         COM_Port_t port,
                         PGM_P string,
                         va_list args)
{
  uint8_t localBuffer[LOCAL_BUFFER_SIZE + MAX_SINGLE_COMMAND_BYTES];
  uint8_t *localBufferPointer = localBuffer;
  uint8_t *localBufferLimit = localBuffer + LOCAL_BUFFER_SIZE;
  uint8_t count;
  uint8_t total = 0;
  bool stillInsideFormatSpecifier = false;

  while (*string != '\0') {
    uint8_t next = *string;
    if (next != '%' && !(stillInsideFormatSpecifier)) {
      addByte(next);
    } else {
      if (stillInsideFormatSpecifier) {
        stillInsideFormatSpecifier = false;
      } else {
        string += 1;
      }
      switch (*string) {
        case '%':
          // escape for printing "%"
          addByte('%');
          break;
        case 'b': {
          const uint8_t *bytes = va_arg(args, const uint8_t *);
          uint32_t length = va_arg(args, unsigned int);
          uint32_t i;

          for (i = 0; i < length; i++) {
            localBufferPointer =
              emWriteHexInternal(localBufferPointer, bytes[i], 2);

            if (localBufferLimit <= localBufferPointer) {
              flushBuffer();
            }

            if (i < length - 1) {
              addByte(' ');
            }

            if (localBufferLimit <= localBufferPointer) {
              flushBuffer();
            }
          }
          break;
        }
        case 'c':
          // character
          addByte(va_arg(args, unsigned int) & 0xFF);
          break;
        case 'p':
          // only avr needs to special-case the pgm handling, all other current
          //  platforms fall through to standard string handling.
        #ifdef AVR_ATMEGA
          {
            // flash string
            PGM_P arg = va_arg(args, PGM_P);
            while (true) {
              uint8_t ch = *arg++;
              if (ch == '\0') {
                break;
              }
              *(localBufferPointer++) = ch;
              if (localBufferLimit <= localBufferPointer) {
                flushBuffer();
              }
            }
            break;
          }
        #endif
        case 's': {
          // string
          uint8_t len;
          uint8_t *arg = va_arg(args, uint8_t *);
          flushBuffer();
          for (len = 0; arg[len] != '\0'; len++) {
          }
          ;
          if (flushHandler(port, arg, len) != EMBER_SUCCESS) {
            goto fail;
          }
          total += len;
          break;
        }

        case 'l':       // 4-byte, %l and %ld = signed, %lu = unsigned
        case 'u':       // unsigned 2-byte
        case 'd': {     // signed 2-byte
          uint32_t value;
          bool signedValue;
          if (*string == 'l') {
            value = va_arg(args, long int);
            signedValue = (string[1] != 'u');
            if (string[1] == 'u' || string[1] == 'd') {
              string += 1;
            }
          } else if (*string == 'u') { // Need when sizeof(int) != sizeof(uint16_t)
            value = va_arg(args, unsigned int);
            signedValue = false;
          } else {
            value = va_arg(args, int);
            signedValue = true;
          }
          localBufferPointer += emDecimalStringWrite(value,
                                                     signedValue,
                                                     localBufferPointer);
          break;
        }
        case 'x':
        case 'X': {
          // single hex byte (always prints 2 chars, ex: 0A)
          uint8_t data = va_arg(args, int);

          localBufferPointer = emWriteHexInternal(localBufferPointer, data, 2);
          break;
        }

        case '0':
        case '1':
        case '3':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        // We don't support width specifiers, but we want to accept
        // them so they can be ignored and thus we are compatible with
        // code that tries to use them.

        case '2':
        // %2x only, 2 hex bytes (always prints 4 chars)
        case '4':
          // %4x only, 4 hex bytes (always prints 8 chars)

          stillInsideFormatSpecifier = true;
          if (*(string + 1) == 'x' || *(string + 1) == 'X') {
            string++;
            stillInsideFormatSpecifier = false;
            if (*(string - 1) == '2') {
              uint16_t data = va_arg(args, int);
              localBufferPointer = emWriteHexInternal(localBufferPointer, data, 4);
            } else if (*(string - 1) == '4') {
              uint32_t data = va_arg(args, uint32_t);
              // On the AVR at least, the code size is smaller if we limit the
              // emWriteHexInternal() code to 16-bit numbers and call it twice in
              // this case.  Other processors may have a different tradeoff.
              localBufferPointer = emWriteHexInternal(localBufferPointer,
                                                      (uint16_t) (data >> 16),
                                                      4);
              localBufferPointer = emWriteHexInternal(localBufferPointer,
                                                      (uint16_t) data,
                                                      4);
            } else {
              stillInsideFormatSpecifier = true;
              string--;
            }
          }

          break;
        case '\0':
          goto done;
        default: {
        }
      } //close switch.
    } // close else
    if (localBufferLimit <= localBufferPointer) {
      flushBuffer();
    }
    string++;
  } // close while

  done:
  flushBuffer();
  return total;

  fail:
  return 0;
}

EmberStatus emberSerialPrintBytes(uint8_t port,
                                  PGM_P prefix,
                                  const uint8_t *bytes,
                                  uint16_t count)
{
  EmberStatus status;
  uint16_t i;
  status = emberSerialWriteString(port, prefix);
  if (status != EMBER_SUCCESS) {
    return status;
  }
  for (i = 0; i < count; i++) {
    status = emberSerialPrintf(port,
                               "%x%s",
                               bytes[i],
                               (i < count - 1
                                ? " "
                                : ""));
    if (status != EMBER_SUCCESS) {
      return status;
    }
  }
  return status;
}

EmberStatus emberSerialPrintBytesLine(uint8_t port,
                                      PGM_P prefix,
                                      const uint8_t *bytes,
                                      uint16_t count)
{
  EmberStatus result = emberSerialPrintBytes(port, prefix, bytes, count);

  if (result != EMBER_SUCCESS) {
    return result;
  }

  return emberSerialPrintfLine(port, "");
}

EmberStatus emberSerialPrintCarriageReturn(uint8_t port)
{
  return emberSerialPrintf(port, "\r\n");
}

EmberStatus emberSerialPrintfLine(uint8_t port, PGM_P formatString, ...)
{
  EmberStatus stat;
  va_list ap;
  va_start(ap, formatString);
  stat = emberSerialPrintfVarArg(port, formatString, ap);
  va_end(ap);
  emberSerialPrintCarriageReturn(port);
  return stat;
}

EmberStatus emberSerialPrintf(uint8_t port, PGM_P formatString, ...)
{
  EmberStatus stat;
  va_list ap;
  va_start(ap, formatString);
  stat = emberSerialPrintfVarArg(port, formatString, ap);
  va_end(ap);
  return stat;
}

#ifndef EMBER_MANAGEMENT_SERIAL
  #if CORTEXM3_EFM32_MICRO

EmberStatus emberSerialWriteHex(uint8_t port, uint8_t dataByte)
{
  return COM_WriteHex((COM_Port_t) port, dataByte);
}

EmberStatus emberSerialPrintfVarArg(uint8_t port, PGM_P formatString, va_list ap)
{
  return COM_PrintfVarArg((COM_Port_t) port, formatString, ap);
}

  #else //CORTEXM3_EFM32_MICRO

EmberStatus emberSerialWriteHex(uint8_t port, uint8_t dataByte)
{
  uint8_t hex[2];
  emWriteHexInternal(hex, dataByte, 2);
  return emberSerialWriteData(port, hex, 2);
}

EmberStatus emberSerialPrintfVarArg(uint8_t port, PGM_P formatString, va_list ap)
{
  EmberStatus stat = EMBER_SUCCESS;

  #ifdef EMBER_SERIAL_USE_STDIO
  if (!emPrintfInternal(emberSerialWriteData, port, formatString, ap)) {
    stat = EMBER_ERR_FATAL;
  }
  #else //EMBER_SERIAL_USE_STDIO

  switch (emSerialPortModes[port]) {
  #ifdef EM_ENABLE_SERIAL_FIFO
    case EMBER_SERIAL_FIFO: {
      if (!emPrintfInternal(emberSerialWriteData, port, formatString, ap)) {
        stat = EMBER_ERR_FATAL;
      }
      break;
    }
  #endif
  #ifdef EM_ENABLE_SERIAL_BUFFER
    case EMBER_SERIAL_BUFFER: {
      EmberMessageBuffer buff = emberAllocateStackBuffer();
      if (buff == EMBER_NULL_MESSAGE_BUFFER) {
        stat = EMBER_NO_BUFFERS;
        break;
      }
      if (emPrintfInternal(emberAppendToLinkedBuffers,
                           buff,
                           formatString,
                           ap)) {
        stat = emberSerialWriteBuffer(port, buff, 0, emberMessageBufferLength(buff));
      } else {
        stat = EMBER_NO_BUFFERS;
      }
      // Refcounts may be manipulated in ISR if DMA used
      {
        DECLARE_INTERRUPT_STATE;
        DISABLE_INTERRUPTS();
        emberReleaseMessageBuffer(buff);
        RESTORE_INTERRUPTS();
      }

      break;
    }
  #endif
    default: {
    }
  }   //close switch.
  #endif //EMBER_SERIAL_USE_STDIO
  return stat;
}

  #endif //CORTEXM3_EFM32_MICRO
#endif //EMBER_MANAGEMENT_SERIAL
