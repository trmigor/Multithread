#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    char buf[100];
    int rc;
    int fd[2];
    pipe(fd);
    if (fork() == 0) {
        dup2(fd[1], 1);
        close(fd[1]);
        close(fd[0]);
        execlp("ls", "ls", NULL);
        perror("ls");
        exit(1);
    }
    close(fd[1]);
    while((rc = read(fd[0], buf, sizeof(buf)))>0) {
        /* ... */
    }
    wait(NULL);
}