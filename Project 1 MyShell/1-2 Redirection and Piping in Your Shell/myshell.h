#include<errno.h>
#define MAXARGS   128
#define MAXPIPES  30

/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);
int parsepipe(char** argv, char* argv_pipe[MAXPIPES][MAXARGS]);
void piping(char* argv_pipe[MAXPIPES][MAXARGS], int argv_index, int bg, char* cmdline);