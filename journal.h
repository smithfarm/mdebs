/*
 * journal.h
 *
 * Function prototypes for journal.c
 */

void init_jent(struct journal_entry *);
struct journal_entry *query_journal_entry(int, int);
struct short_cut *query_shortcut(int *, char *);
int verify_journal_entry(struct journal_entry *);
int write_journal_entry(struct journal_entry *);
void print_journal_entry (FILE *, struct journal_entry);
void wprint_journal_entry (FILE *, struct journal_entry);
