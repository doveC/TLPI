# 概述

## 文件描述符

所有执行I/O操作的系统调用都用文件描述符（一个非负整数）来**指代一个打开的文件**。文件描述符可以表示所有类型的已打开文件。

每个进程都有一套自己的文件描述符以标志当前进程打开的文件。

在shell的日常操作中有三个始终打开的文件，而所有在shell中运行的程序都会继承到这三个文件。

| **文件描述符** |   用途   | stdio流 |
| :------------: | :------: | :-----: |
|       0        | 标准输入 |  stdin  |
|       1        | 标准输出 | stdout  |
|       2        | 标准错误 | stderr  |

下面是文件I/O操作的4个主要的系统调用（编程语言和软件包通常会利用I/O函数库对它们进行间接调用）。

- fd = open(pathname, flags, mode) 函数打开pathname所标识的文件，并返回文件描述符，用以在后续函数调用中指代这个打开的文件。可以通过设置flags来新建一个pathname不存在的文件，flags还可以指定打开文件的方式，只读只写或者读写。mode参数则制定了新建文件的权限。
- numread = read(fd, buffer, count) 调用从fd所指代的的打开文件中读取至多count字节的数据，并存储到buffer中。返回值numread为实际读取到的字节数
- numwritten = write(fd, buffer, count) 调用从buffer中读取最多count个字节并写入fd所表示的文件中。返回值numwritten表示实际写入文件的字节数。
- status = close(fd) 在所有输入/输出操作完成后，调用close()，释放文件描述符fd以及与之相关的内核资源。

## 示例程序1

```c
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#ifndef BUF_SIZE
#define BUF_SIZE 1024
#endif

int main(int argc, char* argv[])
{
    if(argc != 3) {
        return 0;
    }   

    // open input and output file
    
    int inputFd = open(argv[1], O_RDONLY);
    if(inputFd == -1) {
        perror("open input file error");
        exit(-1);
    }   

    int outputFd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(outputFd == -1) {
        perror("open output file error");
        exit(-1);
    }   

    // transfer data
    
    char buffer[BUFSIZ] = { 0 };
    
    int nn; 
    while((nn = read(inputFd, buffer, BUFSIZ)) > 0) {                                   
        write(outputFd, buffer, nn);
    }

    // close
        
    if(close(inputFd) == -1) {
        perror("close input file error");
    }
    if(close(outputFd) == -1) {
        perror("close output file error");
    }

    return 0;
}

```

# 通用I/O

## 通用性

UNIX I/O模型的显著特点之一是其输入/输出的通用性概念。即open()、close()、read()、write()这一套屠龙宝典可以应对所有类型的文件。因此，用这些系统调用编写的程序，将对任何文件都有效。

# 打开文件：open()

```c
int open(const char *pathname, int flags, mode_t mode);
// returns file descriptor on success, or -1 on error
```

mode_t 为整数类型

## 文件描述符

SUSv3规定，如果open()调用成功，必须保证其返回值为进程未用文件描述符中数值最小者。

## flags

> 读取位于/proc/PID/fdinfo 目录下的linux系统专有文件，可以获取系统内任一进程中文件描述符的相关信息。针对进程中每一个已打开的文件描述符，该目录下都有相应文件，以对应文件描述符的数值命名。文件中pos字段表示当前文件的偏移量。flags字段为一个八进制数，表征文件访问标志和已打开文件的状态标志。

下面介绍几个最常用的flags

| 标志     | 用途                                                         |
| -------- | ------------------------------------------------------------ |
| O_RDONLY | 以只读方式打开                                               |
| O_WRONLY | 以只写方式打开                                               |
| O_RDWR   | 以读写方式打开                                               |
| O_CREAT  | 如果文件不存在则创建之                                       |
| O_EXCL   | 与O_CREAT搭配使用，表示如果文件存在，则不会打开文件并open() 调用失败，并返回错误，errno设置为EEXIST。换言之，此操作确保了调用者就是创建文件的进程。 |
| O_TRUNC  | 截断已有文件，使其长度为0                                    |
| O_APPEND | 总在文件末尾追加数据，不论是否用了lseek()                    |

# 读取文件内容：read()

```c
ssize_t read(int fd, void *buffer, size_t count);
// returns number of bytes read, 0 on EOF, or -1 on error
```

# 数据写入文件：write()

```c
ssize_t write(int fd, void* buffer, size_t count);
// returns number of bytes written, or -1 on error
```

# 关闭文件：close()

```c
int close(int fd);
// returns 0 on success, or -1 on error
```

# 改变文件偏移量：lseek()

**对于每个打开的文件，系统内核会记录其文件偏移量，也可以将文件偏移量称为读写偏移量或指针。文件偏移量是执行下一个 read()或 write()操作的起始位置，会以相对于文件起始位置相差多少字节来表示。**

文件打开时，会将文件偏移量设置为指向文件开始，以后每次 read()或 write()调用将自动对其调整，会指向已读或已写数据后的下一个字节。

```c
off_t lseek(int fd, off_t offset, int whence);
// returns new file offset if successful, or -1 on error
```

offset 参数指定了一个以字节为单位的数值。（SUSv3规定 off_t 数据类型为有符号整型数。）whence参数表明应按照哪个基点来移动偏移量，应为下列其中之一：

| whence   | 解释                           |
| -------- | ------------------------------ |
| SEEK_SET | 以文件头部开始计算偏移量       |
| SEEK_CUR | 以当前文件偏移量开始计算偏移量 |
| SEEK_END | 以文件末尾开始计算偏移量       |

如果whence参数值为 SEEK_CUR 或 SEEK_END ，offset参数可以为正数也可以为负数。但如果whence为 SEEK_SET ，offset必须是非负数。

lseek() 并不适用于所有类型的文件。不能用于管道、FIFO、socket或者终端。

## 文件空洞

如果文件偏移量已然跨越了文件结尾，read() 调用将返回0，表示文件结尾。但可以继续 write() 。

从文件结尾到新写入的数据间的这段空间被称为文件空洞（file holes）。从编程角度来看，文件空洞中是存在字节的，读取空洞将返回0（空字节）填充的缓冲区。

然而文件空洞其实不存储字节也就是不占用物理硬盘空间或者占用的很少，下面这张图比较形象：

![1558268186762](C:\Users\a1599\AppData\Roaming\Typora\typora-user-images\1558268186762.png)

具体细节不深究，需要时google把（google真好用，wiki真好用 xD）

# 通用I/O模型以外的操作：ioctl()

在上面通用I/O模型之外，ioctl() 系统调用又为执行文件和设备操作提供了一种多用途的机制。