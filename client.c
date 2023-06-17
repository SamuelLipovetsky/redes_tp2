#include "common.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 2000
#define MAX_CLIENTS 15
#define MAX_MESSAGE_LENGTH 2000

int my_id = -1;
int users[15];
int break_flag = 1;
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
    char Message[2000];
};

void *receive_thread(void *arg)
{
    int server_socket = *((int *)arg);
    char buf[MAX_MESSAGE_LENGTH];
    struct Message res;
    int read_size;

    while ((read_size = recv(server_socket, buf, MAX_MESSAGE_LENGTH, 0)) > 0)
    {

        res = createMessageFromAttributes(buf);

        if (res.IdMsg == 6)
        {
            printf("%s\n", res.Message);
            // assuming that the first response always contains the ID allocated to this client
            if (my_id == -1)
            {
                my_id = res.IdSender;
            }
            // addin other users to the local users list
            if (res.IdReceiver != -1)
            {
                if (users[res.IdSender] == -1)
                {
                    users[res.IdSender] = 1;
                }
            }
        }
        if (res.IdMsg == 4)
        {
            printf("%s\n", res.Message);
        }
        if (res.IdMsg == 2)
        {
            users[res.IdSender] = -1;
            printf("%s\n", res.Message);
        }
        if (res.IdMsg == 7)
        {
            printf("%s\n", res.Message);
            if (strcmp(res.Message, "User limit exceeded")==0)
            {
                exit(0);
            }
        }
        if (res.IdMsg == 8)
        {
            // request to disconect was sucessfull
            if (res.IdReceiver == my_id)
            {
                printf("%s\n", res.Message);
                close(server_socket);
                break_flag = 0;
                exit(0);
            }
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
    char buf[BUFSZ];
    memset(buf, 0, BUFSZ);
    memset(users, -1, sizeof(users));
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

    // Creating permanent listening thread
    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_thread, &s) != 0)
    {
        exit(EXIT_FAILURE);
    }

    // sending request to join
    msg.IdMsg = 1;
    char *to_send = concatenateMessageAttributes(msg);
    send(s, to_send, strlen(to_send) + 1, 0);
    free(to_send);

    // reading input from user
    while (break_flag)
    {

        memset(buf, 0, BUFSZ);
        fgets(buf, BUFSZ - 1, stdin);

        if (!strncmp(buf, "list users", strlen("list users")))
        {
            msg.IdMsg = 4;
            to_send = concatenateMessageAttributes(msg);

            size_t count = send(s, to_send, strlen(to_send) + 1, 0);

            if (count != strlen(to_send) + 1)
            {
                break;
            }
            free(to_send);
        }
        else if (!strncmp(buf, "send all", strlen("send all")))
        {
            msg.IdMsg = 6;

            char substr[] = "send all ";
            char *pos = strstr(buf, substr);
            pos += strlen(substr);

            const char *start = strchr(pos, '"');
            const char *end = strchr(start + 1, '"');
            size_t length = end - start - 1;

            strncpy(msg.Message, start + 1, length);
            msg.Message[length] = '\0';

            msg.IdSender = my_id;
            msg.IdReceiver = -1;
            to_send = concatenateMessageAttributes(msg);

            size_t count = send(s, to_send, strlen(to_send) + 1, 0);

            if (count != strlen(to_send) + 1)
            {

                break;
            }
            free(to_send);
        }
        else if (!strncmp(buf, "send to", strlen("send to")))
        {
            msg.IdMsg = 6;

            char substr[] = "send to ";
            char *pos = strstr(buf, substr);
            pos += strlen(substr);
            char temp[2];
            strncpy(temp, pos, 2);
            msg.IdReceiver = atoi(temp) - 1;

            pos += 2;
            const char *start = strchr(pos, '"');
            const char *end = strchr(start + 1, '"');
            size_t length = end - start - 1;

            strncpy(msg.Message, start + 1, length);
            msg.Message[length] = '\0';

            msg.IdSender = my_id;

            to_send = concatenateMessageAttributes(msg);

            size_t count = send(s, to_send, strlen(to_send) + 1, 0);

            if (count != strlen(to_send) + 1)
            {
                break;
            }
            free(to_send);
        }
        else if (!strncmp(buf, "close connection", strlen("close connection")))
        {
            msg.IdMsg = 2;
            msg.IdSender = my_id;
            msg.IdReceiver = -1;
            strcpy(msg.Message, "");
            to_send = concatenateMessageAttributes(msg);

            size_t count = send(s, to_send, strlen(to_send) + 1, 0);

            if (count != strlen(to_send) + 1)
            {
                break;
            }
            free(to_send);
        }
    }

    exit(EXIT_SUCCESS);
}