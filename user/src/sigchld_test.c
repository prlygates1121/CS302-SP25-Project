#include "../lib/user.h"
#include "../../os/signal/signal.h"

static int reaped_count = 0;
static const int total_children = 3;

void sigchld_handler(int signo, siginfo_t *info, void *ctx) {
    printf("=== SIGCHLD Handler Called ===\n");
    // printf("Received SIGCHLD (signo=%d) from Child PID=%d\n", 
    //        signo, info->si_pid);
    // printf("Exit Status (si_code)=%d\n", info->si_code);

    int status;
    int pid = wait(-1, &status);
    printf("Received SIGCHLD (signo=%d) from Child PID=%d\n", 
        signo, pid);
    printf("Exit Status (si_code)=%d\n", info->si_code);
    if (pid > 0) {
        reaped_count++;
        int exit_status = status;
        printf("Successfully reaped Child PID=%d, Exit Status=%d\n", 
               pid, exit_status);
        printf("Total reaped: %d/%d\n", reaped_count, total_children);
    } else if (pid == -ECHILD) {
        printf("No more children to reap\n");
    } else {
        printf("wait failed with error: %d\n", pid);
    }

    if (reaped_count == total_children) {
        printf("All children reaped. Parent exiting.\n");
        exit(0);
    }
}

int main() {
    sigaction_t sa = {0};
    sa.sa_sigaction = sigchld_handler;
    sa.sa_restorer = sigreturn; 
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        printf("Error: Failed to register SIGCHLD handler\n");
        exit(1);
    }

    for (int i = 0; i < total_children; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("Error: fork failed\n");
            exit(1);
        } else if (pid == 0) {
            sleep((i + 1) * 2);
            exit(10 + i);
        }
    }

    while (1) {
        sleep(1);
        printf("Parent is running... %d children reaped\n", reaped_count);
        if (reaped_count == total_children) {
            break;
        }
    }

    printf("Parent finished successfully.\n");
    return 0;
}