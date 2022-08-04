
#ifndef UTIL_H
#define UTIL_H

void InitCons();
void OutBoldText(char *s);
void OutUnderlineText(char *s);
void ShError(const char *fmt, ...);
void FlushConsole();

char *ws(char *s);
char *lws(char *s);
char *wipe_suffix(char *s);
char *c2ostr(const char *s);
char **line2arg(char *line, int *ac);
#endif /* UTIL_H */

