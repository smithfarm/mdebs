/*
 * mdebs_bf.h
 *
 * Header file specific to mdebs.y (bison) and mdebs.yy (flex); hence "bf"
 */
#ifndef TOKENLEN
#define TOKENLEN 64
#endif

int flexdebug;

char bf_buf[13];
int bf_acct, bf_anal;
int bf_date, bf_sernum;
int bf_shcode;
char *bf_shdesig;
