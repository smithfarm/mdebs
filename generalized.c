#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>
#include "mdebs.h"

#include "journal.h"
#include "messages.h"
#include "pgresfunc.h"

#include "generalized.h"

/*
 * generalized.c
 *
 * this file contains generalized functions of use in various parts of
 * mdebs.
 */

/*
 * Determine definition status of environment variable MDEBSDB
 * Return NULL if undefined, contents of MDEBSDB otherwise.
 */
char *query_environment()
{
  return getenv("MDEBSDB");
}

/*
/*
 * dbname_to_lower()
 *
 * Converts dbname to lowercase and prints message if it encounters any
 * uppercase letters
 *
 * This function is internal to generalized.c and its purpose is to avoid
 * duplicating code in dbQuery() and dbQueryfull(), which essentially do
 * the same thing.
 *
 * Returns -1 if dbname is too long; 0 otherwise.
 */
int dbname_to_lower(char *dbname)
{
  int n;
  int flag;

  flag = 0; /* Will be set to 1 if we encounter uppercase */

  /* Check for overly long dbname and convert any upper case letters to
   * lowercase (postgresql database names are not case sensitive) */
  for (n = 0; n < strlen(dbname); n++) {
    if (n > 24) {
      mdebs_err(MDEBSERR_DBNAMTOOLONG);
      return -1;
    }
    if (isupper(*(dbname+n))) {
      *(dbname+n) = tolower(*(dbname+n));
      flag = 1;
    }
  }
  if (flag)
    mdebs_msg(MDEBSMSG_DBUPCASE);
  return 0;
}

/*
 * dbQuery()
 *
 * Given the name of a database, determines if it exists in the system
 * Returns -1 on backend or dbname-too-long error, 0 if not found, 1 otherwise.
 */
int dbQuery(char *dbname)
{
  char *fname = "dbQuery()";
  char *buf;
  int n, retval;

  if (dbname_to_lower(dbname) == -1) 
    return -1;
  asprintf(&buf, "SELECT * FROM pg_database WHERE datname='%s';", dbname);
  retval = runQuery(buf);
  if (retval == -1) 
    mdebs_queryerr(buf, fname);
  free(buf);
  return retval;
}

/*
 * dbQueryfull()
 *
 * Given the name of a database, determines if it exists in the system.
 * Assumes backend connection not open!
 *
 * Returns
 * -1 on backend-related error or if dbname too long
 *  0 if database not found or failed sanity check
 *  1 database exists
 */
int dbQueryfull(char *dbname)
{
  char *fname = "dbQueryfull()";
  char *buf;
  int retval;

  if (debugflag) 
    fprintf(stderr, "dbQueryfull(): " \
	    "Function called with argument %s.\n", dbname);

  if (dbname_to_lower(dbname) == -1)
    return -1;

  if (debugflag)
    fprintf(stderr, "dbQueryfull(): " \
            "Opening backend connection to template1...\n");

  if (openBackend((char *)NULL) < 0) {
    fprintf(stderr, "dbQueryfull(): Failed to open template1 database?!\n");
    fprintf(stderr, "%s", pgresfunc_ERROR);
    return -1;
  }
  
  if (debugflag)
    fprintf(stderr, "dbQueryfull(): " \
            "Backend connection to template1 open...\n");

  asprintf(&buf, "SELECT * FROM pg_database WHERE datname='%s';", dbname);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
  }
  free(buf);

  closeBackend();

  return retval;
}

/*
 * query_mdebsdb()
 *
 * Assumes an open backend connection.
 *
 * Returns
 * -1 on backend-related error
 *  0 connection is to a non-mdebs database
 *  1 connection is to an mdebs database
 */
int query_mdebsdb(void)
{
  char *fname = "query_mdebsdb()";
  char *sqlquery = "SELECT * FROM pg_tables WHERE tablename='shortcut';";
  int retval;

  retval = runQuery(sqlquery);
  if (retval == -1) {
    mdebs_queryerr(sqlquery, fname);
    return -1;
  }
  if (retval == 0) return 0;
  else return 1;
}

/*
 * transQuery()
 *
 * Given a transaction (i.e., a journal entry, determines if it exists in the
 * database.
 * Returns -1 on backend choke, 0 if not found, 1 otherwise.
 */
int transQuery(int date, int sernum)
{
  char *fname = "transQuery()";
  char *buf;
  int retval, noofrecs;
  char *pgDatum;

  if (debugflag) {
    fprintf(stderr, "Entering function transQuery()\n");
    fprintf(stderr, "Arguments passed were %d:%03d\n", date, sernum);
  }

  pgDatum = pgConvertDate(date);

  if (debugflag)
    fprintf(stderr, "Date converted to '%s'\n", pgDatum);

  asprintf(&buf, "SELECT * FROM jou_head WHERE ent_date='%s' and ser_num=%d;", 
      pgDatum, sernum);
  free(pgDatum);

  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return -1;
  }
  free(buf);

  if (debugflag) {
    fprintf(stderr, "Query returned %d\n", retval);
  }

  return retval;
}

/*
 * NathanConvertDate()
 *
 * This function converts dates from a 'mm-dd-yyyy' character string to
 * an yyyymmdd integer.
 *
 * This is a "garbage in, garbage out" function.
 */
int NathanConvertDate(char *pgdate)
{
  short year, month, day;
  char buf[4];

  buf[0] = *pgdate;
  buf[1] = *(pgdate + 1);
  buf[2] = '\0';
  month = atoi(buf);
  
  buf[0] = *(pgdate + 3);
  buf[1] = *(pgdate + 4);
  buf[2] = '\0';
  day = atoi(buf);

  buf[0] = *(pgdate + 6);
  buf[1] = *(pgdate + 7);
  buf[2] = *(pgdate + 8);
  buf[3] = *(pgdate + 9);
  buf[4] = '\0';
  year = atoi(buf);

  return ((year*10000)+(month*100)+day);
}

/*
 * pgConvertDate()
 *
 * This function is the reverse of the NathanConvertDate()
 * function. It converts dates from a yyyymmdd integer to a
 * 'mm-dd-yyyy' character string.
 *
 * This is a "garbage in, garbage out" function.  
 *
 * The value returned is allocated with malloc.  Use free() to avoid
 * potential memory leaks (although the potential is not all that great).
 */
char *pgConvertDate(int nathandate)
{
  char *pgdate;
  short year, month, day;

  pgdate = malloc(11);

  year = nathandate / 10000;
  nathandate = nathandate - (year*10000);
  month = nathandate / 100;
  nathandate = nathandate - (month*100);
  day = nathandate;

  sprintf(pgdate, "%02d-%02d-%4d", month, day, year);
  return pgdate;
}

/*
 * countNonWhiteSpace()
 *
 * Counts the number of non-white-space characters in a particular string
 */
int countNonWhiteSpace(char *buff)
{
  int i, numnonwhite;
  i = 0; numnonwhite = 0;
  while (*(buff+i) != '\0') {
      if ((*(buff+i) != ' ') &&
	  (*(buff+i) != '\t') &&
	  (*(buff+i) != '\n')) {
	numnonwhite++;
      }
      i++;
  }
  return numnonwhite;
}

/*
 * valiDate()
 *
 * Checks an integer date to see if it is valid
 * (still quite rudimentary)
 */
int valiDate(int datum)
{
  struct tm intDatum;
  time_t intrep;
  char *yyyymmdd;
  int ymd;
  int year, month, day;

  if (debugflag)
    fprintf(stderr, "Entering function valiDate()\n");
  /*
   * is the given date before 1900?
   */
  if ((datum - 19000000) < 0) {
    return 0;
  }

  /*
   * Initialize the broken down time structure
   */
  intDatum.tm_sec = 0;
  intDatum.tm_min = 0;
  intDatum.tm_hour = 12;
  day = datum % 100;
  intDatum.tm_mday = day;
  month = (datum % 10000) / 100 - 1;
  intDatum.tm_mon = month;
  year = (datum - 19000000) / 10000;   /* Y2K note: tm_year is the number
					  of years that have elapsed since
					  1900 */
  intDatum.tm_year = year;
  intDatum.tm_isdst = -1;

  if (debugflag)
    fprintf(stderr, "valiDate(): year %d, month %d, day %d\n", 
	year, month, day);

  /*
   * Get the calendar time expressed in seconds relative to Jan. 1, 1970
   */
  intrep = mktime(&intDatum);

  /*
   * If the call to mktime failed, then the date is invalid
   */
  if (intrep == -1) {
    return 0;
  }

  /*
   * Now construct a YYYYMMDD string with the time returned by mktim()
   */
  yyyymmdd = malloc(9);
  strftime(yyyymmdd, 9, "%Y%m%d", &intDatum);

  /*
   * Convert the string to an integer
   */
  ymd = atoi(yyyymmdd);

  /*
   * Compare ymd with the date in question -- if they are the same, then
   * the date is valid.  If they are different, then the date is invalid.
   */
  if (ymd != datum) {
    return 0;
  }
  return 1;
}

/*
 * This function returns a string containing the current date and time
 * Useful for logging, I think.
 */
char *theTimeIs(void)
{
  time_t now;
  char *output;
  now = time(NULL);
  output = ctime(&now);
  *(output+24) = '\0';
  return output;
}

/*
 * strntok()
 *
 * Returns number of tokens in string, -1 if string contains non-printing
 * character and is of length > 0.
 */
int strntok(char *stringy)
{
  int i, countme;

  if (strlen(stringy) == 0) return 0;
  if (!isprint(*(stringy))) return -1;

  countme = 0;
  i = 1;
  while (1) {
    if ((!isspace(*(stringy+i-1)) && isspace(*(stringy+i))) ||
	(!isspace(*(stringy+i-1)) && (*(stringy+i) == '\0'))) {
      countme++;
    }
    i++;
    if (*(stringy+i) == '\0') break;
  }
  return countme;
}

/*
 * accountName()
 *
 * Given an account number, returns the account's designation.
 * Returns ERROR on error, NULL if account not found.
 */
char *accountName(int acct1, int acct2)
{
  char *fname = "accountName()";
  char *buf;
  int retval;

  asprintf(&buf, "SELECT desig FROM chart WHERE acct='%03d' and anal='%03d';", 
      acct1, acct2);
  retval = runQuery(buf);
  if (retval == -1) {
    mdebs_queryerr(buf, fname);
    free(buf);
    return "ERROR";
  }
  if (retval == 0)
    return (char *)NULL;
  if (retval == 1)
    return getField("desig");
  /* What? Query returned more than one record? */
  mdebs_err(MDEBSERR_MULTACCT, acct1, acct2);
  return "ERROR";
}

void *
xmalloc (size_t size)
{
  register void *value = malloc (size);
  if (value == 0)
  {
    mdebs_err(MDEBSERR_OUTOFMEM);
    assert(1 != 1);
  }
  return value;
}

char *get_regerror (int errcode, regex_t *compiled)
{
  size_t length = regerror (errcode, compiled, NULL, 0);
  char *buffer = xmalloc (length);
  (void) regerror (errcode, compiled, buffer, length);
  return buffer;
}

/*
 * Attempts to compile regexp, returns 1 on success, 0 on error
 *
 * Note that this code doesn't seem to catch many errors.  PostgreSQL uses
 * its own regex implementation which is a lot more picky about what it
 * accepts.  Therefore it is necessary to handle backend errors on all SQL
 * queries that use regexps, preferably using MDEBSERR_REGEXPCHOKE.  In
 * other words, the use of this function does not guarantee that the
 * backend will like a given regexp.
 */
int verify_regexp(char *arg)
{
  regex_t holdingtank;
  int retval;

  if (debugflag)
    fprintf(stderr, "Checking validity of regexp ->%s<-\n", arg);

  retval = regcomp(&holdingtank, arg, REG_EXTENDED);
  if (debugflag) {
    fprintf(stderr, "regcomp() returned %d\nPaused - press a key", retval);
    getchar();
  }
  if (retval != 0)
  {
    fprintf(stderr, "%s\n", get_regerror(retval, &holdingtank));
    return 0;
  }
  regfree(&holdingtank);
  return 1;
}

/*
 * Function backslash_single_quotes()
 *
 * Historical note:
 *   This function used to be strip_quotes().  However, since I figured out
 *   how to get the lexanalyzer to strip quotes from quoted strings, this
 *   function was obsolete.
 *
 * Adds backslash characters before all non-backslashed single-quotes in a
 * given string.
 *
 * Takes one argument:
 * 	str	Pointer to char
 *
 * Returns:
 * 	Newly allocated character string guaranteed not to contain
 * 	non-backslashed single-quotes
 */
char *backslash_single_quotes(char *str)
{
  int i, j;
  int cntr = 0;
  char *new_str;

  if (debugflag) {
    fprintf(stderr, "Entering function backslash_single_quotes()...\n");
  }

  if (str == (char *)NULL) {
    if (debugflag) {
      fprintf(stderr, "backslash_single_quotes(): String is %s\n", str);
      fprintf(stderr, "backslash_single_quotes(): Nothing to do\n");
    }
    return str;
  }

  /* Count non-backslashed single-quote characters in string */
  for (i=0; i<strlen(str); i++) {
    /* Case when '\'' occurs at beginning of string */
    if ( (i == 0) && ( *(str) == '\'' ) ) cntr++;
    /* Other cases */
    if ( (i > 0) && (*(str+i-1) != '\\') && (*(str+i) == '\'') ) cntr++;
  }
  if (debugflag)
    fprintf(stderr, 
	"%d non-backslashed single-quote characters counted\n", cntr);
  if (cntr == 0) /* Return the same pointer */
    return str;

  /* Allocate memory for new string 
   * (old string length) + (number of single quotes) + (null-termination)
   */
  new_str = (char *)malloc(strlen(str) + cntr + 1);
  
  for (i=j=0; i < strlen(str); i++) 
  {
    if (debugflag)
      fprintf(stderr, 
	  "backslash_single_quotes(): processing character \'%c\'\n", *(str+i));
    if (i == 0) /* No previous character */
    {
      if (*(str) == '\'') {
        if (debugflag)
          fprintf(stderr, 
	      "Single-quote encountered at beginning of string\n");
        *(new_str + (j++)) = '\\';
        *(new_str + (j++)) = *(str + i);
      } else 
        *(new_str + (j++)) = *(str + i);
    }
    else /* We can check previous character */
    {
      if ( (*(str+i-1) != '\\') && (*(str+i) == '\'') ) {
        if (debugflag)
          fprintf(stderr, 
	      "Single-quote encountered\n");
        *(new_str + (j++)) = '\\';
        *(new_str + (j++)) = *(str + i);
      } else 
        *(new_str + (j++)) = *(str + i);
    }
  }
  *(new_str + j) = '\0';

  if (debugflag)
    fprintf(stderr, "Backslashified string is... \n ->%s<-\n", new_str);

  /* Return (str contains pointer to backslashified string) */
  return new_str;
}

/*
 * Function unbackslash_single_quotes()
 *
 * Removes backslash characters before all backslashed single-quotes in a
 * given string.
 *
 * Takes one argument:
 * 	str	Pointer to char
 *
 * Returns:
 * 	Newly allocated character string guaranteed not to contain
 * 	backslashed single-quotes
 */
char *unbackslash_single_quotes(char *str)
{
  int i, j;
  int cntr = 0;
  char *new_str;

  if (debugflag)
    fprintf(stderr, "Entering function unbackslash_single_quotes()...\n");

  /* Count backslashed single-quote characters in string */
  for (i=0; i<strlen(str); i++) {
    /* We can't have a backslashed single-quote in the first position in
     * the string */
    if (i == 0) continue;
    /* Other cases */
    if ( (i > 0) && (*(str+i-1) == '\\') && (*(str+i) == '\'') ) cntr++;
  }
  if (debugflag)
    fprintf(stderr, 
	"%d backslashed single-quote characters counted\n", cntr);
  if (cntr == 0) /* Return the same pointer */
    return str;

  /* Allocate memory for new string 
   * (old string length) - (no. of backslashed single-quotes) + (null)
   */
  new_str = (char *)malloc(strlen(str) - cntr + 1);
  
  for (i=j=0; i < strlen(str); i++) 
  {
    if (debugflag)
      fprintf(stderr, 
	  "unbackslash_single_quotes(): processing character \'%c\'\n", *(str+i));
    if ( (*(str + i) == '\\') && (*(str + i + 1) == '\'') ) {
      if (debugflag)
        fprintf(stderr, "Backslashed single-quote encountered\n");
      *(new_str + (j++)) = '\'';
      i++; /* Skip the single-quote, since we know it's there */
      continue;
    } else 
      *(new_str + (j++)) = *(str + i);
  }
  *(new_str + j) = '\0';

  if (debugflag)
    fprintf(stderr, "Unbackslashified string is... \n ->%s<-\n", new_str);

  /* Return (str contains pointer to backslashified string) */
  return new_str;
}

/*
 * open_tmp()
 *
 * Opens a temporary file for "dumping".
 *
 * Takes one argument:
 *   tmpfilname (pointer to char)
 * to which memory is allocated.
 *
 * Returns:
 *   pointer to FILE, (FILE *)NULL on failure.
 *
 * Puts name of temp file in tmpfilname.
 */
FILE *open_tmp(char **tmpfilname)
{
  FILE *fp;

  /* Determine the name of our temporary file for writing */
  if (debugflag)
    fprintf(stderr,
           "open_tmp(): Allocating space (%d chars) for filename\n", L_tmpnam);
  *tmpfilname = malloc(L_tmpnam);
  tmpnam_r(*tmpfilname);
  if (debugflag)
    fprintf(stderr, "open_tmp(): temporary file name generated:\n%s\n", 
		    *tmpfilname);
 
  /* Open the temporary file for writing */
  fp = fopen(*tmpfilname, "w");
  return fp;
}

/*
 * close_tmp()
 *
 * Closes an open temporary file using the file stream pointer as an
 * argument
 */
int close_tmp(FILE *fp)
{
  return fclose(fp);
}

/* browse_tmp()
 *
 * Browse a temporary file (actually, any file known to exist)
 *
 * Takes argument (a file name)
 */
void browse_tmp(char *tmpfilname)
{
  int n;
  char *pager_cmd;

  /* Browse the temporary file */
  n = strlen(MDEBS_PAGER_DIRECTORY 
	     MDEBS_PAGER_COMMAND " "
	     MDEBS_PAGER_OPTION " ") + strlen(tmpfilname);
  n++;
  if (debugflag)
    fprintf(stderr, 
      "Allocating %d chars of memory to accomodate following string:\n%s%s\n", 
      n, MDEBS_PAGER_DIRECTORY MDEBS_PAGER_COMMAND " " MDEBS_PAGER_OPTION " ", 
      tmpfilname);
  pager_cmd = (char *)xmalloc(n);
  *pager_cmd = '\0'; /* Musn't forget to initialize the string! */
  if (debugflag) fprintf(stderr, "Building pager_cmd from ->%s<-\n", pager_cmd);
  strcat((char *)pager_cmd, MDEBS_PAGER_DIRECTORY 
		            MDEBS_PAGER_COMMAND " "
			    MDEBS_PAGER_OPTION " ");
  if (debugflag) fprintf(stderr, "... ->%s<-\n", pager_cmd);
  strcat((char *)pager_cmd, tmpfilname);
  if (debugflag) fprintf(stderr, "... ->%s<-\n", pager_cmd);

  if (debugflag) {
    fprintf(stderr, "dump_journal(): "
	    "Preparing to execute the following command using system()\n"
	    "%s\n", pager_cmd);
    fprintf(stderr, "Press a key to continue...\n");
    getchar();
  }
  system(pager_cmd);
}

/* print_tmp()
 *
 * Browse a temporary file (actually, any file known to exist)
 *
 * Takes argument (a file name)
 */
void print_tmp(char *tmpfilname)
{
  int n;
  char *print_cmd;

  /* Print the temporary file */
  n = strlen(MDEBS_PRINT_COMMAND " "
	     MDEBS_PRINT_OPTION " ") + strlen(tmpfilname) +
      strlen(" | " MDEBS_PRINT_PIPE_CMD);
  n++;
  if (debugflag)
    fprintf(stderr, "Allocating memory for print command.\n");
  print_cmd = (char *)xmalloc(n);
  *print_cmd = '\0'; /* Musn't forget to initialize the string! */
  if (debugflag) fprintf(stderr, "Building print_cmd from ->%s<-\n", print_cmd);
  strcat((char *)print_cmd, MDEBS_PRINT_COMMAND " "
			    MDEBS_PRINT_OPTION " ");
  if (debugflag) fprintf(stderr, "... ->%s<-\n", print_cmd);
  strcat((char *)print_cmd, tmpfilname);
  if (debugflag) fprintf(stderr, "... ->%s<-\n", print_cmd);
  strcat((char *)print_cmd, " | " MDEBS_PRINT_PIPE_CMD);
  fprintf(stderr, "... ->%s<-\n", print_cmd);

  if (debugflag) {
    fprintf(stderr, "dump_journal(): "
	    "Preparing to execute the following command using system()\n"
	    "%s\n", print_cmd);
    fprintf(stderr, "Press a key to continue...\n");
    getchar();
  }
  system(print_cmd);
}

/*
 * remove_tmp()
 *
 * Removes a temporary file (actually, any existing file, provided process
 * has required privileges
 */
void remove_tmp(char *tmpfilname)
{
  if (remove(tmpfilname) == 0) 
  {
    if (debugflag)
      fprintf(stderr, "remove_tmp(): temporary file successfully removed\n");
  } 
  else 
  {
    if (debugflag)
      fprintf(stderr, "remove_tmp(): problem encountered removing tmpfile\n");
  }
}

int power(int base, int exponent)
{
  int i, retval;

  if ( (base < 0) || (base > 100) )
    return -1;

  if ( (exponent < 0) || (exponent > 64) )
    return -1;

  retval = 1;
  for (i=0; i<exponent; i++)
    retval = retval * base;
  return retval;
}

char *to_bin(char *ret, int i)
{
  int n, x, pos, digs;
  
  digs = 1;
  n = i;
  do {
    if (n >= 2) {
      digs++;
      n = n / 2;
    } else break;
  }
  while (1);
  /* printf("digits: %02d  ", digs); */
  ret = (char *)malloc(digs+1);
  for (pos=0; pos<digs; pos++) *(ret+pos) = ' ';
  *(ret+digs) = '\0';
  if (debugflag)
    fprintf(stderr, "Initialized buffer ->%s<- Length %d\n", ret, strlen(ret));
  x = i;
  pos = 0;
  /* printf("x == %02d  ", x); */
  for (n=(digs-1); n>0; n--) {
    if ( (x - power(2, n)) >= 0 ) {
      *(ret+pos) = '1'; 
      x = x - power(2, n);
    } else 
      *(ret+pos) = '0';
    pos++;
  }
  if (x == 0) *(ret+pos) = '0';
  else *(ret+pos) = '1';

  return ret;
}

/*
 * to_bool()
 *
 * Takes a character value of 't' or 'f' and returns corresponding bool
 * value.
 */
bool to_bool(char *val)
{
  bool retval;

/*  if (debugflag) {
    fprintf(stderr, "to_bool(): true is %d, false is %d\n", true, false);
    fprintf(stderr, "to_bool(): evaluating value '%s'... ", val);
  } */
  if (strcmp(val, "t") == 0) {
    retval = true;
  }
  else if (strcmp(val, "f") == 0) {
    retval = false;
  }
  else retval = (bool)NULL;
/*  if (debugflag)
    fprintf(stderr, "returning %d\n", retval); */
  return retval;
}
