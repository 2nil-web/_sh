
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "util.h"
#include "alias.h"

/*
typedef struct sAlias {
  char *sym;
  char *alias;
} sAlias;

typedef struct sAliasList {
  int nalias;
  int npage;
  sAlias *a;
} sAliasList;
*/

sAliasList al;


sAlias *addalias(sAliasList *al)
{
  if (!al) return NULL;

  if (!al->a) {
    al->nalias=0;
    al->npage=1;
    al->a=(sAlias *)malloc(ALIAS_PAGESIZE*sizeof(sAlias));
  }

  if (al->nalias >= al->npage*ALIAS_PAGESIZE) {
    al->npage++;
    al->a=(sAlias *)realloc(al->a, al->npage*ALIAS_PAGESIZE*sizeof(sAlias));
  }

  al->a[al->nalias].sym=NULL;
  al->a[al->nalias].alias=NULL;
  return &al->a[al->nalias++];
}

sAlias *searchalias(char *alias, sAliasList *al)
{
  int i;

  if (alias) 
    for (i=0; i < al->nalias; i++) {
      if (al->a[i].alias && strcmp(al->a[i].alias, alias) == 0) {
        return &al->a[i];
      }
    }

  return NULL;
}

int listalias(sAliasList *al)
{
  int i, noal=1;

  for (i=0; i < al->nalias; i++) {
    printf("%s = %s\n", al->a[i].sym, al->a[i].alias); fflush(stdout);
    if (noal) noal=0;
  }

  if (noal) puts("no aliases");
  fflush(stdout);
  return 0;
}

/* Liste ou ajoute un alias */
int com_alias(char *arg)
{
  if (arg && *arg && strcmp(arg, "") != 0) {
    char *p, *p2;
    p=ws(arg);
    p2=strchr(p, ' ');

    if (p2 && *p2) {
      sAlias *a;
      *p2++='\0';
      a=searchalias(p2, &al);

      if (a) {
        fprintf(stderr, "Avertissement : l'alias %s existe déjà ==> redéfinition\n", p2);
        fflush(stderr);
      } else {
        a=addalias(&al);
        a->alias=strdup(p2);
      }

      a->sym=strdup(p);
    } else return 1;
  } else listalias(&al);

  return 0;
}

char *find_alias(char *alias)
{
  sAlias *a=searchalias(alias, &al);

  if (a) return a->sym;
  return NULL;
}

