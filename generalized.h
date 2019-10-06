/*
 * generalized.h
 *
 * Function prototypes for generalized.c
 */

char *query_environment(void);
int dbQuery(char *);
int dbQueryfull(char *);
int query_mdebsdb(void);
int transQuery(int, int);
int NathanConvertDate(char *);
char *pgConvertDate(int);
int countNonWhiteSpace(char *);
int valiDate(int);
char *theTimeIs(void);
int strntok(char *);
char *accountName(int, int);
int verify_regexp(char *);
char *backslash_single_quotes(char *);
char *unbackslash_single_quotes(char *);
FILE *open_tmp(char **);
int close_tmp(FILE *);
void browse_tmp(char *);
void print_tmp(char *);
void remove_tmp(char *);
int power(int, int);
char *to_bin(char *, int);
bool to_bool(char *);

