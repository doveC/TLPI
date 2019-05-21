// 
// typedef void (*sighandler_t)(int);
// sighandler_t signal(int signum, sighandler_t handler);
//      signum      信号编号
//      handler     函数指针（回调函数）
//          SIG_DFL     信号的默认处理方式
//          SIG_IGN     忽略信号
//      使用handler函数替换signum信号的默认处理方式
//
// int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
// 使用act动作替换signum原有的处理动作，并且将原有的处理动作拷贝到oldact中 
//
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

void sigcb(int signo) {
    printf("\nrecv signo:%d\n", signo);
}

int main() {
    // signal(SIGINT, SIG_IGN);  // 忽略ctrl v信号
    signal(SIGINT, sigcb);  // 自定义2号信号处理方式
    signal(SIGQUIT, sigcb);  // 自定义3号信号处理方式
    signal(SIGKILL, sigcb);  // 自定义9号信号处理方式
    while(1) {
        printf(" ------ \n");
        sleep(1);
    }

    return 0;
}
