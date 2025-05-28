#include "../lib/user.h"
#include "../../os/signal/signal.h"

void sigchld_handler(int signo, siginfo_t *info, void *ctx) {
    printf("Received SIGCHLD: child PID=%d, exit code=%d\n", info->si_pid, info->si_code);
    
    int status;
    int pid;

    // 循环调用 wait(-1, &status) 回收所有终止的子进程
    while ((pid = wait(-1, &status)) > 0) {
        if (pid < 0) break; // 无更多子进程
        printf("Reaped child PID=%d, exit code=%d\n", pid, status);
    }

    sigreturn();
}

int main() {
    // 注册 SIGCHLD 处理函数
    struct sigaction sa = {
        .sa_sigaction = sigchld_handler,
        .sa_restorer = sigreturn,
    };
    sigaction(SIGCHLD, &sa, NULL);

    // 创建子进程
    int pid = fork();
    if (pid == 0) {
        // 子进程执行任务后退出
        printf("Child running...\n");
        sleep(1);
        exit(42);
    } else {
        // 父进程等待信号
        while (1) sleep(1);
    }
    return 0;
}
