/*
 * dump.h
 *
 * Function prototypes for dump.c
 */

void dump_chart(const int, int, int);
void dump_account(const int, FILE *, const int, char *, char *, char *);
int fetchNextJournalEntry(struct journal_entry *, char *);
void dump_journal(const int, FILE *, const int, char *, char *, char *);
void dump_shortcuts(const int);
void print_shortcut(FILE *, struct short_cut *);
