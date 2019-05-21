# 概念和概述

**信号是事件发生时对进程的通知机制**。有时也被称为软件中断。信号与硬件中断的相似在于打断了程序的正常执行流程，大多数情况下，无法预测信号到达的精确时间。

一个（具有合适权限的）的进程能够向另一进程发送信号。信号的这一用法可作为一种同步技术，甚至是进程间通信（IPC）的原始形式。进程也可以向自身发送信号。然而发往进程的信号多数来自于内核。下面几种情况会引发内核向进程发送信号：

- 硬件发生异常：即硬件检测到一个错误并通知内核，随后再由内核发送相应的信号给相关进程。硬件异常的例子包括执行一条异常的机器语言指令，例如除0，或者引用了无法访问的内存。
- 键盘输入一些能产生信号的特殊字符（Ctrl + C）（Ctrl + Z）
- 软事件如alarm函数
- 系统调用发送信号（kill、raise）

针对每个信号，都定义了一个唯一的小整数，从1开始顺序展开。在<signal.h> 中用 SIGxxxx 形式的宏对这些小整数做了定义。

信号分为两大类。一类用于内核向进程通知特定的事件，构成所谓的**标准信号**集。Linux中这些信号的编号为 1~31。而另一组信号被称为**实时信号**。是后续添加的信号，没有对应特定的事件。

信号因某些事而产生（generated）。信号产生后，会于稍后递达（delivered）到某一进程，而进程也会采取某些动作来响应信号。在产生和递达之间的这段时间，信号处于等待（pending）状态。

通常，当内核接下来要调度这个程序运行，处于等待的信号将会立刻被送达。或者当进程在运行的时候收到信号，信号会立刻被递达（例如，进程向自身发送信号）。然而，有时需要确保一段代码不被一个信号的递达所打断，为了做到这点，可以将信号添加到进程的信号掩码（signal mask）中（一组递达会被阻塞（block）的信号），**一旦进程收到了一个处于信号掩码中的信号的时候，该信号会一直处于等待（pending）状态，直到稍后将该信号从信号掩码中移除**。

信号到达后，进程视具体的信号执行如下默认操作之一。

- 忽略信号：即内核将该信号丢弃，信号对进程没有任何影响。
- 中止（杀死）进程：这有时指的是进程异常终止，与之对应的是调用 exit() 或者return 而发生的正常终止。
- 产生核心转储文件，并且杀死进程：核心转储文件记录进程虚拟内存的映像，可将其加载到调试器中以检查进程终止时的状态。
- 停止进程：暂停进程的执行。
- 恢复进程：将之前暂停的进程恢复。

进程对每个信号都有一个默认行为，即上面几种中的一个。而程序也能改变信号递达时候的响应行为。可以将响应行为设置成如下之一：

- 采取默认行为。用于撤销之前对信号递达的时候的响应行为的设置。
- 忽略信号
- 执行用户自定义的信号处理器程序

信号处理器程序是由程序员编写的程序，用于响应传递来的信号而执行相应的动作。例如，shell为SIGINT信号（Ctrl-C）提供了一个处理器程序，令其停止当前正在执行的工作，并将控制权交还给shell主程序。

注意，无法将信号的响应设置为终止进程或者核心转储，除非这是该信号的默认行为。效果最为接近的是自定义一个处理器程序，在内部调用exit() 或者abort() 。abort() 函数为当前进程产生一个SIGABRT信号，该信号将导致进程产生核心转储文件并终止。

# 改变信号处置：signal()

unix提供了两种方法来改变信号的处置：signal() 和sigaction()。signal() 比sigaction() 用起来简单，但同样也没有后者提供的功能多。signal() 的行为在不同UNIX实现间存在差异，这也意味着需要考虑可移植性的程序一定不能用signal() 。所以sigaction() 是我们的首选API。

```c
void ( *signal(int sig, void (*handler)(int)))(int);
// ----------------- or -------------------------
typedef void( *sighandler_t)(int);
sighandler_t signal(int sig, sighandler_t handler);
```

参数sig为希望修改处置方式的信号的编号，而参数handler为信号抵达时所调用函数的函数指针。

signal() 的返回值是这个函数之前的响应函数。

> 使用signal()，无法在不改变信号处置函数的同时获取到这个信号处置函数。要做到这一点只能使用sigaction()。

在为signal() 指定handler参数时，可以使用如下值来代替函数地址：

- SIG_DFL：将信号处置函数设置为这个信号的默认处置函数
- SIG_IGN：忽略该信号

signal() 也会返回上述两个常量

```c
// define a function pointer
void (*oldHandler)(int);
// set a new handler function for SIGINT
oldHandler = signal(SIGINT， newHandler);
// recover
signal(SIGINT, oldHandler);
```

# 信号处理器

信号处理器程序即进程收到指定信号后会调用的函数。

由于进程可能在任一时刻收到信号，所以信号处理器可能随时会打断主程序的流程。之后内核代表进程来调用处理器程序，当处理器返回后，主程序继续在被打断的地方继续执行。

![1558420381295](C:\Users\a1599\AppData\Roaming\Typora\typora-user-images\1558420381295.png)

```c
void sigHandler(int sig) {
    printf("wow!\n");
}

int main() {
    int j;
    signal(SIGINT, sigHandler);
    
    for(j = 0; j < 3; j++) {
        printf("%d\n", j);
        sleep(3);  // Loop slowly
    }
}
```

上述程序为SIGINT设置了信号处理函数，用户输入ctrl + c时，处理器只是简单打印一条信息，随即返回。

内核在调用处理器函数时会向函数的sig参数传入引发调用的信号的编号，这看似没什么用，但其实是用在对多个信号设置同一个处理函数时，使得程序内部可以知道是哪个信号触发的信号处理器调用。

# 发送信号：kill()

与 shell 的 kill 命令类似，一个进程能够使用 kill() 系统调用向另一个进程发送信号。（之所以取名 kill，是因为早期UNIX的大部分信号的默认处理方式都是终止进程）

```c
int kill(pid_t pid, int sig);
// returns 0 on success, or -1 on error
```

pid 参数标识一个或多个进程。对，可以标识多个进程，如果 pid 大于0则只标识 pid 一个进程，而小于等于0的数可以实现更复杂的标识逻辑，这里不赘述。

如果没有进程与指定的 pid 匹配，则调用失败，errno 设置为 ESRCH。

进程发送信号给另一进程，还需要适当的权限，不赘述。

# 检查进程的存在

可以将 kill() 的参数 sig 设置为0，即空信号，则无信号发送。而 kill() 只会进行错误检查，可以根据检查的结果直到指定进程是否存在。如果调用失败（从返回值看出），且 errno 为 ESRCH，表示目标进程不存在。如果调用失败，且 errno 为 EPERM（表示进程存在，但无权发送信号）或者调用成功都表示指定进程存在。

但这种检查进程存在的方法并不可靠，因为PID是可以被循环使用的。而且这种检查不能保证进程的状态是怎样的（例如僵尸进程）。

还可以用其他技术来检查某一特定进程是否在运行中，其中包括如下技术：

- wait() 系统调用：可以监控子进程是否终止。
- 信号量和排他文件锁：如果进程持续持有某一信号量或文件锁，并且一直处于被监控状态，那么如果能获取到信号量或锁的时候，即表明该进程已经终止。
- 诸如管道和FIFO之类的 IPC 通道：可以让被监控程序在其生命周期内一直持有一个管道的写文件描述符。然后让监控程序持有同一个管道的读文件描述符，如果被监控程序退出了，则监控程序可以知道（？？？？？？？）
- /proc/PID：检查对应PID的目录是否存在。

除了最后一个方法，其他都不会受到循环使用PID的影响。

# 发送信号的其他方式：raise() 和killpg()

raise() 用来向进程自身发送信号：

```c
int raise(int sig);
```

单线程程序中，调用raise() 相当于对kill() 的如下调用：

```c
kill(getpid(), sig);
```

支持线程的系统会将 raise(sig) 实现为：

```c
pthread_kill(pthread_self(), sig);
```

目前仅需了解可以将信号传递给调用 raise() 的特定线程。

当进程使用 raise() （或者 kill()）向自身发送信号时，信号将立刻被递达（即在 raise() 返回之前）。

killpg() 系统调用向进程组的所有进程发送信号。

# 显示信号描述

每个信号都有一串与之相关的字符串说明。这些描述位于 <signal.h> 中的全局变量数组 sys_siglist 中，也可以通过 strsignal() 函数获取这串说明。

```c
extern const char* const sys_siglist[];
char* strsignal(int sig);
```

# 信号集（signals set）

许多信号相关的系统调用都需要设法表示一组信号，而不是 SIGINT SIGTSTP 这样一个一个列举。例如 sigaction() 和 sigprocmask() 允许程序指定一组将被进程阻塞的信号（即改变进程的信号掩码），而 sigpending() 则返回一组目前正在等待送达给进程的信号。

多个信号可使用一个称之为信号集的数据结构来表示，其在系统中的数据类型为 sigset_t。下面的定义来自头文件 <sigset.h>：

```c
# define _SIGSET_NWORDS	(1024 / (8 * sizeof (unsigned long int)))
typedef struct
  {
    unsigned long int __val[_SIGSET_NWORDS];
  } __sigset_t;
```

可以看出，__val[ ] 数组总共在内存中所占的位数为1024位（为毛不直接定义成1024啊！！！）

SUSv3定义了一系列函数来操纵信号集，下面描述这些函数。

> 和大多数UNIX实现一样，sigset_t 数据类型在Liunx中也是一个位图 / 位掩码（bit mask）。但SUSv3对此并无要求。使用其他数据结构来表示信号集也是有可能的。

## sigemptyset() / sigfillset()

sigemptyset() 函数将一个信号集初始化成一个未包含任何信号的信号集。sigfullset() 函数则将一个信号集初始化成一个包含所有信号的信号集（也包含所有实时信号）。

```c
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
// both return 0 on success, or -1 on error
```

必须使用上述两个函数来初始化一个信号集。这是因为C语言不会对自动变量进行初始化（存储在栈上），并且，将信号集显示的初始化为全0的做法在可移植性上有问题，虽然这在使用位图的系统上可能没什么关系，但不要忽视还有其他的系统的存在。

## sigaddset() / sigdelset()

信号集初始化后，可以分别使用 sigaddset() 和 sigdelset() 函数向这个信号集中添加或者移除一个信号。

```c
int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
// both return 0 on success, or -1 on error
```

## sigismember()

sigismember() 函数用来测试信号 sig 是否是信号集 set 的成员。

```c
int sigismember(const sigset_t *set, int sig);
// returns 1 if sig is a member of set, otherwise 0
```

## sigandset() / sigorset() / sigisemptyset()

GNU C库还实现了3个非标准函数，是对上述信号集标准函数的补充。

```c
int sigandset(sigset_t *dest, sigset_t *left, sigset_t *right);
int sigorset(sigset_t *dest, sigset_t *left, sigset_t *right);
// both return 0 on success, or -1 on error
int sigisemptyset(const sigset_t *set);
// returns 1 if sig is empty, otherwise 0
```

这些函数执行如下任务。

- sigandset() 将 left 集和 right 集的交集置于 dest 集
- sigorset() 将 left 集和 right 集的并集置于 dest 集
- sigisemptyset() 检查 set 集是否未包含信号

## 示例程序

下面有几个工具函数供后续使用：

```c
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
```

# 信号掩码（阻塞信号传递）

内核会为每个进程维护一个信号掩码（signal mask），即一组信号（一个信号集），并将阻塞该组信号对该进程的递达（保持pending状态），直至从进程信号掩码中移除该信号，从而解除阻塞。（信号掩码实际属于线程属性，在多线程进程中，每个线程都可以使用 pthread_sigmask() 函数来独立检查和修改其信号掩码）。

向信号掩码中添加一个信号，有如下几种方式。

- 当调用信号处理器程序时，可将引发调用的信号自动添加到信号掩码中。（防止信号处理程序被同一个信号打断）
- 使用 sigaction() 函数建立信号处理器程序时，可以指定一组额外信号，当调用该处理器程序时，会将这组信号阻塞。
- 使用 sigprocmask() 系统调用，随时可以显式向信号掩码中添加或移除信号。

前两种情况在介绍完 sigaction() 函数后讨论，现在先来讨论 sigprocmask()函数。

## sigprocmask()

```c
int sigprocmask(int how, const siget_t *set, sigset_t *oldset);
```

使用该函数既可以修改进程的信号掩码，又可以获取现有的进程掩码，或者两者兼备。how 参数指定了该函数的功能。

- SIG_BLOCK：将 set 指向的信号集中的信号添加到进程的当前信号掩码中。
- SIG_UNBLOCK：将 set 指向的信号集中的信号从进程的当前信号掩码中移除。
- SIG_SETMASK：将 set 指向的信号集中的信号赋值给进程的当前信号掩码。（即先清空原信号掩码）

上述各种情况下，如果 oldset 参数不为NULL，则该参数为输出型参数，返回旧的信号掩码。

如果只想获得信号掩码又不修改它，可以通过 sigprocmask() 先随便增加一个信号以获得旧的信号掩码，然后再将信号掩码赋值回去。

SUSv3规定，如果某信号被 sigprocmask() 解除了阻塞，则该信号在 sigprocmask() 返回之前就被递达。

系统不允许阻塞 SIGKILL 和 SIGSTOP 信号，试图阻塞这两个信号不会产生任何结果。

# 处于等待状态的信号

如果进程收到了处于阻塞掩码中的信号，那么会将该信号添加到进程的**等待信号集**中（即处于 pending 状态）。

sigpending() 可以查看正在处于等待状态的信号。

```c
int sigpending(sigset_t *set);
// returns 0 on success, or -1 on error
```

set 为输出型参数，会返回处于等待状态的信号集，可以用之前的 sigmember() 检查某个信号是否在其中。

# 不对信号进行排队处理

等待信号集只是一个位图，而不是一个可以排队的队列。所以只能表示一个信号是否存在，而不能记录收到了多少次这个信号。所以我们说，非实时信号可能会丢失。即在pending的时候又收到了同一个信号。

# 改变信号的处置：sigaction()

除去 signal() ，sigaction() 也可以设置信号的处置方式。sigaction() 比 signal() 更加复杂，作为回报，也更具灵活性。尤其是，sigaction() 允许在获取旧的信号处理程序的时候无需将其改变。并且，还可以设置各种属性对调用信号处理程序时的行为施以更加精准的控制。

```c
int sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);
// returns 0 on success, or -1 on error
```

sig 是想要获取或改变的信号编号。该参数可以是除去 SIGKILL 和 SIGSTOP 之外的任何信号。

act 参数是一个指针，指向描述新信号处置的结构体。如果仅对信号的现有处置感兴趣，那么可以将该参数指定为NULL。oldact 参数是指向同一结构类型的指针，输出型参数，返回之前的信号处置相关信息。如果无意获取该信息，可以将这个参数设为NULL。act 和 oldact 所指向的结构体如下所示：

```c
struct sigaction {
    void (*sa_handler)(int);  // Adress of handler
    sigset_t sa_mask;  // Signals blocked during handler invocation
    int sa_flags;  // flags controlling handler invocation
    void (*sa_restorer)(void);  // Not for application use
}
```

> sigaction 结构体其实比上面展示的更加复杂

sa_handler 字段对应于 signal() 的 handler 参数。其所指定的值为信号的处理器函数的地址，亦或是常量 SIG_IGN 、SIG_DFL 之一。仅当 sa_handler 是新的信号处理函数的地址的时候，sa_mask 和 sa_flags 参数才有意义。sa_restorer 参数不供应用程序使用。

sa_mask 信号集定义了一组信号，在调用 sa_handler 所定义的信号处理函数的时候会阻塞这组信号。即在调用新的信号处理函数的时候这些信号会自动加到进程的信号掩码中，而引发这个信号处理函数的信号也会自动加入到信号掩码中，这意味着不允许程序被同一个信号递归中断。而信号处理函数返回后这些新加到信号掩码中的信号会被自动删除。

sa_flags 字段是一个位掩码，指定用于控制信号处理过程中的各种选项。该字段可以包含的位如下（可以想与（ | ））。

SA_NOCLDSTOP、SA_NOCLDWAIT、SA_NODEFER、SA_ONSTACK、SA_RESETHAND、SA_RESTART、SA_SIGINFO

# 等待信号：pause()

调用 pause() 系统调用将暂停进程的执行，进入可中断睡眠状态（S），直到进程收到一个信号，而这个信号的信号处理器是用户自定义函数或者是默认的终止进程，pause() 才会返回。

```c
int pause();
// always return -1 unless terminated
```

