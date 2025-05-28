#include "../lib/user.h"
#include "../../os/signal/signal.h"

volatile int triggered = 0;
volatile int old_remaining = 0;

void handler(int signo, siginfo_t *info, void *ctx) {
    printf("Alarm triggered!\n");
    triggered = 1;
    sigreturn();
}

int main() {
    struct sigaction sa = {
        .sa_sigaction = handler,
        .sa_restorer = sigreturn,
    };
    if (sigaction(SIGALRM, &sa, NULL) < 0) {
        printf("sigaction failed\n");
        exit(-1);
    }

    old_remaining = alarm(5);
    assert_eq(old_remaining, 0);
    printf("Test 1 passed: alarm(5) returns %d (expected 0)\n", old_remaining);

    sleep(1);
    old_remaining = alarm(3);
    assert(old_remaining >= 4 && old_remaining <= 5);
    printf("Test 2 passed: alarm(3) returns %d (expected ~4-5)\n", old_remaining);

    while (!triggered) {
      sleep(1);
      printf("Waiting for alarm...\n");
    }
    triggered = 0;

    old_remaining = alarm(5);
    assert_eq(old_remaining, 0);
    sleep(1);
    old_remaining = alarm(0);
    assert(old_remaining >= 4 && old_remaining <= 5);
    printf("Test 3 passed: alarm(0) returns %d (expected ~4-5)\n", old_remaining);

    sleep(5);
    assert(triggered == 0);
    printf("Test 3 passed: SIGALRM not triggered after cancel\n");

    printf("All tests passed!\n");
    return 0;
}