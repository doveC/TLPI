#include <stdio.h>
#include <string.h>
#include <signal.h>

extern const char* const sys_siglist[];

int main()
{
    printf("SIGINT: [%s]\n", sys_siglist[SIGINT]);
    printf("SIGTSTP: [%s]\n", strsignal(SIGTSTP));
    return 0;
}

