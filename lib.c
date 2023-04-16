
#include <stdio.h>
#include <libgen.h>
#include <ctype.h>
#include <windows.h>

#include "util.h"
#include "var.h"
#include "lib.h"


typedef struct sDllFuncList {
  char *libname;
  HINSTANCE h;
  DWORD nb;
  sDllFunc *list;
} sDllFuncList;

#define MAX_DLL 10

int ndll=0;
int currlib=-1;
sDllFuncList dl[MAX_DLL];


int getconsolewidth()
{
#ifdef WIN32
#include <wincon.h>
//extern BOOL WINAPI GetCurrentConsoleFont(HANDLE hConsoleOutput, BOOL bMaximumWindow, PCONSOLE_FONT_INFO lpConsoleCurrentFont);
  CONSOLE_FONT_INFO ccf;
  GetCurrentConsoleFont(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &ccf);
  return ccf.dwFontSize.X;
#else
  if (getenv("TERM") == NULL) {
    static char term_buffer[2048];
    tgetent(term_buffer, "vt100");
  }
  return tgetnum("co");
#endif
}

void listfunc(sDllFuncList *fl)
{
  int width;
  size_t i;
  size_t maxl=0, l;
  char fmt[10];

  width=getconsolewidth();

  /* Cheche le nom de fonction le plus long */
  for (i=0; i < fl->nb; i++) {
    l=strlen(fl->list[i].name);
    if (l > maxl) maxl=l;
  }

  maxl++;
  sprintf(fmt, " -%lus", (long unsigned)maxl);
  fmt[0]='%';
  l=0;

  for (i=0; i < fl->nb; i++) {
    printf(fmt, fl->list[i].name);
    l += maxl;

    if (l > width-maxl) {
      putchar('\n');
      l=0;
    } else putchar('\t');
  }

  putchar('\n');
  printf("%s contient %lu fonctions\n", basename(fl->libname), fl->nb); fflush(stdout);
}


sDllFunc *searchfunc(sDllFuncList *fl, char *funcname)
{
  DWORD i;
  for (i=0; i < fl->nb; i++) {
    if (strcmp(fl->list[i].name, funcname) == 0) return &fl->list[i];
  }

  return NULL;
}

sDllFunc *find_func(char *funcname)
{
  int i;
  sDllFunc *df;

  for (i=0; i < ndll; i++) {
    df=searchfunc(&dl[i], funcname);
    if (df) return df;
  }
  
  return NULL;
}


// Charge une dll et ses fonctions, en mémoire
int LoadDLL(char *szImagePath, sDllFuncList *fl)
{
  IMAGE_OPTIONAL_HEADER32 *pPEHeader;

  // on charge le fichier
  BYTE *pBase = (BYTE*) LoadLibrary(szImagePath);

  if(!pBase) {
    ShError("Erreur lors du chargement du module %s.", szImagePath);
    return 0;
  }

  // trouve et verifie la signature du fichier
  IMAGE_NT_HEADERS32 *pNTHeader = (IMAGE_NT_HEADERS32*)(pBase + ((IMAGE_DOS_HEADER*)pBase)->e_lfanew);

  if(pNTHeader->Signature != IMAGE_NT_SIGNATURE) {
    ShError("Signature PE invalide.");
    FreeLibrary((HMODULE)pBase);
    return 0;
  }

  // trouve les addresses des header COFF file et Optionnal/PE
  pPEHeader = (IMAGE_OPTIONAL_HEADER32*) &pNTHeader->OptionalHeader;
  // on trouve les RVA de l'Export Directory Table ainsi que leurs addresses virtuelles & leurs tailles
  DWORD dwExportRVA = pPEHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
  IMAGE_EXPORT_DIRECTORY  *pExportDirectoryTable = (IMAGE_EXPORT_DIRECTORY*)(pBase + dwExportRVA);
  DWORD dwExportDirectorySize = (DWORD) pPEHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
  DWORD dwExportEndRVA = dwExportRVA + dwExportDirectorySize;
  // ajoute les items enfants de l'item hImport 
//  DWORD dwTotalCount = 0;

  // ajoute les items enfants de l'item hExport
//  dwTotalCount = 0;
  if(dwExportDirectorySize) {
     DWORD dwForwardsCount = 0, dwOrdinal = 0, dwFuncRVA = 0;
     fl->nb = 0;
    // on trouve le nombre de symbole exportés, ainsi que les addresses de diverses choses:
    // Export Function Table: contient les RVA des fonctions, 
    // Export Name Table: contient les RVA des noms des fonctions au format asciiz
    // Export Ordinal Table: index des RVA des fonctions
    DWORD dwExportNamesCount = (DWORD) pExportDirectoryTable->NumberOfNames;
    fl->list=(sDllFunc *)malloc(dwExportNamesCount*sizeof(sDllFunc));

    DWORD *pAddressTable = (DWORD*)(pBase + pExportDirectoryTable->AddressOfFunctions);
    DWORD *pNameTable = (DWORD*)(pBase + pExportDirectoryTable->AddressOfNames);
    WORD *pOrdinalTable = (WORD*)(pBase + pExportDirectoryTable->AddressOfNameOrdinals);
    // on cherche le type du symbole (forwarder ou fonction/variable)
    for(; dwExportNamesCount; pOrdinalTable++, pNameTable++, dwExportNamesCount--) {
      dwOrdinal = (DWORD) *pOrdinalTable;
      dwFuncRVA = pAddressTable[dwOrdinal];
      dwOrdinal += pExportDirectoryTable->Base;

      // si dwFuncRVA est dans l'Export Table, c'est un forwarder, sinon c'est une fonction
      if(dwExportRVA < dwFuncRVA && dwFuncRVA < dwExportEndRVA) {
        dwForwardsCount++;
//        printf("%03lu  %s --->  %s\n", dwOrdinal, pBase + *pNameTable, pBase + dwFuncRVA);fflush(stdout);
      } else {
        fl->list[fl->nb].name=(char *)(pBase + *pNameTable);
        fl->list[fl->nb].func=(FUNCTYPE)GetProcAddress((HINSTANCE)pBase, fl->list[fl->nb].name);
        fl->nb++;
//        printf("%03lu  0x%08lx  %s\n", dwOrdinal, dwFuncRVA, pBase + *pNameTable);fflush(stdout);
      }
    }

/*    dwTotalCount = fl->nb + dwForwardsCount;
    printf("non %ld dwTotalCount %ld = fl->nb %ld + dwForwardsCount %ld\n", pExportDirectoryTable->NumberOfNames, dwTotalCount, fl->nb, dwForwardsCount);*/
  }

  if (fl->nb < pExportDirectoryTable->NumberOfNames) fl->list=(sDllFunc *)realloc(fl->list, fl->nb*sizeof(sDllFunc));
  fl->h=(HMODULE)pBase;
  fl->libname=strdup(szImagePath);

  return 1;
}




int loadlib(char *lib)
{
  if (ndll < MAX_DLL) {
    if (LoadDLL(lib, &dl[ndll])) {
      extern int silent_run;
      if (!silent_run) printf("[%d ==> %s chargée]\n", ndll, dl[ndll].libname);
      fflush(stdout);
      currlib=ndll;
      ndll++;
      return 1;
    }
  } else ShError("too much dll loaded");

  return 0;
}

int setlib(char *arg)
{
  int nl;
  
  if (arg && *arg) nl=atoi(arg);
  else nl=-1;

  if (nl >= 0 && nl < ndll) {
    currlib=nl;
    return 1;
  } else {
    if (currlib >= 0 && currlib < ndll) {
      printf("%d ==> %s\n", currlib, dl[currlib].libname);
      fflush(stdout);
    } else {
      fprintf(stderr, "Pas de dll chargée\n");
      fflush(stderr);
    }
  }

  return 0;
}

int lfunc(char *)
{
  if (currlib >= 0 && currlib < ndll) {
    listfunc(&dl[currlib]);
    return 1;
  }

  return 0;
}

int llib(char *)
{
  if (ndll > 0) {
    int i;

    for (i=0; i < ndll; i++) {
      printf("%d ==> %s\n", i, dl[i].libname);
      fflush(stdout);
    }
  } else return 0;

  return 1;
}

typedef enum ePType {
  STRING, SCALAR, VAR, UNKNOWN
} ePType;


#define MAX_ARG 80

static int Argc;
static char *Argv[MAX_ARG];
static ePType Argt[MAX_ARG];

/* Retourne 1 si bonne analyse de paramètre, sinon zéro
 Un paramètre commence par :
" si c'est une constante chaine
un chiffre un point ou un moins si c'est une constante numérique
une lettre ou _ si c'est un nom de variable */
int GetArgs(char *args)
{
  /* 3 cas : scalaire, chaine, variable */
  size_t l;
  char *p, *p2;

  Argc=0;
  l=strlen(args);
  for (p=args; p < args+l && Argc < MAX_ARG; p++) {
    p=lws(p);

    if (*p == '"') {
      p++;
      p2=strchr(p, '"');
      if (p2) {
        *p2='\0';
        Argv[Argc]=strdup(p);
        Argt[Argc++]=STRING;
        p=p2;
      } else {
        perror("paramètre chaine non terminé");
        return 0;
      }
    } else if (isdigit((int)*p) || *p == '.' || *p == '-') {
      p2=strchr(p, ' ');
      if (p2) *p2='\0';
      Argv[Argc]=strdup(p);
      Argt[Argc++]=SCALAR;
      if (p2) p=p2;
      else p=args+l;
    } else if (isalpha((int)*p) || *p == '_') {
      p2=strchr(p, ' ');
      if (p2) *p2='\0';
      Argv[Argc]=strdup(p);
      Argt[Argc++]=VAR;
      if (p2) p=p2;
      else p=args+l;
    }
  }

  return 1;
}

const char *TypeArg(ePType t)
{
  switch(t) {
  case STRING:
    return "STRING";
  case SCALAR:
    return "SCALAR";
  case VAR:
    return "VAR";
   default :
    return "UNKNOWN";
  }
}

void ListArgs()
{
  int i;

  for (i=0; i < Argc; i++) {
    printf("Paramètre no%d de type %s [%s]\n", i, TypeArg(Argt[i]), Argv[i]); fflush(stdout);
  }
}

typedef struct sParam {
  eType type;
  union {
    char c;
    int d;
    float f;
    char *s;
  };
} sParam;

#define MAX_PARAM 100

#define trc fprintf(stderr, "l %d\n", __LINE__);fflush(stderr);

typedef struct sParamList {
  int n;
  sParam list[MAX_PARAM];
} sParamList;

sParamList par;

void ReinitParam()
{
  int i;

  for(i=0; i < par.n; i++) {
    if (par.list[i].type == DL_STRING) {
      free(par.list[i].s);
      par.list[i].s=NULL;
    }
    par.list[i].type=DL_VOID;
  }

  par.n=0;
}

void InitParam()
{
  static int un=1;

  if (un) {
    int i;
    for(i=0; i < MAX_PARAM; i++) {
      ZeroMemory(&par.list[i], sizeof(sParam));
      par.list[i].type=DL_VOID;
    }

    par.n=0;
    un=0;
  } else ReinitParam();
}

char *reps(char *s)
{
  size_t is, ip;
  char *p;

  p=(char *)malloc(strlen(s)+1);

  for (is=0, ip=0; s[is] && s[is+1]; is++, ip++) {
    if (s[is] == '\\' && (s[is+1] == 'n' || s[is+1] == 'r')) {
      p[ip]='\n';
      puts("bip"); fflush(stdout);
      is++;
    } else {
      p[ip]=s[is];
    }
  }

  return p;
}


size_t StructArg(char *args, void **pp)
{
  /* 3 cas : constante scalaire, constante chaine ou variable */
  size_t l;
  char *p, *p2;
  void *ptr=NULL;

  InitParam();
  l=strlen(args);
  for (p=args; p < args+l; p++) {
    p=lws(p);

    if (*p == '"') {
      p++;
      p2=strchr(p, '"');
      if (p2) { // Ajout d'une constante char *
        *p2='\0';
        par.list[par.n].type=DL_STRING;
        par.list[par.n++].s=strdup(p);
        p=p2;
      } else {
        perror("paramètre chaine non terminé");
        *pp=NULL;
        return 0;
      }
    } else if (isdigit((int)*p) || *p == '.' || *p == '-') { // Ajout d'une constante Entier ou flottant
      p2=strchr(p, ' ');
      if (p2) *p2='\0';

      if (strchr(p, '.')) {
        par.list[par.n].type=DL_FLOAT;
        par.list[par.n++].f=atof(p);
      } else {
        par.list[par.n].type=DL_INT;
        par.list[par.n++].d=atoi(p);
      }
      if (p2) p=p2;
      else p=args+l;
    } else if (isalpha((int)*p) || *p == '_') { // Ajout d'une variable
      sVar *sv;

      p2=strchr(p, ' ');
      if (p2) *p2='\0';
      sv=vsrch(p);

//      printf("Utilisation de la variable [%s]\n", p); fflush(stdout);

      if (sv && !sv->deleted) {
//        printf("Valeur de [%s] %s\n", sv->name, sv->val); fflush(stdout);
        par.list[par.n].type=sv->type;
        if (sv->type == DL_CHAR && sv->isPointer) {
          par.list[par.n].type=DL_STRING;
          par.list[par.n].s=strdup(sv->val);
        } else if (sv->type ==  DL_FLOAT && sv->type == DL_DOUBLE && sv->type == DL_LDOUBLE) par.list[par.n].f=atof(sv->val);
        else par.list[par.n].d=atoi(sv->val);
        par.n++;
      } else {
        fprintf(stderr, "Undefined var '%s'\n", p);
        return 0;
      }

      if (p2) p=p2;
      else p=args+l;
    }
  }

  *pp=ptr;
  return 1;
}


/* Appel de fonction en 32 bits */
/* L'appel d'une fonction en stdcall reviens à lui passer un seul paramètre
 * d'un type structuré constitué des paramétres tels que déclarés pour cette fonction */
int callfunc(sDllFunc *dllf, char *)
{
  if (dllf && dllf->func) {
#ifdef WIN64
#else
    void *p;
    size_t sz;
    if ((sz=StructArg(args, &p))) {
      int i;
      /* Empile les parametres avant appel */
      for (i=par.n-1; i >=0 ; i--) {
        switch (par.list[i].type) {
          case DL_FLOAT:
            asm("pushl %0" :: "r" ( par.list[i].f ) );
            break;
          case DL_INT:
            asm("pushl %0" :: "r" ( par.list[i].d ) );
            break;
          case DL_STRING:
            asm("pushl %0" :: "r" ( par.list[i].s ) );
            break;
          default :
            break;
        }
      }
      dllf->func((void *)p);

      return 1;
    }
#endif
  }
  
  return 0;
}

