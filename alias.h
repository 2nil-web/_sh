
#ifndef ALIAS_H
#define ALIAS_H

#define ALIAS_PAGESIZE 100

typedef struct sAlias {
  char *sym;
  char *alias;
} sAlias;

typedef struct sAliasList {
  int nalias;
  int npage;
  sAlias *a;
} sAliasList;

/* Liste ou ajoute un alias */
int com_alias(char *args);
char *find_alias(char *alias);

#endif /* ALIAS_H */

