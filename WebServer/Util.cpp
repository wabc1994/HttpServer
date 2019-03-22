
#include "Util.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

const int MAX_BUFF = 4096;

//1.小于0，出错

//2.等于0，对方关闭

//3.大于0，但是小于count，对方关闭，只有

//4.count，代表读满count个字节




//当剩余长度大于0的时候就一直读啊读
//
//   1. 当read的返回值小于0的时候，做异常检测
//  2. 当read的返回值等于0的时候，退出循环
//  3. 当read的返回值大于0的时候，拿剩余长度减read的返回值，拿到新的剩余长度，读的入口指针加上read的返回值，进入步骤1
//  4. 返回参数n减去剩余长度，即实际读取的总长度



//readn在未出错或者fd没有关闭的情况下，会读满count个字节。
ssize_t readn(int fd, void *buff, size_t n)
{
    size_t nleft = n;
    ssize_t nread = 0;
    ssize_t readSum = 0;
    char *ptr = (char*)buff;
    while (nleft > 0)
    {
        if ((nread = read(fd, ptr, nleft)) < 0)
        {
            // 如果是EINTER判断错误返回码
            if (errno == EINTR)
                nread = 0;
            else if (errno == EAGAIN)
            {
                return readSum;
            }
            else
            {
                return -1;
            }  
        }
        // 判断对方已经关闭了连接
        else if (nread == 0)
            break;
        readSum += nread;
        nleft -= nread;
        ptr += nread;
    }
    return readSum;
}

ssize_t readn(int fd, std::string &inBuffer, bool &zero)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            // 判断读取了多少，如果是nread<0  进一步判断错误返回码，EINTR代表被中断的系统调用
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return readSum;
            }  
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (nread == 0)
        {
            //printf("redsum = %d\n", readSum);
            zero = true;
            // 本次读取数据已经结束
            break;
        }
        //printf("before inBuffer.size() = %d\n", inBuffer.size());
        //printf("nread = %d\n", nread);
        readSum += nread;
        //buff += nread;
        inBuffer += std::string(buff, buff + nread);
        //printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}


// 主要是处理两种错误码的情况

//一种是EINTE


//#define EINTR            4      /* Interrupted system call */
ssize_t readn(int fd, std::string &inBuffer)
{
    ssize_t nread = 0;
    ssize_t readSum = 0;
    while (true)
    {
        char buff[MAX_BUFF];
        if ((nread = read(fd, buff, MAX_BUFF)) < 0)
        {
            //由于信号中断，没读到任何数据,这里面主要是借鉴unp 的思想，直接continue 即可
            if (errno == EINTR)
                continue;
            else if (errno == EAGAIN)
            {
                return readSum;
            }  
            else
            {
                perror("read error");
                return -1;
            }
        }
        else if (nread == 0)
        {
            //printf("redsum = %d\n", readSum);
            break;
        }
        //printf("before inBuffer.size() = %d\n", inBuffer.size());
        //printf("nread = %d\n", nread);
        readSum += nread;
        //buff += nread;
        inBuffer += std::string(buff, buff + nread);
        //printf("after inBuffer.size() = %d\n", inBuffer.size());
    }
    return readSum;
}

// 要么读取n 字节的数据，要么读取失败，只有两种可能性， 要么读取成功，要么读取失败，本质也是通过循环



//当要写入的剩余长度大于0的时候就一直写啊写
  //      当write的返回值小于0的时候，做异常检测
    //    当write的返回值等于0的时候，出错退出程序
      //  当write的返回值大于0的时候，拿剩余长度减去write的返回值，拿到新的剩余长度，写的入口指针加上write的返回值，进入步骤1
        //返回参数n的值，即期望写入的总长度

ssize_t writen(int fd, void *buff, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    char *ptr = (char*)buff;
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0)
            {

                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }
                else if (errno == EAGAIN)
                {
                    return writeSum;
                    // 代表本次系统调用结束，下次再重新调用，这是两次不同的系统调用了
                }
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    return writeSum;
}

ssize_t writen(int fd, std::string &sbuff)
{
    size_t nleft = sbuff.size();
    ssize_t nwritten = 0;
    ssize_t writeSum = 0;
    const char *ptr = sbuff.c_str();
    while (nleft > 0)
    {
        if ((nwritten = write(fd, ptr, nleft)) <= 0)
        {
            if (nwritten < 0)
            {
                if (errno == EINTR)
                {
                    nwritten = 0;
                    continue;
                }

               // 非阻塞socket才会产生EAGAIN的错误，意思是当前不可读写，只要继续重试就好
                else if (errno == EAGAIN)
                    break;
                else
                    return -1;
            }
        }
        writeSum += nwritten;
        nleft -= nwritten;
        ptr += nwritten;
    }
    if (writeSum == static_cast<int>(sbuff.size()))
        sbuff.clear();
    else
        sbuff = sbuff.substr(writeSum);
    return writeSum;
}

void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if(sigaction(SIGPIPE, &sa, NULL))
        return;
}

int setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
        return -1;

    flag |= O_NONBLOCK;

    // 设置成为非阻塞式的服务情况
    if(fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

void setSocketNodelay(int fd) 
{
    int enable = 1;
    // 设置关闭
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable));
}

void setSocketNoLinger(int fd) 
{
    struct linger linger_;
    linger_.l_onoff = 1;
    linger_.l_linger = 30;
    setsockopt(fd, SOL_SOCKET, SO_LINGER,(const char *) &linger_, sizeof(linger_));
}


// 调用shutDownWR
void shutDownWR(int fd)
{
    shutdown(fd, SHUT_WR);
    //printf("shutdown\n");
}

int socket_bind_listen(int port)
{
    // 检查port值，取正确区间范围
    if (port < 0 || port > 65535)
        return -1;

    // 创建socket(IPv4 + TCP)，返回监听描述符
    int listen_fd = 0;
    // AF_INET 代表ipv4协议家族， SOCK_STREAM 代表使用TCP字节流
    if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        return -1;

    // 消除bind时"Address already in use"错误
    int optval = 1;

    // setsockopt() 设置套接字的一些属性情况，SO_REUSEADDR 进入timetou1阶段的可以立即使用该套接字情况
     // 允许端口重用该种情况、

//
//    level指定控制套接字的层次.可以取三种值:
//    1)SOL_SOCKET:通用套接字选项.
//    2)IPPROTO_IP:IP选项.
//    3)IPPROTO_TCP:TCP选项.　
//    optname指定控制的方式(选项的名称),我们下面详细解释　


    if(setsockopt(listen_fd, SOL_SOCKET,  SO_REUSEADDR, &optval, sizeof(optval)) == -1)
        return -1;

    // 设置服务器IP和Port，和监听描述副绑定
    struct sockaddr_in server_addr;
    bzero((char*)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons((unsigned short)port);
    if(bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    // 开始监听，最大等待队列长为LISTENQ

    //关于20148这个参数其实就是backlog accept queue队列的大小情况
    if(listen(listen_fd, 2048) == -1)
        return -1;

    // 无效监听描述符
    if(listen_fd == -1)
    {
        // 服务器端端要关闭一定要关闭高套接字
        close(listen_fd);
        return -1;
    }
    return listen_fd;
}