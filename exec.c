
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "cmd.h"
#include "alias.h"
#include "lib.h"
#include "util.h"

#define trc printf("%s %d\n", __FILE__, __LINE__);fflush(stdout)

/* Look up NAME as the name of a command, and return a pointer to that
   command.  Return a NULL pointer if NAME isn't a command name. */
COMMAND *find_command(char *name)
{
  int i;

  for(i=0; commands[i].name || commands[i].func || commands[i].doc; i++)
    if (commands[i].name && strcmp(name, commands[i].name) == 0)
      return &commands[i];

  return (COMMAND *)NULL;
}


/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state(i.e. STATE == 0), then we
   start at the top of the list. */
char *command_generator(const char *text, int state)
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state) {
      list_index=0;
      len=strlen(text);
    }

  /* Return the next name which partially matches from the command list. */
  while(commands[list_index].name || commands[list_index].func || commands[list_index].doc) {
    name=commands[list_index].name;
    list_index++;
    if (name) {
      if (strncmp(name, text, len) == 0) return strdup(name);
    }
  }

  /* If no names matched, then return NULL. */
  return (char *)NULL;
}

/* Attempt to complete on the contents of TEXT.  START and END bound the
   region of rl_line_buffer that contains the word to complete.  TEXT is
   the word to complete.  We can use the entire contents of rl_line_buffer
   in case we want to do some simple parsing.  Return the array of matches,
   or NULL if there aren't any. */
char **_sh_completion(const char *text, int start, int end)
{
  char **matches;

  matches=(char **)NULL;

  /* If this word is at the start of the line, then it is a command
     to complete.  Otherwise it is the name of a file in the current
     directory. */
  if (start == 0)
    matches=rl_completion_matches(text, command_generator);

  return matches;
}

/* Interface to Readline Completion */
/* Tell the GNU Readline library how to complete.  We want to try to complete
   on command names if this is the first word in the line, or on filenames if not. */
void initialize_readline(char *pname)
{
  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name=strdup(pname);

  /* Tell the completer that we want a crack first. */
  rl_attempted_completion_function=_sh_completion;
}


/* Retourne le résultat de la commande */
int run_out_sh(char *cmd)
{
  return system(cmd);
  /*
  UINT retwe;
  retwe=WinExec(cmd, 1);

  switch (retwe) {
  case 0 :
    ShError("The system is out of memory or resources.");
    break;
  case ERROR_BAD_FORMAT :
    ShError("The .EXE file is invalid (non-Win32 .EXE or error in .EXE image).");
    break;
  case ERROR_FILE_NOT_FOUND :
    ShError("The specified file was not found.");
    break;
  case ERROR_PATH_NOT_FOUND :
    ShError("The specified path was not found.");
    break;
  default : break;
  }

  fflush(stdout);
  FlushConsole();
  return retwe;*/
}

/* Execute a command line. */
int execute_line(char *line)
{
  int i;
  COMMAND *command;
  sDllFunc *dllf;
  char *word, *full_cmd, *p=NULL;

  /* Isolate the command word. */
  i=0;
  while (isspace((int)line[i])) i++;
  full_cmd=strdup(line);
  word=line+i;
  while(line[i] && !isspace((int)line[i])) i++;
  if (line[i]) line[i++]='\0';
  command=find_command(word);

  if (!command) { /* Si la commande n'est pas reconnue on la cherche dans les alias */
    p=find_alias(word);
    if (p) command=find_command(p);
  }

  if (!command) { /* Si la commande n'est pas reconnue on cherche dans les fonctions de dll */
    dllf=find_func(word);
    if (!dllf && p) dllf=find_func(p);
  }

  if (!command && !dllf) { /* Si ce n'est pas une commande interne on tente comme commande externe */
    return run_out_sh(full_cmd);
  }

  /* Get argument to command, if any. */
  while(isspace((int)line[i])) i++;
  word=line + i;
  /* Call the function. */

  /* Expansion des variables */
  static char *s=NULL;
  DWORD nSize=ExpandEnvironmentStrings(word, NULL, 0);
  if (s) free(s);
  s=(char *)malloc(nSize+1);
  ExpandEnvironmentStrings(word, s, nSize);

  if (command) return (*(command->func))(s);
  if (dllf) return callfunc(dllf, s);

  return 0;
}

int recall_cmd(char *arg)
{
  HISTORY_STATE *hs;
  static char *cmd=NULL;
  int n;
  
  hs=history_get_history_state();
  if (!arg || !*arg) n=hs->length;
  else n=atoi(arg);

  if (n < 1 || n > hs->length) {
    fprintf(stderr, "Mauvais numéro de rappel de commande (%d)\n", n); fflush(stderr);
    return 0;
  }

  if (cmd) free(cmd);
  cmd=strdup(hs->entries[n-1]->line);
  puts(cmd); fflush(stdout);
  execute_line(cmd);
  return 1;
}

void filtered_add_history(char *s)
{
  HISTORY_STATE *hs=history_get_history_state();

  if (hs && strcmp(s, "h") !=0 && strcmp(s, "hist") !=0 &&
      ( hs->length <=0 || strcmp(s, hs->entries[hs->length-1]->line) !=0))
    add_history(s);
}


char *_sh_prompt=NULL;

// traduit tout ce qui est entre $() par la variable ou la commande correspondante
char *trad_prompt(char *prompt) {
  static char tp[1024], ttp[2];
  char *p, *pr=strdup(prompt), *e, *ge;

  tp[0]=ttp[1]='\0';

  for (p=pr; ; p++) {
    if (!*p) break;

    if (*p == '$' && p[1] == '(') {
      e=strchr(&p[2], ')'); 

      if (e) {
        *e='\0';
        ge=getenv(&p[2]);

        if (ge) { puts(ge); strncat(tp, ge, 1023); }
        else {
          execute_line(&p[2]);
        }
        p=e;
        continue;
      }
    }

    ttp[0]=*p;
    strncat(tp, ttp, 1023);
  }

  return tp;
}

char *strrecat(char *buf, char *s, size_t l)
{
  buf=(char *)realloc(buf, l+1);
  strcat(buf, s);
  return buf;
}

int done;
int silent_run=0;


void exec_cmds(char *pn, char *prmpt)
{
  char *histfile, *line, *s, *pname;
  static char *buf=NULL;

  if (!silent_run) puts("dll_sh v1.0.\nTaper '?' pour avoir la liste des commandes.");

  pname=c2ostr(pn);
  _sh_prompt=strdup(prmpt);

  histfile=(char *)malloc(strlen(pname)+20);
  sprintf(histfile, ".%s_history", pname);

  initialize_readline(pname);
  if (read_history(histfile) !=0) perror("read_history");

  /* Loop reading and executing lines until the user quits. */
  for(done=0; done == 0;) {
    fflush(stdout);
    fflush(stderr);
    if (silent_run) line=readline(NULL);
    else {
      if (!buf) {
        line=readline(trad_prompt(_sh_prompt));
      } else {
        line=readline("?");
      }
    }

    if (!line) break;
    s=ws(line);

    if (*s) {
      /* Commentaire */
      if ((s[0]) == '#') continue;
      /* Rappel de commande */
      if ((s[0]) == '!') recall_cmd(ws(&s[1]));
      else {
        size_t l=strlen(s);

        if (s[l-1] == '\\') {
          s[l-1]='\0';

          if (buf) buf=strrecat(buf, s, l);
          else buf=strdup(s);

          continue;
        }

        if (buf) {
          buf=strrecat(buf, s, l);
          filtered_add_history(buf);
          execute_line(buf);
          free(buf);
          buf=NULL;
        } else {
          filtered_add_history(s);
          execute_line(s);
        }
      }
    }

    free(line);
  }

  if (write_history(histfile) != 0) perror("write_history");
}

