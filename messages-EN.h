/*
 * messages-EN.h
 *
 * Default English-language messages
 */
void mdebs_err(const char *fmt, ...); /* Error-priority messages */
void mdebs_res(FILE *, const char *fmt, ...); /* Result-priority messages */
void mdebs_msg(const char *fmt, ...); /* Message-priority messages */
void mdebs_queryerr();		/* Query errors */

/*
 * Error-priority messages
 */
#define MDEBSERR_OUTOFMEM "Out of virtual memory\n"
#define MDEBSERR_NOHELPSYSTEM "No help system available\n"
#define MDEBSERR_BADCMDOPT "Bad command line option: %s\n"
#define MDEBSERR_NODBOPEN "No database open\n"
#define MDEBSERR_BADACCTNOFMT "Bad account no. format\n"
#define MDEBSERR_NOSUCHDATE "The date %d is invalid\n"
#define MDEBSERR_NOSUCHACCT "Account no. %03d-%03d does not exist\n"
#define MDEBSERR_ACCTALREADY "Account no. %03d-%03d already exists\n"
#define MDEBSERR_DBNAMTOOLONG "Database name is too long (maxlen 24)\n"
#define MDEBSERR_DBNOTSPEC "No database specified (run mdebs --help for help)\n"
#define MDEBSERR_NONEXISTENTDB "Database %s does not exist\n"
#define MDEBSERR_TABLESFOUND "Non-system table(s) found in ->%s<-\n"
#define MDEBSERR_CANTINIT "Cannot initialize database ->%s<-\n"
#define MDEBSERR_NOTMDEBSDB \
  "Database %s is not an mdebs " MDEBS_VERSION_NUMBER " database!\n"
#define MDEBSERR_NODELDB \
  "You can only change the database, not delete it\n"
#define MDEBSERR_NOFISC "The fiscal year is not initialized\n"
#define MDEBSERR_MULTFISC \
  "Corrupt database: Multiple fiscal years defined\n"
#define MDEBSERR_MULTSERIAL \
  "Corrupt database: Multiple serial numbers defined for date %d\n"
#define MDEBSERR_MULTJOUENT \
  "Corrupt database: Multiple journal entries registered under %d:%03d\n"
#define MDEBSERR_MULTACCT \
  "Corrupt database: Multiple chart entries for account %03d-%03d\n"
#define MDEBSERR_FISCALREADY "The fiscal year is already initialized\n"
#define MDEBSERR_FISCBADDATES \
  "%d and %d are not appropriate starting and ending dates\n"
#define MDEBSERR_CHARTEMPTY "The Chart of Accounts is empty\n"
#define MDEBSERR_BADDATERANGE "Date range must fall within fiscal year\n"
#define MDEBSERR_DATENOINFISC "Date %d is not in fiscal year\n"
#define MDEBSERR_DEBCREDZERO \
  "Journal entry contains improper line\n"
#define MDEBSERR_JOUIMBAL \
  "Imbalanced journal entry; debit and credit sides don't add up!\n"
#define MDEBSERR_INVALIDJOU "Invalid journal entry encountered\n"
#define MDEBSERR_JOUNOEXIST "Journal entry %d:%d does not exist\n"
#define MDEBSERR_NOJOUMATCH "No journal entries matched\n"
#define MDEBSERR_JOUEXCEED \
  "Query overflow; only displaying first 10 matches\n"
#define MDEBSERR_BADREGEXP "Invalid regular expression\n"
#define MDEBSERR_REGEXPCHOKE "Backend probably choked on regular expression\n"
#define MDEBSERR_SHORTCUTEMPTY "Short cut table is empty\n"
#define MDEBSERR_SHORTALREADYNUM \
  "There is already a shortcut no. %d in the database\n"
#define MDEBSERR_MAXSHCBADVAL "SELECT MAX(code) FROM shortcut; returned %d!?\n"
#define MDEBSERR_SHORTALREADYNAME \
  "That shortcut is already registered in the database under no. %d\n"
#define MDEBSERR_SHCINSTPREZ \
  "Instances of that shortcut found. Cannot delete.\n"
#define MDEBSERR_SHCODEINVALID "Short cut code must be >= 1\n"
#define MDEBSERR_SHQUEINCOMP "You must provide a code or shortcut to query\n"
#define MDEBSERR_NOSUCHSHCODE "Shortcut ->%d<- not found\n"
#define MDEBSERR_NOSUCHSHDESIG "Shortcut ->%s<- not found\n"
#define MDEBSERR_PARSEERR "Command not recognized\n"
#define MDEBSERR_CANTDECLCURSOR "Unable to declare cursor\n"
#define MDEBSERR_DUMJOUILLEGAL \
  "Account number specifier doesn't work for DUMP JOURNAL.  Sorry.\n"
#define MDEBSERR_BACKENDERR "Backend related error in function %s\n"
#define MDEBSERR_QUERYWAS "The offending query was: \n %s\n"
#define MDEBSERR_PAUSEPRESSKEY "Paused. Press any key...\n"
#define MDEBSERR_FATALERR "FATAL ERROR\n"

/*
 * Lexical errors
 */
#define LEXERR_APOSTROPHE \
  "Apostrophe encountered. ANATHEMA!!!\n Don't use apostrophes - " \
  "postgresql is not set up to handle them <lame shrug>\n"
#define LEXERR_STREXCTOKEN "String exceeds maximum token length\n"
#define LEXERR_NUMEXCTOKEN "Number exceeds maximum token length\n"
#define LEXERR_NOENDQUOTE "No closing double-quote found\n"
#define LEXERR_SERNUMTOOBIG "Serial number cannot exceed 4 digits\n"
#define LEXERR_MINNUMTOOBIG "Minor number cannot exceed 3 digits\n"

/*
 * Result-priority messages
 */
#define MDEBSRES_CURRENTDB "The current database is %s.\n"
#define MDEBSRES_FISCIS "The fiscal year is set to %d-%d\n"
#define MDEBSRES_CHARTHEAD "CHART OF ACCOUNTS: %s\n\n"
#define MDEBSRES_SHORTCUTHEAD "Short cut header\n"
#define MDEBSRES_CHARTEXIST "Account exists in Chart of Accounts.\n"
#define MDEBSRES_CHARTNOEXIST "No such account(s) in Chart of Accounts.\n"
#define MDEBSRES_CHARTACCTN "%d accounts found.\n"
#define MDEBSRES_SHCUTNOTFOUND "Short cut not found\n"
#define MDEBSRES_SHORTCUT "Short cut %d is %s\n"
#define MDEBSRES_NOJOUFOUND "No journal entry(ies) found\n"
#define MDEBSRES_NOJOUFORREG "No journal entry(ies) found for regexp ->%s<\n"
#define MDEBSRES_NOJOUFORSHC "No journal entry(ies) found for "

/*
 * Message-priority (low-priority) messages; only shown in verbose mode.
 */
#define MDEBSMSG_WELCOME "This is mdebs Version %s\n"
#define MDEBSMSG_WELCOMEHELP "Welcome to the mdebs help system\n"
#define MDEBSMSG_VERBOSE "Verbosity is ON\n"
#define MDEBSMSG_PROMPTISOFF "Prompt is OFF\n"
#define MDEBSMSG_TABLESCREATED "Tables created\n"
#define MDEBSMSG_DBFROMENV \
  "Setting active database from environment (MDEBSDB).\n"
#define MDEBSMSG_CHANGINGDB "Changing the current database to ->%s<-\n"
#define MDEBSMSG_DBUPCASE \
  "Upper case letters in database name converted to lowercase \n" \
  "  (PostgreSQL database names are not case sensitive)\n"
#define MDEBSMSG_NOTHING2DO "Nothing to do\n"
#define MDEBSMSG_FISCDELETED "The fiscal year has been deleted\n"
#define MDEBSMSG_SHCDELETED "Shortcut has been deleted\n"
#define MDEBSMSG_QUERYRETURN "Query %s returned %d\n"
#define MDEBSMSG_ATTINSACCT "Attempting to insert account no. %03d-%03d\n"
#define MDEBSMSG_ATTDELACCT "Attempting to delete account no. %03d-%03d\n"
#define MDEBSMSG_SUCCEEDED "Succeeded\n"
#define MDEBSMSG_QUEACCTPREP "Querying account no. %03d-%03d\n"
#define MDEBSMSG_ENTRYNOEXIST \
  "Non-existent journal entry %d:%03d; skipping\n\n"
#define MDEBSMSG_NONEXISTENTDB "Database %s does not exist\n"
#define MDEBSMSG_EXISTENTDB "Database %s exists\n"
#define MDEBSMSG_BYE "Goodbye.\n"

