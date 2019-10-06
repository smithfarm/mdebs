/*
 * dump.c
 *
 * Functions to "dump" things
 */

#include "mdebs.h"

#include "generalized.h"
#include "journal.h"
#include "messages.h"
#include "pgresfunc.h"
#include "printouts.h"
#include "query.h"

#include "dump.h"

void processDumpedData(const int mode, FILE *fp, char *fp_name)
{
  /* Close temporary file */
  close_tmp(fp);
  
  switch (mode) {

  case (DUMP_TO_SCREEN):
    browse_tmp(fp_name);
    break;

  case (DUMP_TO_PRINTER):
    print_tmp(fp_name);
    break;

  case (DUMP_TO_FILE):
    if (debugflag)
      fprintf(stderr, "DUMP_TO_FILE not implemented yet\n");
    break;

  } /* switch */

  remove_tmp(fp_name);
  free(fp_name);
}

/*
 * dump_all_to_file()
 *
 * Function to dump contents of entire database to a file.
 *
 * Based on dump_chart()
 *
 * command must be ALL CAPS:  DUMP ALL TO FILE
 */
void dump_all_to_file(void)
{
  FILE *fp;
  char *fp_name, *buf, *buf1, *buf2;
  char *countquery;
  char *buffrag;
  int flag, da_flags;
  char *tmpacct;
  char *tmpanal;
  char *headerstring;
  char *fname = "dump_all_to_file()";
  int retval, noofinstances;

  /* If the global variable mdebsdb is not defined, then presumably we
   * don't have an active database connection */
  if (mdebsdb == (char *)NULL) { 
    mdebs_err(MDEBSERR_NODBOPEN);
    return;
  }

  /* Determine the count */
  asprintf(&buf, "SELECT COUNT(*) FROM chart;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return;
  }
  free(buf);
  noofinstances = atoi(getField("count"));
  if (noofinstances == 0) {
    fprintf(fp, "MDEBSERR: Chart of Accounts empty or does not exist.\n");
    fprintf(fp, "Bailing out.\n");
    return;
  }
  if (debugflag) {
    mdebs_res(stderr, MDEBSRES_CHARTACCTN, noofinstances);
    fprintf(stderr, "Paused. Press <ENTER>\n");
    getchar();
  }

  asprintf(&fp_name, "./mdebs-dump-%s", mdebsdb);
  fp = fopen(fp_name, "w");
  fprintf(fp, "COMPLETE DUMP OF MDEBS DATABASE %s\n", mdebsdb);
  fprintf(fp, "=========================================\n\n");

  /* Start an atomic transaction (required) */
  startTransaction();

  /* Run the SQL query */
  asprintf(&buf, "DECLARE b1 CURSOR FOR SELECT * FROM chart "
      "ORDER BY acct, anal;");
  if (debugflag)
    fprintf(stderr, "About to run query: %s\n", buf);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);

  /* Fetch loop */
  while (fetchNext("b1", 1)) {
    tmpacct = getField("acct");
    tmpanal = getField("anal");
    if (debugflag) {
      fprintf(stderr, "tmpacct == '%s'; tmpanal == %s\n", tmpacct, tmpanal);
    }

    /* Print the header */
    asprintf(&headerstring, "****\n****\n****\n****\n\nÚèet %s-%s: %s\n", tmpacct, tmpanal, getField("desig"));

    /* Should be a function call to dump the account */
    /* but ended up just copying and pasting from mdebs.y */

    /* Set up initial header and query strings */
    asprintf(&buf1, "SELECT W1.ent_date, W1.ser_num, W2.document, W2.desig, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND (acct='%s' AND anal='%s') ORDER BY W1.ent_date, W1.ser_num;", tmpacct, tmpanal); 
    asprintf(&buf2, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id AND (acct='%s' AND anal='%s');", tmpacct, tmpanal); 
    da_flags = 0;
    if (debugflag) 
      fprintf(stderr, "%s\n%s\n", buf1, buf2);
    dump_account(DUMP_TO_FILE, fp, da_flags, headerstring, buf1, buf2);
  }

  /* Close cursor, the atomic transaction, and the pipeline */
  asprintf(&buf, "CLOSE b1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);

  fprintf(fp, "\n\n");

  /*
   * Now dump all the journal entries
   */

  asprintf(&headerstring, "ALL JOURNAL ENTRIES FOLLOW...\n\n");

  /* Set up initial header and query strings */
  asprintf(&buf1, "SELECT W1.ent_date, W1.ser_num, W2.desig, W2.document, W2.shortcut, W1.debit, W1.acct, W1.anal, W1.amount FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id ORDER BY W1.ent_date, W1.ser_num;");
  asprintf(&buf2, "SELECT count(*) FROM jou_lines W1, jou_head W2 WHERE W1.id=W2.id;");
  da_flags = 0;
  if (debugflag) 
    fprintf(stderr, "%s\n%s\n", buf1, buf2);
  dump_journal(DUMP_TO_FILE, fp, da_flags, headerstring, buf1, buf2);

  endTransaction();
}


/*
 * dump_chart()
 *
 * Function to dump contents (partial or full) of Chart of Accounts based
 * on account number values given
 *
 * Takes following arguments:
 * 	mode	What to do with the dump
 * 	acct	Account major number
 * 	anal	Account minor number
 *
 * Note: Either acct or anal (or both) may be -1.  If only acct is -1, it
 * is the same as if both are -1.
 * 
 * Returns:
 * 	doesn't return anything; since this is an interactive command, we
 * 	inform the user of error conditions using mdebs_err()
 */
void dump_chart(int mode, int acct, int anal)
{
  FILE *fp;
  char *fp_name, *buf;
  char *buffrag;
  int flag;
  char tmpval;
  char *tmpacct;
  char *fname = "dump_chart()";
  int retval, noofinstances;

  /* If the global variable mdebsdb is not defined, then presumably we
   * don't have an active database connection */
  if (mdebsdb == (char *)NULL) { 
    mdebs_err(MDEBSERR_NODBOPEN);
    return;
  }

  /* Set up the WHERE clause */
  if (acct == -1) /* No WHERE clause */
    buffrag = '\0';
  else if (anal == -1)
    asprintf(&buffrag, "WHERE acct='%03d'", acct);
  else 
    asprintf(&buffrag, "WHERE acct='%03d' AND anal='%03d'", acct, anal);
	
  /* Determine the count */
  if (buffrag == '\0')
    asprintf(&buf, "SELECT COUNT(*) FROM chart;");
  else
    asprintf(&buf, "SELECT COUNT(*) FROM chart %s;", buffrag);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return;
  }
  free(buf);
  noofinstances = atoi(getField("count"));
  if (noofinstances == 0) {
    if (anal == -1)
      mdebs_err(MDEBSERR_CHARTEMPTY);
    else
      mdebs_err(MDEBSRES_CHARTNOEXIST);
    return;
  }
  if (debugflag) {
    mdebs_res(stderr, MDEBSRES_CHARTACCTN, noofinstances);
    fprintf(stderr, "Paused. Press <ENTER>\n");
    getchar();
  }

  /* Open temporary file */
  if (noofinstances > 19) {
    if (debugflag)
      fprintf(stderr, "Number of instances exceeds 19, opening tmpfile\n");
    fp = open_tmp(&fp_name);
  }
  else {
    if (debugflag)
      fprintf(stderr, "Number of instances less than 19, writing to stdout\n");
    fp = stdout;
  }

  /* Start an atomic transaction (required) */
  startTransaction();

  /* Write the chart dump header if this is a full dump */
  if (acct == -1)
    mdebs_res(fp, MDEBSRES_CHARTHEAD, mdebsdb);

  /* Write table header */
  if (noofinstances > 1)
    fprintf(fp, MDEBSHEAD_CHART);

  /* Run the SQL query */
  if (buffrag == '\0')
    asprintf(&buf, "DECLARE a1 CURSOR FOR SELECT * FROM chart "
      "ORDER BY acct, anal;");
  else
    asprintf(&buf, "DECLARE a1 CURSOR FOR SELECT * FROM chart %s "
      "ORDER BY acct, anal;", buffrag);
  if (debugflag)
    fprintf(stderr, "About to run query: %s\n", buf);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);

  /* Fetch loop */
  flag = 1;
  while (fetchNext("a1", 1)) {
    tmpacct = getField("acct");
    if (flag) /* First time 'round */
    {
      tmpval = *(tmpacct); /* Set tmpval to first char of acct field */
      flag = 0;	           /* Turn off flag */
    }
    if (tmpval != *(tmpacct)) { /* If first char of acct field has changed */
      fprintf(fp, "              \n");
      tmpval = *(tmpacct);      /* reset */
    }
    if (debugflag)
      fprintf(stderr, "tmpval == '%c'; tmpacct == %s\n", tmpval, tmpacct);
    fprintf(fp, "%s-%s       %s\n", tmpacct, getField("anal"),
	getField("desig"));
  }
  if (noofinstances > 1)
    fprintf(fp, MDEBSFOOT_CHART);

  /* Close cursor, the atomic transaction, and the pipeline */
  asprintf(&buf, "CLOSE a1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);

  endTransaction();

  if (fp != stdout)
    processDumpedData(mode, fp, fp_name);
}

/*
 * Function dump_account()
 *
 * Takes following arguments
 *   mode	DUMP_TO_SCREEN, DUMP_TO_PRINTER, DUMP_TO_FILE
 *   fp         pointer to file (can be NULL)
 *   flags	Bitwise flags:
 *		  1st bit from left: Whether to display Maj-Min column
 *   hdr	Header string
 *   slct	Select query for cursor
 */
void 
dump_account(const int mode, 
             FILE *dumpfile,
             const int flags, 
	     char *hdr, 
	     char *slct,
	     char *countslct)
{
  char *fname = "dump_account()";

  FILE *fp;
  char *fp_name;
  char *buf;
  char *desigBuff, *docBuff;
  int amount, retval, noofinstances;
  bool debit;
  double deb_counter, cre_counter;

  /* First run just the SELECT count(*)... 
   * any errors should show up here */
  retval = runQuery(countslct);
  if (retval == -1) {
    mdebs_queryerr(countslct, fname);
    return;
  }
  if (mode == DUMP_TO_FILE) {
    noofinstances = atoi(getField("count"));
    fprintf(stderr, "noofinstances == %d\n", noofinstances);
    if (noofinstances == 0) {
      fprintf(dumpfile, "%s\n", hdr);
      fprintf(dumpfile, "Na tomto úètu nebyly ve sledovaném období pohyby.\n\n\n");
      return;
    }
  }
  if (debugflag)
    fprintf(stderr, "Now in dump_account()...\n");

  /* Start atomic transaction (required) */
  if (mode != DUMP_TO_FILE) {
    startTransaction();
  }

  /* Initialize debit and credit side counters */
  deb_counter = cre_counter = (long)0;

  /* Assemble the declare cursor query */
  asprintf(&buf, "DECLARE a1 CURSOR FOR %s", slct);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    if (mode != DUMP_TO_FILE) {
      rollbackTransaction();
    }
    return;
  }
  free(buf);

  if (!fetchNext("a1", 1)) {
    mdebs_err(MDEBSERR_NOJOUMATCH);
    if (mode != DUMP_TO_FILE) {
      rollbackTransaction();
    }
    return;
  }

  /* OK, since fetchNext() didn't fail, we know we have at
   * least one entry to dump */

  /* Open tempfile for writing */
  if (debugflag)
    fp = stderr;
  else {
    if (mode == DUMP_TO_FILE)
      fp = dumpfile;
    else 
      fp = open_tmp(&fp_name);
  }

  /* Set fp to wide character mode */
  if (fwide(fp, 0) == 0) /* 0 queries the current mode */
  { /* fp has no specific char mode yet, attempt to set to wide */
  if (fwide(fp, 1) <= 0) /* a value greater than zero switches to
                                wide character mode */
    printf("could not switch fp to wide char mode!\n");
  else
    fwprintf(fp, L"switched to wide char mode.\n");
  }

  fwprintf(fp, L"%s\n", hdr);

  if (flags & 01) {
    /* Yes, display the Maj-Min column */
    fwprintf(fp, L"%s", MDEBSHEAD_ACCOUNTMIN);
  }
  else {
    /* No, don't display the Maj-Min column */
    fwprintf(fp, L"%s", MDEBSHEAD_ACCOUNTNOMIN);
  }

  /* Set up a while loop to fetch through the cursor (fetchNext() returns 0
   * when it reaches the end of the selection */

  do {
    /* Load "document" and "desig" into buffers because we'll need to use
     * them later */
    docBuff=strdup(getField("document"));
    desigBuff=strdup(getField("desig"));

    /* Print out first section of the current line, consisting of date and
     * serial number (we won't need those anymore so there's no point in
     * saving them to a buffer) */
    fwprintf(fp, L"%d:%03d", (NathanConvertDate(getField("ent_date"))), atoi(getField("ser_num")));

    /* Print out the account number, if applicable */ 
    if (flags & 01) {
    /* Yes, display the Maj-Min column */
      fwprintf(fp, L"|%03d-%03d", atoi(getField("acct")),
      atoi(getField("anal")));
    }

    amount = atoi(getField("amount"));

    /* What we print out next depends on whether this particular entry is a
     * debit or a credit */
    if ( to_bool(getField("debit")) ) /* It's a debit */
    {
      /* Print out the entry designation, and
       * the amount (in units and hundredths of units) */
      fwprintf(fp, L"|%-12s|%-25s|%13.2f|\n", (wchar_t *)strndup(docBuff, 12), strndup(desigBuff, 25), (double)amount/(double)100);

      /* Increment the debit side counter by the integer amount */
      deb_counter = deb_counter + ( (double)amount/(double)100 );
    } 
    else /* It's a credit */
    {
      /* Print out the entry designation,
       * and the amount (in units and hundredths of units) */
      fwprintf(fp, L"|%-12s|%-25s|             |%13.2f\n", strndup(docBuff, 12), strndup(desigBuff, 25), (double)amount/(double)100);

      /* Increment the credit side counter by the integer amount */
      cre_counter = cre_counter + ( (double)(amount)/(double)(100) );
    }
  } while (fetchNext("a1", 1));

  if (flags & 01) {
    /* Yes, display the Maj-Min column */
    fwprintf(fp, L"%s", MDEBSFOOT_ACCOUNTMIN);
  }
  else {
    /* No, don't display the Maj-Min column */
    fwprintf(fp, L"%s", MDEBSFOOT_ACCOUNTNOMIN);
  }

  /* At the end of the while loop, signalling that the body of the report has
   * been completely printed out, we print out some totals taken from the
   * credit and debit counters */
  fwprintf(fp, L"\nCELKEM:                                             %13.2f %13.2f\n", (deb_counter), (cre_counter));

  /* If the sides are imbalanced, we print out the difference between the two
   * counters */
  if (cre_counter < deb_counter) {
    fwprintf(fp, L"                                 balance:                        (%13.2f)\n", deb_counter - cre_counter);
  } else if (cre_counter > deb_counter)
    fwprintf(fp, L"                                 balance:          (%13.2f)\n", cre_counter - deb_counter);
    
  /* Close the cursor, the atomic transaction, and the pipeline */
  asprintf(&buf, "CLOSE a1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    if (mode != DUMP_TO_FILE) { 
      rollbackTransaction();
    }
    return;
  }
  free(buf);
  if (mode != DUMP_TO_FILE) {
    endTransaction();
  }

  if (mode != DUMP_TO_FILE) {
    processDumpedData(mode, fp, fp_name);
  }
  else {
    fwprintf(fp, L"\n\n");
  }
}

/*
 * Function fetchNextJournalEntry()
 *
 * Fetch next journal entry in a given cursor
 *
 * Takes arguments:
 * 	jent	pointer to journal_entry structure (assumes memory already
 * 							allocated)
 * 	curs	string buffer containing cursor identifier
 *
 * Returns 
 * 	1 on success
 * 	0 on failure
 *     -1 when fetchNext() returns a -1 (fatal error) 
 */
int fetchNextJournalEntry(struct journal_entry *jent, char *curs)
{
  char *fname = "fetchNextJournalEntry()";
  int i, date, sernum;
  int retval;

  /* Memory allocation */
  init_jent(jent);
  
  /* Debugging message */
  if (debugflag)
    fprintf(stderr, "fetchNextJournalEntry(): preparing to FETCH from %s\n",
	curs);

  retval = fetchNext(curs, 1);
  if (retval == -1) {
    /* Fatal error (?) */
    /* mdebs_err("Fatal error in fetchNext();\n"); */
    return -1;
  }
  if (retval == 0) {
    /* We've reached the end */
    if (debugflag) {
      fprintf(stderr, "fetchNextJournalEntry(): FETCH NEXT failed\n");
    }
    return 0;
  }

  /* Get the header values */
  /* Save date and serial number at beginning of journal entry */
  strcpy(jent->desig, getField("desig"));
  strcpy(jent->document, getField("document"));
  jent->shortcut = atoi(getField("shortcut"));
  date = jent->ent_date = NathanConvertDate(getField("ent_date"));
  sernum = jent->ser_num = atoi(getField("ser_num"));

  i = 0;
  do
  {
    if (debugflag) {
      fprintf(stderr, "Initial entry tag: %d:%d\n", jent->ent_date, 
	  jent->ser_num);
      fprintf(stderr, "Current entry tag: %d:%d\n", date, sernum);
    }

    /* If ent_date/ser_num does not match beginning, backtrack and bail
     * out of loop */
    if ( !(date == jent->ent_date && sernum == jent->ser_num) )
    {
      moveBack(1, curs);
      break;
    }
      
    /* Determine debit/credit */
    jent->values[i].debit = to_bool(getField("debit"));
    jent->values[i].acct = atoi(getField("acct"));
    jent->values[i].anal = atoi(getField("anal"));
    jent->values[i].amount = atoi(getField("amount"));

    /* Get debit/credit amount */
    if (jent->values[i].debit) /* It's a debit */
    {
      if (debugflag)
	fprintf(stderr, "DEBIT %d\n", jent->values[i].amount);
    }
    else /* It's a credit */
    {
      if (debugflag)
	fprintf(stderr, "CREDIT %d\n", jent->values[i].amount);
    }

    /* Fetch next row in cursor */
    if (!fetchNext(curs, 1))
    { /* We've reached the end */
      if (debugflag) {
        fprintf(stderr, "fetchNextJournalEntry(): FETCH NEXT failed\n");
      }
      break;
    }

    /* Get current date:sernum tag */
    date = NathanConvertDate(getField("ent_date"));
    sernum = atoi(getField("ser_num"));

    if (i++ > 10)
    {
      printf("Fatal error: too many lines in journal entry.\n");
      exit_nicely();
    }
  } while (true);

  /* Print debugging message */
  if (debugflag) {
    fprintf(stderr, "fetchNextJournalEntry(): End of journal entry reached\n");
    print_journal_entry(stderr, *jent);
  }

  return 1;
}

/*
 * Function dump_journal
 *
 * Given a count query and a declare cursor, dumps all journal entries to a
 * temporary file and runs the MDEBS_PAGE_COMMAND (defined in mdebs.h) on
 * it.
 *
 * Takes following arguments:
 *      mode		Spec. whether dump to screen, printer, or file
 *      dumpfile	Pointer to file (used only with DUMP_TO_FILE)
 *      flags		Not used at the moment
 *      hdr		String to print at top of listing
 *      query		SELECT query
 *
 * mode can be one of:
 * 	DUMP_TO_SCREEN
 * 	DUMP_TO_PRINTER
 * 	DUMP_TO_FILE 
 * 	(all of which are defined in mdebs.h)
 *
 * Interactive function; no return value.
 *
 */
void 
dump_journal(const int mode, 
             FILE *dumpfile,
             const int flags, 
	     char *hdr, 
	     char *slct,
	     char *countslct)
{
  char *fname = "dump_journal()";
  FILE *fp;
  char *fp_name;
  char *buf;
  int retval;
  int i, j;
  struct journal_entry jent;

  if (debugflag)
    fprintf(stderr, "Entering dump_journal()...\n");

  /* First run just the SELECT count(*)... 
   * any errors should show up here */
  retval = runQuery(countslct);
  if (retval == -1) {
    mdebs_queryerr(countslct, fname);
    return;
  }

  if (mode != DUMP_TO_FILE)
    startTransaction();

  asprintf(&buf, "DECLARE jou1 CURSOR FOR %s", slct);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    if (mode != DUMP_TO_FILE)
      rollbackTransaction();
    return;
  }

  if (debugflag)
    fprintf(stderr, "Declare cursor returned %d\n", retval);
  
  retval = fetchNextJournalEntry(&jent, "jou1"); 
  if (retval == -1) {
    /* Fatal error */
    /* mdebs_err("Fatal error in fetchNextJournalEntry();\n"); * */
    /* runQuery("CLOSE jou1;"); */
    if (mode != DUMP_TO_FILE)
      rollbackTransaction();
    return;
  }
  if (retval == 0) 
  {
    mdebs_err(MDEBSERR_NOJOUMATCH);
    if (mode != DUMP_TO_FILE)
      rollbackTransaction();
    return;
  }

  /* OK, since fetchNextJournalEntry() didn't fail, we know we have at
   * least one journal entry to dump */

  /* Open temporary file */
  if (mode == DUMP_TO_FILE)
    fp = dumpfile;
  else 
    fp = open_tmp(&fp_name);
  
  /* Set fp to wide character mode */
  if (fwide(fp, 0) == 0) /* 0 queries the current mode */
  { /* fp has no specific char mode yet, attempt to set to wide */
  if (fwide(fp, 1) <= 0) /* a value greater than zero switches to
                                wide character mode */
    printf("could not switch fp to wide char mode!\n");
  else
    fwprintf(fp, L"switched to wide char mode.\n");
  }

  fwprintf(fp, L"%s\n", hdr);

  if (debugflag)
    fprintf(stderr, "dump_journal(): Using temp file %s\n", fp_name);

  do {
    if (debugflag) {
      if (!verify_journal_entry(&jent))
      {
        mdebs_err(MDEBSERR_INVALIDJOU);
        mdebs_err(MDEBSERR_FATALERR);
        exit_nicely();
      }
    }
    wprint_journal_entry(fp, jent);
    fwprintf(fp, L"\n");
  } while (fetchNextJournalEntry(&jent, "jou1"));
  
  /* Close the cursor, the pipeline, the transaction */
  asprintf(&buf, "CLOSE jou1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    if (mode != DUMP_TO_FILE)
      rollbackTransaction();
    return;
  }
  if (mode != DUMP_TO_FILE) {
    endTransaction();
    processDumpedData(mode, fp, fp_name);
  }
}

void dump_shortcuts(const int mode)
{
  FILE *fp;
  char *fname = "dump_shortcuts()";
  char *fp_name;
  char *buf;
  int retval;

  /* 
   * Sanity Checks 
   */

  /* No. 1: If the mdebsdb global variable is not defined, then presumably we 
   * don't have an active database connection */
  if (mdebsdb == (char *)NULL) { 
    mdebs_err(MDEBSERR_NODBOPEN);
    return;
  }

  /* No. 2: If the shortcuts table is empty, there's no point in continuing */
  asprintf(&buf, "SELECT * FROM shortcut;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return;
  }
  free(buf);
  if (retval == 0) {
    mdebs_err(MDEBSERR_SHORTCUTEMPTY);
    return;
  }

  /* Open output pipe ("w" = for writing) */
  fp = open_tmp(&fp_name);

  /* Start an atomic transaction (required) */
  startTransaction();

  /* Write the shortcut dump header to the pipe */
  mdebs_res(fp, MDEBSRES_SHORTCUTHEAD, mdebsdb);

  /* Run the SQL query */
  asprintf(&buf, 
      "DECLARE a1 CURSOR FOR SELECT * FROM shortcut ORDER BY code;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);

  /* Use our clever function "fetchAndDisplay" to display the data */
  fetchAndDisplay(fp, "a1");

  /* Close cursor, the atomic transaction, and the pipeline */
  asprintf(&buf, "CLOSE a1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    rollbackTransaction();
    return;
  }
  free(buf);
  endTransaction();

  processDumpedData(mode, fp, fp_name);
}

void print_shortcut(FILE *fp, struct short_cut *shp)
{
  char *fname = "print_shortcut()";

  if (shp == (struct short_cut *)NULL) {
    mdebs_res(fp, MDEBSRES_SHCUTNOTFOUND);
  }
  else {
    if (debugflag)
      fprintf(stderr, "print_shortcut(): Shortcut No. %d is %s.\n",
              shp->code, shp->desig);
    fprintf(fp, "Shortcut %d: %s\n", shp->code, shp->desig);
  }
}
