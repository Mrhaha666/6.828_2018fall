//
// Created by haha on 2019/12/20.
//
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

// Simplifed xv6 shell.

#define MAXARGS 10
#define MAXLINE 100
#define DEF_MODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH | S_IWOTH
#define DEF_UNMASK S_IWGRP | S_IWOTH
char *default_dir[] = {"\0", "/usr/bin/", "/bin/"};
// All commands have at least a type. Have looked at the type, the code
// typically casts the *cmd to some specific cmd type.
struct cmd {
    int type;          //  ' ' (exec), | (pipe), '<' or '>' for redirection
};

struct execcmd {
    int type;              // ' '
    char *argv[MAXARGS];   // arguments to the command to be exec-ed
};

struct redircmd {
    int type;          // < or >
    struct cmd *cmd;   // the command to be run (e.g., an execcmd)
    char *file;        // the input/output file
    int flags;         // flags for open() indicating read or write
    int fd;            // the file descriptor number to use for the file
};

struct pipecmd {
    int type;          // |
    struct cmd *left;  // left side of pipe
    struct cmd *right; // right side of pipe
};



void unix_error(char *msg);
int Open(const char *pathname, int flags, mode_t mode);
int Fork(void);  // Fork but exits on failure.
void Execv(const char *filename, char *const argv[]);
void Close(int fd);
int search_file(const char *file);
pid_t Wait(int *status);
int Dup(int fd);

struct cmd *parsecmd(char*);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd)
{
    int p[2], r;
    struct execcmd *ecmd;
    struct pipecmd *pcmd;
    struct redircmd *rcmd;

    if(cmd == 0)
        _exit(0);

    switch(cmd->type){
        default:
            fprintf(stderr, "unknown runcmd\n");
            _exit(-1);

        case ' ':
            ecmd = (struct execcmd*)cmd;
            if(ecmd->argv[0] == 0)
                _exit(0);
            // Your code here ...
            int i = search_file(ecmd->argv[0]);
            int len = strlen(ecmd->argv[0]) + strlen(default_dir[i]) + 1;
            char *filename = malloc(len);
            strcpy(filename, default_dir[i]);
            strncat(filename, ecmd->argv[0], MAXLINE);
            Execv(filename, &ecmd->argv[0]);
            break;

        case '>':
        case '<':
            rcmd = (struct redircmd*)cmd;
            // Your code here ...
            umask(DEF_UNMASK);
            Close(rcmd->fd);
            Open(rcmd->file, rcmd->flags, DEF_MODE);
            runcmd(rcmd->cmd);
            break;

        case '|':
            pcmd = (struct pipecmd*)cmd;
            // Your code here ...
            int fd[2];
	    int status;
            pipe(fd);
            if(Fork() == 0){
                Close(fd[0]);
                Close(1);
                Dup(fd[1]);
                Close(fd[1]);
                runcmd(pcmd->left);
            }
	    if(Fork() == 0){
                Close(fd[1]);
                Close(0);
                Dup(fd[0]);
                Close(fd[0]);
                runcmd(pcmd->right);
            }
	    Close(fd[0]);
	    Close(fd[1]);
            Wait(&status);
	    Wait(&status);
            break;
    }
    _exit(0);
}

int getcmd(char *buf, int nbuf)
{
    if (isatty(fileno(stdin)))
        fprintf(stdout, "6.828$ ");
    memset(buf, 0, nbuf);
    if(fgets(buf, nbuf, stdin) == 0)
        return -1; // EOF
    return 0;
}

int main(void)
{
    static char buf[100];
    int fd, r;

    // Read and run input commands.
    while(getcmd(buf, sizeof(buf)) >= 0){
        if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
            // Clumsy but will have to do for now.
            // Chdir has no effect on the parent if run in the child.
            buf[strlen(buf)-1] = 0;  // chop \n
            if(chdir(buf+3) < 0)
                fprintf(stderr, "cannot cd %s\n", buf+3);
            continue;
        }
        if(Fork() == 0)
            runcmd(parsecmd(buf));
        Wait(&r);
    }
    exit(0);
}

int Dup(int fd){
    int rc = dup(fd);
    if(rc < 0)
        unix_error("Dup error");
    return rc;
}
pid_t  Wait(int *status){
    pid_t pid;

    if ((pid  = wait(status)) < 0)
    unix_error("Wait error");
    return pid;
}

void Close(int fd)
{
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
}
pid_t Fork(void)
{
    pid_t pid;

    if ((pid = fork()) < 0)
        unix_error("Fork error");
    return pid;
}

void Execv(const char *filename, char *const argv[])
{
    if (execv(filename, argv) < 0)
        unix_error("Execve error");
}

int Dup2(int fd1, int fd2)
{
    int rc;

    if ((rc = dup2(fd1, fd2)) < 0)
        unix_error("Dup2 error");
    return rc;
}

int Open(const char *pathname, int flags, mode_t mode)
{
    int rc;

    if ((rc = open(pathname, flags, mode))  < 0)
        unix_error("Open error");
    return rc;
}

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(-1);
}

int search_file(const char *file){
    DIR *streamp;
    struct dirent *dep;
    for(int i = 1; i < 3; ++i){
        streamp = opendir(default_dir[i]);
        errno = 0;
        while((dep = readdir(streamp))!= NULL){
            if(strcmp(file, dep->d_name) == 0){
                return i;
            }
        }
    }
    return 0;
}
struct cmd* execcmd(void)
{
    struct execcmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = ' ';
    return (struct cmd*)cmd;
}

struct cmd* redircmd(struct cmd *subcmd, char *file, int type)
{
    struct redircmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = type;
    cmd->cmd = subcmd;
    cmd->file = file;
    cmd->flags = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
    cmd->fd = (type == '<') ? 0 : 1;
    return (struct cmd*)cmd;
}

struct cmd* pipecmd(struct cmd *left, struct cmd *right)
{
    struct pipecmd *cmd;

    cmd = malloc(sizeof(*cmd));
    memset(cmd, 0, sizeof(*cmd));
    cmd->type = '|';
    cmd->left = left;
    cmd->right = right;
    return (struct cmd*)cmd;
}

// Parsing

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int gettoken(char **ps, char *es, char **q, char **eq)
{
    char *s;
    int ret;

    s = *ps;
    while(s < es && strchr(whitespace, *s))
        s++;
    if(q)
        *q = s;
    ret = *s;
    switch(*s){
        case 0:
            break;
        case '|':
        case '<':
            s++;
            break;
        case '>':
            s++;
            break;
        default:
            ret = 'a';
            while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
                s++;
            break;
    }
    if(eq)
        *eq = s;

    while(s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return ret;
}

int peek(char **ps, char *es, char *toks)
{
    char *s;

    s = *ps;
    while(s < es && strchr(whitespace, *s))
        s++;
    *ps = s;
    return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

// make a copy of the characters in the input buffer, starting from s through es.
// null-terminate the copy to make it a string.
char *mkcopy(char *s, char *es)
{
    int n = es - s;
    char *c = malloc(n+1);
    assert(c);
    strncpy(c, s, n);
    c[n] = 0;
    return c;
}

struct cmd* parsecmd(char *s)
{
    char *es;
    struct cmd *cmd;

    es = s + strlen(s);
    cmd = parseline(&s, es);
    peek(&s, es, "");
    if(s != es){
        fprintf(stderr, "leftovers: %s\n", s);
        exit(-1);
    }
    return cmd;
}

struct cmd* parseline(char **ps, char *es)
{
    struct cmd *cmd;
    cmd = parsepipe(ps, es);
    return cmd;
}

struct cmd* parsepipe(char **ps, char *es)
{
    struct cmd *cmd;

    cmd = parseexec(ps, es);
    if(peek(ps, es, "|")){
        gettoken(ps, es, 0, 0);
        cmd = pipecmd(cmd, parsepipe(ps, es));
    }
    return cmd;
}

struct cmd* parseredirs(struct cmd *cmd, char **ps, char *es)
{
    int tok;
    char *q, *eq;

    while(peek(ps, es, "<>")){
        tok = gettoken(ps, es, 0, 0);
        if(gettoken(ps, es, &q, &eq) != 'a') {
            fprintf(stderr, "missing file for redirection\n");
            exit(-1);
        }
        switch(tok){
            case '<':
                cmd = redircmd(cmd, mkcopy(q, eq), '<');
                break;
            case '>':
                cmd = redircmd(cmd, mkcopy(q, eq), '>');
                break;
        }
    }
    return cmd;
}

struct cmd* parseexec(char **ps, char *es)
{
    char *q, *eq;
    int tok, argc;
    struct execcmd *cmd;
    struct cmd *ret;

    ret = execcmd();
    cmd = (struct execcmd*)ret;

    argc = 0;
    ret = parseredirs(ret, ps, es);
    while(!peek(ps, es, "|")){
        if((tok=gettoken(ps, es, &q, &eq)) == 0)
            break;
        if(tok != 'a') {
            fprintf(stderr, "syntax error\n");
            exit(-1);
        }
        cmd->argv[argc] = mkcopy(q, eq);
        argc++;
        if(argc >= MAXARGS) {
            fprintf(stderr, "too many args\n");
            exit(-1);
        }
        ret = parseredirs(ret, ps, es);
    }
    cmd->argv[argc] = 0;
    return ret;
}
