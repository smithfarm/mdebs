#include <stdarg.h>
#include "mdebs.h"
#include "messages.h"

/*
 * messages.c
 *
 * this file contains functions for printing out messages in mdebs
 */

/*
 * mdebs_err()
 */
void mdebs_err(const char *fmt, ...)
{
  va_list args;

  /* fprintf(stderr, "mdebs_err: "); */

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

/*
 * mdebs_res()
 */
void mdebs_res(FILE *fp, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vfprintf(fp, fmt, args);
  va_end(args);
}

/*
 * mdebs_msg()
 */
void mdebs_msg(const char *fmt, ...)
{
  va_list args;

  /*
   * If quiet command line option was given, we don't print any
   * mdebs_msg priority messages; only mdebs_err.
   */
  if (!verboseflag) return;

  fprintf(stderr, "mdebs_msg: ");

  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
}

/*
 * mdebs_queryerr() (reacts to a function returning -1)
 */
void mdebs_queryerr(char *quer, char *fname)
{
  if (debugflag) {
    mdebs_err(MDEBSERR_BACKENDERR, fname);
    mdebs_err(MDEBSERR_QUERYWAS, quer);
    mdebs_err(MDEBSERR_PAUSEPRESSKEY);
    getchar();
  }
}
