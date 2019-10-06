#include <stdlib.h>
#include <stdio.h>

void errorExit(char *);

/*
 * odpisplan.c
 *
 * Prints out an accelerated odpisovy plan, based on vstupni cena
 * and odpisova skupina
 */

int main(int argc, char *argv[])
{
  float zustcena, odpis;
  int rok, koef1, koef2, trvani;
  int i;

  if (argc < 4) {
    errorExit("odpisplan");
  }
  switch (atoi(argv[3])) {
  case (1):
    koef1 = 4;
    koef2 = 5;
    trvani = 4;
    break;
  case (2):
    koef1 = 6;
    koef2 = 7;
    trvani = 6;
    break;
  case (3):
    koef1 = 12;
    koef2 = 13;
    trvani = 12;
    break;
  case (4):
    koef1 = 20;
    koef2 = 21;
    trvani = 20;
    break;
  case (5):
    koef1 = 30;
    koef2 = 31;
    trvani = 30;
    break;
  default:
    errorExit("odpisplan");
  }

  rok = atoi(argv[1]);
  zustcena = atol(argv[2]);
  odpis = 2*zustcena/(koef1);
  printf("%d 2 * %10.02f / ( %d - %2d ) = %10.02f\n", rok, zustcena, koef1, 0, odpis);
  for (i = 1; i <= trvani; i++) {
    zustcena = zustcena - odpis;
    odpis = 2*zustcena/(koef2-i);
    printf("%d 2 * %10.02f / ( %d - %2d ) = %10.02f\n", (rok+i), zustcena, koef1, i, odpis);
  }
}
	    
void errorExit(char *callname)
{
    fprintf(stderr, "Usage: %s [vstupni_rok vstupni_cena odpis_skupina]\n", callname);
    exit(1);
}
