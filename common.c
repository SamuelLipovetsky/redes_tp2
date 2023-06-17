#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>

void logexit(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

struct Message
{
    int IdMsg;
    int IdSender;
    int IdReceiver;
    char Message[2000];
};

// Returns allocated string with time in format [HH:MM]
char *get_current_time()
{
    time_t t = time(NULL);
    struct tm *currentTime = localtime(&t);

    int hours = currentTime->tm_hour;
    int minutes = currentTime->tm_min;

    // char timeString[8]; // Allocate a string to store the formatted time (including null terminator)
    char *timeString = malloc(8 * sizeof(char));
    sprintf(timeString, "[%02d:%02d]", hours, minutes);

    return timeString;
}

// receives a Message Struct and creates a string with concatenated attributes
char *concatenateMessageAttributes(struct Message message)
{
    int size = (30 + strlen(message.Message));
    char *concatenatedString = calloc(size, sizeof(char));

    // Concatenate the attributes into the resulting string
    snprintf(concatenatedString, size * sizeof(char), "%d,%d,%d,%s",
             message.IdMsg, message.IdSender, message.IdReceiver, message.Message);

    return concatenatedString;
}
/*

Creates a Message struct from a string of concatenated atributes
attributesStrings -> string with format int IdMsg,int IdSender,int IdReceiver,char Message[2000];
returns a Message struct with the atributes
*/
struct Message createMessageFromAttributes(const char *attributesString)
{
    struct Message message;

    // Tokenize the attributes string using commas as delimiters
    char *token;
    token = strtok((char *)attributesString, ",");

    // Extract and assign the attribute values to the message object
    if (token != NULL)
    {
        message.IdMsg = atoi(token);
        token = strtok(NULL, ",");
    }
    if (token != NULL)
    {
        message.IdSender = atoi(token);
        token = strtok(NULL, ",");
    }
    if (token != NULL)
    {
        message.IdReceiver = atoi(token);
        token = strtok(NULL, ",");
    }
    if (token != NULL)
    {
        strcpy(message.Message, strdup(token));
    }

    return message;
}
/*
Initilizaes a sockaddr_storage with address and port for the client
char *addrstr = string representation of the adress
char *portstr = string representation of the port
sockaddr_storage *storage = struct to be initilizaed
returns -1 in case of an error
*/
int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage)
{
    if (addrstr == NULL || portstr == NULL)
    {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port);

    struct in_addr inaddr4;
    if (inet_pton(AF_INET, addrstr, &inaddr4))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6;
    if (inet_pton(AF_INET6, addrstr, &inaddr6))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

/*
initiliazes a sockaddr_storage struct depending on the type of protocol for the server
char * proto = type of protocol , it can be v4 or v6
char * portstr = string representation of the port to be used
sockaddr_storage *storage = pointer to the struct to be initialized
returns -1 in case of an error
*/
int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage)
{
    uint16_t port = (uint16_t)atoi(portstr);
    if (port == 0)
    {
        return -1;
    }
    port = htons(port);

    memset(storage, 0, sizeof(*storage));
    if (!strcmp(proto, "v4"))
    {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    }
    else if (!strcmp(proto, "v6"))
    {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    }
    else
    {
        return -1;
    }
}
