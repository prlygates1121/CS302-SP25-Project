// user/test_siginfo.c
#include "../lib/user.h"
#include "../../os/signal/signal.h"

volatile int test_done = 0;

void handler(int signo, siginfo_t *info, void *ctx) {
    if (info->si_signo != signo) {
        printf("ERROR: si_signo=%d, expected=%d\n", info->si_signo, signo);
        exit(-1);
    }

    if (info->si_code != 0 || info->si_status != 0 || info->addr != 0) {
        printf("ERROR: si_code=%d, si_status=%d, addr=%p (expected all 0)\n",
               info->si_code, info->si_status, info->addr);
        exit(-1);
    }

    if (signo == SIGALRM) {
        if (info->si_pid != -1) {
            printf("ERROR: SIGALRM si_pid=%d (expected -1)\n", info->si_pid);
            exit(-1);
        }
    } else if (signo == SIGUSR1) {
        int ppid = getppid();
        if (info->si_pid != ppid) {
            printf("ERROR: SIGUSR1 si_pid=%d (expected %d)\n", info->si_pid, ppid);
            exit(-1);
        }
    }

    printf("Test passed: signal %d from PID %d\n", info->si_signo, info->si_pid);
    test_done = 1;
    sigreturn();
}

int main() {
    struct sigaction sa = {
        .sa_sigaction = handler,
        .sa_restorer = sigreturn,
    };
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    int cpid = fork();
    if (cpid == 0) {
        while (!test_done) {
            sleep(1);
        }
        exit(0);
    } else {
        sleep(1);

        sigkill(cpid, SIGUSR1, 0);
        
        printf("Testing kernel-sent SIGALRM...\n");
        alarm(2);
        
        while (!test_done) {
            sleep(1);
        }
        
        wait(cpid, NULL);
    }
    return 0;
}