#ifndef _MBPOLL_RUNTIME_H_
#define _MBPOLL_RUNTIME_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include "mbpoll.h"

#define MBPOLL_RESULT_TEXT_SIZE 64

typedef struct xMbPollResultRow {
  int iReference;
  char sValue[MBPOLL_RESULT_TEXT_SIZE];
} xMbPollResultRow;

typedef struct xMbPollReadResult {
  xMbPollResultRow * pxRows;
  int iRowCount;
} xMbPollReadResult;

void vMbPollContextInit (xMbPollContext * ctx);
bool bMbPollFunctionUsesBinary (eFunctions eFunction);
bool bMbPollFunctionIsReadOnly (eFunctions eFunction);
int iMbPollRegisterCount (const xMbPollContext * ctx);
int iMbPollAllocateData (xMbPollContext * ctx, char * sError, size_t xErrorSize);
void vMbPollFreeData (xMbPollContext * ctx);
int iMbPollSetWriteValueString (xMbPollContext * ctx, int iIndex,
                                const char * sValue, char * sError,
                                size_t xErrorSize);
int iMbPollOpen (xMbPollContext * ctx, char * sError, size_t xErrorSize);
void vMbPollClose (xMbPollContext * ctx);
void vMbPollReadResultFree (xMbPollReadResult * xResult);
const char * sMbPollModeToStr (eModes eMode);
const char * sMbPollFunctionToStr (eFunctions eFunction);
int iMbPollDescribeConnection (const xMbPollContext * ctx, char * sBuffer,
                               size_t xBufferSize);
int iMbPollDescribeDataType (const xMbPollContext * ctx, char * sBuffer,
                             size_t xBufferSize);
int iMbPollFormatValue (const xMbPollContext * ctx, int iReference, int iIndex,
                        char * sBuffer, size_t xBufferSize);
int iMbPollReadOnce (xMbPollContext * ctx, int iSlaveAddr, int iStartRef,
                     xMbPollReadResult * xResult, char * sError,
                     size_t xErrorSize);
int iMbPollWriteOnce (xMbPollContext * ctx, int iSlaveAddr, int iStartRef,
                      char * sError, size_t xErrorSize);

#ifdef __cplusplus
}
#endif

#endif /* _MBPOLL_RUNTIME_H_ */
