/*
 * query.h
 *
 * Function prototypes for query.c
 */

int query_account_by_regexp(char *);
struct date_range *query_fiscal(int);
int query_serial_by_date(int);
struct short_cut *query_shortcut(int *, char *);

