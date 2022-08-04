
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <readline/readline.h>
#include <readline/history.h>

#ifdef WIN32
#include <tchar.h>
#include <windows.h>
#include <mmsystem.h>
#endif

#include "util.h"
#include "exec.h"
#include "alias.h"
#include "var.h"
#include "lib.h"
#include "cmd.h"


/* Pour indiquer un titre */
int fUL(char *s) { return 0; }
/* Pour indiquer une remarque */
int fREM(char *s) { return 0; }

/* Pour indiquer que le commentaire est identique au précédent */
#define sPrec "idprev"

void disp_cmd_help(int i, int prev, int lng)
{
  int d;

  // Titre à afficher en souligné
  if (!commands[i].name && commands[i].func && commands[i].doc) {
    if (!lng && commands[i].func != fREM) putchar('\n');
    if (lng || commands[i].func != fREM) OutUnderlineText(commands[i].doc);
    if (lng) putchar('\n');
    else putchar(' ');
  }

  /* Commande à afficher */
  if (commands[i].name && commands[i].func && commands[i].doc) {
    if ((strcmp(commands[i].doc, sPrec) == 0 && prev > -1) || strcmp(commands[i].doc, sPrec) != 0) {
    if (strcmp(commands[i].doc, sPrec) == 0) d=prev;
    else d=i;
    OutBoldText(commands[d].name);
    if (lng) printf("\t%s\n", c2ostr(commands[d].doc));
    else putchar (' ');
    fflush(stdout);
    }
  }
  // Commentaire simple
  if (lng && !commands[i].name && !commands[i].func && commands[i].doc) {
    printf("%s\n", c2ostr(commands[i].doc)); fflush(stdout);
  }
}

/* Print out help for ARG, or for all of the commands if ARG is not present. */
int com_help(char *arg, int lng)
{
  int i;
  int printed=0;

  if (*arg) {
    int prev=-1;
    for(i=0; commands[i].name || commands[i].func || commands[i].doc; i++) {
      if(commands[i].name && strcmp(arg, commands[i].name) == 0) {
        disp_cmd_help(i, prev, 1);
        printed++;
      }
      if (commands[i].doc && strcmp(commands[i].doc, sPrec) != 0) prev=i;
    }
  } else {
    for(i=0; commands[i].name || commands[i].func || commands[i].doc; i++) {
      disp_cmd_help(i, -1, lng);
    }
    if (!lng) puts("\nTaper 'help' pour plus de détail\n");
    fflush(stdout);
    printed++;
  }

  if (!printed) {
      printf("No commands match `%s'.  Possibilities are:\n", arg);

      for(i=0; commands[i].name || commands[i].func || commands[i].doc; i++) {
        if (commands[i].name && commands[i].func) {
          /* Print in six columns. */
          if(printed == 6) {
              printed=0;
              printf("\n");
          }

          printf("%s\t", c2ostr(commands[i].name));
          printed++;
        }
      }

      if(printed) printf("\n");
      fflush(stdout);
  }

  fflush(stdout);

  return 0;
}

int short_help(char *arg) { com_help(arg, 0); return 1; }

int  long_help(char *arg) { com_help(arg, 1); return 1; }

/* echo ARG */
int com_echo(char *s)
{
  puts(s); fflush(stdout);
  return 0;
}

extern char **environ;

/* print env */
int com_env(char *arg)
{
  char **env=environ;
  while (*env) {
    if (arg && *arg) {
      if (strstr(*env, arg)) puts(*env);
      env++;
    } else puts(*env++);
    fflush(stdout);
  }
  return 0;
}

int com_hist(char *arg)
{
  HISTORY_STATE *hs;
  int i=0;

  hs=history_get_history_state();
  if (hs->length > 30) i=hs->length-30;
  for (; i < hs->length; i++)
    printf("%d %s\n", i + 1, hs->entries[i]->line);
  fflush(stdout);

  return 0;
}

int com_prmpt(char *arg)
{
  extern char *_sh_prompt; /* Déclaré dans exec.c */

  if (_sh_prompt) free(_sh_prompt);
  _sh_prompt=strdup(arg);

  return 0;
}

/* The user wishes to quit using this program.  Just set DONE non-zero. */
int com_quit(char *arg)
{
  extern int done;
  done=1;
  return 0;
}

/* Print out the current working directory. */
int com_pwd(char *ignore)
{
  char dir[PATH_MAX], *s;

  s=getcwd(dir, PATH_MAX-1);
  if(s == 0) {
      fprintf(stderr, "Error getting pwd: %s\n", dir); fflush(stderr);
      return 1;
  }

  printf("Current directory is %s\n", dir); fflush(stdout);
  return 0;
}

/* Change to the directory ARG. */
int com_cd(char *arg)
{
  static char lastdir[PATH_MAX]="";
  char dir[PATH_MAX];

  if (arg && *arg != '\0') strncpy(dir, arg, PATH_MAX);
  else strncpy(dir, getenv("HOME"), PATH_MAX);

  if (strcmp(dir, "-") == 0) {
    if (lastdir[0]) strncpy(dir, lastdir, PATH_MAX);
    else return 0;
  }
  
  getcwd(lastdir, PATH_MAX-1);

  if(chdir(dir) == -1) {
      ShError(dir);
      return 1;
  }

//  com_pwd("");
  return 0;
}

int haswildcard(char *s)
{
  size_t i;
  for (i=0; i < strlen(s); i++) {
    if (s[i] == '?' || s[i] == '*') return 1;
  }

  return 0;
}

int com_ls(char *arg)
{
#ifdef WIN32
   WIN32_FIND_DATA ffd;
   const char *dir;
   LARGE_INTEGER filesize;
   TCHAR szDir[PATH_MAX];
   HANDLE hFind=INVALID_HANDLE_VALUE;
   DWORD dwError=0;
 
   if (arg&&*arg!='\0') dir=arg;
   else dir=".";

   if (strlen(dir) > PATH_MAX-2) {
      ShError("\nDirectory path is too long.\n");
      return 0;
   }

   // Prepare string for use with FindFile functions.  First, copy the
   // string to a buffer, then append '\*' to the directory name.
   strncpy(szDir, dir, PATH_MAX);
   if (!haswildcard(szDir)) strncat(szDir, "\\*", 3);

   // Find the first file in the directory.
   hFind=FindFirstFile(szDir, &ffd);

   if (INVALID_HANDLE_VALUE == hFind) {
      ShError("FindFirstFile");
      return 0;
   } 
   
   // List all the files in the directory with some info about them.
   do {
      if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
         if (strcmp(ffd.cFileName, ".") != 0 && strcmp(ffd.cFileName, "..") != 0) printf("   <DIR>     %s\n", ffd.cFileName);
      } else {
         filesize.LowPart=ffd.nFileSizeLow;
         filesize.HighPart=ffd.nFileSizeHigh;
         printf("%12ld ", (long int)filesize.QuadPart);
         _tprintf(TEXT("%s\n"), ffd.cFileName);
      }
   } while (FindNextFile(hFind, &ffd) !=0);
 
   dwError=GetLastError();
   if (dwError!=ERROR_NO_MORE_FILES) {
      ShError("FindFirstFile");
   }

   FindClose(hFind);
   return dwError;
#else
   return 0;
#endif
}

int com_cat(char *arg)
{
  if (arg && arg[0]) {
    char c[2];
    FILE *fp=fopen(arg, "r");

    if (fp) {
      c[1]='\0';
      for (;;) {
        c[0]=fgetc(fp);
        if (feof(fp)) break;
        OutBoldText(c);
      }
      fclose(fp);
      return 1;
    } else ShError(arg);
  } else ShError("cat Argument manquant.");

  return 0;
}

#ifdef WIN32
int com_play(char *arg)
{
  if (arg && arg[0]) {
      if (!sndPlaySound(arg, SND_ASYNC)) ShError(arg);
  } else sndPlaySound(NULL, SND_ASYNC);

  return 0;
}

int com_vol(char *arg)
{
  DWORD bothVol;
  WORD leftVol, rightVol;
  int ac;
  char **av;
  av=line2arg(arg, &ac);

  waveOutGetVolume(0, &bothVol);

  leftVol=bothVol&0xFFFF;
  rightVol=(bothVol>>16)&0xFFFF;

  printf("Volume HP avant, gauche %d, droit %d (max 65535)\n", leftVol, rightVol); fflush(stdout);

  if (ac > 0) { /* Si au moins un arg on régle le volume gauche */
    if (av[0][0] != '-') leftVol=atoi(av[0]);

    if (ac > 1) /* Si au moins 2 args on régle le volume droit aussi */
      if (av[1][0] != '-') rightVol=atoi(av[1]);

    bothVol=leftVol|(rightVol<<16);
    printf(c2ostr("Volume HP après, gauche %d, droit %d\n"), leftVol, rightVol); fflush(stdout);
    waveOutSetVolume(0, bothVol);
  }

  return 0;
}
#endif

COMMAND commands[]={
  { (char *)NULL, fUL, (char *)"Commandes générales :" },
  { (char *)"?", short_help, (char *)"Liste abrégée, des commandes ou décrit la commande passée en paramètre." },
  { (char *)"help", long_help, (char *)"Liste détaillée, des commandes ou détail la commande passée en paramètre." },
  { (char *)"echo", com_echo, (char *)"Affiche la chaine passée en paramétre. Abrégé 'e'" },
  { (char *)"e", com_echo, (char *)sPrec },
  { (char *)"env", com_env, (char *)"Affiche l'environnement avec un éventuel paramétre pour filtrer." },
  { (char *)"hist", com_hist, (char *)"Affiche l'historique des commandes. Abrégé 'h'" },
  { (char *)"h", com_hist, (char *)sPrec },
  { (char *)"prompt", com_prmpt, (char *)"Modifie le prompt.\n\tPeut contenir des variables d'environnement ou des commandes contenues dans \"$()\"\n\tet qui sont rafraichies à chaque affichage. Abrégé 'p'" },
  { (char *)"h", com_prmpt, (char *)sPrec },
  { (char *)"!", recall_cmd, (char *)"Rappel la dernière commande ou la numéro N." },
  { (char *)"alias", com_alias, (char *)"Sans paramétre, liste les alias, avec paramètres ajoute un alias.\n\tUn alias est le synonyme d'une commande\n\tEx : \"alias alias typedef\" ou \"alias ushort WORD\" ou \"alias ulong DWORD\" ou \" typedef ulong unsigned_long\"." },
  { (char *)"quit", com_quit, (char *)"Sort de _sh. Abrégé 'q' ou 'x', syn. 'exit' " },
  { (char *)"exit", com_quit, (char *)sPrec },
  { (char *)"x", com_quit, (char *)sPrec },
  { (char *)"q", com_quit, (char *)sPrec },

  { (char *)NULL, fUL, (char *)"Manipulation de fichiers :" },
  { (char *)"pwd", com_pwd, (char *)"Affiche le répertoire courant." },
  { (char *)"cd", com_cd, (char *)"Change de répertoire." },
  { (char *)"ls", com_ls, (char *)"Liste les fichiers du répertoire. Abrégé 'l' ou 'd', syn. 'dir'" },
  { (char *)"dir", com_ls, (char *)sPrec },
  { (char *)"l", com_ls, (char *)sPrec },
  { (char *)"d", com_ls, (char *)sPrec },
  { (char *)"cat", com_cat, (char *)"Affiche le contenu d'un fichier. Abrégé 'c'" },
  { (char *)"c", com_cat, (char *)sPrec },

  { (char *)NULL, fUL, (char *)"Manipulation de variables internes :" },
  { (char *)"char", defchar, (char *)"Définie et/ou affecte une valeur initiale à un caractère\n\tEx : char toto=c\n\tOu : char s[10]=tutu tat\n\tOu : char s [10]=tutu tat" },
  { (char *)"uchar", defuchar, (char *)"Même chose pour un caractére non signé" },
  { (char *)"int", defint, (char *)"Définie et/ou affecte une valeur à un entier\n\tEx : int toto=12" },
  { (char *)"uint", defuint, (char *)"Même chose pour un entier non signé" },
  { (char *)"long", deflong, (char *)"Définie et/ou affecte une valeur à un entier long" },
  { (char *)"ulong", defulong, (char *)"Même chose pour un entier long non signé" },
  { (char *)"longlong", defllong, (char *)"Définie et/ou affecte une valeur à un entier long" },
  { (char *)"ulonglong", defullong, (char *)"Même chose pour un entier long non signé" },
  { (char *)"float", deffloat, (char *)"Définie et/ou affecte une valeur à un nombre à virgule\n\tEx : float toto=12.345" },
  { (char *)"double", defdouble, (char *)"Même chose pour un double" },
  { (char *)"longdouble", defldouble, (char *)"Même chose pour un long double" },
  { (char *)"struct", defstruct, (char *)"Définie un type structuré ou déclare une variable d'un type structuré déjà défini\n\tEx : struct stoto { double toto int tata char tutu }\n\tOu : struct stoto toto\n\tPas d'intialisation en bloque, on peut initialiser champ par champ ( Ex : int stoto.tata 12 )\n\tPas de tableau de structure et que des type simples dans la structure." },
  { (char *)"sizeof", vsizeof, (char *)"Affiche la taille d'un type (simple pour l'instant). Abrégé 's'" },
  { (char *)"s", vsizeof, (char *)sPrec },
  { (char *)"vecho", vecho, (char *)"Affiche les variables internes ou chaines passées en paramétre, les chaines doivent être délimité par \". Abrégé 've'" },
  { (char *)"ve", vecho, (char *)sPrec },
  { (char *)"vlist", vlist, (char *)"Liste toutes les variables internes existantes, le paramétre peut servir à filtrer les noms de variables. Abrégé 'vl'" },
  { (char *)"vl", vlist, (char *)sPrec },
  { (char *)"unset", vunset, (char *)"Efface une variable. Abrégé 'u'" },
  { (char *)"u", vunset, (char *)sPrec },

  { (char *)NULL, fUL, (char *)"Manipulation de bibliothéques dynamiques :" },
  { (char *)"loadlib", loadlib, (char *)"Charge la bibliothéques dynamique indiquée et la positionne comme bibliothéque courante" },
  { (char *)"ldl", loadlib, (char *)sPrec },
  { (char *)"listlib", llib, (char *)"Liste toutes les bibliothéques chargées en indiquant leur numéro." },
  { (char *)"ll", llib, (char *)sPrec },
  { (char *)"setlib", setlib, (char *)"Positionne comme bibliothéque courante celle dont le numéro est passé en paramètre." },
  { (char *)"sl", setlib, (char *)sPrec },
  { (char *)"listfunc", lfunc, (char *)"Liste toutes les fonctions de la bibliothéque courante (6 par ligne)." },
  { (char *)"lf", lfunc, (char *)sPrec },
  { (char *)"call", (int (*)(char *))callfunc, (char *)"Execute la fonction contenu dans la bibliothéque dynamique,\n\ten lui passant les paramètres indiqués, par adresse (précédé par &) ou par valeur." },
#ifdef WIN32
  { (char *)NULL, fUL, (char *)"Reproduction sonore :" },
  { (char *)"play", com_play, (char *)"Joue le fichier passé en paramètre ou arrête si pas de paramètre." },
  { (char *)"vol", com_vol, (char *)"Affiche et si paramètres, régle le volume des hp gauche (1er paramétre) et droit (2éme paramétre)." },
#endif
  { (char *)NULL, fREM, (char *)"Remarques générales :" },
  { (char *)NULL, NULL, (char *)"Les variables d'environnement sont développées avant toute exécution de commande." },
  { (char *)NULL, NULL, (char *)"Si une commande n'est pas identifiée comme commande interne, un alias ou une fonction de bibliothéque, on tente de l'exécuter dans un shell." },
  { (char *)NULL, NULL, (char *)"Toute déclaration de variable interne dont le type est suivi de l'astérique (*) indique un pointeur\n\rEx : char *toto." },
  { (char *)NULL, NULL, (char *)"Toute ligne terminée par le caractére '\' indique son prolongement sur la ligne suivante." },
  { (char *)NULL, NULL, (char *)"Les caractéres '%' '$' '\"' et '\\' pouvant avoir une signification particuliéres, il peuvent être neutralisé par \\." },

  {(char *)NULL,(rl_icpfunc_t *)NULL,(char *)NULL }
};

