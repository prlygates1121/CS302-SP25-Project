#include "ksignal.h"

#include <defs.h>
#include <proc.h>
#include <trap.h>

/**
 * @brief init the signal struct inside a PCB.
 * 
 * @param p 
 * @return int 
 */
int siginit(struct proc *p) {
    assert(holding(&p->lock));
    // init p->signal
    memset(&p->signal, 0, sizeof(struct ksignal));
    return 0;
}

int siginit_fork(struct proc *parent, struct proc *child) {
    assert(holding(&parent->lock));
    // copy parent's sigactions and signal mask
    // but clear all pending signals
    for (int signo = SIGMIN; signo <= SIGMAX; signo++) {
        child->signal.sa[signo] = parent->signal.sa[signo];
    }
    child->signal.sigmask = parent->signal.sigmask;
    child->signal.sigpending = 0;
    return 0;
}

int siginit_exec(struct proc *p) {
    assert(holding(&p->lock));
    // inherit signal mask and pending signals.
    // but reset all sigactions (except ignored) to default.
    for (int signo = SIGMIN; signo <= SIGMAX; signo++) {
        if (p->signal.sa[signo].sa_sigaction != SIG_IGN) {
            p->signal.sa[signo].sa_sigaction = SIG_DFL;
        }
    }
    p->signal.sigmask = curr_proc()->signal.sigmask;
    p->signal.sigpending = curr_proc()->signal.sigpending;
    return 0;
}

int do_signal(void) {
    assert(!intr_get());
    struct proc* p = curr_proc();
    acquire(&p->lock);
    for (int signo = SIGMIN; signo <= SIGMAX; signo++) {
        // from SIGMIN to SIGMAX, check if each signal is pending
        if (((p->signal.sigpending) & (1 << signo)) != 0) {
            // printf("do_signal: signal %d is pending!\n", signo);
            // for SIGKILL and SIGSTOP, we handle them directly without checking the mask/action
            if (signo == SIGKILL) {
                release(&p->lock);
                setkilled(p, -10 - signo);
                return 0;
            } else if (signo == SIGSTOP) {
                
            } else {
                // check if the signal is masked
                if ((p->signal.sigmask & (1 << signo)) == 0) {
                    // printf("do_signal: signal %d is not masked!\n", signo);
                    // clear pending bit of this signal
                    sigdelset(&p->signal.sigpending, signo);
                    sigaction_t *sa = &(p->signal.sa[signo]);
                    if (sa->sa_sigaction == SIG_DFL) {
                        // printf("do_signal: signal %d is default!\n", signo);
                        // if action is set to default
                        switch (signo) {
                            case (SIGUSR0):
                            case (SIGUSR1):
                            case (SIGUSR2):
                            case (SIGTERM):
                            case (SIGSEGV):
                            case (SIGINT) :
                                release(&p->lock);
                                setkilled(p, -10 - signo);
                                return 0;
                            case (SIGCONT):
                                p->stopped = 0;
                                wakeup(&p->stopped);
                                return 0;
                            case (SIGALRM):
                                break;
                            case (SIGCHLD):
                            default:
                                break;
                        }
                    } else if (sa->sa_sigaction == SIG_IGN) {
                        // if action is set to ignore
                        // printf("do_signal: signal %d is ignored!\n", signo);
                        p->stopped = 1;   
                        while (p->stopped) {
                            sleep(&p->stopped, &p->lock); 
                        }
                    } else {
                        // if a handler is specified
                        // printf("do_signal: signal %d has a handler!\n", signo);
                        struct trapframe *tf = p->trapframe;

                        // ucontext created in kernel stack (temporarily)
                        struct ucontext k_ucontext;
                        memset(&k_ucontext, 0, sizeof(k_ucontext));
                        // store original user sigmask
                        k_ucontext.uc_sigmask = p->signal.sigmask;
                        // save original user epc
                        k_ucontext.uc_mcontext.epc = tf->epc;
                        // save registers
                        k_ucontext.uc_mcontext.regs[0]  = tf->ra;
                        k_ucontext.uc_mcontext.regs[1]  = tf->sp;
                        k_ucontext.uc_mcontext.regs[2]  = tf->gp;
                        k_ucontext.uc_mcontext.regs[3]  = tf->tp;
                        k_ucontext.uc_mcontext.regs[4]  = tf->t0;
                        k_ucontext.uc_mcontext.regs[5]  = tf->t1;
                        k_ucontext.uc_mcontext.regs[6]  = tf->t2;
                        k_ucontext.uc_mcontext.regs[7]  = tf->s0;
                        k_ucontext.uc_mcontext.regs[8]  = tf->s1;
                        k_ucontext.uc_mcontext.regs[9]  = tf->a0;
                        k_ucontext.uc_mcontext.regs[10] = tf->a1;
                        k_ucontext.uc_mcontext.regs[11] = tf->a2;
                        k_ucontext.uc_mcontext.regs[12] = tf->a3;
                        k_ucontext.uc_mcontext.regs[13] = tf->a4;
                        k_ucontext.uc_mcontext.regs[14] = tf->a5;
                        k_ucontext.uc_mcontext.regs[15] = tf->a6;
                        k_ucontext.uc_mcontext.regs[16] = tf->a7;
                        k_ucontext.uc_mcontext.regs[17] = tf->s2;
                        k_ucontext.uc_mcontext.regs[18] = tf->s3;
                        k_ucontext.uc_mcontext.regs[19] = tf->s4;
                        k_ucontext.uc_mcontext.regs[20] = tf->s5;
                        k_ucontext.uc_mcontext.regs[21] = tf->s6;
                        k_ucontext.uc_mcontext.regs[22] = tf->s7;
                        k_ucontext.uc_mcontext.regs[23] = tf->s8;
                        k_ucontext.uc_mcontext.regs[24] = tf->s9;
                        k_ucontext.uc_mcontext.regs[25] = tf->s10;
                        k_ucontext.uc_mcontext.regs[26] = tf->s11;
                        k_ucontext.uc_mcontext.regs[27] = tf->t3;
                        k_ucontext.uc_mcontext.regs[28] = tf->t4;
                        k_ucontext.uc_mcontext.regs[29] = tf->t5;
                        k_ucontext.uc_mcontext.regs[30] = tf->t6;
                        // save previous ucontext addr
                        k_ucontext.last_ucontext = p->signal.ucontext_user_addr;

                        siginfo_t k_siginfo;
                        memset(&k_siginfo, 0, sizeof(k_siginfo));
                        k_siginfo.si_signo = signo;
                        k_siginfo.si_pid = p->signal.siginfos[signo].si_pid;
                        k_siginfo.si_code = p->signal.siginfos[signo].si_code;

                        uint64 user_sp = tf->sp;
                        uint64 ucontext_user_addr, siginfo_user_addr;

                        // reserve space on stack for ucontext
                        user_sp -= sizeof(k_ucontext);
                        // align sp to 64 bits
                        user_sp &= ~((uint64)15);
                        
                        ucontext_user_addr = user_sp;
                        p->signal.ucontext_user_addr = ucontext_user_addr;

                        // reserve space on stack for siginfo
                        user_sp -= sizeof(siginfo_t);
                        // align sp to 64 bits
                        user_sp &= ~((uint64)15);

                        siginfo_user_addr = user_sp;

                        acquire(&p->mm->lock);
                        if (copy_to_user(p->mm, ucontext_user_addr, (char*)(&k_ucontext), sizeof(k_ucontext)) < 0 ||
                            copy_to_user(p->mm, siginfo_user_addr, (char*)(&k_siginfo), sizeof(k_siginfo)) < 0) {
                            release(&p->mm->lock);
                            release(&p->lock);
                            printf("copyout to user stack failed for signal %d\n", signo);
                            setkilled(p, -1); 
                            break;
                        }
                        release(&p->mm->lock);

                        // add sa->mask and current signal to sigmask
                        p->signal.sigmask |= sa->sa_mask;
                        sigaddset(&p->signal.sigmask, signo);

                        // let sret go to handler
                        tf->epc = (uint64)sa->sa_sigaction;

                        // update sp
                        tf->sp = user_sp;

                        // parameters passed to handler
                        tf->a0 = signo;
                        tf->a1 = siginfo_user_addr;
                        tf->a2 = ucontext_user_addr;

                        // let handler return to sigreturn
                        tf->ra = (uint64)sa->sa_restorer;
                        
                        release(&p->lock);

                        // do_signal --> usertrapret --> userret --> sret --> handler --> sigreturn --> user code
                        return 0;
                    }
                }
            }
        }
    }
    release(&p->lock);
    return 0;
}

// syscall handlers:
//  sys_* functions are called by syscall.c

int sys_sigaction(int signo, const sigaction_t __user *act, sigaction_t __user *oldact) {
    struct proc* p = curr_proc();
    sigaction_t k_sigaction;

    if (oldact != NULL) {
        acquire(&p->lock);
        k_sigaction = p->signal.sa[signo];
        acquire(&p->mm->lock);
        if (copy_to_user(p->mm, (uint64)oldact, (char*)(&k_sigaction), sizeof(k_sigaction)) < 0) {
            release(&p->mm->lock);
            release(&p->lock);
            printf("sys_sigaction: copy to user *oldact failed.\n");
            return -1;
        }
        release(&p->mm->lock);
        release(&p->lock);
    }

    if (act != NULL) {
        acquire(&p->lock);
        k_sigaction = p->signal.sa[signo];
        acquire(&p->mm->lock);
        if (copy_from_user(p->mm, (char*)(&k_sigaction), (uint64)act, sizeof(k_sigaction)) < 0) {
            release(&p->mm->lock);
            release(&p->lock);
            printf("sys_sigaction: copy from user *act failed.\n");
            return -1;
        }
        release(&p->mm->lock);
        // signal action for SIGKILL and SIGSTOP cannot be modified
        if (signo != SIGKILL && signo != SIGSTOP && act != NULL) {
            p->signal.sa[signo] = k_sigaction;
        }
        release(&p->lock);
    }

    return 0;
}

int sys_sigreturn() {
    // printf("sys_sigreturn: begin.\n");
    struct proc* p = curr_proc();
    acquire(&p->lock);
    struct trapframe* tf = p->trapframe;

    uint64 ucontext_user_addr = p->signal.ucontext_user_addr;

    if (ucontext_user_addr == 0) {
        printf("sys_sigreturn: ucontext_user_addr is zero\n");
        release(&p->lock);
        setkilled(p, -1);
        return -1;
    }

    struct ucontext k_ucontext;
    acquire(&p->mm->lock);
    if (copy_from_user(p->mm, (char*)(&k_ucontext), ucontext_user_addr, sizeof(k_ucontext)) < 0) {
        release(&p->mm->lock);
        release(&p->lock);
        printf("sys_sigreturn: copy from user stack failed.\n");
        setkilled(p, -1);
        return -1;
    }
    release(&p->mm->lock);

    p->signal.ucontext_user_addr = k_ucontext.last_ucontext;

    // restore trapframe
    tf->epc = k_ucontext.uc_mcontext.epc;
    tf->ra  = k_ucontext.uc_mcontext.regs[0];
    tf->sp  = k_ucontext.uc_mcontext.regs[1];
    tf->gp  = k_ucontext.uc_mcontext.regs[2];
    tf->tp  = k_ucontext.uc_mcontext.regs[3];
    tf->t0  = k_ucontext.uc_mcontext.regs[4];
    tf->t1  = k_ucontext.uc_mcontext.regs[5];
    tf->t2  = k_ucontext.uc_mcontext.regs[6];
    tf->s0  = k_ucontext.uc_mcontext.regs[7];
    tf->s1  = k_ucontext.uc_mcontext.regs[8];
    tf->a0  = k_ucontext.uc_mcontext.regs[9];
    tf->a1  = k_ucontext.uc_mcontext.regs[10];
    tf->a2  = k_ucontext.uc_mcontext.regs[11];
    tf->a3  = k_ucontext.uc_mcontext.regs[12];
    tf->a4  = k_ucontext.uc_mcontext.regs[13];
    tf->a5  = k_ucontext.uc_mcontext.regs[14];
    tf->a6  = k_ucontext.uc_mcontext.regs[15];
    tf->a7  = k_ucontext.uc_mcontext.regs[16];
    tf->s2  = k_ucontext.uc_mcontext.regs[17];
    tf->s3  = k_ucontext.uc_mcontext.regs[18];
    tf->s4  = k_ucontext.uc_mcontext.regs[19];
    tf->s5  = k_ucontext.uc_mcontext.regs[20];
    tf->s6  = k_ucontext.uc_mcontext.regs[21];
    tf->s7  = k_ucontext.uc_mcontext.regs[22];
    tf->s8  = k_ucontext.uc_mcontext.regs[23];
    tf->s9  = k_ucontext.uc_mcontext.regs[24];
    tf->s10 = k_ucontext.uc_mcontext.regs[25];
    tf->s11 = k_ucontext.uc_mcontext.regs[26];
    tf->t3  = k_ucontext.uc_mcontext.regs[27];
    tf->t4  = k_ucontext.uc_mcontext.regs[28];
    tf->t5  = k_ucontext.uc_mcontext.regs[29];
    tf->t6  = k_ucontext.uc_mcontext.regs[30];

    // printf("sys_sigreturn: return.\n");
    // restore sigmask
    p->signal.sigmask = k_ucontext.uc_sigmask;
    
    release(&p->lock);

    return 0;
}

int sys_sigprocmask(int how, const sigset_t __user *set, sigset_t __user *oldset) {
    struct proc* p = curr_proc();
    sigset_t k_set;

    if (oldset != NULL) {
        acquire(&p->lock);
        k_set = p->signal.sigmask;

        acquire(&p->mm->lock);
        if (copy_to_user(p->mm, (uint64)oldset, (char*)(&k_set), sizeof(k_set)) < 0) {
            release(&p->mm->lock);
            release(&p->lock);
            printf("sys_sigaction: copy to user *oldact failed.\n");
            return -1;
        }
        release(&p->mm->lock);
        release(&p->lock);
    }

    if (set != NULL) {
        acquire(&p->lock);
        acquire(&p->mm->lock);
        if (copy_from_user(p->mm, (char*)(&k_set), (uint64)set, sizeof(sigset_t)) < 0) {
            release(&p->mm->lock);
            printf("sys_sigprocmask: copy from user failed.\n");
            setkilled(p, -1);
            return -1;
        }
        release(&p->mm->lock);
        // actual_set = set excluding SIGKILL and SIGSTOP
        // this ignores attempts to mask SIGKILL or SIGSTOP
        sigset_t actual_set = k_set & ~sigmask(SIGKILL) & ~sigmask(SIGSTOP);
        switch (how) {
            case (SIG_BLOCK):
                p->signal.sigmask |= actual_set;
                break;
            case (SIG_UNBLOCK):
                p->signal.sigmask &= ~(actual_set);
                break;
            case (SIG_SETMASK):
                p->signal.sigmask = actual_set;
                break;
            default:
                release(&p->lock);
                return -1;
        }
        release(&p->lock);
    }
    return 0;
}

int sys_sigpending(sigset_t __user *set) {
    if (set != NULL) {
        struct proc* p = curr_proc();
        acquire(&p->lock);
        sigset_t k_set = p->signal.sigpending;

        acquire(&p->mm->lock);
        if (copy_to_user(p->mm, (uint64)set, (char*)(&k_set), sizeof(sigset_t)) < 0) {
            release(&p->mm->lock);
            printf("sys_sigprocmask: copy from user failed.\n");
            setkilled(p, -1);
            return -1;
        }
        release(&p->mm->lock);
        release(&p->lock);
    }
    return 0;
}

// int sys_sigkill(int pid, int signo, int code, int from_kernel) {
//     struct proc *sender = curr_proc();
//     struct proc *target_p = NULL;
//     int found = 0;

//     for (int i = 0; i < NPROC; i++) {
//         target_p = pool[i];
//         // acquire(&target_p->lock);
//         if (pool[i]->pid == pid) {
//             printf("sys_sigkill: send signal %d to process %d.\n", signo, pid);
//             acquire(&target_p->lock);

//             if (from_kernel) {
//                 target_p->signal.siginfos[signo].si_pid = -1;
//             } else {
//                 target_p->signal.siginfos[signo].si_pid = sender->pid;
//             }

//             sigaddset(&target_p->signal.sigpending, signo);
//             target_p->signal.siginfos[signo].si_signo = signo;
//             target_p->signal.siginfos[signo].si_code = code;
//             release(&target_p->lock);
//             return 0;
//         }
//         // release(&target_p->lock);
//     }

//     printf("sys_sigkill: process with pid %d not found.\n");
//     return -1;
// }

int sys_sigkill(int pid, int signo, int code, int from_kernel) {
    struct proc *sender = curr_proc();
    struct proc *target_p = NULL;
    int found = 0;

    for (int i = 0; i < NPROC; i++) {
        target_p = pool[i];
        
        if (target_p->state == UNUSED) 
            continue;
        
        if (target_p->pid == pid) {
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("sys_sigkill: process with pid %d not found.\n", pid);
        return -1;
    }

    printf("sys_sigkill: send signal %d to process %d.\n", signo, pid);
    acquire(&target_p->lock);

    target_p->signal.siginfos[signo].si_signo = signo;
    target_p->signal.siginfos[signo].si_code = code;
    
    if (from_kernel) {
        target_p->signal.siginfos[signo].si_pid = -1;
    } else {
        target_p->signal.siginfos[signo].si_pid = sender->pid;
    }

    sigaddset(&target_p->signal.sigpending, signo);
    
    if (signo == SIGKILL) {
        setkilled(target_p, -10 - SIGKILL);
    }
    
    release(&target_p->lock);
    return 0;
}
