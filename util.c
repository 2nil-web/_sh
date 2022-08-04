
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef WIN32
#include <tchar.h>
#include <windows.h>
#endif

#include "util.h"

/* Souligné on/off */
#define SMUL "[4m"
#define RMUL "[24m"

/* Gras on/off */
#define BOLD "[1m"
#define SGR0 "[m(B"

/* Combin. val. pour SetConsoleTextAttribute
FOREGROUND_RED FOREGROUND_GREEN FOREGROUND_BLUE	FOREGROUND_INTENSITY
BACKGROUND_RED BACKGROUND_GREEN BACKGROUND_BLUE BACKGROUND_INTENSITY
*/

#ifdef WIN32
static HANDLE hConsOut=NULL;
static CONSOLE_SCREEN_BUFFER_INFO csbi;
void InitCons()
{
  if (!hConsOut) {
    hConsOut=GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsOut == INVALID_HANDLE_VALUE) ShError("GetStdHandle");
    else {
//      printf("%d %X\n", GetConsoleCP(), GetConsoleCP()); fflush(stdout);
      GetConsoleScreenBufferInfo(hConsOut, &csbi);
    }
  }
}
#endif

void FlushConsole()
{
#ifdef WIN32
  InitCons();
  FlushFileBuffers(hConsOut);
#endif
}

void OutBoldText(char *s)
{
#ifdef WIN32
  InitCons();

  if (hConsOut) {
    DWORD l;
    SetConsoleTextAttribute(hConsOut, FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE|FOREGROUND_INTENSITY);
    WriteConsole(hConsOut, c2ostr(s), strlen(s), &l, NULL);
    SetConsoleTextAttribute(hConsOut, csbi.wAttributes);
  } else {
#endif
    printf(BOLD "%s" SGR0, s); fflush(stdout);
#ifdef WIN32
  }
#endif
}

void OutUnderlineText(char *s)
{
#ifdef WIN32
  InitCons();

  if (hConsOut) {
    DWORD l;
    SetConsoleTextAttribute(hConsOut, BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY|FOREGROUND_GREEN);
    WriteConsole(hConsOut, c2ostr(s), strlen(s), &l, NULL);
    SetConsoleTextAttribute(hConsOut, csbi.wAttributes);
  } else {
#endif
    printf(SMUL "%s" RMUL, s); fflush(stdout);
#ifdef WIN32
  }
#endif
}


/* Affiche l'erreur windows sur stderr */
void ShError(const char *fmt, ...)
{
#ifdef WIN32
  TCHAR *lpMsgBuf;
  TCHAR ls[512];
  TCHAR ls2[1024];
  va_list ap;

  va_start(ap, fmt);
  _vsntprintf(ls, 512, TEXT(fmt), ap);
  va_end(ap);

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
      NULL, GetLastError(),
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
      (LPTSTR) &lpMsgBuf, 0, NULL);

  /* Display the string. */
//  MessageBox(NULL, lpMsgBuf, ls, MB_OK);
  _stprintf(ls2, TEXT("%s : %s\n"), ls, lpMsgBuf);//fflush(stderr);
  OutBoldText(ls2);
  /* Free the buffer. */
  LocalFree(lpMsgBuf);
#endif
}


/* Strip whitespace from the left start of STRING.  Return a pointer into STRING. */
char *lws(char *string)
{
  char *s;

  for(s=string; isspace(*s); s++) ;

  return s;
}

/* Strip whitespace from the start and end of STRING.  Return a pointer into STRING. */
char *ws(char *string)
{
  char *s, *t;

  for(s=string; isspace(*s); s++) ;
  if (*s == 0) return s;
  t=s + strlen(s)- 1;
  while(t > s && isspace(*t)) t--;
  *++t='\0';
  return s;
} 


char *wipe_suffix(char *s)
{
  char *p=strrchr(s, '.');
  if (p) *p='\0';
  return s;
}

char *c2ostr(const char *s)
{
    static char *o=NULL;

    if (o) free(o);
    o=strdup(s);
#ifdef WIN32
    CharToOem(s, o);
#endif
    return o;
}



/* Extrait les chaines séparées par des espaces ou ".
 " a la priorité sur <ESPACE> 
 La chaine doit-être terminée par '\0' */
char *mystrtok(char *s)
{
  static char *ptrptr=NULL, *pcurr, *pnext;
  char *p=NULL;

  if (s) {
    if (ptrptr) free(ptrptr);
    ptrptr=strdup(s);
    if (ptrptr) pcurr=ptrptr;
    else return NULL;
  } else {
    if (pnext) pcurr=pnext;
    else return NULL;
  }

  p=pcurr;

  /* Passe les espaces si il y en a */
  while (*p && isspace(*p)) p++;

  if (*p) pcurr=p;
  else return NULL;

  /* Si " recherche le " suivant */
  if (*p == '"') {
    p++;
    if (!*p) return NULL;
    pcurr=p;
    while (*p && *p != '"') p++;

    if (*p) {
      *p='\0';
      p++;
      if (*p) pnext=p;
      else pnext=NULL;
    } else pnext=NULL;
      
    return pcurr;
  }

  /* Sinon recherche l'espace suivant */
  while (*p && !isspace(*p)) p++;
  if (*p) {
    *p='\0';
    p++;
    pnext=p;
  } else pnext=NULL;

  return pcurr;
}


/* Eclate une ligne en arguments */
char **line2arg(char *line, int *ac)
{
  if (line && line[0]) {
    char **av=NULL, *l, *s;
    
    *ac=0;
    l=strdup(line);
    s=mystrtok(l);

    while(s) {
      av=(char **)realloc(av, (*ac+1)*sizeof(char **));
      av[*ac]=s;
      (*ac)++;
      s=mystrtok(NULL);
    }

    return av;
  }

  return NULL;
}

