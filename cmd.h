
#ifndef CMD_H
#define CMD_H

/* A structure which contains information on the commands this program can understand. */
typedef struct {
  char *name;			/* User printable name of the function. */
  rl_icpfunc_t *func;		/* Function to call to do the job. */
  char *doc;			/* Documentation for this function.  */
} COMMAND;

extern COMMAND commands[];

#endif /* CMD_H */

