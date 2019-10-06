/*
 * insert.c
 *
 * Functions for inserting things
 */

#include "mdebs.h"

#include "generalized.h"
#include "journal.h"
#include "messages.h"
#include "pgresfunc.h"
#include "query.h"

#include "insert.h"

int insert_fiscal(int start_date, int end_date)
{
  char *fname = "insert_fiscal()";
  char *buf;
  int retval;

  if (query_fiscal(QUIET)) {
    mdebs_err(MDEBSERR_FISCALREADY);
    return 0; 
  }

  if ((start_date < 19700000) ||
      (end_date   < 19700000) ||
      (start_date > 20991231) ||
      (end_date   > 20991231) ||
      (!valiDate(start_date)) ||
      (!valiDate(end_date))   ||
      (start_date > end_date)) {
    mdebs_err(MDEBSERR_FISCBADDATES, start_date, end_date);
    return 0;
  }

  asprintf(&buf, "INSERT INTO fiscyear VALUES ('%s', '%s');", 
      pgConvertDate(start_date), pgConvertDate(end_date));
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval == 1) {
    if (verboseflag) mdebs_msg(MDEBSRES_FISCIS, start_date, end_date);
    return 1;
  } else {
    return -1;
  }
}
    
int update_fiscal(int start_date, int end_date)
{
  char *fname = "update_fiscal()";
  char *buf;
  int retval;

  if ((start_date < 19700000) ||
      (end_date   < 19700000) ||
      (start_date > 20991231) ||
      (end_date   > 20991231) ||
      (!valiDate(start_date)) ||
      (!valiDate(end_date))   ||
      (start_date > end_date)) {
    mdebs_err(MDEBSERR_FISCBADDATES, start_date, end_date);
    return 0;
  }

  if (!query_fiscal(QUIET)) {
    mdebs_err(MDEBSERR_NOFISC);
    return 0; 
  }

  asprintf(&buf, "UPDATE fiscyear SET startd='%s', endd='%s';", 
      pgConvertDate(start_date), pgConvertDate(end_date));
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval == 1) {
    mdebs_msg(MDEBSRES_FISCIS, start_date, end_date);
    return 1;
  } else {
    return -1;
  }
}

int insert_chart(int acct1, int acct2, char *desig)
{
  char *fname = "insert_chart()";
  char *buf;
  int retval;

  if ((acct1 < 0 || acct1 > 999) ||
      (acct2 < 0 || acct2 > 999)) {
    mdebs_err(MDEBSERR_BADACCTNOFMT);
    return -1;
  }
  if (accountName(acct1, acct2) != (char *)NULL) {
    mdebs_err(MDEBSERR_ACCTALREADY, acct1, acct2);
    return -1;
  }
  mdebs_msg(MDEBSMSG_ATTINSACCT, acct1, acct2);
  asprintf(&buf, "INSERT INTO chart VALUES ('%03d', '%03d', '%s');", 
      acct1, acct2, desig);
  retval = runQuery(buf);
  if (retval == -1)
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}

int insert_shortcut(char *shb)
{
  char *fname = "insert_shortcut()";
  int sccode;
  struct short_cut *tmpbuff;
  char *scdesig, *buf;
  int retval;

  /*
   * Sanity checks
   */

  /* Backslash any single-quotes in shb to create new value, scdesig */
  scdesig = backslash_single_quotes(shb);

  /*
   * Look up the shortcut by name
   */
  tmpbuff = query_shortcut((int *)NULL, scdesig);
  if (tmpbuff != (struct short_cut *)NULL)
  { /* This shortcut is already in the database */
    free(tmpbuff);
    mdebs_err(MDEBSERR_SHORTALREADYNAME, tmpbuff->code);
    return 0;
  }
  free(tmpbuff);

  /*
   * Determine next available shortcut number
   */
  asprintf(&buf, "SELECT MAX(code) FROM shortcut;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval != 1) {
    mdebs_err(MDEBSERR_MAXSHCBADVAL, retval);
    return -1;
  }
  sccode = atoi(getField("max"));
  sccode++;  /* Increment the maximum value! */
  if (debugflag)
    fprintf(stderr, "Next available shortcut code is %d\n", sccode);

  asprintf(&buf, "INSERT INTO shortcut VALUES ('%d', '%s');", sccode, scdesig);
  retval = runQuery(buf);
  if (retval == -1)
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}
