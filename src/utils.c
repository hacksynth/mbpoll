#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include "utils.h"

#ifdef DEBUG
#define PDEBUG(fmt,...) printf("utils.c:%d: %s(): " fmt, __LINE__, __func__, ##__VA_ARGS__)
#else
#define PDEBUG(...)
#endif

static const char * gFailureExitProgramName = "mbpoll";
static mbpoll_failure_exit_cleanup_fn gFailureExitCleanup = NULL;

// -----------------------------------------------------------------------------
void
vSetFailureExitContext (const char * program_name,
                        mbpoll_failure_exit_cleanup_fn cleanup_fn) {
  if (program_name && *program_name) {
    gFailureExitProgramName = program_name;
  }
  gFailureExitCleanup = cleanup_fn;
}

// -----------------------------------------------------------------------------
#ifndef SKIP_FAILURE_EXIT
void
vFailureExit (bool bHelp, const char *format, ...) {
  va_list va;

  va_start (va, format);
  fprintf (stderr, "%s: ", gFailureExitProgramName);
  vfprintf (stderr, format, va);
  if (bHelp) {
    fprintf (stderr, " ! Try -h for help.\n");
  }
  else {
    fprintf (stderr, ".\n");
  }
  va_end (va);
  fflush (stderr);
  if (gFailureExitCleanup) {
    gFailureExitCleanup ();
  }
  exit (EXIT_FAILURE);
}
#endif

// -----------------------------------------------------------------------------
int *
iGetIntList (const char * name, const char * sList, int * iLen) {
  // 12,3,5:9,45

  int * iList = NULL;
  int i, iFirst = 0, iCount = 0;
  bool bIsLast = false;
  const char * p = sList;
  char * endptr;

  PDEBUG ("iGetIntList(%s)\n", sList);

  // Count and verify the integer list
  while (*p) {

    i = strtol (p, &endptr, 0);
    if (endptr == p) {

      vSyntaxErrorExit ("Illegal %s value: %s", name, p);
    }
    p = endptr;
    PDEBUG ("Integer found: %d\n", i);

    if (*p == ':') {

      // i is the first of a range first:last
      if (bIsLast) {
        // Cannot have 2 ':' in a row!
        vSyntaxErrorExit ("Illegal %s delimiter: '%c'", name, *p);
      }
      PDEBUG ("Is First\n");
      iFirst = i;
      bIsLast = true;
    }
    else if ( (*p == ',') || (*p == 0)) {

      if (bIsLast) {
        int iRange, iLast;

        // i is the last of a range first:last
        iLast = MAX (iFirst, i);
        iFirst = MIN (iFirst, i);
        iRange = iLast - iFirst + 1;
        PDEBUG ("Is Last, add %d items\n", iRange);
        iCount += iRange;
        bIsLast = false;
      }
      else {

        iCount++;
      }
    }
    else {

      vSyntaxErrorExit ("Illegal %s delimiter: '%c'", name, *p);
    }

    if (*p) {

      p++; // Skip the delimiter
    }
    PDEBUG ("iCount=%d\n", iCount);
  }

  if (iCount > 0) {
    int iIndex = 0;

    // Allocation
    if ((size_t)iCount > SIZE_MAX / sizeof(int)) {
      vIoErrorExit ("Memory allocation size overflow for %s list", name);
    }
    iList = calloc (iCount, sizeof (int));
    if (iList == NULL) {
      vIoErrorExit ("Memory allocation failed for %s list", name);
    }

    // Assignment
    p = sList;
    while (*p) {

      i = strtol (p, &endptr, 0);
      p = endptr;

      if (*p == ':') {

        // i is the first of a range first:last
        iFirst = i;
        bIsLast = true;
      }
      else if ( (*p == ',') || (*p == 0)) {

        if (bIsLast) {

          // i is the last of a range first:last
          int iLast = MAX (iFirst, i);
          iFirst = MIN (iFirst, i);

          for (i = iFirst; i <= iLast; i++) {

            iList[iIndex++] = i;
          }
          bIsLast = false;
        }
        else {

          iList[iIndex++] = i;
        }
      }

      if (*p) {

        p++; // Skip the delimiter
      }
    }
#ifdef DEBUG
    // Removed dependency on ctx.bIsVerbose
    // if (ctx.bIsVerbose) {
    //   vPrintIntList (iList, iCount);
    //   putchar ('\n');
    // }
#endif
  }
  *iLen = iCount;
  return iList;
}

// -----------------------------------------------------------------------------
void
vPrintIntList (int * iList, int iLen) {
  int i;
  putchar ('[');
  for (i = 0; i < iLen; i++) {
    printf ("%d", iList[i]);
    if (i != (iLen - 1)) {
      putchar (',');
    }
    else {
      putchar (']');
    }
  }
}

// -----------------------------------------------------------------------------
int32_t lSwapLong(int32_t l, bool bIsBigEndian) {
    if (!bIsBigEndian) {
        return l;
    }
    int32_t ret;
    uint16_t tmp[2];
    uint16_t swapped[2];

    memcpy(tmp, &l, sizeof(l));

    swapped[0] = tmp[1];
    swapped[1] = tmp[0];

    memcpy(&ret, swapped, sizeof(ret));
    return ret;
}

// -----------------------------------------------------------------------------
float fSwapFloat(float f, bool bIsBigEndian) {
    if (!bIsBigEndian) {
        return f;
    }
    float ret;
    uint16_t tmp[2];
    uint16_t swapped[2];

    memcpy(tmp, &f, sizeof(f));

    swapped[0] = tmp[1];
    swapped[1] = tmp[0];

    memcpy(&ret, swapped, sizeof(ret));
    return ret;
}
