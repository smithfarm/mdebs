#include <stdio.h>		/* We use fprintf in this file */
#include <stdlib.h>		/* Forgot why we need this */
#include <getopt.h>		/* Needed for long options in main() */
#include <string.h>		/* Needed for strdup() */
#include "mdebs.h"		/* Global declarations */
#include "generalized.h"
#include "journal.h"		/* Journal declaration */
#include "messages.h"		/* Message declarations */
#include "optproc.h"		/* Options processing declaration */
#include "query.h"		/* Query declarations */

int i, j, k, l, stopflag;
char *s, t, u;
struct journal_entry jent;

/*
 * Ensure that the backend connection is set up to use DateStyle NonEuropean
 * (MM-DD-YYYY)
 */
int setDateStyle(void)
{
  char *fname = "setDateStyle()";
  char *buf;
  int retval;

  buf = strdup("SET DATESTYLE TO PostgreSQL,US;");
  retval = runQuery(buf);
  if (retval == -1)
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}

int showDateStyle(void)
{
  char *fname = "showDateStyle()";
  char *buf;
  int retval;

  buf = strdup("SHOW DateStyle;");
  retval = runQuery(buf);
  if (retval == -1)
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}

int main(int argc, char **argv)
{
  /*
   * Declare local variables
   */
  int c; /* Used in option processing loop */
  char *c_ptr; /* Temporary variable to hold database name */

  /* Initialize global program behavior flags */
  verboseflag = 1;
  promptflag = 1;

  /* Initialize global variable to hold database name */
  mdebsdb = (char *)NULL;
  c_ptr = (char *)NULL;

  /* opterr = 0; */
  stopflag = 0;

  /* Initialize jent to contain a journal entry */
  init_jent(&jent);

  /* Parse options */
  c_ptr = parse_options(argc, argv);

  if (debugflag) {
    fprintf(stderr, "The following non-debugging flags are set:\n");
    if (verboseflag)
      fprintf(stderr, "  Verbose flag\n");
    if (promptflag)
      fprintf(stderr, "  Prompt flag\n");
    if (!verboseflag && !promptflag)
      fprintf(stderr, "  None\n");
  }

  /* Write welcome message */
  mdebs_msg(MDEBSMSG_WELCOME, MDEBS_VERSION_NUMBER);

  /* Report verbose status if in verbose mode */
  mdebs_msg(MDEBSMSG_VERBOSE);
  if (!promptflag)
    mdebs_msg(MDEBSMSG_PROMPTISOFF);

  /*
   * If we were not given a database name on the command line, check the
   * MDEBSDB environment variable.  If it is empty, bail out with an error
   * message.
   */
  if (debugflag)
    fprintf(stderr, "c_ptr is set to %s\n", c_ptr);

  if (c_ptr == (char *)NULL) { /* Database name not specified in args */

    if (debugflag)
      fprintf(stderr, "Database name not specified in args (c_ptr == NULL)\n");

    c_ptr = query_environment();
    if (debugflag)
      fprintf(stderr, "Environment contains %s.\n", c_ptr);

    mdebs_msg(MDEBSMSG_DBFROMENV);

    if (c_ptr == (char *)NULL) {
      mdebs_err(MDEBSERR_DBNOTSPEC);
      exit(1);
    }
  }

  /*
   * c_ptr is now set to some kind of non-NULL string.
   */
  if (debugflag)
    fprintf(stderr, "Length of c_ptr (proposed mdebsdb) string is %d.\n",
              strlen(c_ptr));

  if (strlen(c_ptr) > 24) {
    mdebs_err(MDEBSERR_DBNAMTOOLONG);
    exit(1);
  }

  i = dbQueryfull(c_ptr);
  if (i < 0) {
    mdebs_err(MDEBSERR_BACKENDERR, "main()");
    exit_nicely();
  }
  if (!i) {
    mdebs_err(MDEBSERR_NONEXISTENTDB, c_ptr);
    exit(1);
  }
  mdebs_msg(MDEBSMSG_EXISTENTDB, c_ptr);

  /* Initialize backend */
  if (openBackend(c_ptr) != 1) {
    mdebs_err(MDEBSERR_BACKENDERR, "main()");
    exit_nicely();
  }

  if (!query_mdebsdb()) {
    mdebs_err(MDEBSERR_NOTMDEBSDB, c_ptr);
    exit_nicely();
  }

  /*
   * The open database appears to be a valid (version >= 0.03) mdebs database
   */
  mdebsdb = (char *)strndup(c_ptr, (size_t)25);
  setDateStyle();
  showDateStyle(); 
  mdebs_msg(MDEBSRES_CURRENTDB, mdebsdb);

  /*
   * Fire up the bison parser
   */
  while (!stopflag) {
    yyparse();
  }

  /*
   * Exit nicely
   */
  exit_nicely();
}

