#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (fork() == 0) {
        const char *myargs[2];
        myargs[0] = "sort";
        myargs[1] = NULL;

        int out = open("out", O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        int in = open("in", O_RDONLY);
        dup2(in, 0);
        dup2(out, 1);
        close(in);
        close(out);
        execvp(myargs[0], myargs);
        printf("not success\n");
    } else {
        pid_t wpid;
        int status = 0;
        wpid = wait(&status);
    }
}


