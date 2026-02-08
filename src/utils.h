#ifndef _MBPOLL_UTILS_H_
#define _MBPOLL_UTILS_H_

#include <stdbool.h>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

void vFailureExit (bool bHelp, const char *format, ...);

#define vSyntaxErrorExit(fmt,...) vFailureExit(true,fmt,##__VA_ARGS__)
#define vIoErrorExit(fmt,...) vFailureExit(false,fmt,##__VA_ARGS__)

int * iGetIntList (const char * name, const char * sList, int * iLen);
void vPrintIntList (int * iList, int iLen);

#include <stdint.h>

/**
 * @brief Swaps the two 16-bit words of a 32-bit integer if big endian flag is set.
 *
 * @param l The 32-bit integer to process.
 * @param bIsBigEndian If true, the two 16-bit halves are swapped.
 * @return The processed 32-bit integer.
 */
int32_t lSwapLong(int32_t l, bool bIsBigEndian);

/**
 * @brief Swaps the two 16-bit words of a 32-bit float if big endian flag is set.
 *
 * @param f The float to process.
 * @param bIsBigEndian If true, the two 16-bit halves are swapped.
 * @return The processed float.
 */
float fSwapFloat(float f, bool bIsBigEndian);

#endif /* _MBPOLL_UTILS_H_ */
