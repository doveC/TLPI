# 原子操作和竞争条件

所有系统调用都是以**原子操作**方式执行的。之所以这么说，是因为内核保证了某系统调用中所有步骤会作为独立操作而一次性加以执行，其间不会为其他进程或线程中断。（即该系统调用运行期间，不会把时间片再分给别的进程）

原子操作是某些操作得以圆满成功的关键所在。特别是它规避了**竞争状态**（race conditions）。竞争状态就是：操作共享资源的两个进程（或线程），其运行结果取决于一个无法预期的顺序，即取决于CPU时间片的分配。

下面的内容讨论文件I/O的两种竞争状态，并展示了如何用open() 系统调用的标志位来规避这种竞争状态。

## 以独占方式创建一个文件

上一章open() 曾说道，如果同时指定O_EXCL 和 O_CREAT 作为open() 的标志位时，如果要打开的文件已经存在，则open() 将返回一个错误。这提供了一种机制，保证进程是打开文件的创建者。对文件是否存在的检查和创建文件属于同一原子操作。要理解这一重要性：需要思考下面的代码：

```c
fd = open(argv[1], O_WRONLY);  // open if file exists
if(fd != -1) {  // open succeeded
    printf("PID [%ld] file %s already exists\n", getpid(), argv[1]);
}
else { 
    if(errno != ENOENT) {  // falied for unexpected reason
        perror("open error");
    }
    else {  // file or directory dont exist
        fd = open(argv[1], O_WRONLY | O_CREAT, 0666);
        if(fd == -1) {
            perror("open error");
        }
        
        printf("PID [%ld] created file %s exclusively\n", getpid(), argv[1]);
    }
}
```

思考有两个进程同时运行这个程序，可能出现下图的情况：

![1558271564310](C:\Users\a1599\AppData\Roaming\Typora\typora-user-images\1558271564310.png)

即进程A调用第一个open后发现文件不存在，但在调用第二个open之前时间片耗尽。此时进程B同样发现文件不存在，于是调用第二个open，之后声称文件是自己创建的。但此时进程A得到了时间片，并调用了第二个open，之后同样声称文件是自己创建的，这就产生了BUG。

这种情况就可以用O_EXCL和O_CREAT结合起来一次性调用来保证文件一定是自己创建的，即检查文件是否存在和创建文件属于一个单一的原子。

## 向文件尾部追加数据

用以说明原子操作的重要性的第二个例子是：多个进程同时向同一个文件（如，全局数据文件）尾部添加数据。为了达到这一目的，考虑下面的代码：

```c
lseek(fd, 0, SEEK_END);
write(fd, buf, len);
```

两个进程同时运行这段程序，假如进程A运行完第一句用完时间片，然后进程B将两条语句一次性运行完，之后进程A再运行第二句。则可知进程A写的位置就不是新的文件末尾了，而是之前的旧的文件末尾，这之后插入了进程B输入的内容。而此时进程A再写入的话就会覆盖之前B写的内容，这就是没有保证原子操作的坏处。

这个情况同样可以通过加一个O_APPEND标志来解决，O_APPEND标志保证了每一次write() 一定会写在文件末尾，不论你用了lseek与否。即将文件偏移量的移动和文件写入纳入同一原子操作。

# 文件控制操作：fcntl()

```c
int fcntl(int fd, int cmd, ...);
// return on success depends on cmd, or -1 on error
```

fcntl() 系统调用对一个打开的文件描述符执行一系列控制操作。

cmd参数支持的操作范围很广。例如获取或者修改对应的打开文件的访问模式和状态标志。（即读写方式等）这里不细说了。

# 文件描述符和打开文件之间的关系

在现实中，文件描述符和打开的文件并不是一一对应的。其实，多个文件描述符指向同一打开的文件，这既有可能，也很必要。这些文件描述符可以在不同的进程中打开，以使得多个进程操作同一个文件。

要理解具体的情形，需要查看由内核维护的3个数据结构：

- 进程级的文件描述符
- 系统级的打开文件表
- 文件系统的 i-node 表

针对每个进程，内核为其维护**打开文件的描述符表**（open file descriptor table）。该表的每一条目都记录了该进程的一个打开文件的描述符的相关信息，如下所示：

- 控制文件描述符的一组标志。（目前，此类标志仅定义了一个，即 close-on-exec标志）
- 对打开文件表条目（下面介绍）的引用

内核对所有打开的文件维护有一个系统级的**打开文件表**（open file table）。一个打开文件表条目存储了与一个打开文件相关的全部信息，如下所示：

- 当前文件的偏移量（调用read() 或write() 时更新，或使用lseek() 直接修改）。
- 打开文件时所使用的状态标志（即open() 的flags 参数）。
- 文件访问模式（只读、只写、读写）。
- 与信号驱动I/O相关的设置。
- 对该文件 i-node对象的引用。

每个文件系统都会为驻留其上的所有文件建立一个 i-node 表。下面是每个文件的 i-node 信息，如下所示：

- 文件类型和访问权限
- 一个指针，通常指向该文件所持有的锁的列表
- 文件的各种属性，包括文件大小及与不同类型操作相关的时间戳

下图表示了这三者的关系：

![1558314859479](C:\Users\a1599\AppData\Roaming\Typora\typora-user-images\1558314859479.png)

为了完成文件输入输出，进程通过系统调用把文件描述符传递给内核，然后内核替进程操作文件。进程不能直接接触到文件或者是 i-node 表。

图中有多种对应关系，现在一一解读：

在进程A中，文件描述符1和20都指向同一个打开的文件表条目（标号为23）。这可能是通过调用 dup()、dup2() 或 fcntl() 而形成的。

进程A的文件描述符2和进程B的文件描述符2指向同一个打开的文件表条目（标号为73）。这种情形可能在调用fork() 后出现，或者当某进程通过UNIX域套接字将一个打开的文件描述符传递给另一个进程时，也会发生。

此外，进程A的文件描述符0和进程B的文件描述符3分别指向不同的打开文件表条目，但这些条目均指向 i-node 表中相同的条目（1976），换言之，指向同一个文件。这种情况是因为进程各自对同一文件发起了open() 调用。同一个进程打开两次同一文件也会发生类似的情况。

上面的知识可以推导出下面的几个要点：

- 如果两个不同的文件描述符，指向同一个打开文件表条目，将共享同一文件偏移量。因此如果通过其中的一个文件描述符来修改文件偏移量，从另一个文件描述符中也会观察到这一变化。无论两个文件描述符分属于不同的进程，还是属于同一进程。
- 与上一条类似，修改打开文件标志和文件访问模式，也会影响到指向同一打开文件表条目的不同文件描述符。
- 相比之下，文件描述符表中进程私有的标志如close-on-exec ，修改后不会影响任何其他的文件描述符。

# 复制文件描述符

Bourne shell的 I/O重定向语法 2>&1，意在通知shell把标准错误（文件描述符2）重定向到标准输出（文件描述符1）所指向的位置。因此，下面的命令会（shell按照从左到右的顺序处理 I/O 重定向语句）把标准输出和标准错误都写入到文件 results.log中。

```shell
$ ./mysrcipt > results.log 2>&1
```

因为 > 标志将 myscript 的标准输出重定向到resluts.log，而下一句又把标准错误重定向到标准输出，即也指向 results.log。标准输出和标准错误指向同一个打开文件表条目，共享相同的文件偏移量。

shell 通过修改文件描述符2所指向的打开文件表条目以实现了标准错误的重定向操作，因此文件描述符2此时和文件描述符1指向同一个打开文件表条目（类似于上一节的图中进程A的描述符1和20指向同一打开文件表条目的情况）。可以通过 dup() 和 dup2() 来实现此功能。

## dup/dup2

dup() 调用复制一个打开的文件描述符oldfd，并返回一个新的文件描述符，二者都指向同一个打开文件表条目。系统会保证新文件描述符一定是编号值最低的未使用文件描述符。

```c
int dup(int oldfd);
// return new file descriptor on success, or -1 on error
```

dup2() 系统调用会为oldfd参数所指定的文件描述符创建副本，其标号由newfd指定。即newfd文件描述符指向oldfd所指向的打开文件表条目。如果newfd 所指定的文件描述符之前已经打开，则dup2() 会首先将其关闭，而且会忽略关闭过程中出现的任何错误。所以更安全的做法是先显示关闭newfd 所指定的文件描述符。