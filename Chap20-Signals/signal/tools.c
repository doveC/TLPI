#include <stdio.h>
#include <string.h>
#include <signal.h>

void printSigset(FILE* fp, const char* prefix, const sigset_t* sigset) {
    int sig, cnt;

    cnt = 0;
    for(sig = 1; sig < NSIG; sig++) {
        if(sigismember(sigset, sig)) {
            cnt++;
            fprintf(fp, "%s%d (%s\n)", prefix, sig, strsignal(sig));
        }
    }

    if(cnt == 0) {
        fprintf(fp, "%s<empty signal set>\n", prefix);
    }
}

int printSigMask(FILE* fp, const char* msg) {
    sigset_t currMask;

    if(msg != NULL) {
        fprintf(fp, "%s", msg);
    }

    if(sigprocmask(SIG_BLOCK, NULL, &currMask) == -1) {
        return -1;
    }

    printSigset(fp, "\t\t", &currMask);

    return 0;
}

int printPendingSigs(FILE* fp, const char* msg) {
    sigset_t pendingSigs;

    if(msg != NULL) {
        fprintf(fp, "%s", msg);
    }

    if(sigpending(&pendingSigs) == -1) {
        return -1;
    }

    printSigset(fp, "\t\t", &pendingSigs);

    return 0;
}

int main()
{
    return 0;
}

