#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 4000
#define MAX_CLIENTS 15
#define MAX_MESSAGE_LENGTH 2048

int my_id = -1;
void usage(int argc, char **argv)
{
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

struct Message
{
    int IdMsg;
    int IdSender;
    int IdReceiver;
    char Message[2048];
};

void *receive_thread(void *arg)
{
    int server_socket = *((int *)arg);
    char buf[MAX_MESSAGE_LENGTH];
    struct Message res;
    int read_size;
    
      while ((read_size = recv(server_socket, buf, MAX_MESSAGE_LENGTH, 0)) > 0) {
        

        res = createMessageFromAttributes(buf) ;

        if (res.IdMsg==6){
    
            printf("%s\n",res.Message);
            if(my_id==-1){
                my_id=res.IdSender;
                
            }
        }
        if (res.IdMsg==4){
            printf("%s\n",res.Message);
        }

        memset(buf, 0, MAX_MESSAGE_LENGTH);
    }

    pthread_exit(NULL);
}
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argc, argv);
    }
    // int my_id = NULL;
    char buf[BUFSZ];
 
    memset(buf, 0, BUFSZ);

    // creating connection
    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage)))
    {
        logexit("connect");
    }
    struct Message msg;
 
    msg.IdMsg = 1;
    char *to_send =concatenateMessageAttributes(msg);
    
    send(s, to_send, strlen(to_send), 0);

    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &s) != 0)
    {
        
        exit(EXIT_FAILURE);
    }
    while (1)
    {

        memset(buf, 0, BUFSZ);
        fgets(buf, BUFSZ - 1, stdin);
        if(!strcmp(buf,"list users")){
            msg.IdMsg=4;
            to_send =concatenateMessageAttributes(msg);

        }
        if(!strncmp(buf,"send all",strlen("send all"))){
            msg.IdMsg=6;
        
            char substr[] = "send all";
            char *pos = strstr(buf, substr);
            pos += strlen(substr); 
            strcpy(msg.Message,pos);
            msg.IdSender=my_id;
            msg.IdReceiver=NULL;
            to_send =concatenateMessageAttributes(msg);
        }
        size_t count = send(s, to_send, strlen(to_send), 0);
        if (count != strlen(to_send))
        {
            break;
        }

    }

    exit(EXIT_SUCCESS);
}