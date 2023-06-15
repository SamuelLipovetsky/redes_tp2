#pragma once

#include <stdlib.h>

#include <arpa/inet.h>

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage);

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage);
struct Message createMessageFromAttributes(const char *attributesString) ;
char* concatenateMessageAttributes(struct Message message) ;
char*  get_current_time();