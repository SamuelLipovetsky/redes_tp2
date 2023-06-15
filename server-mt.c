#include "common.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 2000

#define MAX_CLIENTS 15
#define MAX_MESSAGE_LENGTH 2000

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
    char Message[2000];
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
        // int position_array = 0;
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

        msg = createMessageFromAttributes(buf);

        // request to join
        if (msg.IdMsg == 1)
        {
            //more than 15 clients
          
            if (client_count == 15)
            {
                answer.IdMsg=7;
                answer.IdReceiver=-1;
                answer.IdSender=-1;
                strcpy(answer.Message,"User limit exceeded");
                char *response = concatenateMessageAttributes(answer);
                int count = send(cdata->csock, response, strlen(response) + 1, 0);
                if (count != strlen(response) + 1)
                {
                    logexit("send");
                }
                // printf("User %02d remove", msg.IdSender + 1);
                free(response);

            }
            else
            {
                answer.IdMsg = 6;
                for (int i = 0; i < 15; i++)
                {
                    if (clients_port[i] == -1)
                    {
                        clients_sockets[i] = cdata->csock;
                        clients_port[i] = port;
                        memcpy(clients_ip[i], ip, strlen(ip));
                        client_count += 1;
                        answer.IdSender = i;

                        printf("User %02d added \n", i + 1);

                        sprintf(answer.Message, "User %02d joined the group!", i + 1);

                        break;
                    }
                }

                char *response = concatenateMessageAttributes(answer);

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
                free(response);
            }
        }
        if (msg.IdMsg == 2)
        {

            // valid user asked for end of connection
            if (clients_sockets[msg.IdSender] != -1)
            {

                // answer to requester
                answer.IdSender = -1;
                answer.IdReceiver = msg.IdSender;
                answer.IdMsg = 8;
                strcpy(answer.Message, "Removed Successfully");
                char *response = concatenateMessageAttributes(answer);
                int count = send(cdata->csock, response, strlen(response) + 1, 0);
                if (count != strlen(response) + 1)
                {
                    logexit("send");
                }
                printf("User %02d remove", msg.IdSender + 1);
                // removing client

                strcpy(clients_ip[answer.IdReceiver], "");
                clients_port[answer.IdReceiver] = -1;
                clients_sockets[answer.IdReceiver] = -1;
                free(response);
                // answer to other users
                answer.IdSender = msg.IdSender;
                answer.IdMsg = 2;
                answer.IdReceiver = -1;
                client_count -= 1;
                sprintf(answer.Message, "User %02d left the group!", msg.IdSender + 1);
                char *users_response = concatenateMessageAttributes(answer);

                for (int i = 0; i < 15; i++)
                {
                    // answers to everyone
                    if (clients_port[i] != -1)
                    {

                        int count = send(clients_sockets[i], users_response, strlen(users_response) + 1, 0);
                        if (count != strlen(users_response) + 1)
                        {
                            logexit("send");
                        }
                    }
                }

                free(users_response);
                break;
            }
        }
        if (msg.IdMsg == 4)
        {
            answer.IdSender = -1;
            answer.IdMsg = 4;

            strcpy(answer.Message, "");

            int size = 15;
            int resultIndex = 0;
            for (int i = 0; i < size; i++)
            {
                if (clients_sockets[i] != -1 && clients_sockets[i] != cdata->csock)
                {
                    char indexChar[5];
                    sprintf(indexChar, "%02d ", i + 1);
                    strcat(answer.Message, indexChar);
                    resultIndex += strlen(indexChar);
                }
            }

            if (resultIndex != 0)
            {
                answer.Message[resultIndex - 1] = '\0';
            }
            else
            {
                strcpy(answer.Message, " \0");
            }
            char *response = concatenateMessageAttributes(answer);
            // printf("%s\n", response);
            int count = send(cdata->csock, response, strlen(response) + 1, 0);
            if (count != strlen(response) + 1)
            {
                logexit("send");
            }
        }
        if (msg.IdMsg == 6)
        {

            // broadcast
            if (msg.IdReceiver == -1)
            {

                answer.IdSender = msg.IdSender;
                memset(answer.Message, 0, MAX_MESSAGE_LENGTH);
                char *time = get_current_time();

                sprintf(answer.Message, "%s %02d: %s", time, msg.IdSender + 1, msg.Message);

                char *generic_response = concatenateMessageAttributes(answer);

                printf("%s \n", answer.Message);
                memset(answer.Message, 0, MAX_MESSAGE_LENGTH);
                sprintf(answer.Message, "%s -> all: %s ", time, msg.Message);
                char *response_to_sender = concatenateMessageAttributes(answer);

                for (int i = 0; i < 15; i++)
                {
                    // answers to everyone but the original requester
                    if (clients_port[i] != -1 && clients_sockets[i] != cdata->csock)
                    {
                        int count = send(clients_sockets[i], generic_response, strlen(generic_response) + 1, 0);
                        if (count != strlen(generic_response) + 1)
                        {
                            logexit("send");
                        }
                    }
                    // answers only to the original requester
                    if (clients_port[i] != -1 && clients_sockets[i] == cdata->csock)
                    {
                        int count = send(clients_sockets[i], response_to_sender, strlen(response_to_sender) + 1, 0);
                        if (count != strlen(response_to_sender) + 1)
                        {
                            logexit("send");
                        }
                    }
                }
                free(time);
                free(generic_response);
                free(response_to_sender);
            }
            else
            {
                // invalid receiver
                if (clients_port[msg.IdReceiver] == -1)
                {
                    answer.IdMsg == 7;
                    sprintf(answer.Message, "03: \"Receiver not found\"");
                    char *response = concatenateMessageAttributes(answer);
                    int count = send(cdata->csock, response, strlen(response) + 1, 0);
                    if (count != strlen(response) + 1)
                    {
                        logexit("send");
                    }
                    free(response);
                }
                else
                {
                    memset(answer.Message, 0, MAX_MESSAGE_LENGTH);
                    char *time = get_current_time();

                    sprintf(answer.Message, "P %s %02d: %s", time, msg.IdSender + 1, msg.Message);

                    char *generic_response = concatenateMessageAttributes(answer);

                    // memset(answer.Message, 0, MAX_MESSAGE_LENGTH);
                    sprintf(answer.Message, "P %s -> %02d: %s ", time, msg.IdSender, msg.Message);
                    char *response_to_sender = concatenateMessageAttributes(answer);

                    int count = send(clients_sockets[msg.IdReceiver], generic_response, strlen(generic_response) + 1, 0);
                    if (count != strlen(generic_response) + 1)
                    {
                        logexit("send");
                    }
                    count = send(cdata->csock, response_to_sender, strlen(response_to_sender) + 1, 0);
                    if (count != strlen(response_to_sender) + 1)
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
