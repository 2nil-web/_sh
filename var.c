
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "util.h"
#include "var.h"

/* Testé sous linux/gcc, cygwin, mingwin, dec/gcc, sun/gcc, hp/gcc */
#ifdef __GNUC__
#define myalignof __alignof__
#else
/* windows/msvc */
#ifdef _MSC_VER
#define myalignof __alignof
#endif

/* dec/osf compilo. natif */
#ifdef _OSF_SOURCE
#include <alignof.h>
#define myalignof alignof
#endif

/* A tester/modifier pour les compilo. natifs hpux, sun, aix ... */
#ifndef myalignof
#define myalignof sizeof
#endif
#endif


#define VAR_PAGESIZE 1000

typedef struct sVarList {
  int nvar;
  int npage;
  sVar *v;
} sVarList;

sVarList vl={ 0, 0, NULL };

sVar *addvar(sVarList *vl)
{
  if (!vl) return NULL;

  if (!vl->v) {
    vl->nvar=0;
    vl->npage=1;
    vl->v=(sVar *)malloc(VAR_PAGESIZE*sizeof(sVar));
  }

  if (vl->nvar >= vl->npage*VAR_PAGESIZE) {
    vl->npage++;
    vl->v=(sVar *)realloc(vl->v, vl->npage*VAR_PAGESIZE*sizeof(sVar));
  }

  vl->v[vl->nvar].type=DL_VOID;
  vl->v[vl->nvar].isPointer=0;
  vl->v[vl->nvar].name=NULL;
  vl->v[vl->nvar].sz=1;
  vl->v[vl->nvar].val=NULL;
  vl->v[vl->nvar].deleted=1;
  return &vl->v[vl->nvar++];
}

sVar *searchvar(char *vname, sVarList *vl)
{
  int i;

  if (vname) 
    for (i=0; i < vl->nvar; i++) {
      if (vl->v[i].name && strcmp(vl->v[i].name, vname) == 0) {
        return &vl->v[i];
      }
    }

  return NULL;
}

sVar *vsrch(char *vname) { return searchvar(vname, &vl); }

int defvar(eType typ, char *vardef)
{
  char *vname=NULL, *p, *p2, c;
  sVar *v, *sv;
  // Cherche * puis nom variable puis [x] puis '=' puis valeur

  v=addvar(&vl);

  // Type
  v->type=typ;

  // Pointeur ou pas
  if (vardef && vardef[0] == '*') {
    v->isPointer=1;
    vardef++;
    vardef=lws(vardef);
  } else v->isPointer=0;

  // Nom
  p=vardef;
  while (p && *p && !isspace(*p) && *p != '[' && *p != '=') p++;
  c=*p;
  *p='\0';
  vname=strdup(vardef);
  *p=c;
//  printf("[%s]\n", vname);fflush(stdout);

  // Taille
  if (*p == '[') {
    *p='\0';
    p++;
    p2=p;
    while (p2 && *p2 && *p2 != ']') p2++;
    *p2='\0';
    p2++;
    v->sz=atoi(p);
    if (v->sz < 1) v->sz=1;
    p=lws(p);
  }

  // Valeur initiale
  if (v->val) free(v->val);
  v->val=NULL;

  if (*p == '=') {
    *p='\0';
    p++;
    if (*p) v->val=strdup(p);
  }

  // Si le nom de variable existe déjà on redéfini l'existant avec les nouvelles caractéristiques
  if ((sv=searchvar(vname, &vl))) {
    if (!sv->deleted) {
      fprintf(stderr, "Avertissement : la variable %s existe déjà ==> redéfinition\n", vname);
      fflush(stderr);
    }

    free(vname);
    if (sv->val) free(sv->val);
    sv->type=v->type;
    sv->isPointer=v->isPointer;
    sv->sz=v->sz;
    sv->val=v->val;
    v=sv;
    if (vl.nvar > 0) vl.nvar--;
  } else v->name=vname;

  v->deleted=0;

  return 1;
}

int defchar   (char *arg) { return defvar(   DL_CHAR, arg); }
int defint    (char *arg) { return defvar(    DL_INT, arg); }
int deflong   (char *arg) { return defvar(   DL_LONG, arg); }
int defllong  (char *arg) { return defvar(  DL_LLONG, arg); }

int defuchar  (char *arg) { return defvar(  DL_UCHAR, arg); }
int defuint   (char *arg) { return defvar(   DL_UINT, arg); }
int defulong  (char *arg) { return defvar(  DL_ULONG, arg); }
int defullong (char *arg) { return defvar( DL_ULLONG, arg); }

int deffloat  (char *arg) { return defvar(  DL_FLOAT, arg); }
int defdouble (char *arg) { return defvar( DL_DOUBLE, arg); }
int defldouble(char *arg) { return defvar(DL_LDOUBLE, arg); }


#define sots(Tvar1, Tvar2) { \
  typedef struct sTvar {  Tvar1 v1; Tvar2 v2;} sTvar; \
  printf("sizeof(Struct { %s, %s} )=%lu\n", #Tvar1, #Tvar2, (long unsigned int)sizeof(sTvar)); \
}

int old_defstruct(char *arg)
{/*
  typedef struct sTest {
    char c;
    short s;
    int i;
    long l;
    long long ll;
    float f;
    double d;
    long double ld;
  } sTest;*/

//  printf("sos s %d, soc %d, sos %d soi %d, sol %d, soll %d, sof %d, sod %d, sold %d\n",
//      sizeof(sTest), sizeof(char), sizeof(short), sizeof(int), sizeof(long), sizeof(long long), sizeof(float), sizeof(double), sizeof(long double)
//  );

  sots(long, char);
	sots(long, short);
	sots(long, int);
	sots(long, long);
	sots(long, long long);
	sots(long, float);
	sots(long, double);
	sots(long, long double);

  return 1;
}

int defstruct(char *arg)
{

  return 1;
}

size_t getstrvarval(sVar *v, char **s)
{
  if (v->val) *s=strdup(v->val);
  else {
    if (v->type != DL_CHAR && v->type != DL_UCHAR && v->type != DL_STRUCT) *s=strdup("0");
    else {
      *s=NULL;
      return 0;
    }
  }

  return strlen(*s);
}

void dispvar(FILE *fpout, sVar *v)
{
  if (v->val) fputs(v->val, fpout);
  else {
    if (v->type != DL_CHAR && v->type != DL_UCHAR && v->type != DL_STRUCT) fputs("0", fpout);
  }
}

void search_and_dispvar(FILE *fpout, char *vname)
{
  dispvar(fpout, searchvar(vname, &vl));
}

int vecho(char *args)
{
  search_and_dispvar(stdout, args);
  puts("");
  fflush(stdout);

  return 1;
}


const char *stype(eType type)
{
  switch(type) {
  case DL_CHAR:
    return "char";
  case DL_UCHAR:
    return "uchar";
  case DL_SHORT:
    return "short";
  case DL_USHORT:
    return "ushort";
  case DL_INT:
    return "int";
  case DL_UINT:
    return "uint";
  case DL_LONG:
    return "long";
  case DL_ULONG:
    return "ulong";
  case DL_LLONG:
    return "longlong";
  case DL_ULLONG:
    return "ulonglong";
  case DL_FLOAT:
    return "float";
  case DL_DOUBLE:
    return "double";
  case DL_LDOUBLE:
    return "longdouble";
  case DL_STRUCT:
    return "struct";
  default :
    return "float";
  }
}

eType types(char *s)
{
  if (s) {
    if (strcasecmp(s, "char") == 0) return DL_CHAR;
    if (strcasecmp(s, "uchar") == 0) return DL_UCHAR;
    if (strcasecmp(s, "short") == 0) return DL_SHORT;
    if (strcasecmp(s, "ushort") == 0) return DL_USHORT;
    if (strcasecmp(s, "int") == 0) return DL_INT;
    if (strcasecmp(s, "uint") == 0) return DL_UINT;
    if (strcasecmp(s, "long") == 0) return DL_LONG;
    if (strcasecmp(s, "ulong") == 0) return DL_ULONG;
    if (strcasecmp(s, "longlong") == 0) return DL_LLONG;
    if (strcasecmp(s, "ulonglong") == 0) return DL_ULLONG;
    if (strcasecmp(s, "float") == 0) return DL_FLOAT;
    if (strcasecmp(s, "double") == 0) return DL_DOUBLE;
    if (strcasecmp(s, "longdouble") == 0) return DL_LDOUBLE;
    if (strcasecmp(s, "struct") == 0) return DL_STRUCT;
  }

  return DL_UNKNOWN;
}

/* Renvoie la taille d'un type aligné (utile pour former une structure) */
size_t AlignOfType(eType typ)
{
  switch(typ) {
  case DL_CHAR :
    return myalignof(char) ;
  case DL_UCHAR :
    return myalignof(unsigned char) ;
  case DL_SHORT :
    return myalignof(short) ;
  case DL_USHORT :
    return myalignof(unsigned short) ;
  case DL_INT :
    return myalignof(int) ;
  case DL_UINT :
    return myalignof(unsigned int) ;
  case DL_LONG :
    return myalignof(long) ;
  case DL_ULONG :
    return myalignof(unsigned long) ;
  case DL_LLONG :
    return myalignof(long long) ;
  case DL_ULLONG :
    return myalignof(unsigned long long) ;
  case DL_FLOAT :
    return myalignof(float) ;
  case DL_DOUBLE :
    return myalignof(double) ;
  case DL_LDOUBLE :
    return myalignof(long double) ;
  default :
    return myalignof(float) ;
  }
}

/* Renvoie la taille d'un type aligné (utile pour former une structure) */
size_t SizeOfType(eType typ)
{
  switch(typ) {
  case DL_CHAR :
    return sizeof(char) ;
  case DL_UCHAR :
    return sizeof(unsigned char) ;
  case DL_SHORT :
    return sizeof(short) ;
  case DL_USHORT :
    return sizeof(unsigned short) ;
  case DL_INT :
    return sizeof(int) ;
  case DL_UINT :
    return sizeof(unsigned int) ;
  case DL_LONG :
    return sizeof(long) ;
  case DL_ULONG :
    return sizeof(unsigned long) ;
  case DL_LLONG :
    return sizeof(long long) ;
  case DL_ULLONG :
    return sizeof(unsigned long long) ;
  case DL_FLOAT :
    return sizeof(float) ;
  case DL_DOUBLE :
    return sizeof(double) ;
  case DL_LDOUBLE :
    return sizeof(long double) ;
  default :
    return sizeof(float) ;
  }
}


int vsizeof(char *arg)
{
  eType t=types(arg);

  if (t == DL_UNKNOWN) {
    fprintf(stderr, "unknown type \"%s\"\n", arg); fflush(stderr);
  } else {
    printf("%lu\n", (unsigned long)SizeOfType(t)); fflush(stdout);
  }

  return 1;
}

int vlist(char *args)
{
  int i;
  char *s;

  for (i=0; i < vl.nvar; i++) {
    if (args && strncmp(vl.v[i].name, args, strlen(args)) != 0) continue;
    if (!vl.v[i].deleted) {
      printf("%s %s", stype(vl.v[i].type), vl.v[i].name);
      if (getstrvarval(&vl.v[i], &s)) {
        printf(" = %s", s);
      }
      puts(""); fflush(stdout);
    }
  }

  return 1;
}

int vunset(char *arg)
{
  sVar *v;

  if ((v=searchvar(arg, &vl))) v->deleted=1;
  else {
    fprintf(stderr, "La variable [%s] n'existe pas\n", arg);
    fflush(stderr);
    return 0;
  }

  return 1;
}

#define copmem(PT, TY, FU, SV) { TY d=(TY)FU(SV); memcpy(PT, &d, sizeof(TY)) }

/* recopie une val exprimée en chaine dans la zone mémoire pointé par p, suivant son type */
int svaltomem(void *p, eType t, char *sval)
{
  switch(t) {
    case DL_CHAR : case DL_UCHAR : {
      char c=*sval;
      memcpy(p, &c, sizeof(char));
      break;
    }
    case DL_SHORT : case DL_USHORT : {
      short d=(short)atoi(sval);
      memcpy(p, &d, sizeof(short));
      break;
    }
    case DL_INT : case DL_UINT : {
      int d=(int)atoi(sval);
      memcpy(p, &d, sizeof(int));
      break;
    }
    case DL_LONG : case DL_ULONG : {
      long d=(long)atol(sval);
      memcpy(p, &d, sizeof(long));
      break;
    }

    case DL_LLONG : case DL_ULLONG : {
      long long d=(long long)atoll(sval);
      memcpy(p, &d, sizeof(long long));
      break;
    }

    case DL_FLOAT : {
      float d=(float)atof(sval);
      memcpy(p, &d, sizeof(float));
      break;
    }

    case DL_DOUBLE : {
      double d=(double)atof(sval);
      memcpy(p, &d, sizeof(double));
      break;
    }

    case DL_LDOUBLE : {
      long double d=(long double)atof(sval);
      memcpy(p, &d, sizeof(long double));
      break;
    }
    default :
      return 0;
  }

  return 1;
}

size_t add2struct(void **ptr, eType t, int isPointer, int isConstant, char *str_val_or_name)
{
  static size_t curr_offset=0, next_offset;
  size_t next_align, szo;
  char *sval;

  if (*ptr == NULL) {
    curr_offset=next_offset=0;
  } else curr_offset=next_offset;

  /* Décalage sur le prochain champ de la structure */
  if (isPointer) {
    szo=sizeof(void *);
    next_align=myalignof(void *);
  } else {
    szo=SizeOfType(t);
    next_align=AlignOfType(t);
  }

  next_offset += szo;
  while (next_offset%next_align != 0) next_offset++;
//  printf("so %d, na %d, no %d\n", szo, next_align, next_offset); fflush(stdout);

  if (*ptr == NULL) *ptr=malloc(next_offset);
  else *ptr=realloc(*ptr, next_offset);

  if (isConstant) { // Si c'est une constante on prend la valeur litéral */
    sval=str_val_or_name;
  } else {  // Sinon on va chercher sa valeur si c'est une variable
    sval=str_val_or_name;
  }

  if (isPointer) {
    if (t == DL_CHAR || t == DL_UCHAR) {
      char *s=strdup(sval);
//      printf("add2 [%s] %p\n", s, s);
      memcpy((*ptr+curr_offset), &s, sizeof(void **));
//      printf("add2bis [%s] %p\n", *(char **)(*ptr+curr_offset), *(char **)(*ptr+curr_offset));
    }
  } else {
    svaltomem(*ptr+curr_offset, t, sval);
//    printf("svtm %d\n", *d);
  }

//  if (curr_offset >= 4 && curr_offset <= 8) printf("pptr %p %s\n", *ptr+curr_offset, *(char **)(*ptr+curr_offset));fflush(stdout);
  return next_offset;
}

