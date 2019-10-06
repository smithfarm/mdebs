/*
 * dbinit.c
 *
 * Functions for initializing database.  
 *
 * Returns nothing.
 *
 */

#include "mdebs.h" /* First */

#include "generalized.h"
#include "messages.h"
#include "pgresfunc.h"

#include "dbinit.h" /* Last */

#define NOOFQUERIES 6

/*
 * dbinit_create()
 *
 * Function to set up a newly created database as an mdebs database by
 * creating the mdebs tables in it.  Please note that the CREATE TABLE
 * queries themselves are buried in the code below because I couldn't
 * figure out a viable way to put them into a #define precompiler directive
 * while at the same time preserving the functionality of having the
 * lengths of the varchar fields as macros (e.g. CHART_DESIG_LENGTH etc.,
 * see dbinit.h).
 * 
 * The function runs some sanity checks (Does database exist?  Is it really
 * empty?) before it attempts to create the tables. 
 *
 * This function is designed to be run using the command line
 *   $MDEBSDIR/mdebs --initialize
 *
 * No return value.
 */
void dbinit_create(char *db)
{
  char *query[NOOFQUERIES], buf;
  int i, retval;

  /* Open a connection to template1 */
  openBackend((char *)NULL);

  /* Verify that the parameter passed is an existing database */
  if (dbQuery(db) < 1) {
    mdebs_err(MDEBSERR_NONEXISTENTDB, db);
    closeBackend();
    return;
  }
  if (debugflag)
    fprintf(stderr, "dbinit_create(): Database ->%s<- exists. Good.\n", db);
  closeBackend();

  /* Close connection to template1 and open the database name supplied */
  openBackend(db);

  /* Determine if the given database already has non-system tables, i.e.
   * any tables that don't start with pg_ - this means the database is not
   * pristine */
  retval = runQuery(MDEBS_DB_CONTAIN_TABLES_QUERY);
  if (retval == -1) {
    mdebs_queryerr(retval, "dbinit_create()");
    closeBackend();
    return;
  }
  if (retval > 0) {
    /* Non-system tables found! Danger! */
    mdebs_err(MDEBSERR_TABLESFOUND, db);
    mdebs_err(MDEBSERR_CANTINIT, db);
    closeBackend();
    return;
  }
  if (debugflag)
    fprintf(stderr, "dbinit_create(): No non-system tables found in %s\n", db);

  /* OK, no non-system tables found. Proceed to create tables. */

  for (i=0; i<NOOFQUERIES; i++)
  {
    switch (i)
    {
      case (0):
asprintf(&query[i], "CREATE TABLE chart ( acct char(3), anal char(3), " 
		   "desig varchar(%d) );", CHART_DESIG_LENGTH);
break;
      case (1):
asprintf(&query[i], "CREATE TABLE jou_lines ( id int, ent_date date, "
   "ser_num int, debit bool, acct char(3), anal char(3), amount int );");
break;
      case (2):
asprintf(&query[i], "CREATE SEQUENCE %s;", MDEBS_SERIAL);
break;
      case (3):
asprintf(&query[i], "CREATE TABLE jou_head ( id int default nextval('%s') not null, "
   "ent_date date, ser_num int, desig varchar(%d), document varchar(%d), "
   "shortcut int );", MDEBS_SERIAL, JOURNAL_DESIG_LENGTH, 
   DOCUMENT_DESIG_LENGTH);
break;
      case (4):
asprintf(&query[i], "CREATE TABLE fiscyear ( startd date, endd date );");
break;
      case (5):
asprintf(&query[i], "CREATE TABLE shortcut ( code int, desig varchar(%d) );",
	 	   SHORTCUT_DESIG_LENGTH);
break;
    } /* switch */
  } /* for */

  if (!debugflag) { /* This is for real */

     /* Initialize the backend */
     openBackend(db);

     /* start a transaction block */
     startTransaction();

     /* run the queries */
     for (i=0; i<NOOFQUERIES; i++)
     {
       retval = runQuery(query[i]);
       if (retval == -1)
       {
          mdebs_queryerr(query[i], "dbinit_create()");
          free(query[i]);
	  rollbackTransaction();
	  closeBackend(db);
          return;
       }
     }

     /* End the transaction */
     endTransaction();

     /* Display a helpful message */
     mdebs_msg(MDEBSMSG_TABLESCREATED);
   }
   else { /* Just display the queries on stderr */
     
     for (i=0; i<NOOFQUERIES; i++)
     {
       fprintf(stderr, "%s\n", query[i]);
       free(query[i]);
     }
   }

   return;
}
