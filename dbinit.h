/*
 * dbinit.h
 */

/*
 * String length declares that give the maximum lengths for certain fields
 * in the database (see dbinit.c)
 */

#define CHART_DESIG_LENGTH 60
#define JOURNAL_DESIG_LENGTH 80
#define DOCUMENT_DESIG_LENGTH 20
#define SHORTCUT_DESIG_LENGTH 20

#define MDEBS_DB_CONTAIN_TABLES_QUERY \
  "SELECT * FROM pg_tables WHERE tablename NOT LIKE 'pg_%' AND tablename NOT LIKE 'sql_%';"

/*
 * Function prototypes for dbinit.c
 */

void dbinit_create(char *);

