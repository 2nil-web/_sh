
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <getopt.h>

#include "util.h"
#include "exec.h"

char *prmpt="_sh>";


void runfile(char *filename)
{
  char s[PATH_MAX];
  FILE *fp=fopen(filename, "r");

  if (fp) {
    for (;;) {
      if (fgets(s, PATH_MAX, fp) == NULL) break;
      /* Enléve le <CR> */
      s[strlen(s)-1]='\0';
      execute_line(s);
    }
    fclose(fp);
  }
}

int keep=0;
int not_c_or_f=1;

int parse_args(int argc, char **argv)
{
  int opt;
  while((opt=getopt(argc, argv, "c:f:kp:s")) != EOF) switch(opt) {
  case 's' : {
    extern int silent_run;
    silent_run=1;
    } break;
  case 'k' :
    keep=1;
    break;
  case 'p' :
    prmpt=strdup(optarg);
    break;
  case 'c' :
    execute_line(optarg);
    not_c_or_f=0;
    break;
  case 'f' :
    runfile(optarg);
    not_c_or_f=0;
    break;
  }

  return 1;
}

int main(int argc, char **argv)
{
  parse_args(argc, argv);
  if (keep || not_c_or_f)
    exec_cmds(wipe_suffix(basename(argv[0])), prmpt);

  fflush(stdout);
  return 0;
}

