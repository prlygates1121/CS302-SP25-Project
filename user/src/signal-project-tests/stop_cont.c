#include "../../os/ktest/ktest.h"
#include "../lib/user.h"

// Checkpoint 2: SIGSTOP + SIGCONT

void sigstop_sigcont(char* s) {
    int pid = fork();
    if (pid == 0) {
        for (int i = 0; i < 5; i++) {
            printf("child running %d\n", i);
            sleep(20); 
        }
        exit(0); 
    } else {
        sleep(25); 
        sigkill(pid, SIGSTOP, 0); 
        int stopped_ticks = uptime();
        sleep(60); 

        sigkill(pid, SIGCONT, 0); 
        sleep(100); 

        int ret;
        wait(0, &ret);

        assert(ret == 0);
        assert(uptime() - stopped_ticks >= 60); 
    }
}
