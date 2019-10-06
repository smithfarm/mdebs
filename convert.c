/*
 * convert.c
 *
 * Example of how to use pgresfunc.c outside of mdebs
 */
#include "global.h"
#include "mdebs.h"
#include "messages.h"
#include "pgresfunc.h"
#include "generalized.h"

int main(void)
{
  char *query;
  int i, count, id, ser_num, amount;
  bool debit;
  char ent_date[11];
  char acct[3], anal[3];

  debugflag = 0;
  verboseflag = 1;

  openBackend("liv1999");
  runQuery("SELECT count(*) FROM jou_head;");
  count = atoi(getField("count"));
  runQuery("BEGIN WORK;");
  runQuery("DECLARE a1 CURSOR FOR SELECT * FROM jou_head;");
  i = 0;
  while (fetchNext("a1"))
  {
    strcpy(ent_date, getField("ent_date"));
    ser_num = atoi(getField("ser_num"));
    id = atoi(getField("id"));
    printf("(%d/%d) %s:%d - %d\n", ++i, count, ent_date, ser_num, id );
    asprintf(&query, "UPDATE jou_lines SET id=%d WHERE ent_date='%s' AND ser_num=%d;", id, ent_date, ser_num);
    printf("%s\n", query);
    runQuery(query);
    free(query);  
  }
  runQuery("END WORK;");
  closeBackend();
}
