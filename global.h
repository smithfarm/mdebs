/*
 * global.h
 *
 * File containing global variables that need to be accessible to all
 * functions.
 */

/*
 * command line options flags (global definitions)
 */
int verboseflag;
int debugflag; 		/* Regular debugging mode flag */
int promptflag;

/*
 * global content variables
 */

/* Database name (replaces dumb use of environment variable)
 * Space will be allocated using strndup() in mdebs.y */
char *mdebsdb;

