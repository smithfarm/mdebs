#include <stdarg.h>
#include "mdebs.h"
#include "dump.h"
#include "generalized.h"
#include "journal.h"
#include "messages.h"
#include "pgresfunc.h"
#include "query.h"

/*
 * Simple query function. Returns number of accounts satisfying request.
 * or -1 on error.
 */
int query_account_by_regexp(char *regexp)
{
  char *fname = "query_account_by_regexp()";
  int retval;
  char *buf;

  if (debugflag)
    fprintf(stderr, "Entering function query_account_by_regexp()\n");

  if (mdebsdb == (char *)NULL) { 
    /* MDEBSDB is not defined; what are we doing? */
    mdebs_err(MDEBSERR_NODBOPEN);
    return -1;
  }
  asprintf(&buf, "SELECT * FROM chart WHERE desig ~ '%s';", regexp);
  retval = runQuery(buf);
  if (retval == -1)
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}

/*
 * Function query_fiscal()
 *
 * Multipurpose function for querying the fiscal year.
 *
 */ 
struct date_range *query_fiscal(int flag)
{
  char *fname = "query_fiscal()";
  struct date_range *fisc;
  int retval;
  char *buf;

  if (debugflag)
    fprintf(stderr, "Entering function query_fiscal()\n");

  fisc = (struct date_range *) malloc (sizeof(struct date_range));

  buf = strdup("SELECT * FROM fiscyear;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return (struct date_range *)NULL;
  }
  free(buf);
  if (retval == 0) {
    mdebs_err(MDEBSERR_NOFISC);
    return (struct date_range *)NULL;
  }
  if (retval == 1) {
    fisc->datefr = NathanConvertDate(getField("startd"));
    fisc->dateto = NathanConvertDate(getField("endd"));
    if (flag == VERBO)
      mdebs_res(stderr, MDEBSRES_FISCIS, fisc->datefr, fisc->dateto);
    return fisc;
  }
  /* Since there should never be more than one record in the fiscyear
   * table, runQuery() > 1 is a fatal error */
  mdebs_err(MDEBSERR_MULTFISC);
  mdebs_err(MDEBSERR_FATALERR);
  exit_nicely();
}

/*
 * Function query_serial_by_date()
 * 
 * Function to determine largest serial number "on file" for a particular date
 * (returned by SELECT MAX(ser_num) FROM jou_head)
 *
 * Takes following argument(s):
 * 	thisdate	Date in YYYYMMDD format
 *
 * Returns:
 * 	-1 	on fatal error
 * 	0	if no serial numbers for that date
 *	otherwise, largest serial number encountered
 */
int query_serial_by_date(int thisdate)
{
  char *fname = "query_serial_by_date()";
  char *buf;
  int retval;

  if (debugflag)
    fprintf(stderr, "Entering function query_serial_by_date()\n");

  if (mdebsdb == (char *)NULL) { 
    /* MDEBSDB is not defined; what are we doing? */
    mdebs_err(MDEBSERR_NODBOPEN);
    return -1;
  }
  asprintf(&buf, "SELECT MAX(ser_num) FROM jou_head WHERE ent_date='%s';", pgConvertDate(thisdate));
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval == 0)
    return 0;
  if (retval == 1) /* Query returned 1 record */
    return atoi(getField("max"));

  assert ((1 + 1) != 2);
}

/*
 * query_shortcut()
 *
 * Takes a pointer to an integer (the shortcut code) and
 * a pointer to char (the shortcut itself)
 *
 * Possible calls:
 * code, NULL = look up shortcut by code
 * NULL, string = lookup shortcut by string
 * code, string = verify existence of this shortcut
 *
 * Returns:
 * struct short_cut of shortcut if found
 * (struct short_cut *)NULL if not found
 *
 * Special case:
 * If code is zero, desig in returned structure will be MDEBS_NOSCSTRING
 */
struct short_cut *
query_shortcut(int *code, char *shb)
{
  char *fname = "query_shortcut()";
  char *buf;
  struct short_cut *shb_out;
  int retval;

  if (debugflag)
    fprintf(stderr, "Entering function query_shortcut()\n");

  shb_out = (struct short_cut *) malloc (sizeof(struct short_cut));

  /* Case No. 1: code is NULL, desig is non-NULL */
  if (code == (int *)NULL && shb != (char *)NULL)
	asprintf(&buf, "SELECT * FROM shortcut WHERE desig = '%s';", 
	    backslash_single_quotes(shb));
  else 
  /* Case No. 2: code is non-NULL, desig is NULL */
    if (code != (int *)NULL && shb == (char *)NULL)
    {
    /* Sub-Case: We're looking up code zero (not in database) */
      if (*code == 0)
      {
        shb_out->code = 0;
	/* Don't need to allocate memory because desig is an array */
        strcpy(shb_out->desig, MDEBS_NOSCSTRING);
        return shb_out;
      }
    /* Sub-Case: We're looking up a non-zero code */
      asprintf(&buf, "SELECT * FROM shortcut WHERE code = %d;", *code);
    }
  else
  /* Case No. 3: both code and desig are non-NULL */
    asprintf(&buf, "SELECT * FROM shortcut WHERE code = %d AND desig = '%s';", 
	     code, backslash_single_quotes(shb));

  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return (struct short_cut *)NULL;
  }
  free(buf);
  if (retval == 0) {
    if (debugflag)
      fprintf(stderr, "query_shortcut(): Shortcut not found\n");
    return (struct short_cut *)NULL;
  }
  if (retval == 1) {
    shb_out->code = atoi(getField("code"));
    strcpy(shb_out->desig, getField("desig"));
    return shb_out;
  }
  fprintf(stderr, 
      "query_shortcut(): runQuery() returned unexpected value %d\n", retval);
  mdebs_err(MDEBSERR_FATALERR);
  exit_nicely();
}
