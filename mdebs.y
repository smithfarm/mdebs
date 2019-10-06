%{
/*
 * mdebs.y
 *
 * This is the core of mdebs.  It is a .y file that is processed by bison.
 * See "info bison" for details.
 */

#include <stdio.h>		/* We use fprintf in this file */
#include <stdlib.h>		/* Forgot why we need this */
#include <string.h>		/* Needed for strdup() */
#include "global.h"		/* Global definitions */
#include "mdebs.h"		/* Global declarations */
#include "mdebs_bf.h"		/* Use by both mdebs.y and mdebs.yy */
#include "generalized.h"
#include "messages.h"
#include "journal.h"		/* Journal entry structure declaration */
#include "query.h"

#define ENDS_IMPROPER 111
#define ENDS_PROPER   222

#define DUMP_ACCOUNT  5
#define DUMP_JOURNAL  6

int i, j, k, l, stopflag, querygood, ject;
char hdr[MAX_QUERY];
char query[MAX_QUERY];
char countquery[MAX_QUERY];
int ending, shave; 
int da_flags, cmdspec;
char *tmpbuf1, *tmpbuf2;
int *date_from, *date_to;
int *acctptr, *analptr;
char *docptr, *desigptr;
int *shcode, *shptr;
char *shdesig;
char *s, t, u;
char *shortcut_buff;
struct journal_entry jent;
struct journal_entry *jeptr;
struct short_cut *shcut_ptr;
struct date_range *dates, *fisc;

/*
 * mdebs_que_sho()
 *
 * Internal function to mdebs.y to avoid duplicated code
 * Takes no arguments, but uses the pointers shcode and shdesig, which are
 * declared local to this file
 */
int mdebs_que_sho(void)
{
  if (debugflag) 
  {
    if (shcode == (int *)NULL) 
      fprintf(stderr, "Querying shortcut ->%s<-...\n", shdesig);
    else if (shdesig == (char *)NULL) 
      fprintf(stderr, "Querying shortcut ->%d<-...\n", *shcode);
    else 
      fprintf(stderr, "Arguments to QUERY SHORTCUT incorrectly parsed\n");
  }
  shcut_ptr = query_shortcut(shcode, backslash_single_quotes(shdesig));
  free(shcode); free(shdesig);
  if (debugflag)
    fprintf(stderr, "mdebs.y: Returned from query_shortcut()\n");
  if (shcut_ptr == (struct short_cut *)NULL) {
    return 0;
  } else {
    if (debugflag)
      fprintf(stderr, "mdebs.y: Shortcut No. %d is %s.\n", 
              shcut_ptr->code, shcut_ptr->desig);
    return 1;
  }
}

%}

%union {
  int num;
  char str[TOKENLEN];
}

%token <str> TOK_STRING
%token <num> TOK_NUMBER
%token TOK_ACCTNUM
%token TOK_JOUSPEC
%token SHORTCUT
%token DOCUMENT
%token DOCU_REGEX
%token DESIGNATION
%token DESIG_REGEX
%token CMD_BYE
%token CMD_HEL
%token CMD_QUE
%token CMD_DUM
%token CMD_ALL
%token CMD_INS
%token CMD_DEL
%token CMD_CHG
%token CMD_CHA
%token CMD_FIS
%token CMD_JOU
%token CMD_ACC
%token CMD_DB
%token CMD_DATESTYLE
%token CMD_BINARY
%token CMD_SHO
%token CMD_PRI
%token CMD_COMMENT
%token CMD_AND
%token CMD_OR
%%

input:	line {
	  /* There can be two kinds of input.  Valid input or error. */
	  /* Valid input, represented by `line', is dealt with below. */
	  /* In the case of a syntax error we write a message, bison */
	  /* calls yyerror() (in our case all we do is flush the input */
	  /* buffer so YY_INPUT is called again), and the parser is */
	  /* restarted. */
	  }
	| error {
	    mdebs_err(MDEBSERR_PARSEERR);
	    YYABORT;
	  }
	;

line: 	cmd {
	    YYACCEPT;
	  }
	| journal_entry '\n' { 
            /* Run sanity checks on the journal entry */
            if (!verify_journal_entry(&jent))
            {
              mdebs_err(MDEBSERR_INVALIDJOU);
	      init_jent(&jent); /* Reinitialize jent for next journal entry */
              YYABORT;
            }
	    write_journal_entry(&jent);
	    print_journal_entry(stdout, jent);
	    init_jent(&jent); /* Reinitialize jent for next journal entry */
	    YYACCEPT;
	  }
	| CMD_COMMENT {
	    /* Comment */
	    YYACCEPT;
	  }
	| '\n' {
	    YYACCEPT;
	  }
	;

journal_entry: header operation { }
	| journal_entry operation { }
	;

header: TOK_NUMBER TOK_STRING TOK_STRING '\n' { 
  /* date + document + designation (no shortcut) */
          if (debugflag)
	    fprintf(stderr, "mdebs.y: Journal entry header encountered\n");
	  /* First line of journal entry */
	  ject = 0; /* Initialize journal entry counter */
	  jent.ent_date = $1;
	  strcpy(jent.document, backslash_single_quotes($2));
	  strcpy(jent.desig, backslash_single_quotes($3));
	  jent.shortcut = 0;
	}
	| TOK_NUMBER TOK_STRING TOK_STRING TOK_STRING '\n' {
  /* date + document + designation + shortcut (string) */
	  /* First line of journal entry */
	  ject = 0; /* Initialize journal entry counter */
	  jent.ent_date = $1;
	  strcpy(jent.document, backslash_single_quotes($2));
	  strcpy(jent.desig, backslash_single_quotes($3));
	  if (query_shortcut((int *)NULL, $4) == (struct short_cut *)NULL)
	  {
	    mdebs_err(MDEBSERR_NOSUCHSHDESIG, $4);
	    YYABORT;
	  }
	  jent.shortcut = query_shortcut((int *)NULL, $4)->code;
	}
	| TOK_NUMBER TOK_STRING TOK_STRING TOK_NUMBER '\n' {
  /* date + document + designation + shortcut (number) */
	  /* First line of journal entry */
	  ject = 0; /* Initialize journal entry counter */
	  jent.ent_date = $1;
	  strcpy(jent.document, backslash_single_quotes($2));
	  strcpy(jent.desig, backslash_single_quotes($3));
	  if (query_shortcut(&$4, (char *)NULL) == (struct short_cut *)NULL)
	  {
	    mdebs_err(MDEBSERR_NOSUCHSHCODE, $4);
	    YYABORT;
	  }
	  jent.shortcut = (int)$4;
	}
;

operation: TOK_ACCTNUM TOK_NUMBER TOK_NUMBER '\n' { 
  /* account number + debit amount + credit amount */	
	  if ($2 == 0 && $3 != 0) /* This is a credit entry */
	  {
	    jent.values[ject].debit = false;
	    jent.values[ject].acct = bf_acct;
	    jent.values[ject].anal = bf_anal;
	    jent.values[ject].amount = $3;
	  }
	  else if ($3 == 0 && $2 != 0) /* This is a debit entry */
	  {
	    jent.values[ject].debit = true;
	    jent.values[ject].acct = bf_acct;
	    jent.values[ject].anal = bf_anal;
	    jent.values[ject].amount = $2;
	  }
	  else /* Neither $2 nor $3 is 0, or both are 0 -- bad news */
	  {
	    mdebs_err(MDEBSERR_DEBCREDZERO);
	    init_jent(&jent);
	    YYABORT;
	  }
	  ject++; /* Increment journal entry counter */
  	  if (debugflag)
	    fprintf(stderr, "%03d-%03d %d %d\n", bf_acct, bf_anal, $2, $3);
	}
      |	CMD_BYE { 
	    /* Exit */
	    mdebs_msg(MDEBSMSG_BYE); 
	    stopflag = 1;
	    YYACCEPT; /* macro that exits the parser with value 0 */
	  }
;

cmd:	CMD_BYE '\n' { 
	    /* Exit */
	    mdebs_msg(MDEBSMSG_BYE); 
	    stopflag = 1;
	    YYACCEPT; /* macro that exits the parser with value 0 */
	  }
	| CMD_HEL '\n' { 
	    /* Help */
	    mdebs_msg(MDEBSMSG_WELCOMEHELP);
	    mdebs_err(MDEBSERR_NOHELPSYSTEM);
	  }
/*
 * QUERY AND DUMP COMMANDS
 */
	| CMD_QUE CMD_DB '\n' { 
/* query database */
            if (mdebsdb == (char *)NULL)
	      mdebs_err(MDEBSERR_NODBOPEN);
	    else
              mdebs_res(stderr, MDEBSRES_CURRENTDB, mdebsdb);
	  }
	| CMD_QUE CMD_DB TOK_STRING '\n' {
/* query database <string> */
	    if (!dbQuery(backslash_single_quotes($3)))
	      mdebs_err(MDEBSERR_NONEXISTENTDB, $3);
	    else
	      printf("That database seems to exist.\n");
          }
	| CMD_QUE CMD_DATESTYLE '\n' { 
/* query datestyle */
	    showDateStyle();
	  }
	| CMD_QUE CMD_BINARY TOK_NUMBER '\n' {
	    if (debugflag)
	      fprintf(stderr, "Range 0 to %d\n", power(2, 24));
	    if ($3 > power(2, 24))
	      fprintf(stderr, "Number is too big\n");
	    else if ($3 < 0)
	      fprintf(stderr, "Unsigned numbers only, please\n");
	    else {
  	      fprintf(stdout, "%s\n", to_bin(s, $3));
	      free(s);
	    }
	  }
	| CMD_QUE CMD_SHO '\n' {
/* query shortcut */
	    mdebs_err(MDEBSERR_SHQUEINCOMP);
	  }
	| que_sho '\n' {
/* query shortcut <arg> (`shcode' is number, `shdesig' is designation) */
	    mdebs_que_sho(); /* Initializes shcut_ptr */
	    print_shortcut(stdout, shcut_ptr);
	  }
	| CMD_DUM CMD_ALL '\n' {
/* dump entire database to file */
	    dump_all_to_file();
	  }
	| CMD_DUM CMD_SHO '\n' {
/* dump shortcut */
   	    dump_shortcuts(DUMP_TO_SCREEN);
	  }
	| dum_cha '\n' {
/* dump chart */
	    dump_chart(DUMP_TO_SCREEN, -1, -1);
	  }
	| dum_cha TOK_ACCTNUM '\n' {
	    if (accountName(bf_acct, ((bf_anal == -1) ? 0 : bf_anal) ) 
	   						   == (char *)NULL )
	      mdebs_res(stderr, MDEBSRES_CHARTNOEXIST);
	    else
	      dump_chart(DUMP_TO_SCREEN, bf_acct, bf_anal);
	  }
	| CMD_PRI CMD_CHA '\n' {
/* print chart */
	    dump_chart(DUMP_TO_PRINTER, -1, -1);    /* OOPS */
	  }
	| dum_acc '\n' { /* Dump an account in whole or in part */
/* dump account */
	    if (querygood) {
	      if (ending == ENDS_IMPROPER) {
	        /* Shave off dangling bit */
		query[strlen(query)-shave] = '\0';
		countquery[strlen(countquery)-shave] = '\0';
	      }
	      strcat(query, " ORDER BY ent_date, ser_num;");
	      strcat(countquery, ";");
	      if (debugflag) {
	        fprintf(stderr, "Final query == %s\n", query);
		fprintf(stderr, "Paused. Press a key... \n");
		getchar();
              }
  	      dump_account(DUMP_TO_SCREEN, (FILE *)NULL, da_flags, hdr, query, countquery);
	    }
	  }
	| pri_acc '\n' { /* Print an account in whole or in part */
/* print account */
	    if (querygood) {
	      if (ending == ENDS_IMPROPER) {
	        /* Shave off dangling bit */
		query[strlen(query)-shave] = '\0';
		countquery[strlen(countquery)-shave] = '\0';
	      }
	      strcat(query, " ORDER BY ent_date, ser_num;");
	      strcat(countquery, ";");
	      if (debugflag) {
	        fprintf(stderr, "Final query == %s\n", query);
		fprintf(stderr, "Paused. Press a key... \n");
		getchar();
              }
  	      dump_account(DUMP_TO_PRINTER, (FILE *)NULL, da_flags, hdr, query, countquery);
	    }
	  }
	| dum_jou '\n' { /* Dump an account in whole or in part */
/* dump journal */
	    if (querygood) {
	      if (ending == ENDS_IMPROPER) {
	        /* Shave off dangling bit */
		query[strlen(query)-shave] = '\0';
		countquery[strlen(countquery)-shave] = '\0';
	      }
	      strcat(query, " ORDER BY ent_date, ser_num;");
	      strcat(countquery, ";");
	      if (debugflag) {
	        fprintf(stderr, "Final query == %s\n", query);
		fprintf(stderr, "Paused. Press a key... \n");
		getchar();
              }
  	      dump_journal(DUMP_TO_SCREEN, (FILE *)NULL, da_flags, hdr, query, countquery);
	    }
	  }
	| pri_jou '\n' { /* Print an account in whole or in part */
/* print journal */
	    if (querygood) {
	      if (ending == ENDS_IMPROPER) {
	        /* Shave off dangling bit */
		query[strlen(query)-shave] = '\0';
		countquery[strlen(countquery)-shave] = '\0';
	      }
	      strcat(query, " ORDER BY ent_date, ser_num;");
	      strcat(countquery, ";");
	      if (debugflag) {
	        fprintf(stderr, "Final query == %s\n", query);
		fprintf(stderr, "Paused. Press a key... \n");
		getchar();
              }
              dump_journal(DUMP_TO_PRINTER, (FILE *)NULL, da_flags, hdr, query, countquery);
	    }
	  }
	| que_fis '\n' { /* Query Fiscal Year */
/* query fiscal */
	    query_fiscal(VERBO); 
	  } 
	| CMD_QUE CMD_JOU TOK_NUMBER TOK_NUMBER '\n' {
/* query journal <date serial_number> (old syntax) */
	    /* Query journal by date and serial number */
	    /* printf("Query Journal command encountered.\n"); */
	    if ((jeptr = query_journal_entry($3, $4)) != NULL)
  	      print_journal_entry(stderr, *jeptr);
	  }
	| CMD_QUE CMD_JOU TOK_JOUSPEC '\n' {
/* query journal <date:serial_number> (new syntax) */
	    /* Query Journal command */
	    /* printf("Query Journal command - new syntax\n"); */
	    if ((jeptr = query_journal_entry(bf_date, bf_sernum)) != NULL)
  	      print_journal_entry(stderr, *jeptr);
	  }
/*
 * INSERT COMMANDS
 */
 	| ins_db TOK_STRING '\n' {
  /* "Insert" (i.e., "Change") database */
  /* ins_db + database name */
	    /* Perform initial checks */
	    if (strcmp(mdebsdb, $2) == 0)
	    {
	      /* Nothing to do! */
	      mdebs_msg(MDEBSMSG_NOTHING2DO);
	      goto ins_db_done;
	    }
	    if (strlen(s) > 24) 
	    {
	      mdebs_err(MDEBSERR_DBNAMTOOLONG);
	      goto ins_db_done;
	    } 
	    /* Check if given dbname exists */
            if (!dbQuery($2)) 
	    {
	      mdebs_err(MDEBSERR_NONEXISTENTDB, $2);
	      goto ins_db_done;
            }
            /* OK, it seems to exist */
	    mdebs_msg(MDEBSMSG_EXISTENTDB, $2);
	    if (debugflag)
	      fprintf(stderr, "ins_db: Closing backend connection.\n");
            closeBackend();
	    mdebs_msg(MDEBSMSG_CHANGINGDB, $2);
	    if (debugflag) 
	      fprintf(stderr, "ins_db: Opening backend connection.\n");
	    if (openBackend(s) < 1) 
	    {
	      /* Couldn't open given database name, even though it
	       * exists? */
	      mdebs_err(MDEBSERR_BACKENDERR);
	      /* Return to database we had open before */
	      if (openBackend(mdebsdb) < 1) 
	      {
	        /* What? That doesn't work either? */
	        mdebs_err(MDEBSERR_FATALERR);
	        exit_nicely();
	      } 
	      else assert (1 != 1);
	      goto ins_db_done;
            }
	    /* Backend connection to proposed new database now open */
            if (!query_mdebsdb()) {
              mdebs_err(MDEBSERR_NOTMDEBSDB, $2);
	      /* Return to database we had open before */
	      if (openBackend(mdebsdb) < 1) 
	      {
	        /* What? That db was open just a moment ago... Why can't I
		 * open it now? */
	        mdebs_err(MDEBSERR_FATALERR);
	        exit_nicely();
	      } 
	      goto ins_db_done;
            }
            /*
             * The now open database appears to be a valid mdebs 0.03 database
             */
            if (debugflag)
	      fprintf(stderr, "ins_db: Changing mdebsdb variable to %s.\n", $2);
	    mdebsdb = (char *)strndup($2, (size_t)25); 
ins_db_done:
	    setDateStyle();
         /* showDateStyle(); */
	    mdebs_res(stderr, MDEBSRES_CURRENTDB, mdebsdb);
	  } /* ins_db */
	| ins_cha '\n' {
	    insert_chart(bf_acct, bf_anal, s);
	  }
	| CMD_INS CMD_FIS TOK_NUMBER TOK_NUMBER '\n' { 
	    insert_fiscal($3, $4);
	  }
	| CMD_CHG CMD_FIS TOK_NUMBER TOK_NUMBER '\n' { 
	    update_fiscal($3, $4);
	  }
	| CMD_INS CMD_JOU '\n' { /* NOT IMPLEMENTED */ 
	    printf("See manual for correct journal entry method.\n"); 
	  } 
	| CMD_INS CMD_SHO TOK_STRING '\n' {
      /* The Insert Shortcut Command */
            insert_shortcut(backslash_single_quotes($3));
	  }
/*
 * DELETE COMMANDS
 */
 	| CMD_DEL CMD_DB '\n' {
	    mdebs_err(MDEBSERR_NODELDB);
	  }
	| del_acc_by_num '\n' {
	    delete_account_by_number(bf_acct, bf_anal);
	  }
	| CMD_DEL CMD_FIS '\n' {
	    delete_fiscal();
	  }
	| CMD_DEL CMD_JOU TOK_NUMBER TOK_NUMBER '\n' { 
  /* del_jou + date + serial number (old syntax) */
	    delete_journal_entry($3, $4);
	  }
	| CMD_DEL CMD_JOU TOK_JOUSPEC '\n' { 
  /* del_jou + date + serial number (new syntax) */
  	    /* printf("Delete Journal command - new syntax\n"); */
	    delete_journal_entry(bf_date, bf_sernum);
	  }
	| CMD_DEL CMD_SHO TOK_NUMBER '\n' {
	  /* Delete Shortcut by number */
	  delete_shortcut(&$3, (char *)NULL);
	}
	| CMD_DEL CMD_SHO TOK_STRING '\n' {
	  /* Delete Shortcut by designation string */
	  delete_shortcut((int *)NULL, backslash_single_quotes($3));
	}
	;

que_sho: CMD_QUE CMD_SHO TOK_NUMBER {
/* QUERY SHORTCUT <number> */
	    if (debugflag)
	      fprintf(stderr, "$3 is ->%d<-... ", $3); 
	    shcode = (int *)malloc(sizeof(int));
	    *shcode = $3; shdesig = (char *)NULL;
	    if (debugflag)
	      fprintf(stderr, "shcode is ->%d<-\n", *shcode);
	  }
	| CMD_QUE CMD_SHO TOK_STRING {
/* QUERY SHORTCUT <string> */
	    if (debugflag)
	      fprintf(stderr, "$3 is ->%s<-... ", $3); 
	    shcode = (int *)NULL; shdesig = strdup(backslash_single_quotes($3));
	    if (debugflag)
	      fprintf(stderr, "shdesig is ->%s<-\n", shdesig);
          }
	;

que_fis: CMD_QUE CMD_FIS 
	| CMD_DUM CMD_FIS 
	;

dum_acc:  dum_acc_cmd 
	| dum_acc qualification
	;

pri_acc:  pri_acc_cmd 
	| pri_acc qualification
	;

dum_jou:  dum_jou_cmd
	| dum_jou qualification
	;

pri_jou:  pri_jou_cmd
	| pri_jou qualification
	;

dum_acc_cmd: CMD_DUM CMD_ACC {
	    cmdspec = DUMP_ACCOUNT;
	    /* Set up initial header and query strings */
	    sprintf(hdr, "ACCOUNT DUMP:\n");
	    sprintf(query, "SELECT W1.ent_date, W1.ser_num, W2.document, W2.desig, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    sprintf(countquery, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    ending = ENDS_IMPROPER; shave = 4;
	    da_flags = 01;
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    /* Initialize variables */
            querygood = 1; /* If initial checks fail, this will be set to 0 */
	    fisc = query_fiscal(QUIET);                  /* Get fiscal year */
	    dates = (struct date_range *)malloc(sizeof(struct date_range));
	    memcpy(dates, fisc, sizeof(struct date_range)); 
  	    			 /* Initialize dates to fiscal year as well */
	  }
	;

pri_acc_cmd: CMD_PRI CMD_ACC {
 	    cmdspec = DUMP_ACCOUNT;
	    /* Set up initial header and query strings */
	    sprintf(hdr, "ACCOUNT DUMP:\n");
	    sprintf(query, "SELECT W1.ent_date, W1.ser_num, W2.document, W2.desig, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    sprintf(countquery, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    ending = ENDS_IMPROPER; shave = 4;
	    da_flags = 01;
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    /* Initialize variables */
            querygood = 1; /* If initial checks fail, this will be set to 0 */
	    fisc = query_fiscal(QUIET);                  /* Get fiscal year */
	    dates = (struct date_range *)malloc(sizeof(struct date_range));
	    memcpy(dates, fisc, sizeof(struct date_range)); 
  	    			 /* Initialize dates to fiscal year as well */
	  }
	;

dum_jou_cmd: CMD_DUM CMD_JOU {
	    cmdspec = DUMP_JOURNAL;
            if (debugflag)
	      fprintf(stderr, "Dump journal command\n");
	    sprintf(hdr, "ÚČETNÍ OPERACE:\n");
            sprintf(query, "SELECT W1.ent_date, W1.ser_num, W2.desig, W2.document, W2.shortcut, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
            sprintf(countquery, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    ending = ENDS_IMPROPER; shave = 4;
	    da_flags = 01;
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    /* Initialize variables */
            querygood = 1; /* If initial checks fail, this will be set to 0 */
	    fisc = query_fiscal(QUIET);                  /* Get fiscal year */
	    dates = (struct date_range *)malloc(sizeof(struct date_range));
	    memcpy(dates, fisc, sizeof(struct date_range)); 
  	    			 /* Initialize dates to fiscal year as well */
	  }
	;

pri_jou_cmd: CMD_PRI CMD_JOU {
 	    cmdspec = DUMP_JOURNAL;
            if (debugflag)
	      fprintf(stderr, "Dump journal command\n");
	    sprintf(hdr, "ÚČETNÍ OPERACE:\n");
            sprintf(query, "SELECT W1.ent_date, W1.ser_num, W2.desig, W2.document, W2.shortcut, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
            sprintf(countquery, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND");
	    ending = ENDS_IMPROPER; shave = 4;
	    da_flags = 01;
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    /* Initialize variables */
            querygood = 1; /* If initial checks fail, this will be set to 0 */
	    fisc = query_fiscal(QUIET);                  /* Get fiscal year */
	    dates = (struct date_range *)malloc(sizeof(struct date_range));
	    memcpy(dates, fisc, sizeof(struct date_range)); 
  	    			 /* Initialize dates to fiscal year as well */
	  }
	;

qualification: '(' {
	    /* Check ending */
	    if (ending == ENDS_PROPER) {
	      strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
	      ending = ENDS_IMPROPER; shave = 4;
	    }
            strncat(query, " (", MAX_QUERY - strlen(query) - 1);
            strncat(countquery, " (", MAX_QUERY - strlen(query) - 1);
	    ending = ENDS_IMPROPER; shave = 2;
	  }
	| ')' {
            strcat(query, " )");
            strcat(countquery, " )");
	    ending = ENDS_PROPER; shave = 0;
	  }
	| CMD_AND {
	    if (debugflag)
	      fprintf(stderr, "mdebs.y: appending AND\n");
            strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
            strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
	    ending = ENDS_IMPROPER; shave = 4;
	  }
	| CMD_OR {
	    if (debugflag)
	      fprintf(stderr, "mdebs.y: appending OR\n");
	    strncat(hdr, "   ----> OR <----\n", MAX_QUERY - strlen(query) - 1);
            strncat(query, " OR", MAX_QUERY - strlen(query) - 1);
            strncat(countquery, " OR", MAX_QUERY - strlen(query) - 1);
	    ending = ENDS_IMPROPER; shave = 3;
	  }
	| TOK_ACCTNUM {
	    /* bf_acct and bf_anal are both int and initialized in mdebs.yy */
	    if (accountName(bf_acct, ( (bf_anal == -1) ? 0 : bf_anal )) == 
	                                                      (char *)NULL) {
	      mdebs_err(MDEBSERR_NOSUCHACCT, bf_acct, bf_anal);
	      querygood = 0;
	    } else {
	      /* Check ending */
	      if (ending == ENDS_PROPER) {
	        strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
	        strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
	        ending = ENDS_IMPROPER; shave = 4;
	      }
	      /* We have a valid account number */
              if (bf_anal >= 0) {
                /* Dumping a single account */
                asprintf(&tmpbuf1, "  Account No. %03d-%03d: %s\n", 
  	          bf_acct, bf_anal, strndup(accountName(bf_acct, bf_anal), 60));
		if (cmdspec == DUMP_JOURNAL)
                  asprintf(&tmpbuf2, " W2.id IN (SELECT DISTINCT id FROM jou_lines WHERE acct='%03d' AND anal='%03d')", bf_acct, bf_anal);
		else /* cmdspec == DUMP_ACCOUNT */
                  asprintf(&tmpbuf2, " (acct='%03d' AND anal='%03d')", bf_acct, bf_anal);
		da_flags = 0;
              } else {
                /* Dumping a range of accounts */
                asprintf(&tmpbuf1, "  All %03d accounts (%03d-*)\n", bf_acct, bf_acct);
		if (cmdspec == DUMP_JOURNAL)
                  asprintf(&tmpbuf2, " W2.id IN (SELECT DISTINCT id FROM jou_lines WHERE acct='%03d')", bf_acct);
		else /* cmdspec == DUMP_ACCOUNT */
                  asprintf(&tmpbuf2, " acct='%03d'", bf_acct);
		da_flags = 01;
              }
              strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
  	      strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
  	      strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              free(tmpbuf1);
              free(tmpbuf2);
	      if (debugflag) {
	        fprintf(stderr, "Header == %s\n", hdr);
		fprintf(stderr, "Query == %s\n", query);
              }
	      ending = ENDS_PROPER; shave = 0;
            } 
	  }
	| TOK_NUMBER TOK_NUMBER {
	    /* Interpreted as date range */
	    if (debugflag) {
	      fprintf(stderr, "mdebs.y: Processing date range...\n");
	      fprintf(stderr, "mdebs.y: %d - %d\n", $1, $2);
	    }
	    dates->datefr = $1;  /* Nota Bene: TOK_NUMBER semantic values */
	    dates->dateto = $2;  /* are converted to int in mdebs.yy!!    */
            if (!valiDate(dates->datefr) ||
                !valiDate(dates->dateto) ||
                dates->datefr > dates->dateto || 
		dates->datefr < fisc->datefr ||
		dates->dateto > fisc->dateto) {
              mdebs_err(MDEBSERR_FISCBADDATES, dates->datefr, dates->dateto);
	      querygood = 0;
            } else {
              /* Check query ending */
	      if (ending == ENDS_PROPER) {
	        strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
	        strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
	        ending = ENDS_IMPROPER; shave = 4;
	      }
              /* We have a valid date range */
	      if (debugflag)
	        fprintf(stderr, "mdebs.y: Date range is valid. Assembling "
		  "header and query\n");
              asprintf(&tmpbuf1, "  Date Range %d-%d\n", dates->datefr,
						   dates->dateto);
              asprintf(&tmpbuf2, 
/*                  " ((W1.ent_date>='%s' AND W1.ent_date<='%s'))",  */
		    " (W1.ent_date BETWEEN '%s' AND '%s')",
	          pgConvertDate(dates->datefr), pgConvertDate(dates->dateto));
              strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
              strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              free(tmpbuf1);
              free(tmpbuf2);
              if (debugflag) {
                fprintf(stderr, "Header == %s\n", hdr);
                fprintf(stderr, "Query == %s\n", query);
              }
	      ending = ENDS_PROPER; shave = 0;
            } 
	  }
	| DOCUMENT TOK_STRING {
	    /* Check query ending */
            if (ending == ENDS_PROPER) {
              strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
              strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
              ending = ENDS_IMPROPER; shave = 4;
            }
	    /* A document specification (LIKE) */
            asprintf(&tmpbuf1, "  Accounting document: %s\n", $2);
            asprintf(&tmpbuf2, " W2.document LIKE '%%%s%%'", 
	        backslash_single_quotes($2));
            strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
            strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            free(tmpbuf1);
            free(tmpbuf2);
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	| DOCU_REGEX TOK_STRING {
	    /* Check query ending */
            if (ending == ENDS_PROPER) {
              strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
              strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
              ending = ENDS_IMPROPER; shave = 4;
            }
	    /* A document specification (REGEX) */
            asprintf(&tmpbuf1, 
	        "  Accounting document satisfies regex \"%s\"\n", $2);
            asprintf(&tmpbuf2, " W2.document ~ '%s'", 
	        backslash_single_quotes($2));
            strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
            strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            free(tmpbuf1);
            free(tmpbuf2);
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	| DESIGNATION TOK_STRING {
            /* Check query ending */
            if (ending == ENDS_PROPER) {
              strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
              strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
              ending = ENDS_IMPROPER; shave = 4;
            }
	    /* A designation specification (LIKE) */
            asprintf(&tmpbuf1, "  Entry description: %s\n", $2);
            asprintf(&tmpbuf2, " W2.desig LIKE '%%%s%%'", 
	        backslash_single_quotes($2));
            strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
            strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            free(tmpbuf1);
            free(tmpbuf2);
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	| DESIG_REGEX TOK_STRING {
	    /* Check query ending */
	      if (ending == ENDS_PROPER) {
	        strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
	        strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
	        ending = ENDS_IMPROPER; shave = 4;
	      }
	    /* A designation specification (REGEX) */
            asprintf(&tmpbuf1, "  Entry description satisfies regex \"%s\"\n", 
	    								$2);
            asprintf(&tmpbuf2, " W2.desig ~ '%s'", backslash_single_quotes($2));
            strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
            strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
            free(tmpbuf1);
            free(tmpbuf2);
            if (debugflag) {
              fprintf(stderr, "Header == %s\n", hdr);
              fprintf(stderr, "Query == %s\n", query);
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	| SHORTCUT TOK_NUMBER {
	    /* Check query ending */
            if (ending == ENDS_PROPER) {
              strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
              strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
              ending = ENDS_IMPROPER; shave = 4;
            }
	    /* A shortcut specification (by number) */
	    shcode = (int *)malloc(sizeof(int));
	    memcpy(shcode, &$2, sizeof(int)); 
	    shdesig = (char *)NULL;
	    if (mdebs_que_sho() == 0) {
	      mdebs_err(MDEBSERR_NOSUCHSHCODE, *shcode);
	      querygood = 0;
	    } else {
              asprintf(&tmpbuf1, "  Shortcut %d: %s\n", shcut_ptr->code, 
	                                                shcut_ptr->desig);
              asprintf(&tmpbuf2, " W2.shortcut=%d", shcut_ptr->code);
              strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
              strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              free(tmpbuf1);
              free(tmpbuf2);
              if (debugflag) {
                fprintf(stderr, "Header == %s\n", hdr);
                fprintf(stderr, "Query == %s\n", query);
              }
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	| SHORTCUT TOK_STRING {
	    /* Check query ending */
            if (ending == ENDS_PROPER) {
              strncat(query, " AND", MAX_QUERY - strlen(query) - 1);
              strncat(countquery, " AND", MAX_QUERY - strlen(query) - 1);
              ending = ENDS_IMPROPER; shave = 4;
            }
	    /* A shortcut specification (by string) */
	    shcode = (int *)NULL; shdesig = strdup($2);
	    if (mdebs_que_sho() == 0) {
	      mdebs_err(MDEBSERR_NOSUCHSHDESIG, shdesig);
	      querygood = 0;
	    } else {
              asprintf(&tmpbuf1, "  Shortcut %d: %s\n", shcut_ptr->code, 
	                     unbackslash_single_quotes(shcut_ptr->desig));
              asprintf(&tmpbuf2, " W2.shortcut=%d", shcut_ptr->code);
              strncat(hdr, tmpbuf1, MAX_QUERY - strlen(hdr) - 1);
              strncat(query, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              strncat(countquery, tmpbuf2, MAX_QUERY - strlen(query) - 1);
              free(tmpbuf1);
              free(tmpbuf2);
              if (debugflag) {
                fprintf(stderr, "Header == %s\n", hdr);
                fprintf(stderr, "Query == %s\n", query);
              }
            }
	    ending = ENDS_PROPER; shave = 0;
	  }
	;

dum_cha:  CMD_DUM CMD_CHA 
	| CMD_QUE CMD_CHA
	| CMD_QUE CMD_ACC
	;

ins_db: CMD_INS CMD_DB
	| CMD_CHG CMD_DB
	;

ins_cha:  CMD_INS CMD_CHA TOK_ACCTNUM TOK_STRING {
 	    /* bf_acct and bf_anal are initialized in mdebs.yy */
	    s = backslash_single_quotes($4);
	  }
 	| CMD_INS CMD_ACC TOK_ACCTNUM TOK_STRING {
 	    /* bf_acct and bf_anal are initialized in mdebs.yy */
	    s = backslash_single_quotes($4);
	  }
	;

del_acc_by_num: CMD_DEL CMD_ACC TOK_ACCTNUM {
	    /* bf_acct and bf_anal are initialized by mdebs.yy */
	  }
        | CMD_DEL CMD_CHA TOK_ACCTNUM {
	    /* bf_acct and bf_anal are initialized by mdebs.yy */
	  }
	;

%%

int yyerror(char *s)
{
  /* fprintf(stderr, "%s\n", s); */
  yyrestart((FILE *)NULL);
  /* return 0; */
}

