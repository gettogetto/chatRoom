#include"utility.h"
#include<sys/wait.h>
#include<unistd.h>
static void sigchld_handle(int signo){
    pid_t pid;
    while((pid=wait(NULL))>0){
        printf("pid %d terminal\n",pid);
    }
}

int main(int argc,char* argv[]){
    //serveraddr
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=PF_INET;
    serverAddr.sin_port=htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);

    int sock=socket(PF_INET,SOCK_STREAM,0);
    if(sock<0){perror("socket");exit(-1);}
    //connect to server
    if(connect(sock,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0){
        perror("connect");
        exit(-1);
    }
    //create pipe
    int pipe_fd[2];
    if(pipe(pipe_fd)<0){perror("pipe");exit(-1);}
    //create epoll
    int epfd=epoll_create(EPOLL_SIZE);
    if(epfd<0){perror("epoll_create");exit(-1);}
    static struct epoll_event events[2];
    //add sock and pipe-read-end to epoll
    addfd(epfd,sock,true);
    addfd(epfd,pipe_fd[0],true);
    //
    bool isClientwork=true;
    //chat message buf
    char message[BUF_SIZE];
    //fork
    int pid = fork();
    if(signal(SIGCHLD,sigchld_handle)==SIG_ERR){
        perror("signal error");
        exit(-1);
    }

    if(pid<0){
        perror("fork");
        exit(-1);
    }
    else if(pid==0){
        //child read from stdin and  write message to parent
        close(pipe_fd[0]);//close read-end
        printf("input \"exit\" to leave chat room\n");
        while(isClientwork){
            bzero(message,BUF_SIZE);
            fgets(message,BUF_SIZE,stdin);
            //exit
            if(strncasecmp(message,EXIT,strlen(EXIT))==0){
                isClientwork=0;
            }
            else{
                if(write(pipe_fd[1],message,strlen(message)-1)<0){
                    perror("write error");
                    exit(-1);
                }
            }
        }
    }//child
    else{
        close(pipe_fd[1]);
        while(isClientwork){
            int epoll_events_count=epoll_wait(epfd,events,2,-1);
            if(epoll_events_count<0){
                perror("epoll_wait");
                break;
            }
            //settle ready events
            for(int i=0;i<epoll_events_count;++i){
                bzero(message,BUF_SIZE);
                if(events[i].data.fd==sock){
                    int ret=recv(sock,message,BUF_SIZE,0);
                    if(ret<0){
                        perror("recv");
                        exit(-1);
                    }else if(ret==0){
                        printf("server closed connection %d\n",sock);
                        close(sock);
                        isClientwork=0;
                    }else{
                        printf("%s\n",message);
                    }

                }else{
                    int ret = read(events[i].data.fd,message,BUF_SIZE);
                    if(ret ==0 ){
                        isClientwork=0;
                    }
                    else{
                        int ret = send(sock,message,strlen(message),0);
                        if(ret<0){perror("send");exit(-1);}
                    }
                }
            }//for
        }//while
    }//parent

    if(pid){
        close(pipe_fd[0]);
        close(sock);

    }else{
        close(pipe_fd[1]);
    }

}
