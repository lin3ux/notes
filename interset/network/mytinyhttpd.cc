/*
根据tinyhttpd仿写的httpd实现
*/

#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <iostream>

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: dazhttpd/0.1.0\r\n"

void *accept_request(void *);

// 编写顺序 : main -> startup -> accept_request -> execute_cgi -> ...

int startup(u_short *);
void error_die(const char *);
void *accept_request(void *);
int get_line(int, char*, int);
int unimplemented(int);

/*
此函数在一个特定端口启动监听网页连接的进程
如果初始端口号是0,将动态分配一个端口并且改变原始端口变量(传入传出参数),以反应真实的端口号
参数:指向包含要连接的端口的变量的指针
返回值:socket端口号
*/
int startup(u_short *port)
{
  int httpd = 0;
  httpd = socket(PF_INET, SOCK_STREAM, 0);
  if(httpd == -1) {
    error_die("socket");
  }

  struct sockaddr_in name;
  bzero(&name, sizeof(name));
  name.sin_family = AF_INET;
  name.sin_port = htons(*port);
  name.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(httpd, (struct sockaddr*)&name, sizeof(name)) < 0) {
    error_die("bind");
  }

  if(*port == 0) {
    socklen_t namelen = sizeof(name);
    if(getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1) {
      error_die("getsockname");
    }
    *port = ntohs(name.sin_port);
  }

  if(listen(httpd, 5) < 0) {
    error_die("listen");
  }

  return httpd;
}

/*
一个请求导致服务器端口上的accept()调用返回:处理套接字上的http请求
在正确的时机处理请求
变量:连接到客户端的socket
*/
void *accept_request(void *client1)
{
  int client = *(int*)client1;
  char buf[1024];
  int numchars;
  char method[255];
  char url[255];
  char path[512];

  size_t i,j;
  struct stat st;
  int cgi = 0;// 标识位:当服务器确定这是一个CGI程序时转为真

  char *query_string = NULL;

  // 读取http请求的request line(第一行), 把请求方法存入method
  numchars = get_line(client, buf, sizeof(buf));
  i = 0;
  j = 0;
  // 去掉换行符
  while(!ISspace(buf[j]) && (i < sizeof(method) - 1)) {
    method[i] = buf[j];
    i++;
    j++;
  }
  method[i] = '\0';

  // 如果请求不是GET 或 POST 就发送response告知客户端没实现该方法
  if(strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
    unimplemented(client);
    return NULL;
  }

  

}

/*
使用perror打印错误信息(针对系统错误;基于errno变量值确定系统调用错误)
以及当检测到错误时退出进程
*/
void error_die(const char *sc)
{
  perror(sc);
  exit(1);
}


int main(int argc, char *argv[])
{
  int server_sock = -1;
  u_short port = 0;
  int client_sock = -1;
  struct sockaddr_in client_name;
  socklen_t client_name_len = sizeof(client_name);
  pthread_t newthread;

  server_sock = startup(&port); // 初始化
  std::cout << "httpd running on prot " << port << std::endl;

  while(1) {
    client_sock = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
    if(client_sock == -1) {
      error_die("accept");
    }
    if(pthread_create(&newthread, NULL, accept_request,
                      (void *)&client_sock) != 0) {
      perror("pthread_create");
    }
  }
  
  close(server_sock);

  return 0;
}