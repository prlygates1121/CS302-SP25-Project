#include "../../os/ktest/ktest.h"
#include "../lib/user.h"

// Checkpoint 2: SIGSTOP + SIGCONT

void sigstop_sigcont(char *s) {
    int pid = getpid();  // 获取当前进程 pid
    int child = fork();
    if (child == 0) {
        // 子进程：用于控制 SIGSTOP 和 SIGCONT
        sleep(30); // 等主进程跑一会
        printf("child: sending SIGSTOP to parent\n");
        sigkill(pid, SIGSTOP, 0); // 暂停主进程

        sleep(50); // 让主进程暂停一段时间
        printf("child: sending SIGCONT to parent\n");
        sigkill(pid, SIGCONT, 0); // 恢复主进程

        exit(0);
    } else {
        // 父进程：主逻辑，执行期间被暂停和恢复
        for (int i = 0; i < 10; i++) {
            printf("main loop %d\n", i);
            sleep(20);
        }
        int status;
        wait(0, &status);
        assert(status == 0);
    }
}
