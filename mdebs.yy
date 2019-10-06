/*
 * mdebs.yy
 *
 * Lexical Analyzer (flex) file for mdebs
 */

/*
 * THE DEFINITIONS SECTION
 */

/*
 * OPTIONS
 */

/*
 * This flex option ensures that the analyzer terminates when it encounters
 * an end-of-file.  In other words, we assume that there will only be one
 * input file (either an interactive session or a batch file)
 */
%option noyywrap

/*
 * INCLUDES
 *
 * mdebs.tab.h is created by bison
 * string.h because we use atoi, strcpy, etc.
 */
%{
#include "mdebs.h"
#include "mdebs_bf.h"
#include "messages.h"
#include "y.tab.h"
#include <string.h>
#include <readline/readline.h>
#include <readline/history.h>

#define YY_INPUT(buf, result, max_size) \
  { \
    char *line; \
    if (promptflag) line = readline(MDEBS_PROMPT); \
    else line = readline(""); \
    add_history(line); \
    strcpy(buf, line); \
    buf[strlen(line)]='\n'; \
    buf[strlen(line)+1]='\0'; \
    free (line); \
    result = strlen(buf); \
  } 

int strcnt, numcnt;
%}

/*
 * DEFINITIONS
 *
 * ALNUM means any sequence of characters acceptable to mdebs except
 * whitespace and some punctuation characters
 * NUMBER means any sequence of digits
 * WHITE means any sequence of whitespace (besides newline)
 */
ALNUM   [[:alnum:]]|[\.\,\$\!\@\^\&\[\]\{\}\+\_\|\\\/\;\'\`\~\<\>\?]
		/* Include some punctuation */
CZECH   [áÁćĆčČďĎéÉěĚíÍľĽňŇóÓŕŔřŘšŠťŤúÚůŮýÝžŽ§]  
ZBYTEK  [[:alpha:]]*
NUMBER	[[:digit:]]
WHITE	[[:blank:]]

/*
 * Start conditions
 */
%x string
%x comment

/*
 * The RULES section
 */
%%

\" BEGIN(string);
\# BEGIN(comment);

<string>\" {
  if (flexdebug)
    fprintf(stderr, "End of quoted string encountered\n");
  BEGIN(INITIAL);
  strcnt = 0;
  return TOK_STRING;
}

<string>\n { /* No closing quote on line! */
  mdebs_err(LEXERR_NOENDQUOTE);
  BEGIN(INITIAL);
  strcnt = 0;
  return;
}

<string>(\*|\:|\-|\(|\))+ {
  if (flexdebug)
    fprintf(stderr, "One or more special characters encountered " 
    		    "within string - preserve\n"); 
  if (yyleng > TOKENLEN) {
    mdebs_err(LEXERR_STREXCTOKEN);
    return;
  }
  strcpy(yylval.str+strcnt, yytext);
  strcnt = strcnt + yyleng;
  if (flexdebug) 
    fprintf(stderr, 
            "Valid string portion ->%s<- encountered\n", yytext);
}

<string>({ALNUM}|{CZECH}|{WHITE})+ { /* String enclosed in double quotes 
				      * (may include white space) */
  if (yyleng > TOKENLEN) {
    mdebs_err(LEXERR_STREXCTOKEN);
    return;
  }
  strcpy(yylval.str+strcnt, yytext);
  strcnt = strcnt + yyleng;
  if (flexdebug)
    fprintf(stderr, 
            "Valid string portion ->%s<- encountered\n", yytext);
}

<comment>.* {			/* Comment; do nothing */
  if (flexdebug)
    fprintf(stderr, "Comment encountered - ignore\n");
}

<comment>\n {  /* End of comment. */ 
  if (flexdebug)
    fprintf(stderr, "End of comment encountered\n");
  BEGIN(INITIAL);
  return CMD_COMMENT;
}

\n {
  if (flexdebug)
    fprintf(stderr, "End of line character encountered\n");
  return '\n';
}

(help|HELP).*$ { 			/* the Help command */
  return CMD_HEL;
}

(quit|QUIT|exit|EXIT|bye|BYE) { /* the Bye command */
  /* The rest of the input will be ignored since this exits the entire
   * program */
  return CMD_BYE;
}

(que|QUE)({ZBYTEK}) {
/* the Query command */
  return CMD_QUE;
}

(dum|DUM)({ZBYTEK}) {
/* the Dump command */
  if (flexdebug)
    fprintf(stderr, "DUMP command encountered (%s)\n", yytext);
  return CMD_DUM;
}

(all|ALL)({ZBYTEK}) {
/* the ALL_TO_FILE command */
  if (flexdebug)
    fprintf(stderr, "ALL_TO_FILE command encountered (%s)\n", yytext);
  return CMD_ALL;
}

(ins|INS)({ZBYTEK}) {
/* the Insert command */
  return CMD_INS;
}

(rem|REM|del|DEL)({ZBYTEK}) {
/* the Delete command */
  return CMD_DEL;
}

(chan|CHAN)({ZBYTEK}) {
/* the Change command */
  return CMD_CHG;
}

(char|CHAR)({ZBYTEK}) {
/* the Chart command */
  return CMD_CHA;
}

(fis|FIS)({ZBYTEK}) {
/* the Fiscal Year command */
  return CMD_FIS;
}

(jou|JOU)({ZBYTEK}) {
/* the Journal Entry command */
  return CMD_JOU;
}

(acc|ACC)({ZBYTEK}) {
/* the Account command */
  if (flexdebug)
    fprintf(stderr, "ACCOUNT keyword encountered (%s)\n", yytext);
  return CMD_ACC;
}

(db|DB|datab|DATAB)({ZBYTEK}) {
/* the Database command */
  return CMD_DB;
}

(datestyle|DATESTYLE|DateStyle) {
/* the Datestyle command */
  return CMD_DATESTYLE;
}

(binary|BINARY|Binary) {
/* the Binary command */
  return CMD_BINARY;
}

(shor|SHOR)({ZBYTEK}) {
/* the Shortcut command */
  return CMD_SHO;
}

(pri|PRI)({ZBYTEK}) {
/* the Print command */
  return CMD_PRI;
}

\( {
  return '(';
}

\) {
  return ')';
}

(or|OR) {
/* the OR operator */
  return CMD_OR;
}

(and|AND) {
/* the AND operator */
  return CMD_AND;
}

{NUMBER}{3}\-{NUMBER}+ { /* Account number XXX-YYY */
  if (flexdebug)
    fprintf(stderr, "Possible account number specification encountered\n");
  if ( (yyleng - 4) > 3 ) /* If number following : is over 3 digits */
  {
    mdebs_err(LEXERR_MINNUMTOOBIG);
    return;
  }
  bf_buf[0] = yytext[0];
  bf_buf[1] = yytext[1];
  bf_buf[2] = yytext[2];
  bf_buf[3] = '\0';
  bf_acct = atoi(bf_buf);

  if (flexdebug)
    fprintf(stderr, "Minor number portion consists of %d digits\n",
                    (yyleng - 4) );

  for (numcnt=4; numcnt<=yyleng; numcnt++)
  {
    bf_buf[numcnt-4] = yytext[numcnt];
  }
  bf_buf[yyleng - 4] = '\0';
  bf_anal = atoi(bf_buf);

  bf_buf[0] = yytext[4];
  bf_buf[1] = yytext[5];
  bf_buf[2] = yytext[6];
  bf_buf[3] = '\0';
  bf_anal = atoi(bf_buf);

  if (flexdebug)
    fprintf(stderr, "Account number %03d-%03d encountered.\n", 
    	            bf_acct, bf_anal);
   	
  return TOK_ACCTNUM;
}

{NUMBER}{3}\-\* { /* Account number XXX-\* */
  bf_buf[0] = yytext[0];
  bf_buf[1] = yytext[1];
  bf_buf[2] = yytext[2];
  bf_buf[3] = '\0';
  bf_acct = atoi(bf_buf);

  bf_anal = -1;

  if (flexdebug)
    fprintf(stderr, "Account number %03d-* encountered.\n", 
  	            bf_acct, bf_anal);
   	
  return TOK_ACCTNUM;
}

{NUMBER}{8}\:{NUMBER}+ { /* Journal entry specification YYYYMMDD:X */
  if (flexdebug)
    fprintf(stderr, "Possible journal entry specification encountered\n");
  if ( (yyleng - 9) > 4 ) /* If number following : is over 3 digits */
  {
    mdebs_err(LEXERR_SERNUMTOOBIG);
    return;
  }
  /* Get the date */
  for (numcnt=0; numcnt<=7; numcnt++)
  {
    bf_buf[numcnt] = yytext[numcnt];
  }
  bf_buf[8] = '\0';
  bf_date = atoi(bf_buf);

  if (flexdebug) 
    fprintf(stderr, "%d seen - interpreting as date\n", bf_date);

  /* Get the serial number.
   * (yyleng - 9) is the number of digits in the number (can include
   * initial zeros */

  if (flexdebug)
    fprintf(stderr, "Serial number portion consists of %d digits\n",
                    (yyleng - 9) );

  for (numcnt=9; numcnt<=yyleng; numcnt++)
  {
    bf_buf[numcnt-9] = yytext[numcnt];
  }
  bf_buf[yyleng - 9] = '\0';
  bf_sernum = atoi(bf_buf);

  if (flexdebug) 
    fprintf(stderr, "%d seen - interpreting as serial number\n", bf_sernum);

  if (flexdebug)
    fprintf(stderr, "Journal specification %d:%d lexically analyzed\n",
    		    bf_date, bf_sernum);
  
  return TOK_JOUSPEC;
}

(sho|SHO)\= {
  if (flexdebug)
    fprintf(stderr, "Shortcut specification encountered\n");
  return SHORTCUT;
}

(doc|DOC)\= {
  if (flexdebug)
    fprintf(stderr, "Document LIKE specification encountered\n");
  return DOCUMENT;
}

(des|DES)\= {
  if (flexdebug)
    fprintf(stderr, "Designation LIKE specification encountered\n");
  return DESIGNATION;
}

(doc|DOC)\~ {
  if (flexdebug)
    fprintf(stderr, "Document REGEX specification encountered\n");
  return DOCU_REGEX;
}

(des|DES)\~ {
  if (flexdebug)
    fprintf(stderr, "Designation REGEX specification encountered\n");
  return DESIG_REGEX;
}

(\-{NUMBER}+)|({NUMBER}+) {
  if (flexdebug)
    fprintf(stderr, "Numeric (integer) value %s encountered\n", yytext);
  if (yyleng > TOKENLEN) {
    mdebs_err(LEXERR_NUMEXCTOKEN);
    return;
  }
  yylval.num = atoi(yytext);
  if (flexdebug)
    fprintf(stderr, "Lexanalyzer returning integer value %d\n", yylval.num);
  return TOK_NUMBER;
}

({ALNUM}|{CZECH}|\:|\*)+ { /* Non-quoted string (may not contain white space) */
  if (yyleng > TOKENLEN) {
    mdebs_err(LEXERR_STREXCTOKEN);
    return;
  }
  if (flexdebug)
    fprintf(stderr, "Non-quoted string ->%s<- encountered\n", yytext);
  strcpy (yylval.str, yytext);
  yylval.str[yyleng] = 0;
  return TOK_STRING;
}

{WHITE}+ 

. { /* Non-parsed input? */
  if (flexdebug)
    fprintf(stderr, "Lexical analyzer did not recognize input: %s\n", yytext);
}

%%
