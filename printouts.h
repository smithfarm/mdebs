/*
 * printouts.h
 *
 * This file defines the strings that mdebs prints out when it is asked to
 * dump data (various headers for * reports, etc.)
 */

#define MDEBSHEAD_CHART \
"--------------------------------------------------------------------------\n" \
"Major-Minor   Designation \n" \
"--------------------------------------------------------------------------\n" 
#define MDEBSFOOT_CHART \
"--------------------------------------------------------------------------\n" 

#define MDEBSHEAD_ACCOUNTMIN \
"------------+-------+------------+-------------------------+-------------+-------------\n" \
"Datum:PorCis|Maj-Min|Doklad      |Popis operace            |   MÁ DÁTI   |     DAL\n" \ 
"------------+-------+------------+-------------------------+-------------+-------------\n" 
#define MDEBSFOOT_ACCOUNTMIN \
"------------+-------+------------+-------------------------+-------------+-------------\n" 

#define MDEBSHEAD_ACCOUNTNOMIN "------------+------------+-------------------------+-------------+-------------\nDatum:PorCis|Doklad      |Popis operace            |   MÁ DÁTI   |     DAL\n------------+------------+-------------------------+-------------+-------------\n" 
#define MDEBSFOOT_ACCOUNTNOMIN "------------+------------+-------------------------+-------------+-------------\n" 
