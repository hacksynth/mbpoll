/* Copyright (c) 2015-2026 Pascal JEAN, All rights reserved.
 *
 * mbpoll is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mbpoll is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mbpoll.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <modbus.h>

#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
#include <unistd.h>
#endif
#ifdef _WIN32
#include <windows.h>
#endif

#include "custom-rts.h"
#include "mbpoll-runtime.h"
#include "utils.h"

#define DUINT8(p,i) ((uint8_t *) (p))[i]
#define DUINT16(p,i) ((uint16_t *) (p))[i]
#define DINT32(p,i) ((int32_t *) (p))[i]
#define DFLOAT(p,i) ((float *) (p))[i]

static const char * sModeList[] = {
  "RTU",
  "TCP"
};
static const int iModeList[] = {
  eModeRtu,
  eModeTcp
};
static const char * sFunctionList[] = {
  "discrete output (coil)",
  "discrete input",
  "input register",
  "output (holding) register"
};
static const int iFunctionList[] = {
  eFuncCoil,
  eFuncDiscreteInput,
  eFuncInputReg,
  eFuncHoldingReg
};

static const char sUnknownStr[] = "unknown";
static const char sIntStr[] = "32-bit integer";
static const char sFloatStr[] = "32-bit float";
static const char sWordStr[] = "16-bit register";
static const char sLittleEndianStr[] = "(little endian)";
static const char sBigEndianStr[] = "(big endian)";

static void mb_delay (unsigned long d);
static void vSetError (char * sError, size_t xErrorSize, const char * sFormat, ...);
static const char * sEnumToStr (int iElmt, const int * iList,
                                const char ** psStrList, int iSize);
static int iMbPollDataSize (const xMbPollContext * ctx, size_t * xDataSize,
                            char * sError, size_t xErrorSize);
static int iMbPollFormatBufferValue (const xMbPollContext * ctx,
                                     const void * pvData, int iReference,
                                     int iIndex, char * sBuffer,
                                     size_t xBufferSize);
static int iMbPollParseInt (const char * sValue, int iBase, long * plValue,
                            char * sError, size_t xErrorSize);
static int iMbPollParseDouble (const char * sValue, double * pdValue,
                               char * sError, size_t xErrorSize);
static void vFormatStringByte (unsigned char c, char * sBuffer,
                               size_t xBufferSize);

void
vMbPollContextInit (xMbPollContext * ctx) {

  memset (ctx, 0, sizeof (*ctx));
  ctx->eMode = DEFAULT_MODE;
  ctx->eFunction = DEFAULT_FUNCTION;
  ctx->eFormat = eFormatDec;
  ctx->iSlaveCount = -1;
  ctx->iStartCount = -1;
  ctx->iCount = DEFAULT_NUMOFVALUES;
  ctx->iPollRate = DEFAULT_POLLRATE;
  ctx->dTimeout = DEFAULT_TIMEOUT;
  ctx->sTcpPort = DEFAULT_TCP_PORT;
  ctx->xRtu.baud = DEFAULT_RTU_BAUDRATE;
  ctx->xRtu.dbits = DEFAULT_RTU_DATABITS;
  ctx->xRtu.sbits = DEFAULT_RTU_STOPBITS;
  ctx->xRtu.parity = DEFAULT_RTU_PARITY;
  ctx->xRtu.flow = SERIAL_FLOW_NONE;
  ctx->bIsPolling = true;
  ctx->bIsWrite = true;
  ctx->bIsDefaultMode = true;
  ctx->iPduOffset = 1;
#ifdef MBPOLL_GPIO_RTS
  ctx->iRtsPin = -1;
#endif
}

bool
bMbPollFunctionUsesBinary (eFunctions eFunction) {

  return (eFunction == eFuncCoil) || (eFunction == eFuncDiscreteInput);
}

bool
bMbPollFunctionIsReadOnly (eFunctions eFunction) {

  return (eFunction == eFuncDiscreteInput) || (eFunction == eFuncInputReg);
}

int
iMbPollRegisterCount (const xMbPollContext * ctx) {

  if ( (ctx->eFormat == eFormatInt) || (ctx->eFormat == eFormatFloat)) {
    return ctx->iCount * 2;
  }
  return ctx->iCount;
}

int
iMbPollAllocateData (xMbPollContext * ctx, char * sError, size_t xErrorSize) {
  size_t xDataSize = 0;

  vMbPollFreeData (ctx);
  if (iMbPollDataSize (ctx, &xDataSize, sError, xErrorSize) != 0) {
    return -1;
  }

  ctx->pvData = calloc (1, xDataSize);
  if (ctx->pvData == NULL) {
    vSetError (sError, xErrorSize, "Memory allocation failed for data buffer");
    return -1;
  }
  return 0;
}

void
vMbPollFreeData (xMbPollContext * ctx) {

  free (ctx->pvData);
  ctx->pvData = NULL;
}

int
iMbPollSetWriteValueString (xMbPollContext * ctx, int iIndex,
                            const char * sValue, char * sError,
                            size_t xErrorSize) {
  long lValue;
  double dValue;

  if (ctx->pvData == NULL) {
    vSetError (sError, xErrorSize, "Data buffer is not allocated");
    return -1;
  }
  if ( (iIndex < 0) || (iIndex >= ctx->iCount)) {
    vSetError (sError, xErrorSize, "Write value index %d out of range", iIndex);
    return -1;
  }

  switch (ctx->eFunction) {

    case eFuncDiscreteInput:
    case eFuncInputReg:
      vSetError (sError, xErrorSize, "Unable to write read-only element");
      return -1;

    case eFuncCoil:
      if (iMbPollParseInt (sValue, 10, &lValue, sError, xErrorSize) != 0) {
        return -1;
      }
      if ( (lValue < 0) || (lValue > 1)) {
        vSetError (sError, xErrorSize,
                   "Coil value out of range (%ld), expected 0 or 1", lValue);
        return -1;
      }
      DUINT8 (ctx->pvData, iIndex) = (uint8_t) lValue;
      return 0;

    case eFuncHoldingReg:
      break;

    default:
      vSetError (sError, xErrorSize, "Unsupported Modbus function %d",
                 ctx->eFunction);
      return -1;
  }

  if (ctx->eFormat == eFormatInt) {
    if (iMbPollParseInt (sValue, 10, &lValue, sError, xErrorSize) != 0) {
      return -1;
    }
    if ( (lValue < INT32_MIN) || (lValue > INT32_MAX)) {
      vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
      return -1;
    }
    DINT32 (ctx->pvData, iIndex) = lSwapLong ((int32_t) lValue, ctx->bIsBigEndian);
    return 0;
  }

  if (ctx->eFormat == eFormatFloat) {
    if (iMbPollParseDouble (sValue, &dValue, sError, xErrorSize) != 0) {
      return -1;
    }
    if ( (dValue < -FLT_MAX) || (dValue > FLT_MAX)) {
      vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
      return -1;
    }
    DFLOAT (ctx->pvData, iIndex) =
      fSwapFloat ((float) dValue, ctx->bIsBigEndian);
    return 0;
  }

  if (ctx->eFormat == eFormatString) {
    vSetError (sError, xErrorSize, "You can use string format only for output");
    return -1;
  }

  if (iMbPollParseInt (sValue, 0, &lValue, sError, xErrorSize) != 0) {
    return -1;
  }

  if (ctx->eFormat == eFormatInt16) {
    if ( (lValue < INT16_MIN) || (lValue > INT16_MAX)) {
      vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
      return -1;
    }
    DUINT16 (ctx->pvData, iIndex) = (uint16_t) ((int16_t) lValue);
    return 0;
  }

  if ( (lValue < 0) || (lValue > UINT16_MAX)) {
    vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
    return -1;
  }
  DUINT16 (ctx->pvData, iIndex) = (uint16_t) lValue;
  return 0;
}

int
iMbPollOpen (xMbPollContext * ctx, char * sError, size_t xErrorSize) {
  uint32_t sec;
  uint32_t usec;

  if ( (ctx->sDevice == NULL) || (ctx->sDevice[0] == '\0')) {
    vSetError (sError, xErrorSize, "device or host parameter missing");
    return -1;
  }

  vMbPollClose (ctx);

  switch (ctx->eMode) {

    case eModeRtu:
      ctx->xBus = modbus_new_rtu (ctx->sDevice, ctx->xRtu.baud,
                                  ctx->xRtu.parity, ctx->xRtu.dbits,
                                  ctx->xRtu.sbits);
      break;

    case eModeTcp:
      ctx->xBus = modbus_new_tcp_pi (ctx->sDevice, ctx->sTcpPort);
      break;

    default:
      vSetError (sError, xErrorSize, "Unsupported mode %d", ctx->eMode);
      return -1;
  }

  if (ctx->xBus == NULL) {
    vSetError (sError, xErrorSize,
               "Unable to create the libmodbus context: %s",
               modbus_strerror (errno));
    return -1;
  }

  modbus_set_debug (ctx->xBus, ctx->bIsVerbose);

  if (ctx->bEnableMaxSlaveQuirk || ctx->bEnableReplyToBroadcastQuirk) {
    int iQuirks = 0;

    if (ctx->bEnableMaxSlaveQuirk) {
      iQuirks |= MODBUS_QUIRK_MAX_SLAVE;
    }
    if (ctx->bEnableReplyToBroadcastQuirk) {
      iQuirks |= MODBUS_QUIRK_REPLY_TO_BROADCAST;
    }
    if (modbus_enable_quirks (ctx->xBus, iQuirks) != 0) {
      vSetError (sError, xErrorSize, "Unable to enable quirk(s): %s",
                 modbus_strerror (errno));
      vMbPollClose (ctx);
      return -1;
    }
  }

  if ( (ctx->iRtuMode != MODBUS_RTU_RTS_NONE) && (ctx->eMode == eModeRtu) &&
       !ctx->bIsChipIo) {
#ifdef MBPOLL_GPIO_RTS
    if (ctx->iRtsPin >= 0) {
      double t = 11 / (double) ctx->xRtu.baud / 2 * 1e6;

      if (init_custom_rts (ctx->iRtsPin,
                           ctx->iRtuMode == MODBUS_RTU_RTS_UP) != 0) {
        vSetError (sError, xErrorSize, "Unable to set GPIO RTS pin: %d",
                   ctx->iRtsPin);
        vMbPollClose (ctx);
        return -1;
      }
      modbus_rtu_set_custom_rts (ctx->xBus, set_custom_rts);
      modbus_rtu_set_rts_delay (ctx->xBus, (int) t);
    }
#endif
    modbus_rtu_set_serial_mode (ctx->xBus, MODBUS_RTU_RS485);
    modbus_rtu_set_rts (ctx->xBus, ctx->iRtuMode);
  }

  if (modbus_connect (ctx->xBus) == -1) {
    vSetError (sError, xErrorSize, "Connection failed: %s",
               modbus_strerror (errno));
    vMbPollClose (ctx);
    return -1;
  }

  mb_delay (20);

  sec = (uint32_t) ctx->dTimeout;
  usec = (uint32_t) ((ctx->dTimeout - sec) * 1E6);
  modbus_set_response_timeout (ctx->xBus, sec, usec);
  return 0;
}

void
vMbPollClose (xMbPollContext * ctx) {

  if (ctx->xBus != NULL) {
    modbus_close (ctx->xBus);
    modbus_free (ctx->xBus);
    ctx->xBus = NULL;
  }
}

void
vMbPollReadResultFree (xMbPollReadResult * xResult) {

  if (xResult == NULL) {
    return;
  }
  free (xResult->pxRows);
  xResult->pxRows = NULL;
  xResult->iRowCount = 0;
}

const char *
sMbPollModeToStr (eModes eMode) {

  return sEnumToStr (eMode, iModeList, sModeList,
                     (int) (sizeof (iModeList) / sizeof (iModeList[0])));
}

const char *
sMbPollFunctionToStr (eFunctions eFunction) {

  return sEnumToStr (eFunction, iFunctionList, sFunctionList,
                     (int) (sizeof (iFunctionList) / sizeof (iFunctionList[0])));
}

int
iMbPollDescribeConnection (const xMbPollContext * ctx, char * sBuffer,
                           size_t xBufferSize) {

  if (ctx->eMode == eModeRtu) {
    return snprintf (sBuffer, xBufferSize,
                     "Modbus RTU | device %s | %s | timeout %.2f s | poll %d ms",
                     ctx->sDevice ? ctx->sDevice : "(unset)",
                     sSerialAttrToStr (&ctx->xRtu), ctx->dTimeout,
                     ctx->iPollRate);
  }

  return snprintf (sBuffer, xBufferSize,
                   "Modbus TCP | host %s | port %s | timeout %.2f s | poll %d ms",
                   ctx->sDevice ? ctx->sDevice : "(unset)",
                   ctx->sTcpPort ? ctx->sTcpPort : DEFAULT_TCP_PORT,
                   ctx->dTimeout, ctx->iPollRate);
}

int
iMbPollDescribeDataType (const xMbPollContext * ctx, char * sBuffer,
                         size_t xBufferSize) {

  switch (ctx->eFunction) {

    case eFuncDiscreteInput:
      return snprintf (sBuffer, xBufferSize, "discrete input");

    case eFuncCoil:
      return snprintf (sBuffer, xBufferSize, "discrete output (coil)");

    case eFuncInputReg:
      if (ctx->eFormat == eFormatInt) {
        return snprintf (sBuffer, xBufferSize, "%s %s, input register table",
                         sIntStr,
                         ctx->bIsBigEndian ? sBigEndianStr : sLittleEndianStr);
      }
      if (ctx->eFormat == eFormatFloat) {
        return snprintf (sBuffer, xBufferSize, "%s %s, input register table",
                         sFloatStr,
                         ctx->bIsBigEndian ? sBigEndianStr : sLittleEndianStr);
      }
      return snprintf (sBuffer, xBufferSize,
                       "%s, input register table", sWordStr);

    case eFuncHoldingReg:
      if (ctx->eFormat == eFormatInt) {
        return snprintf (sBuffer, xBufferSize,
                         "%s %s, output (holding) register table", sIntStr,
                         ctx->bIsBigEndian ? sBigEndianStr : sLittleEndianStr);
      }
      if (ctx->eFormat == eFormatFloat) {
        return snprintf (sBuffer, xBufferSize,
                         "%s %s, output (holding) register table", sFloatStr,
                         ctx->bIsBigEndian ? sBigEndianStr : sLittleEndianStr);
      }
      return snprintf (sBuffer, xBufferSize,
                       "%s, output (holding) register table", sWordStr);

    default:
      break;
  }

  return snprintf (sBuffer, xBufferSize, "%s", sUnknownStr);
}

int
iMbPollFormatValue (const xMbPollContext * ctx, int iReference, int iIndex,
                    char * sBuffer, size_t xBufferSize) {

  if (ctx->pvData == NULL) {
    if (xBufferSize > 0) {
      sBuffer[0] = '\0';
    }
    return -1;
  }
  return iMbPollFormatBufferValue (ctx, ctx->pvData, iReference, iIndex,
                                   sBuffer, xBufferSize);
}

int
iMbPollReadOnce (xMbPollContext * ctx, int iSlaveAddr, int iStartRef,
                 xMbPollReadResult * xResult, char * sError,
                 size_t xErrorSize) {
  int iStartReg;
  int iReadQty;
  size_t xDataSize = 0;
  void * pvData = NULL;
  int iRet;
  int i;
  int iStep = 1;

  if (xResult == NULL) {
    vSetError (sError, xErrorSize, "Read result buffer is missing");
    return -1;
  }
  if (ctx->xBus == NULL) {
    vSetError (sError, xErrorSize, "Modbus connection is not open");
    return -1;
  }
  vMbPollReadResultFree (xResult);

  if (iMbPollDataSize (ctx, &xDataSize, sError, xErrorSize) != 0) {
    return -1;
  }

  pvData = calloc (1, xDataSize);
  if (pvData == NULL) {
    vSetError (sError, xErrorSize, "Memory allocation failed for read buffer");
    return -1;
  }

  if (modbus_set_slave (ctx->xBus, iSlaveAddr) != 0) {
    vSetError (sError, xErrorSize, "Setting slave address failed: %s",
               modbus_strerror (errno));
    free (pvData);
    return -1;
  }

  ctx->iTxCount++;
  iReadQty = iMbPollRegisterCount (ctx);
  iStartReg = iStartRef - ctx->iPduOffset;

  switch (ctx->eFunction) {

    case eFuncDiscreteInput:
      iRet = modbus_read_input_bits (ctx->xBus, iStartReg, iReadQty,
                                     (uint8_t *) pvData);
      break;

    case eFuncCoil:
      iRet = modbus_read_bits (ctx->xBus, iStartReg, iReadQty,
                               (uint8_t *) pvData);
      break;

    case eFuncInputReg:
      iRet = modbus_read_input_registers (ctx->xBus, iStartReg, iReadQty,
                                          (uint16_t *) pvData);
      break;

    case eFuncHoldingReg:
      iRet = modbus_read_registers (ctx->xBus, iStartReg, iReadQty,
                                    (uint16_t *) pvData);
      break;

    default:
      iRet = -1;
      break;
  }

  if (iRet != iReadQty) {
    ctx->iErrorCount++;
    vSetError (sError, xErrorSize, "Read %s failed: %s",
               sMbPollFunctionToStr (ctx->eFunction), modbus_strerror (errno));
    free (pvData);
    return -1;
  }

  xResult->pxRows = calloc ((size_t) ctx->iCount, sizeof (*xResult->pxRows));
  if (xResult->pxRows == NULL) {
    vSetError (sError, xErrorSize, "Memory allocation failed for result rows");
    free (pvData);
    return -1;
  }

  if ( (ctx->eFormat == eFormatInt) || (ctx->eFormat == eFormatFloat)) {
    iStep = 2;
  }

  for (i = 0; i < ctx->iCount; i++) {
    xResult->pxRows[i].iReference = iStartRef + (i * iStep);
    iMbPollFormatBufferValue (ctx, pvData, xResult->pxRows[i].iReference, i,
                              xResult->pxRows[i].sValue,
                              sizeof (xResult->pxRows[i].sValue));
  }
  xResult->iRowCount = ctx->iCount;
  ctx->iRxCount++;
  free (pvData);
  return 0;
}

int
iMbPollWriteOnce (xMbPollContext * ctx, int iSlaveAddr, int iStartRef,
                  char * sError, size_t xErrorSize) {
  int iStartReg;
  int iNbReg;
  int iRet;

  if (ctx->xBus == NULL) {
    vSetError (sError, xErrorSize, "Modbus connection is not open");
    return -1;
  }
  if (ctx->pvData == NULL) {
    vSetError (sError, xErrorSize, "No write data available");
    return -1;
  }
  if (bMbPollFunctionIsReadOnly (ctx->eFunction)) {
    vSetError (sError, xErrorSize, "Unable to write read-only element");
    return -1;
  }

  iStartReg = iStartRef - ctx->iPduOffset;
  iNbReg = iMbPollRegisterCount (ctx);

  if (modbus_set_slave (ctx->xBus, iSlaveAddr) != 0) {
    vSetError (sError, xErrorSize, "Setting slave address failed: %s",
               modbus_strerror (errno));
    return -1;
  }

  ctx->iTxCount++;
  switch (ctx->eFunction) {

    case eFuncCoil:
      if (iNbReg == 1) {
        iRet = modbus_write_bit (ctx->xBus, iStartReg, DUINT8 (ctx->pvData, 0));
      }
      else {
        iRet = modbus_write_bits (ctx->xBus, iStartReg, iNbReg,
                                  (const uint8_t *) ctx->pvData);
      }
      break;

    case eFuncHoldingReg:
      if ( (iNbReg == 1) && (!ctx->bWriteSingleAsMany)) {
        iRet = modbus_write_register (ctx->xBus, iStartReg,
                                      DUINT16 (ctx->pvData, 0));
      }
      else {
        iRet = modbus_write_registers (ctx->xBus, iStartReg, iNbReg,
                                       (const uint16_t *) ctx->pvData);
      }
      break;

    default:
      iRet = -1;
      break;
  }

  if (iRet == iNbReg) {
    ctx->iRxCount++;
    return 0;
  }

  ctx->iErrorCount++;
  vSetError (sError, xErrorSize, "Write %s failed: %s",
             sMbPollFunctionToStr (ctx->eFunction), modbus_strerror (errno));
  return -1;
}

static void
mb_delay (unsigned long d) {

  if (d) {
#if defined (__unix__) || (defined (__APPLE__) && defined (__MACH__))
    struct timespec dt;

    dt.tv_nsec = (d % 1000UL) * 1000000UL;
    dt.tv_sec = d / 1000UL;
    nanosleep (&dt, NULL);
#else
    Sleep (d);
#endif
  }
}

static void
vSetError (char * sError, size_t xErrorSize, const char * sFormat, ...) {
  va_list va;

  if ( (sError == NULL) || (xErrorSize == 0)) {
    return;
  }

  va_start (va, sFormat);
  vsnprintf (sError, xErrorSize, sFormat, va);
  va_end (va);
}

static const char *
sEnumToStr (int iElmt, const int * iList, const char ** psStrList, int iSize) {
  int i;

  for (i = 0; i < iSize; i++) {
    if (iElmt == iList[i]) {
      return psStrList[i];
    }
  }
  return sUnknownStr;
}

static int
iMbPollDataSize (const xMbPollContext * ctx, size_t * xDataSize,
                 char * sError, size_t xErrorSize) {
  size_t xSize = (size_t) ctx->iCount;

  switch (ctx->eFunction) {

    case eFuncCoil:
    case eFuncDiscreteInput:
      break;

    case eFuncInputReg:
    case eFuncHoldingReg:
      if ( (ctx->eFormat == eFormatInt) || (ctx->eFormat == eFormatFloat)) {
        if (xSize > SIZE_MAX / sizeof (float)) {
          vSetError (sError, xErrorSize,
                     "Data buffer size overflow (count=%zu, multiplier=4)",
                     xSize);
          return -1;
        }
        xSize *= sizeof (float);
      }
      else {
        if (xSize > SIZE_MAX / sizeof (uint16_t)) {
          vSetError (sError, xErrorSize,
                     "Data buffer size overflow (count=%zu, multiplier=2)",
                     xSize);
          return -1;
        }
        xSize *= sizeof (uint16_t);
      }
      break;

    default:
      vSetError (sError, xErrorSize, "Unsupported Modbus function %d",
                 ctx->eFunction);
      return -1;
  }

  *xDataSize = xSize;
  return 0;
}

static int
iMbPollFormatBufferValue (const xMbPollContext * ctx, const void * pvData,
                          int iReference, int iIndex, char * sBuffer,
                          size_t xBufferSize) {
  uint16_t v;
  char sFirst[8];
  char sSecond[8];

  (void) iReference;
  switch (ctx->eFormat) {

    case eFormatBin:
      return snprintf (sBuffer, xBufferSize, "%c",
                       (DUINT8 (pvData, iIndex) != 0U) ? '1' : '0');

    case eFormatDec:
      v = DUINT16 (pvData, iIndex);
      if (v & 0x8000) {
        return snprintf (sBuffer, xBufferSize, "%u (%d)", v, (int) (int16_t) v);
      }
      return snprintf (sBuffer, xBufferSize, "%u", v);

    case eFormatInt16:
      return snprintf (sBuffer, xBufferSize, "%d",
                       (int) (int16_t) DUINT16 (pvData, iIndex));

    case eFormatHex:
      return snprintf (sBuffer, xBufferSize, "0x%04X",
                       DUINT16 (pvData, iIndex));

    case eFormatString:
      vFormatStringByte ((unsigned char) (DUINT16 (pvData, iIndex) / 256),
                         sFirst, sizeof (sFirst));
      vFormatStringByte ((unsigned char) (DUINT16 (pvData, iIndex) % 256),
                         sSecond, sizeof (sSecond));
      return snprintf (sBuffer, xBufferSize, "%s%s", sFirst, sSecond);

    case eFormatInt:
      return snprintf (sBuffer, xBufferSize, "%d",
                       lSwapLong (DINT32 (pvData, iIndex), ctx->bIsBigEndian));

    case eFormatFloat:
      return snprintf (sBuffer, xBufferSize, "%g",
                       fSwapFloat (DFLOAT (pvData, iIndex), ctx->bIsBigEndian));

    default:
      break;
  }

  if (xBufferSize > 0) {
    sBuffer[0] = '\0';
  }
  return -1;
}

static int
iMbPollParseInt (const char * sValue, int iBase, long * plValue,
                 char * sError, size_t xErrorSize) {
  char * pEnd = NULL;
  long lValue;

  errno = 0;
  lValue = strtol (sValue, &pEnd, iBase);
  if (pEnd == sValue || *pEnd != '\0') {
    vSetError (sError, xErrorSize, "Illegal data value: %s", sValue);
    return -1;
  }
  if (errno == ERANGE) {
    vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
    return -1;
  }

  *plValue = lValue;
  return 0;
}

static int
iMbPollParseDouble (const char * sValue, double * pdValue,
                    char * sError, size_t xErrorSize) {
  char * pEnd = NULL;
  double dValue;

  errno = 0;
  dValue = strtod (sValue, &pEnd);
  if (pEnd == sValue || *pEnd != '\0') {
    vSetError (sError, xErrorSize, "Illegal data value: %s", sValue);
    return -1;
  }
  if (errno == ERANGE || !isfinite (dValue)) {
    vSetError (sError, xErrorSize, "Data value out of range: %s", sValue);
    return -1;
  }

  *pdValue = dValue;
  return 0;
}

static void
vFormatStringByte (unsigned char c, char * sBuffer, size_t xBufferSize) {

  if (isprint (c)) {
    snprintf (sBuffer, xBufferSize, "%c", c);
  }
  else {
    snprintf (sBuffer, xBufferSize, "\\x%02X", c);
  }
}
