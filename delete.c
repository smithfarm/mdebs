/*
 * delete.c
 *
 * Functions for deleting things
 */

#include "mdebs.h"

#include "generalized.h"
#include "journal.h"
#include "messages.h"
#include "query.h"

#include "delete.h"

/*
 * delete_fiscal() - Erases the fiscal year
 *
 * This function is used when the fiscal year is changed; the user cannot
 * actually delete the fiscal year because an initialized database must have
 * its fiscal year defined.
 *
 * Takes no arguments (there can only be one fiscal year associated with any
 * particular database).
 *
 * Returns: 	1	on success
 * 		0	on failed sanity check (probably should be fatal)
 *		-1	on failed SQL query
 */
int delete_fiscal()
{
  char *fname = "delete_fiscal()";
  int resultcode, noofrecs;
  int retval;
  char *buf;

  /*
   * Sanity Check: The fiscal year must be defined for deleting it to make
   * sense
   */
  if (!query_fiscal(QUIET)) {
    mdebs_err(MDEBSERR_NOFISC);
    return 0;
  }

  /*
   * The SQL Query: It's simple because fiscyear can only have one record
   */
  asprintf(&buf, "DELETE FROM fiscyear;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval == 1 ) {
    mdebs_msg(MDEBSMSG_FISCDELETED);
    return 1;
  }
  /* Should never get to here */
  assert (1 == 1);
}

/*
 * Function delete_account_by_number()
 *
 * Given an account number, deletes the account associated with that number
 * from the Chart of Accounts
 *
 * Takes 2 arguments:
 * 	acct1	Account major number
 * 	acct2 	Account minor number
 *
 * Returns:
 * 	-1	on failed sanity check
 * 	Otherwise, returns what runQuery returns
 */
int delete_account_by_number(int acct1, int acct2)
{
  char *fname = "delete_account_by_number()";
  char *buf;
  int n;

  /*
   * Sanity Checks
   */

  /* No. 1: If mdebsdb is not defined, there is no database connection. */
  if (mdebsdb == (char *)NULL) { 
    mdebs_err(MDEBSERR_NODBOPEN);
    return -1;
  }
  
  /* No. 2: Check for existence of account (if it doesn't exist, we can't
   * delete it */
  if (accountName(acct1, acct2) == (char *)NULL) {
    mdebs_err(MDEBSERR_NOSUCHACCT, acct1, acct2);
    return -1;
  }

  /* If we're in verbose mode, write a helpful message */
  mdebs_msg(MDEBSMSG_ATTDELACCT, acct1, acct2);

  /* The SQL Query; helpful message; return query return value */
  asprintf(&buf, "DELETE FROM chart WHERE acct='%03d' AND anal='%03d';", acct1, acct2);
  n = runQuery(buf);
  if (n == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
  }
  free(buf);
  assert (n == 1);
  mdebs_msg(MDEBSMSG_SUCCEEDED);
  return n;
}

/*
 * Function delete_journal_entry()
 *
 * Deletes a journal entry given its date and serial number.
 *
 * Takes 2 arguments:
 * 	datum	Integer date (YYYYMMDD)
 * 	serial	Serial number of journal entry
 *
 * Returns:
 *      -1 	On SQL backend choke
 * 	0	On failed sanity check
 * 	1	On success
 */
int delete_journal_entry(int datum, int serial)
{
  char *fname = "delete_journal_entry()";
  char *buf;
  int resultcode;

  /*
   * Sanity Checks
   */

  /* No. 1: Date provided must be valid */
  if (!valiDate(datum)) {
    mdebs_err(MDEBSERR_NOSUCHDATE, datum);
    return 0;
  }

  if (debugflag) fprintf(stderr, "The date seems to be OK.\n");

  /* No. 2: The date and serial number must correspond to an existing journal
   * entry, otherwise we would have nothing to delete. */
  if (!transQuery(datum, serial)) {
    mdebs_err(MDEBSERR_JOUNOEXIST, datum, serial);
    return 0;
  }

  if (debugflag) 
    fprintf(stderr, "%d:%03d seems to be an existing journal entry.\n", 
		    datum, serial);

  /* Start an atomic transaction */
  startTransaction();

  /* Build first SQL Query string; run query */
  asprintf(&buf, 
	"DELETE FROM jou_head WHERE ent_date='%s' and ser_num=%d;", 
	pgConvertDate(datum), serial);
  resultcode = runQuery(buf);
  if (resultcode == -1) /* Bad News (TM) !!! */
  {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return -1;
  }
  free(buf);
  assert (resultcode == 1);

  /* Build second SQL Query string; run query */
  asprintf(&buf, 
	"DELETE FROM jou_lines WHERE ent_date='%s' and ser_num=%d;", 
	pgConvertDate(datum), serial);

  if (debugflag) fprintf(stderr, "%s\n", buf);

  resultcode = runQuery(buf);
  if (resultcode == -1) /* Bad News (TM) !!! */
  {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return -1;
  }
  free(buf);
  assert (resultcode > 0);

  /* Close atomic transaction */
  endTransaction();
  
  /* Return 1 to indicate success (otherwise we don't make it this far) */
  return 1;
}

/*
 * delete_shortcut()
 *
 * Takes one argument
 * 	struct short_cut * Pointer to shortcut structure
 *
 * Returns:
 * 	0  on failed sanity check.
 * 	1  on success
 * 	-1 in case of backend error
 */
int delete_shortcut(int *code, char *desig)
{
  char *fname = "delete_shortcut()";
  int resultcode, noofrecs;
  int retval;
  struct short_cut *shp;
  char *buf;

  /*
   * Sanity Check No. 1: The shortcut to be deleted has to exist in the
   * shortcut table
   */

  shp = query_shortcut(code, desig);
  if (shp == (struct short_cut *)NULL) {
    if (code == (int *)NULL)
      mdebs_err(MDEBSERR_NOSUCHSHDESIG, desig);
    if (desig == (char *)NULL)
      mdebs_err(MDEBSERR_NOSUCHSHCODE, *desig);
    return 0;
  }
  print_shortcut(stderr, shp);

  /*
   * Sanity Check No. 2: The shortcut to be deleted must not exist in the
   * jou_head table
   */
  asprintf(&buf, "SELECT * FROM jou_head WHERE shortcut=%d;", shp->code);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval != 0) {
    mdebs_err(MDEBSERR_SHCINSTPREZ);
    return 0;
  }

  /*
   * The SQL Query: It's simple because fiscyear can only have one record
   */
  asprintf(&buf, "DELETE FROM shortcut WHERE code='%d' AND desig='%s';", 
		  shp->code, backslash_single_quotes(shp->desig));
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);
  if (retval == 1 ) {
    mdebs_msg(MDEBSMSG_SHCDELETED);
    return 1;
  }
  assert ((1 + 1) != 2);
  return -1;
}

