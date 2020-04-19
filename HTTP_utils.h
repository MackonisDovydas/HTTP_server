
#ifndef HTTP_PROTOCOL_HTML_UTILS_H
#define HTTP_PROTOCOL_HTML_UTILS_H

#endif //HTTP_PROTOCOL_HTML_UTILS_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <vector>
#include <string>

struct connection_info
{
    int socket;
    struct sockaddr_in address;
};
enum message_type
{
    GET,
    HEAD,
    POST,
};
enum file_format
{
    HTML,
    JPG
};
struct message
{
    message_type type;
    file_format format;
    char request[1000000];
};
