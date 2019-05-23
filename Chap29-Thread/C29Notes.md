这里即将讨论 POSIX线程（thread），亦即 Pthreads。可以将之看作 thread的相关标准，定义了各个相关函数的功能。

而有两种主流的线程实现——LinuxThreads和Native POSIX Threads Library（NPTL）。两者都根据 Pthreads 线程标准来实现功能函数，不过都多少比 Pthreads 标准有些出入。现在 LinuxThreads基本已经被淘汰。

# 概述

与多进程类似，线程（thread）是允许应用程序并发执行多个任务的一种机制。

如下图所示，一个进程可以包含多个线程。**同一程序中所有的线程均会独立执行相同程序，且共享同一份全局内存区域，其中包括初始化数据段（initialized data）、未初始化数据段（uninitialized data），以及堆内存段（heap segment）**。

![1558503550960](C:\Users\a1599\AppData\Roaming\Typora\typora-user-images\1558503550960.png)

> 上图其实做了一些简化。特别是，线程栈（thread stack）的位置可能会与共享库和共享内存的区域混杂在一起，这取决于创建线程、加载共享库，以及映射共享内存的具体顺序。而且，对于不同的Linux发行版，线程栈地址也会有所不同。

同一进程中的多个线程可以并发执行。在多处理器环境下，多个线程可以同时并行。如果一线程因等待I/O操作而遭阻塞，那么其他线程依旧可以继续运行。

对于某些应用而言，线程要优于进程。传统UNIX通过创建多个进程来实现并行任务。以网络服务器设计为例，服务器进程（父进程）在接收客户端的连接后，会调用 fork() 来创建一个单独的子进程，以处理和客户端的通信。采用这种设计，服务器就能同时为多个客户端提供服务。虽然这种方法在很多情境下都屡试不爽，但对于某些应用来说也确实存在如下限制：

- **进程间信息难以共享**。除去只读代码段外，父子进程并未共享其他内存区域。因此必须采用进程间通信（IPC）的方式，在进程间交换数据。
- **调用 fork() 来创建进程的代价相对较高**。即便有写时复制和共享代码段等技术，但仍需复制多种进程属性，这意味这 fork() 在调用时间上开销依然不菲。

线程解决了上述两个问题：

- **线程之间能方便、快速的共享数据**。只需将数据复制到共享变量（全局数据段或堆）中即可。不过要避免多个线程同时修改同一份信息的情况。这就要用到同步技术。
- **创建线程比创建进程快10倍甚至更多**。（Linux中是通过 clone() 来创建线程的）。线程创建之所以更快，是因为调用 fork() 创建子进程时需要复制诸多属性，而这些属性本就是线程间共享的。特别是，即无需写时复制内存页，也无需复制页表。

除了全局数据和堆之外，线程还共享一干其他属性：

- 进程ID（PID）和父进程ID
- 控制终端
- 打开的文件描述符（fd）
- 由 fcntl() 创建的记录锁（record lock）
- 信号处置（信号处理器函数、默认、忽略）
- 文件系统相关信息：文件权限掩码（umask）、当前工作目录和根目录
- 资源消耗
- CPU 时间消耗

各线程独有属性：

- 线程ID（thread ID）
- 信号掩码（signal mask）
- 线程特有数据
- errno 变量
- 实时调度策略和优先级
- 栈，本地变量

> 上图所示，所有线程栈均驻留于同一虚拟地址空间。这也意味着，通过合适的指针（地址），各线程可以在对方栈中相互共享数据。这种方法偶尔能派上用场，但严重依赖于其他线程栈的栈帧的生命周期（函数返回），以及其他线程栈的生命周期（线程终止）。

# 线程数据类型

Pthreads API 定义了一干数据类型。列举部分：

| 数据类型            | 描述                                      |
| ------------------- | ----------------------------------------- |
| pthread_t           | 线程ID                                    |
| pthread_mutex_t     | 互斥对象（Mutex）                         |
| pthread_mutexattr_t | 互斥属性对象                              |
| pthread_cond_t      | 条件变量（condition variable）            |
| pthread_condattr_t  | 条件变量的属性对象                        |
| pthread_key_t       | 线程特有数据的键（Key）                   |
| pthread_once_t      | 一次性初始化控制上下文（control context） |
| pthread_attr_t      | 线程的属性对象                            |

**SUSv3 并未规定如何实现这些数据类型，可移植程序应将其视为“不透明”数据。亦即，程序应避免对此类数据类型变量的结构或内容产生依赖。尤其是，不能使用C语言的比较操作符（==）去比较这些数据类型的变量。**

# 创建线程

启动程序时，产生的进程只有单条线程，称之为初始（initial）或主（main）线程。

函数 pthread_create() 负责创建一条新线程。

```c
#include <ptherad.h>
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*start)(void *), void *arg);
// returns 0 on success, or a positive error number on error
```

新线程通过调用带有参数 arg 的函数函数 start（即 start(arg)）而开始执行。调用 pthread_create() 的线程会继续执行该调用之后的语句。

将参数 arg 声明为 void* 类型，意味着可以将指向任意数据的指针传给 start() 函数。一般情况下，arg 指向一个全局或者堆变量，也可将其置为NULL。如果需要向 start() 传递多个参数，可以将 arg 指向一个结构体，结构体内部存储各个要传递的参数。通过强制类型转换，arg 甚至可以传递数据本身。

新线程的入口函数 start() 同样有返回值 void*。后续 pthread_join() 函数将论及对该返回值的使用。

参数 thread 指向 pthread_t 类型的缓冲区，在 pthread_create() 返回之前，会在此保存一个线程的唯一标识。后续的 Pthreads 函数将使用该标识来引用此线程。即输出型参数。

参数 attr 是指向 pthread_attr_t 对象的指针，该对象指定了新线程的各种属性。如果指定为NULL，则使用默认属性。

调用 pthread_create() 后，应用程序无从确定系统接着会调度哪一个线程来使用 CPU 资源。程序最好不要隐含对特定调度顺序的依赖。如果对于执行顺序有要求，应该使用同步技术。

# 终止线程

可以如下方式终止线程的运行。

- 线程 start 函数执行 return 语句并返回指定值
- 线程调用 pthread_exit() 
- 调用 pthread_cancel() 取消线程
- 任意线程调用 exit()，或者主线程执行了 return 语句（main() 函数中），都会导致进程中所有线程立即终止

pthread_exit() 函数将终止调用线程，其返回值可由另一线程通过调用 pthread_join() 来获取。

```c
void pthread_exit(void *retval);
```

调用 pthread_exit() 相当于在线程的 start 函数中执行 return，与之不同的是，可在线程 start 函数所调用的任意函数中调用 pthread_exit()。

参数 retval 指定了线程的返回值。Retval 所指向的内容不应分配于线程栈中，而应分配在全局数据区或者堆上，因为线程返回后之前的线程栈区域将被释放。同理，start 中返回的数据也不应分配于线程栈中。

如果线程调用了 pthread_exit()，而非 exit() 或者return，其他线程将照常运行。

# 线程ID

进程内部的每个线程都有一个唯一标识符，称为线程ID。线程ID会返回给 pthread_create() 的调用者，一个线程可以通过 pthread_self() 来获取自己的线程ID。

```c
pthread_t pthread_self(void);
```

线程ID的作用：

- Pthreads 函数通过线程ID来标识要操作的线程。

函数 pthread_equal() 可检查两个线程的ID是否相同。

```c
int pthread_equal(pthread_t t1, ptherad_t t2);
// returns nonzero value if t1 and t2 are equal, otherwise 0
```

因为必须把 pthread_t 当作不透明的数据类型，所以才需要一个专门的函数检查相同。Linux 将 pthread_t 定义为无符号长整形（unsigned long）。

在Linux的线程实现中，线程ID在所有进程中都是唯一的。但其他实现并不一定，所以在可移植程序中不要用进程ID标识其他进程的线程。此外，在对已终止线程施以 pthread_join()，或者在已分离（detached）线程退出后，实现可以复用该线程ID。

> POSIX 线程ID与Linux专有的系统调用 gettid() 所返回的线程ID并不相同。POSIX线程ID由线程库实现来负责分配和维护。gettid() 返回的线程ID是一个由内核分配的数字，类似于进程ID。应用程序一般无需了解内核分配的线程ID。

# 连接（joining）已终止的线程

函数 pthread_join() 等待由 thread 标识的线程终止。如果已终止，则立刻返回。这种操作称为连接（joining）。

```c
int pthread_join(pthread_t thread, void **retval);
// returns 0 on success, or a positive error number on error
```

若 retval 为一非空指针，将会保存线程终止时返回值的拷贝，该返回值亦即线程调用return 或 pthread_exit() 时所指定的值。（由于return 或者 pthread_exit() 返回的是void*，所以这里的输出型参数的类型是 void**）

如果使用 pthread_join() 传入一个之前已然连接过的线程，这是未定义行为。

**如果线程并未分离（detached，分离相当于标记当前进程，使之终止时系统自动回收相关资源），则必须使用 pthread_join() 来进行连接。**如果未能连接，那么线程终止时将产生僵尸线程，与僵尸进程的概念相类似。除了浪费系统资源外，如果僵尸线程积累过多，应用再也无法创建新线程。

pthread_join() 执行功能类似于针对进程的 waitpid() ，不过二者存在显著差别。

- 线程的关系是对等的（peers）。进程内的线程间可以随意连接。而 waitpid() 只能等待子进程。
- 无法指定连接任一线程（即相当于wait() 或者 waitpid(-1, ..)）。也不能以非阻塞（unblocking）方式连接（waitpid中的WHOHANG标志可以做到）。但使用条件变量可以实现类似的功能。

> 限制pthread_join() 只能连接指定线程ID。程序应只能连接它“知道的”线程。线程间并无层次关系，如果任由连接任意线程，是不合理也无意义的。

例程：

```c
#include <string.h>
#include <stdio.h>
#include <pthread.h>

void* threadFunc(void* arg) {
    char* s = (char*)arg;
    
    printf("%s", s);

    return (void *)strlen(s);
}

int main()
{
    pthread_t tid;
    void* res;

    pthread_create(&tid, NULL, threadFunc, "Hello World\n");

    printf("Message from main\n");
    pthread_join(tid, &res);

    printf("Thread returned %ld\n", (long)res);

    return 0;
}
```

# 线程的分离

默认情况下，线程是可连接的（joinable），也就是说，当线程退出时，其他线程可以通过调用 pthread_join() 获取其返回状态。有时，程序员并不关系线程的返回状态，只是希望系统在线程终止的时候自动清理并移除之。在这种情况下，可以调用 pthread_detach() 并向 thread 参数传入线程的标识符，将该线程标记为处于分离（detached）状态。

```c
int pthread_detach(pthread_t thread);
// returns 0 on success, or a positive error number on error
```

如下使用，线程自行分离：

```c
pthread_detach(pthread_self());
```

**一旦线程处于分离状态，就不能使用 pthread_join() 来连接，也无法使其重返可连接状态**

# 线程VS进程

线程相对的优点：

- 线程间的数据共享很简单。相比之下，进程间的数据共享可能需要更多的投入（IPC）。
- 创建线程要快于创建进程。线程间的上下文切换（context-switch，CPU从一个线程切换到另一个线程），其消耗时间一般也比进程要短。

线程相对的缺点：

- 多线程编程时，需要确保调用线程安全（thread-safe）的函数，或者以线程安全的方式来调用函数。多进程应用则无需关注这些。
- 某个线程中的BUG（例如，通过一个错误的指针来修改内存）可能会危及该进程的所有线程，因为它们共享着相同的地址空间和其他属性。相比之下，进程间的隔离更加彻底。
- 每个线程都在争用宿主进程（host process）中有限的虚拟地址空间。虽然有效虚拟地址空间很大（32位平台为3GB），但终归不如多进程程序，每个进程可以使用全部的虚拟内存，仅受制于实际内存和交换空间。

中立的不同点还有：

- 在多线程应用中处理信号，需要小心设计。（作为通则，一般建议在多线程程序中避免使用信号）
- 多线程程序中，每个线程必须运行同一个（可执行）程序，虽然一般是在不同的函数中。而多进程程序，每个进程运行不同的（可执行）程序。
- 除了数据，线程还共享某些其他信息（例如，文件描述符、信号处置、当前工作目录等）

