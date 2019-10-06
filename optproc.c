#include <stdio.h>		/* We use fprintf in this file */
#include <stdlib.h>		/* Forgot why we need this */
#include <getopt.h>		/* Needed for long options in main() */
#include <string.h>		/* Needed for strdup() */
#include "mdebs.h"		/* Global declarations */
#include "mdebs_bf.h"		/* flexdebug is defined here */
#include "messages.h"
#include "pgresfunc.h"		/* pgdebug is defined here */
#include "optproc.h"

extern char version[];

void mdebs_print_version()
{
  printf("%s\n", version);
  printf("Copyright (C) Nathan L. Cutler\n");
  printf("mdebs comes with NO WARRANTY, to the extent permitted by law.\n");
  printf("You may redistribute copies of mdebs under the terms\n");
  printf("of the GNU General Public License.\n");
}

void mdebs_print_help()
{
  printf("mdebs [options]\n");
  printf("  --help|-h      This message\n");
  printf("  --version|-v   Version information\n");
  printf("  --no-prompt|-p Do not display a prompt\n");
  printf("  --quiet|-q     Relatively quiet operation\n");
  printf("  --debug|-d     Debug mode (see documentation)\n");
  printf("  --db|-D        (takes arg) Database to use\n");
  printf("  --validate     (takes arg) Validate an mdebs-format date\n");
  printf("  --convert      (takes arg) Convert mdebs-format date to MM-DD-YYYY\n");
  printf("  --initialize   Initialize a new mdebs database\n");
  printf("  --existdb      (takes arg) Determine if a database exists\n");
  printf("  --vernum       Output version number of this binary\n");
  printf("\n");
  printf("(Only the options with short equivalents are meant for active use.  The\n");
  printf("long-form-only options are meant for use in shell scripts.)\n\n");
  printf("mdebs will try to open the database name in the MDEBSDB environment\n");
  printf("variable, if it is set.  This is overrided by the --db command-line\n");
  printf("parameter.\n");
  printf("\n");
  printf("Please direct bug reports and other comments to \n");
  printf("Nathan L. Cutler <livingston@iol.cz>\n");
}

char *
parse_options(argc, argv)
  	int argc;
	char **argv;
{

  int c;			/* Return value from getopt_long */
  int i;			/* Temporary variable */
  char *c_ptr = (char *)NULL;	/* Temp. variable to hold dbname */

  /*
   * Parse options
   */
  while (1) {
    static struct option long_options[] =
      {
	{"quiet",      no_argument,       &verboseflag, 0},
	{"no-prompt",  no_argument,       &promptflag,  0},
	{"debug",      optional_argument, (int *)NULL,  'd'},
	{"db",         required_argument, (int *)NULL,  'D'},
        {"version",    no_argument,       (int *)NULL,  'v'},
	{"help",       no_argument,       (int *)NULL,  'h'},
	{"validate",   required_argument, (int *)NULL,  6},
	{"convert",    required_argument, (int *)NULL,  7},
	{"initialize", no_argument,       (int *)NULL,  8},
	{"existdb",    required_argument, (int *)NULL,  9},
	{"vernum",     no_argument,       (int *)NULL,  10},
	{0, 0, 0, 0}
      }; /* static struct */

    /* `getopt_long' stores the option index here. */
    int option_index = 0;

    c = getopt_long (argc, argv, "qpd:D:vh", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) break;

    switch (c) {

      /*
       * Long options that just set a flag
       */
      case (0):
	if (debugflag) {
	  fprintf(stderr, "Long option --%s encountered\n",
	  long_options[option_index].name);
	}
        break;

      /*
       * Short options that just set a flag (but no flag has been set yet
       * because the short version of the option was given.
       */
      case ('q'):
        verboseflag = 0;
        break;
      case ('p'):
        promptflag = 0;
        break;
      
      /*
       * Long options without short equivalents
       */
      case (6): /* Validate date */
        if (!valiDate(atoi(optarg)))
          exit(1);
        exit(0);
      case (7): /* Convert date */
        printf("%s", pgConvertDate(atoi(optarg)));
	exit(0);
      case (8): /* Initialize db */
        /* Allow for things like 
	 *    mdebs --db="some_other_database" --initialize
	 */
	if (c_ptr == (char *)NULL) {
	  if ((char *)query_environment() == (char *)NULL)
	  {
	    fprintf(stderr, "You must either define MDEBSDB in environment or specify --db on command line\n");
	    exit(0);
	  }
	  dbinit_create(query_environment());
	}
	else
	  dbinit_create(c_ptr);
        exit(0);
      case (9): /* Database existence check */
        i = dbQueryfull(optarg);
	if (i < 0) {
	  mdebs_err(MDEBSERR_BACKENDERR);
	  exit(-1);
	}
        if (!i) {
	  mdebs_msg(MDEBSMSG_NONEXISTENTDB, optarg);
	  exit(1);
	}
	mdebs_msg(MDEBSMSG_EXISTENTDB, optarg);
	exit(0);
      case (10): /* Print version number to stdout */
        printf("%s", MDEBS_VERSION_NUMBER);
	exit(0);

      /*
       * Long/short options that take arguments and/or do other stuff
       */
      case ('d'):
	/* Set up debugging mode:
	 *   No argument means regular debugging mode
	 *   Otherwise the option argument is processed as in chmod:
	 *     1 = regular debugging mode only
	 *     2 = postgres function debugging mode only
	 *     4 = lexanalyzer debugging mode only
	 *     sum of any two or all three of the above for combinations
	 */
	fprintf(stderr, "optarg is ->%s<-\n", optarg);
	if (optarg == (char *)NULL) {
	  /* No argument */
	  fprintf(stderr, "Debug option encountered with no argument\n");
	  debugflag = 1; pgdebug = 0; flexdebug = 0;
	  goto d_done;
	}
/* fprintf(stderr, "Debug option encountered with argument %s\n", optarg); */
	/* Argument provided; convert to integer */
	i = atoi(optarg);
/* fprintf(stderr, "Conversion of arg to int resulted in %d\n", i); */
	if (i < 1 || i > 7) {
	  fprintf(stderr, "Invalid argument to -d|--debug, "
	      		  "assuming regular debugging mode.\n");
	  debugflag = 1; pgdebug = 0; flexdebug = 0;
	  goto d_done;
	}
	if (i & 04) {
	  /* Lexanalyzer debugging chosen. */
	  flexdebug = 1;
	} else {
	  flexdebug = 0;
	}
	if (i & 02) {
	  /* Postgres function debugging chosen. */
	  pgdebug = 1;
	} else {
	  pgdebug = 0;
	}
	if (i & 01) {
	  /* Regular debugging mode chosen. */
	  debugflag = 1;
	} else {
	  debugflag = 0;
	}
d_done:
        fprintf(stderr, "The following debugging flags are set:\n");
        if (debugflag)
          fprintf(stderr, "  Regular debugging mode flag\n");
        if (pgdebug)
          fprintf(stderr, "  Postgres function debugging mode flag\n");
        if (flexdebug)
          fprintf(stderr, "  Lexical analyzer debugging mode flag\n");
	break;
      case ('D'):
        c_ptr = strdup(optarg);
	break;
      case ('v'):
        mdebs_print_version();
	exit(0);
      case ('h'):
	mdebs_print_help();
	exit(0);
      case ('?'):
        /* `getopt_long' has already printed error message (really?) */
	break;
      default:
        fprintf(stderr, "main(): Programmer failure! Option processing switch fell through.\n");
	exit(1);
    } /* switch */
  } /* while */

  return c_ptr;
}

