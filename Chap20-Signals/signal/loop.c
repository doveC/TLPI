#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

int main() {
    // int kill(pid_t pid, int sig)
    // 给指定进程发送指定信号
    // kill(getpid(), SIGSEGV);
    
    // int raise(int sig);
    // 给调用进程/线程发送指定信号

    // void abort(void);
    // 给调用进程发送SIGABRT信号
    // abort();

    // unsigned int alarm(unsigned int seconds);
    // seconds秒后给调用进程发送SIGALRM信号
    // seconds为0则表示取消上一个定时器
    // 返回上一个定时器剩余的时间或0
    // alarm(3);

    while(1) {
        printf("Hell\n");
        sleep(1);
    }
    return 0;
}



