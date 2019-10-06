/*
 * journal.c 
 *
 * Functions for manipulating journal entries
 */

#include "mdebs.h"

#include "dbinit.h"
#include "generalized.h"
#include "messages.h"
#include "pgresfunc.h"
#include "query.h"

#include "journal.h"

/*
 * init_jent()
 *
 * Initializes a journal entry (takes pointer to journal entry structure)
 *
 * No return value.
 */
void init_jent(struct journal_entry *jebuff)
{
  int inc;

  jebuff->ent_date = 0;
  jebuff->desig[0] = '\0';
  jebuff->document[0] = '\0';
  jebuff->shortcut = 0;
  for (inc = 0; inc < 10; inc++)
  {
    jebuff->values[inc].debit = false;
    jebuff->values[inc].acct = -1;
    jebuff->values[inc].anal = -1;
    jebuff->values[inc].amount = -1;
  }
}

/*
 * query_journal_entry()
 *
 * takes date and serial number, returns journal_entry structure pointer or
 * (struct journal_entry *)NULL if not found or error
 */
struct journal_entry *
query_journal_entry (int date, int serial)
{
  char *fname = "query_journal_entry()";
  struct journal_entry *jent;
  int retval;
  char *buf;

  if (debugflag)
    fprintf(stderr, "Entering function query_journal_entry()\n");

  /* Sanity check - is the date valid? */
  if (!valiDate(date)) {
    mdebs_err(MDEBSERR_NOSUCHDATE, date);
    return (struct journal_entry *)NULL;
  }
  if (debugflag) fprintf(stderr, "The date seems to be OK.\n");

  /* We might as well find out whether the journal entry exists at all
   * before we go to the trouble of declaring a cursor to find the values
   */
  if (!transQuery(date, serial)) {
    mdebs_res(stderr, MDEBSERR_JOUNOEXIST, date, serial);
    return (struct journal_entry *)NULL;
  }

  /* Initialize jent structure */
  jent = (struct journal_entry *) malloc (sizeof(struct journal_entry));

  init_jent(jent);

  /* Declare a cursor to get all the lines of this transaction */
  asprintf(&buf, "DECLARE a1 CURSOR FOR SELECT W1.ent_date, W1.ser_num, W1.desig, W1.document, W1.shortcut, W2.debit, W2.acct, W2.anal, W2.amount FROM jou_head W1, jou_lines W2 WHERE W1.id=W2.id AND W1.ent_date='%s' AND W1.ser_num=%d;", pgConvertDate(date), serial);

  startTransaction();

  if (debugflag)
    fprintf(stderr, "About to declare cursor...\n");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    rollbackTransaction();
    free(buf);
    return (struct journal_entry *)NULL;
  }
  free(buf);
  if (debugflag)
    fprintf(stderr, "Returned from declaring cursor, return value %d\n",
		    retval);
  /* OK. Now we can be tolerably certain that the cursor contains at least
   * one instance, but woe is us if it contains more than one! */
  if (retval > 1) /* Fatal error: corrupt database */
  {
    mdebs_err(MDEBSERR_MULTJOUENT, date, serial);
    mdebs_err(MDEBSERR_FATALERR);
    exit_nicely();
  }
  /* Pfwyew! */

  fetchNextJournalEntry(jent, "a1");

  asprintf(&buf, "CLOSE a1;");
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    rollbackTransaction();
    free(buf);
    return (struct journal_entry *)NULL;
  }
  endTransaction();
  return jent;
}

/*
/*
 * Function verify_journal_entry()
 *
 * Performs various checks on a journal entry to see if it's OK.
 * Fills in the shortcut member of the structure.
 *
 * Takes 2 arguments:
 * 	je 	Pointer to journal entry structure
 *
 * Returns
 * 	1	on success (entry is OK)
 * 	0	on failure (entry has some kind of defect)
 */
int verify_journal_entry(struct journal_entry *je)
{
  char *fname = "verify_journal_entry()";
  int i, retval, deb_counter, cre_counter;
  struct date_range *fisc;
  struct short_cut *shaddap;

  if (debugflag)
    fprintf(stderr, "Entering function verify_journal_entry()\n");

  retval = 1;

  /*
   * First check the journal entry date 
   */
  
  /* Is it a well-formed date? */
  if (!valiDate(je->ent_date))
  {
    mdebs_err(MDEBSERR_NOSUCHDATE, je->ent_date);
    retval = 0;
  }

  /* Is it within the fiscal year? */
  fisc = query_fiscal(QUIET);
  if (je->ent_date < fisc->datefr || je->ent_date > fisc->dateto)
  {
    mdebs_err(MDEBSERR_DATENOINFISC, je->ent_date);
    retval = 0;
  }

  /* Is the shortcut code valid? */
  if (query_shortcut(&je->shortcut, (char *)NULL) == (struct short_cut *)NULL) 
  { /* No shortcut */
    mdebs_err(MDEBSERR_NOSUCHSHCODE, je->shortcut);
    retval = 0;
  }
	  
  deb_counter = 0;
  cre_counter = 0;

  for (i = 0; i < 10; i++)
  {
    if (je->values[i].acct == -1) break; /* No more lines in entry */

    /* If it's a debit, increment deb_counter, otherwise increment
     * cre_counter */
    if (je->values[i].debit) deb_counter = deb_counter + je->values[i].amount;
    else cre_counter = cre_counter + je->values[i].amount;

    if (accountName(je->values[i].acct, je->values[i].anal) == (char *)NULL)
    {
      mdebs_err(MDEBSERR_NOSUCHACCT, je->values[i].acct, je->values[i].anal);
      retval = 0;
    }
  }

  if (deb_counter != cre_counter)
  {
    mdebs_err(MDEBSERR_JOUIMBAL);
    retval = 0;
  }

  return retval;
}

/*
 * Function write_journal_entry()
 *
 * Runs all the SQL queries necessary to write a journal entry to the database
 * Fills in the ser_num element of the structure.
 * 
 * Takes following argument(s):
 *  	rec	Pointer to journal entry structure
 *
 * Returns:
 * 	serial number of journal entry written
 */
int write_journal_entry(struct journal_entry *rec)
{
  char *fname = "write_journal_entry()";
  char buf[MAX_QUERY];
  char *tmpbuf1;
  int sernum;
  int j;
  int retval;

  if (debugflag)
    fprintf(stderr, "Entering function write_journal_entry()\n");

  startTransaction();

  /* Query maximum serial number */
  if ((sernum = query_serial_by_date(rec->ent_date)) == -1) {
    /* Fatal error */
    fprintf(stderr, "Fatal error in query_serial_by_date()\n");
    exit_nicely();
  }

  if (debugflag)
    fprintf(stderr, "Previous max ser_num was %d\n", sernum);

  /* Increment serial number */
  sernum++;
  rec->ser_num = sernum;

  /* Insert description for this serial number */
  sprintf(buf, "INSERT INTO jou_head VALUES (nextval('%s'), '%s', %d, '%s', '%s', %d);", MDEBS_SERIAL, pgConvertDate(rec->ent_date), rec->ser_num, rec->desig, rec->document, rec->shortcut);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    rollbackTransaction();
    return -1;
  }
  buf[0] = '\0';
  assert (retval == 1);  /* This is fairly draconian, but runQuery() should 
			    return 1 or -1 on an INSERT, nothing else */

  /* Insert account numbers and amounts for this serial number */
  for (j = 0; rec->values[j].acct != -1; j++)
  {
    assert (j < 10);
    sprintf(buf, "INSERT INTO jou_lines VALUES (currval('%s'), '%s', %d, ", 
	MDEBS_SERIAL, pgConvertDate(rec->ent_date), rec->ser_num);
    if (rec->values[j].debit) 
      strncat(buf, "true, ", MAX_QUERY - strlen(buf) - 1);
    else
      strncat(buf, "false, ", MAX_QUERY - strlen(buf) - 1);
    asprintf(&tmpbuf1, "'%03d', '%03d', %d);", rec->values[j].acct, 
	rec->values[j].anal, rec->values[j].amount);
    strncat(buf, tmpbuf1, MAX_QUERY - strlen(buf) - 1);
    retval = runQuery(buf);
    if (retval == -1) {
      mdebs_queryerr(buf, fname);
      rollbackTransaction();
      free(buf);
      return -1;
    }
    buf[0] = '\0';
    assert (retval == 1);
  }

  endTransaction();
  
  return sernum;
}


void print_journal_entry (FILE *fp, struct journal_entry je)
{
  char *fname = "print_journal_entry()";
  int i;

  if (debugflag) {
    fprintf(stderr, "Entering function print_journal_entry()\n");
    fp = stderr;
  }

  fprintf(fp, "%d:%03d -*- %s -*- %s\n", 
	      je.ent_date, je.ser_num, 
	      strndup(unbackslash_single_quotes(je.document), DOCUMENT_DESIG_LENGTH), 
	      strndup(unbackslash_single_quotes( query_shortcut(&je.shortcut, (char *)NULL)->desig ), SHORTCUT_DESIG_LENGTH));
  if (strlen(je.desig) > 0)
    fprintf(fp, "%s\n", 
	      strndup(unbackslash_single_quotes(je.desig), JOURNAL_DESIG_LENGTH));
  fprintf(fp, "-----------------------------------------------------------------\n");
  for (i = 0; i < 10; i++)
  {
    if (je.values[i].acct == -1) break;

    if (debugflag) {
      fprintf(stderr, "i == %d, je.values[i].debit == %d\n", i, je.values[i].debit);
      fprintf(stderr, "         je.values[i].acct == %d\n", je.values[i].acct);
      fprintf(stderr, "         je.values[i].anal == %d\n", je.values[i].anal);
      fprintf(stderr, "         je.values[i].amount == %d\n", je.values[i].amount);
    }
    if (debugflag && je.values[i].amount < 0) {
      fprintf(stderr, "je.values[i].amount is less than zero!\n");
    }
    if (je.values[i].debit) /* It's a debit */
    {
      fprintf(fp, "%03d-%03d|%-29s|", je.values[i].acct, je.values[i].anal, 
	  strndup(accountName(je.values[i].acct, je.values[i].anal), 29));
      fprintf(fp, "%13.2f|\n", ( (double)je.values[i].amount/(double)100 ) );
      continue;
    } 
    /* It's a credit */
    fprintf(fp, "%03d-%03d|%-29s|             |%13.2f\n", 
	  je.values[i].acct, je.values[i].anal, 
	  strndup(accountName(je.values[i].acct, je.values[i].anal), 29),
	  ( (double)je.values[i].amount/(double)100 ) ); 
    continue;
  }
}

void wprint_journal_entry (FILE *fp, struct journal_entry je)
{
  char *fname = "print_journal_entry()";
  int i;

  if (debugflag) {
    fprintf(stderr, "Entering function print_journal_entry()\n");
    fp = stderr;
  }

  fwprintf(fp, L"%d:%03d -*- %s -*- %s\n", 
	      je.ent_date, je.ser_num, 
	      strndup(unbackslash_single_quotes(je.document), DOCUMENT_DESIG_LENGTH), 
	      strndup(unbackslash_single_quotes( query_shortcut(&je.shortcut, (char *)NULL)->desig ), SHORTCUT_DESIG_LENGTH));
  if (strlen(je.desig) > 0)
    fwprintf(fp, L"%s\n", 
	      strndup(unbackslash_single_quotes(je.desig), JOURNAL_DESIG_LENGTH));
  fwprintf(fp, L"-----------------------------------------------------------------\n");
  for (i = 0; i < 10; i++)
  {
    if (je.values[i].acct == -1) break;

    if (debugflag) {
      fprintf(stderr, "i == %d, je.values[i].debit == %d\n", i, je.values[i].debit);
      fprintf(stderr, "         je.values[i].acct == %d\n", je.values[i].acct);
      fprintf(stderr, "         je.values[i].anal == %d\n", je.values[i].anal);
      fprintf(stderr, "         je.values[i].amount == %d\n", je.values[i].amount);
    }
    if (debugflag && je.values[i].amount < 0) {
      fprintf(stderr, "je.values[i].amount is less than zero!\n");
    }
    if (je.values[i].debit) /* It's a debit */
    {
      fwprintf(fp, L"%03d-%03d|%-29s|", je.values[i].acct, je.values[i].anal, 
	  strndup(accountName(je.values[i].acct, je.values[i].anal), 29));
      fwprintf(fp, L"%13.2f|\n", ( (double)je.values[i].amount/(double)100 ) );
      continue;
    } 
    /* It's a credit */
    fwprintf(fp, L"%03d-%03d|%-29s|             |%13.2f\n", 
	  je.values[i].acct, je.values[i].anal, 
	  strndup(accountName(je.values[i].acct, je.values[i].anal), 29),
	  ( (double)je.values[i].amount/(double)100 ) ); 
    continue;
  }
}

