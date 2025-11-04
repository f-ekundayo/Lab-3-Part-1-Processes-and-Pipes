#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define BUF 1024

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static void strip_newline(char *s) {
    size_t n = strlen(s);
    if (n && s[n-1] == '\n') s[n-1] = '\0';
}

int main(void) {
    int p1_to_p2[2]; // P1 writes -> P2 reads
    int p2_to_p1[2]; // P2 writes -> P1 reads
    pid_t pid;

    if (pipe(p1_to_p2) == -1) die("pipe p1_to_p2");
    if (pipe(p2_to_p1) == -1) die("pipe p2_to_p1");

    pid = fork();
    if (pid < 0) die("fork");

    if (pid == 0) {
        // --- Child (P2) ---
        // Close unused ends
        close(p1_to_p2[1]); // child reads from p1_to_p2[0]
        close(p2_to_p1[0]); // child writes to p2_to_p1[1]

        char in1[BUF];
        ssize_t n = read(p1_to_p2[0], in1, sizeof(in1) - 1);
        if (n < 0) die("child read");
        in1[n] = '\0';

        // Remove any trailing newline from P1 input (in case)
        strip_newline(in1);

        // Print required lines and concatenate "howard.edu"
        printf("Other string is: howard.edu\n");
        char cat1[BUF * 2];
        snprintf(cat1, sizeof(cat1), "%showard.edu", in1);
        printf("Input : %s\n", in1);
        printf("Output : %s\n", cat1);
        fflush(stdout);

        // Prompt for second input, append it, then send back
        char in2[BUF];
        printf("Input : ");
        fflush(stdout);
        if (!fgets(in2, sizeof(in2), stdin)) {
            // If EOF, just use empty string
            in2[0] = '\0';
        } else {
            strip_newline(in2);
        }

        char back[BUF * 2];
        snprintf(back, sizeof(back), "%s%s", cat1, in2);

        size_t len = strlen(back);
        if (write(p2_to_p1[1], back, len) != (ssize_t)len) die("child write");

        // Clean up
        close(p1_to_p2[0]);
        close(p2_to_p1[1]);
        _exit(EXIT_SUCCESS);
    } else {
        // --- Parent (P1) ---
        // Close unused ends
        close(p1_to_p2[0]); // parent writes to p1_to_p2[1]
        close(p2_to_p1[1]); // parent reads from p2_to_p1[0]

        char first[BUF];
        printf("Input : ");
        fflush(stdout);
        if (!fgets(first, sizeof(first), stdin)) die("parent fgets");
        strip_newline(first);

        size_t len = strlen(first);
        if (write(p1_to_p2[1], first, len) != (ssize_t)len) die("parent write");
        close(p1_to_p2[1]); // done sending

        // Read back from child
        char mid[BUF * 2];
        ssize_t n = read(p2_to_p1[0], mid, sizeof(mid) - 1);
        if (n < 0) die("parent read");
        mid[n] = '\0';

        // Append "gobison.org" and print final
        char final[BUF * 3];
        snprintf(final, sizeof(final), "%sgobison.org", mid);
        printf("Output : %s\n", final);
        fflush(stdout);

        close(p2_to_p1[0]);
        // Wait for child to avoid a zombie (good hygiene)
        int status = 0;
        (void)waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : EXIT_FAILURE;
    }
}
