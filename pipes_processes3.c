#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <grep-pattern>\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *pattern = argv[1];

    int pipe12[2]; // cat -> grep
    int pipe23[2]; // grep -> sort
    if (pipe(pipe12) == -1) die("pipe12");
    if (pipe(pipe23) == -1) die("pipe23");

    pid_t p2 = fork();
    if (p2 < 0) die("fork p2");

    if (p2 == 0) {
        // --- Child (P2 = grep) ---
        // Create child's child (P3 = sort)
        pid_t p3 = fork();
        if (p3 < 0) die("fork p3");

        if (p3 == 0) {
            // --- Child's Child (P3 = sort) ---
            // stdin <- pipe23[0]
            // stdout -> parent's stdout
            if (dup2(pipe23[0], STDIN_FILENO) == -1) die("dup2 sort stdin");
            // Close all unused fds
            close(pipe23[1]); // write end not used by sort
            close(pipe23[0]);
            close(pipe12[0]);
            close(pipe12[1]);

            execlp("sort", "sort", (char *)NULL);
            die("execlp sort");
        } else {
            // --- Still in P2 (grep) ---
            // stdin <- pipe12[0]
            if (dup2(pipe12[0], STDIN_FILENO) == -1) die("dup2 grep stdin");
            // stdout -> pipe23[1]
            if (dup2(pipe23[1], STDOUT_FILENO) == -1) die("dup2 grep stdout");

            // Close all ends not needed by grep
            close(pipe12[1]);
            close(pipe12[0]);
            close(pipe23[0]);
            close(pipe23[1]);

            execlp("grep", "grep", pattern, (char *)NULL);
            die("execlp grep");
        }
    } else {
        // --- Parent (P1 = cat scores) ---
        // stdout -> pipe12[1]
        if (dup2(pipe12[1], STDOUT_FILENO) == -1) die("dup2 cat stdout");

        // Close all ends not used by cat
        close(pipe12[0]);
        close(pipe12[1]);
        close(pipe23[0]);
        close(pipe23[1]);

        // Note: As in the example, we don't strictly need to wait here;
        // but the shell returns after exec anyway.
        execlp("cat", "cat", "scores", (char *)NULL);
        die("execlp cat");
    }
}
