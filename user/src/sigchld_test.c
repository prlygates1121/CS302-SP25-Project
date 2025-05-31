#include "../lib/user.h"
#include "../../os/signal/signal.h"

// 记录已回收的子进程数量
static int reaped_count = 0;
static const int total_children = 3; // 创建3个子进程

void sigchld_handler(int signo, siginfo_t *info, void *ctx) {
    printf("=== SIGCHLD Handler Called ===");
    printf("Received SIGCHLD (signo=%d) from Child PID=%d", 
           signo, info->si_pid);
    printf("Exit Status (si_code)=%d\n", info->si_code);

    int status;
    while (1) {
        // 使用waitpid非阻塞回收所有子进程（pid=-1表示任意子进程，WNOHANG=非阻塞）
        int pid = wait(-1, &status);
        if (pid <= 0) break; // 无更多子进程或错误

        reaped_count++;
        printf("Successfully reaped Child PID=%d", pid);
        printf("Exit Status (wait)=%d\n", status);
        printf("Total reaped: %d/%d\n", reaped_count, total_children);
    }

    // 所有子进程回收后，父进程退出
    if (reaped_count == total_children) {
        printf("All children reaped. Parent exiting.\n");
        exit(0);
    }
}

int main() {
    // 1. 注册SIGCHLD信号处理函数
    sigaction_t sa = {
        .sa_sigaction = sigchld_handler,
        .sa_restorer = sigreturn, // 必须指向sigreturn系统调用
        .sa_mask = 0 // 处理信号时不额外阻塞其他信号
    };
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        printf("Error: Failed to register SIGCHLD handler\n");
        exit(1);
    }

    // 2. 创建3个子进程，每个子进程以不同状态退出
    for (int i = 0; i < total_children; i++) {
        int pid = fork();
        if (pid < 0) {
            printf("Error: fork failed\n");
            exit(1);
        } else if (pid == 0) {
            // 子进程逻辑：延时退出，携带不同退出状态（10+i）
            sleep((i + 1) * 2); // 避免同时退出，便于观察顺序
            exit(10 + i); // 退出状态将通过siginfo_t.si_code传递
        }
    }

    // 3. 父进程主循环：等待子进程退出或主动退出
    while (1) {
        sleep(1);
        printf("Parent is running... %d children reaped\n", reaped_count);
        if (reaped_count == total_children) {
            break; // 所有子进程回收后退出循环
        }
    }

    printf("Parent finished successfully.\n");
    return 0;
}