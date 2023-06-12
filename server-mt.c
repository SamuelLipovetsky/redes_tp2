#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 2048

#define MAX_CLIENTS 15
#define MAX_MESSAGE_LENGTH 2048

void usage(int argc, char **argv)
{
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}
int client_count = 0;
char clients_ip[MAX_CLIENTS][100];
int clients_port[MAX_CLIENTS];
int clients_sockets[MAX_CLIENTS];
pthread_t tid[MAX_CLIENTS];

struct client_data
{
    int csock;
    struct sockaddr_storage storage;
};
struct Message
{
    int IdMsg;
    int IdSender;
    int IdReceiver;
    char *Message[2048];
};

void *client_thread(void *data)
{
      struct Message answer;
      struct Message msg;
    while (1)
    {
        struct client_data *cdata = (struct client_data *)data;
        struct sockaddr *caddr = (struct sockaddr *)(&cdata->storage);
        int port;
        char ip[400];
        if (caddr->sa_family == AF_INET)
        {
            struct sockaddr_in *caddr_in = (struct sockaddr_in *)caddr;
            port = ntohs(caddr_in->sin_port);

            strcpy(ip, inet_ntoa(caddr_in->sin_addr));
        }

        if (caddr->sa_family == AF_INET6)
        {
            struct sockaddr_in6 *caddr_in6 = (struct sockaddr_in6 *)caddr;
            port = ntohs(caddr_in6->sin6_port);
            inet_ntop(AF_INET6, &(caddr_in6->sin6_addr), ip, INET6_ADDRSTRLEN);
        }

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
       
        recv(cdata->csock, buf, BUFSZ - 1, 0);
   
        msg =createMessageFromAttributes(buf) ;
   
        int flag_full = 0;
        
      
      
        if (msg.IdMsg == 1)
        {
            answer.IdMsg = 6;
            for (int i = 0; i < 15; i++)
            {
                if (clients_port[i] == -1)
                {
                    clients_sockets[i] = cdata->csock;
                    clients_port[i] = port;
                    memcpy(clients_ip[i], ip, strlen(ip));
                    if (i == 14 && clients_port[i] != -1)
                    {
                        flag_full = 1;
                    }
                    answer.IdSender = i;

                    printf("User %d added \n", i + 1);

                    break;
                }
            }

            // char response[BUFSZ];
            char *response =concatenateMessageAttributes(answer);
            
            // memset(response, 0, BUFSZ);
         

            
            for (int i = 0; i < 15; i++)
            {
                if (clients_port[i] != -1)
                {

                    int count = send(clients_sockets[i], response, strlen(response) + 1, 0);
                   
                    if (count != strlen(response) + 1)
                    {
                        logexit("send");
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage))
    {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1)
    {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)))
    {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage)))
    {
        logexit("bind");
    }

    if (0 != listen(s, 10))
    {
        logexit("listen");
    }

    // addrtostr(addr, addrstr, BUFSZ);
    for (int i = 0; i < 15; i++)
    {
        clients_port[i] = -1;
        clients_sockets[i] = -1;
    }

    while (1)
    {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1)
        {
            logexit("accept");
        }

        struct client_data *cdata = malloc(sizeof(*cdata));
        if (!cdata)
        {
            logexit("malloc");
        }
        cdata->csock = csock;
        
        memcpy(&(cdata->storage), &cstorage, sizeof(cstorage));

        pthread_t tid;
        pthread_create(&tid, NULL, client_thread, cdata);
    }

    exit(EXIT_SUCCESS);
}
