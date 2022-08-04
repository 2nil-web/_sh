
#ifndef VAR_H
#define VAR_H

typedef enum eType {
  DL_VOID,
   DL_CHAR,  DL_SHORT,  DL_INT,  DL_LONG,  DL_LLONG,
  DL_UCHAR, DL_USHORT, DL_UINT, DL_ULONG, DL_ULLONG,
  DL_FLOAT, DL_DOUBLE, DL_LDOUBLE, DL_STRUCT, DL_STRING,
  DL_UNKNOWN
} eType;


typedef struct sVar {
  eType type;
  int isPointer;
  char *name;
  size_t sz;
  char *val;
  int deleted;
} sVar;


int defchar(char *arg);
int defint(char *arg);
int deflong(char *arg);
int defllong(char *arg);

int defuchar(char *arg);
int defuint(char *arg);
int defulong(char *arg);
int defullong(char *arg);

int deffloat(char *arg);
int defdouble(char *arg);
int defldouble(char *arg);
int defstruct(char *arg);
size_t add2struct(void **ptr, eType t, int isPointer, int isConstant, char *pval);

int vsizeof(char *arg);
int vecho(char *arg);
int vlist(char *arg);
int vunset(char *arg);
sVar *vsrch(char *vname);

#endif /* VAR_H */

