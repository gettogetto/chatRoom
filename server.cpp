#include"utility.h"
int main(int argc,char* argv[])
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family=PF_INET;
    serverAddr.sin_addr.s_addr=inet_addr(SERVER_IP);
    serverAddr.sin_port=htons(SERVER_PORT);

    // socket
    int listener=socket(PF_INET,SOCK_STREAM,0);
    if(listener<0){perror("listener");exit(-1);}
    printf("listen socket created \n");

    //bind
    if(bind(listener,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0){
        perror("bind");
        exit(-1);
    }

    //listen
    if(listen(listener,5)<0){
        perror("listen");
        exit(-1);
    }
    printf("start to listen: %s\n",SERVER_IP);

    //epoll create
    int epfd=epoll_create(EPOLL_SIZE);
    if(epfd<0){perror("epoll_create");exit(-1);}
    //add event
    addfd(epfd,listener,true);
    static struct epoll_event events[EPOLL_SIZE];
    //main loop
    while(1){
        int epoll_events_count=epoll_wait(epfd,events,EPOLL_SIZE,-1);
        if(epoll_events_count<0){
            perror("epoll_wait");
            break;
        }
        printf("epoll_events_count=%d\n",epoll_events_count);
        //solve the events
        for(int i=0;i<epoll_events_count;i++){
            int sockfd=events[i].data.fd;
            if(sockfd==listener){//new connection
                struct sockaddr_in clientAddr;
                socklen_t clientAddrSize = sizeof(clientAddr);
                int clientfd=accept(listener,(struct sockaddr*)&clientAddr,&clientAddrSize);
                if(clientfd<0){perror("accept");exit(-1);}
                printf("client connection from: %s:%d(IP:PORT),clientfd = %d\n",inet_ntoa(clientAddr.sin_addr),ntohs(clientAddr.sin_port),clientfd);

                addfd(epfd,clientfd,true);

                //list save connected clients
                clients_list.push_back(clientfd);
                printf("Add new clientfd =%d to epoll\n",clientfd);
                printf("Now there are %d clients in the chatroom",(int)clients_list.size());
                //server send message to other clients
                printf("welcome message\n");
                char message[BUF_SIZE];
                bzero(message,BUF_SIZE);
                sprintf(message,SERVER_WELCOME,clientfd);
                int ret = send(clientfd,message,BUF_SIZE,0);
                if(ret<0){
                    perror("send error");
                    exit(-1);
                }
            }else{//broadcast message from a client
                recvOneAndSendBroadcastmessage(sockfd);
            }
        }

    }
    close(listener);
    close(epfd);
    return 0;

}
