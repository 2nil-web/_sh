
#ifndef LIB_H
#define LIB_H

typedef void (*DLLFUNC)(void *);

//#define FUNCTYPE DLLFUNC
#define FUNCTYPE FARPROC

typedef struct sDllFunc {
  char *name;
  FUNCTYPE func;
} sDllFunc;

int loadlib(char *lib);
sDllFunc *find_func(char *funcname);
int callfunc(sDllFunc *dllf, char *args);
int setlib(char *arg);
int lfunc(char *);
int llib(char *);

#endif /* LIB_H */

