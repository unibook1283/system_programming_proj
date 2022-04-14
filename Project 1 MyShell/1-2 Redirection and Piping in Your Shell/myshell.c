/* $begin shellmain */
#include "csapp.h"
#include "myshell.h"

int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	printf("> ");                   
	fgets(cmdline, MAXLINE, stdin); 
	if (feof(stdin))
	    exit(0);

	/* Evaluate */
	eval(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin eval */
/* eval - Evaluate a command line */
void eval(char *cmdline) 
{
    char *argv[MAXARGS];                    /* Argument list execve() */
    char *argv_pipe[MAXPIPES][MAXARGS];     /* Argument list for pipe */
    char buf[MAXLINE];                      /* Holds modified command line */
    int bg;                                 /* Should the job run in bg or fg? */
    pid_t pid;                              /* Process id */
    
    strcpy(buf, cmdline);
    bg = parseline(buf, argv); 
    if (argv[0] == NULL)  
	return;   /* Ignore empty lines */
    //
    int pipe_cnt;
    if (pipe_cnt = parsepipe(argv, argv_pipe))
        piping(argv_pipe, pipe_cnt, bg, cmdline);
    //
    else if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if (!strcmp(argv[0], "exit")) {
            exit(0);
        }
        if ((pid = Fork()) == 0) {
            // ex) argv: ls -> /bin/ls
            char temp[MAXLINE] = "/bin/";
            strcat(temp, argv[0]);
            argv[0] = temp;
            
            if (execve(argv[0], argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
	    }
	    /* Parent waits for foreground job to terminate */
	    if (!bg){ 
	        int status;
	        if (waitpid(pid, &status, 0) < 0)
		    unix_error("waitfg: waitpid error");
	    }
	    else//when there is backgrount process!
	        printf("%d %s", pid, cmdline);
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "cd")) {
        if (chdir(argv[1]) == -1)   // chdir error
            printf("cd: %s: No such file or directory\n", argv[1]);
        return 1;
    }
    if (!strcmp(argv[0], "quit")) /* quit command */
	exit(0);  
    if (!strcmp(argv[0], "&"))    /* Ignore singleton & */
	return 1;
    return 0;                     /* Not a builtin command */
}
/* $end eval */

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;
    /* Build the argv list */
    argc = 0;
    /*
    char* pipe_delim;
    char* temp_buf = buf;
    while (pipe_delim = strchr(buf, '|')) {
        memmove(pipe_delim + 3, pipe_delim, sizeof(pipe_delim));
        memmove(pipe_delim, " | ", 3);
        buf = pipe_delim + 3;
    }
    buf = temp_buf;
    */
    while ((delim = strchr(buf, ' '))) {
	    argv[argc++] = buf;
	    *delim = '\0';
	    buf = delim + 1;
	    while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end parseline */

int parsepipe(char** argv, char* argv_pipe[MAXPIPES][MAXARGS]) {
    int argv_index = 0;
    int pipe_cnt = 0;
    int argv_pipe_index = 0;
    while (argv[argv_index] != NULL) {
        char* to_insert = argv[argv_index];
        char* cur = argv[argv_index];
        int is_first_char = 1;

        while (*cur != '\0') {
            if (*cur == '|') {
                *cur = '\0';
                if (is_first_char != 1) {
                    argv_pipe[pipe_cnt][argv_pipe_index++] = to_insert;
                }
                to_insert = cur + 1;
                pipe_cnt++;
                argv_pipe_index = 0;
            }
            is_first_char = 0;
            cur++;
        }
        cur--;
        if (*cur) argv_pipe[pipe_cnt][argv_pipe_index++] = to_insert;
        argv_index++;
    }

    return pipe_cnt;
}
void piping(char* argv_pipe[MAXPIPES][MAXARGS], int argv_index, int bg, char* cmdline) {
    pid_t pid_child[MAXPIPES];
    int index = 0;
    int fd[MAXPIPES][2];
    for (int i = 0; i < MAXPIPES; i++) {
        pipe(fd[i]);
    }
    while (argv_pipe[index][0] != NULL) {
        if ((pid_child[index] = Fork()) == 0) {
            if (index != argv_index) {
                dup2(fd[index][1], 1);
            }
            if (index != 0) {
                dup2(fd[index - 1][0], 0);
            }
            for (int i = 0; i < MAXPIPES; i++) {
                for (int j = 0; j < 2; j++) {
                    close(fd[i][j]);
                }
            }
            if (execvp(argv_pipe[index][0], argv_pipe[index]) < 0) {
                printf("%s: Command not found.\n", argv_pipe[index][0]);
            }
            exit(0);
        }
        index++;
    }

    for (int i = 0; i < MAXPIPES; i++) {
        for (int j = 0; j < 2; j++) {
            close(fd[i][j]);
        }
    }

    if (!bg) {
        int status;
        for (int i = 0; i <= argv_index; i++) {
            if (waitpid(pid_child[i], &status, 0) < 0)
                unix_error("waitfg: waitpid error");
        }
    }
    else
        printf("%d %s", pid_child[argv_index - 1], cmdline);
}