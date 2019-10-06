/*
 * mdebs.h
 *
 * This file contains declarations that are global to all mdebs functions
 */

/*
 * Macros
 */
#define MDEBS_VERSION_NUMBER    "0.09"
#define MDEBS_PROMPT            "mdebs-> "
#define MDEBS_PAGER_DIRECTORY	"/usr/bin/"
#define MDEBS_PAGER_COMMAND	"less -S"
#define MDEBS_PAGER_OPTION	"-C" /* Only one allowed at this point */
#define MDEBS_PRINT_COMMAND	"paps"
#define MDEBS_PRINT_OPTION	"--font \"Monospace 10\" --top-margin=72"
#define MDEBS_PRINT_PIPE_CMD	"lpr"  /* where we pipe paps output */
#define MDEBS_NOSCSTRING	"<NONE>" /* For shortcut code == 0 */
#define MDEBS_SERIAL		"jou_head_id_seq"

/*
 * Includes for all mdebs source files
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "boolean.h"

/*
 * command line options flags
 */
extern int verboseflag;
extern int debugflag;
extern int promptflag;

/*
 * global content variables
 */

/* Database name (replaces dumb use of environment variable)
 * Space will be allocated using strndup() in mdebs.y */
extern char *mdebsdb;

/* Magic numbers */
#define VERBO 1				/* VERBO tells fn to be verbose */
#define QUIET 0				/* QUIET tells fn to be quiet */

#define DUMP_TO_SCREEN  0
#define DUMP_TO_PRINTER 1
#define DUMP_TO_FILE	2		/* See dump.c dump_journal() */

/* Error codes */
#define MDEBS_ERROR_FATAL		-999

/* Fiscal year query (?) */
#define MDEBS_FISCQUE_START 1
#define MDEBS_FISCQUE_END   2

/* Size limits */
#define MAX_QUERY 4096

/*
 * Type declarations
 */

struct acctside {
  short int debit;
  int acct;
  int anal;
  int amount;
};

struct journal_entry {
  int ent_date;
  int ser_num;
  char document[20];
  char desig[80];
  int shortcut;
  struct acctside values[10];
};

struct date_range {
  int datefr;
  int dateto;
};

struct short_cut {
  int code;
  char desig[20];
};

