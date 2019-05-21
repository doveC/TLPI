#include <stdio.h>
#include <unistd.h>
#include <signal.h>

void handler(int sig) {
    printf("I received signal [%d]\n", sig);
}

int main()
{
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGSEGV);
    /* sigprocmask(SIG_SETMASK, &set, NULL); */
    signal(11, handler);

    for(int i = 0; i < 10; i++) {
        printf("%d\n", i);
        /* sigprocmask(SIG_UNBLOCK, &set, NULL); */
        pause();
    }
    return 0;
}

