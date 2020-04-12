
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <pthread.h>
#include <cstring>
#include <csignal>
#include <fstream>
#include <sys/stat.h>

#include "HTTP_utils.h"

#define MAX_CLIENTS 10


static volatile int keepRunning = 1;

void intHandler(int signum) {
    keepRunning = 0;
}
void initialize_server(connection_info *server_info, int port)
{
    /*int socket(int domain, int type, int protocol);
    *The most correct thing to do is to use
    *AF_INET in your struct sockaddr_in and PF_INET in your call to socket(). - BJ */
    if((server_info->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Failed to create socket");
        exit(1);
    }

    server_info->address.sin_family = AF_INET;
    server_info->address.sin_addr.s_addr = INADDR_ANY;
    server_info->address.sin_port = htons(port);

    //int bind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen);
    if(bind(server_info->socket, (struct sockaddr *)&server_info->address, sizeof(server_info->address)) == -1)
    {
        perror("Binding failed");
        exit(1);
    }

    const int optVal = 1; //true
    const socklen_t optLen = sizeof(optVal);
    // SO_REUSEADDR - Allows other sockets to bind() to this port,unless there is an
    // active listening socket bound to the portal ready. This enables you to get around
    // those “Address already in use” error messages when you try to restart your server after acrash.
    // int setsockopt(int s, int level, int optname, const void *optval, socklen_t optlen);
    if(setsockopt(server_info->socket, SOL_SOCKET, SO_REUSEADDR, (void*) &optVal, optLen) == -1)
    {
        perror("Set socket option failed");
        exit(1);
    }

    //int listen(int s, int backlog);
    if(listen(server_info->socket, MAX_CLIENTS) == -1) {
        perror("Listen failed");
        exit(1);
    }

    printf("Waiting for incoming connections...\n");
}

void stop_server(connection_info connection[])
{
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        close(connection[i].socket);
    }
    exit(0);
}

std::string check_request(message msgGet)
{
    int count=0;
    std::string request = msgGet.request;
    for(int i=0;i<request.length();i++){
        if(isspace(request.at(i))){
            count++;
        }
    }
    if(count !=2){
        request = "HTTP/1.0 400 Bad Request";
        return request;
    }
    else {
        std::string method_name;
        method_name = request.substr(0,4);
        if(method_name !="POST" && method_name != "HEAD" && method_name.substr(0, method_name.size()-1) != "GET"){
                request = "HTTP/1.0 501 Method Not Implemented";
                return request;
        }
    }
    request.erase(0,4);
    if(request.at(0)==' '){
        request.erase(0,1);
    }
    int pos=request.find(' ',0);
    std::string request_URI=request.substr(0,pos);
    std::string HTTP_version=request.substr(pos+1, request.length());
    if(HTTP_version!= "HTTP/1.0" || request_URI.front()!= '/'){
        request = "HTTP/1.0 400 Bad Request";
        return request;
    }
    request_URI.erase(0, 1);

    std::ifstream file(request_URI);
    if(file.is_open() == 0){
        request = "HTTP/1.0 404 Not Found";
        return request;
    }
    file.close();
    request = "HTTP/1.0 200 OK";
    return request;
}
std::string get_file(std::string request_URI)
{
    std::string str;
    std::ifstream file(request_URI);
    if (file.is_open()) {
        char c;
        while (file.get(c)) {
            str += c;
        }
        file.close();
    }
    else {
        perror("File not found");
    }
    return str;
}
std::string process_request(message msgGet)
{
    std::string request = msgGet.request;
    std::string method_name = request.substr(0,4);
    request.erase(0,4);
    int pos=request.find(' ',0);
    std::string request_URI=request.substr(0,pos);
    std::string HTTP_version=request.substr(pos+1);
    request_URI.erase(0, 1);
    if(method_name == "GET "){
        msgGet.type=GET;
    }
    else if(method_name == "POST"){
        msgGet.type=POST;
    }
    else if(method_name == "HEAD"){
        msgGet.type=HEAD;
    }
    std::string add_to_request;
    switch (msgGet.type){
        case GET:
            add_to_request=get_file(request_URI);
            break;

        case HEAD:
            //DO NOTHING
            break;

        case POST:
            break;
    }
    if(add_to_request.empty()){
        return "HTTP/1.0 200 OK";
    }
    else{
        std::string str="HTTP/1.0 200 OK";
        str.append(2u,'\n');
        return str + add_to_request;
    }
}
void handle_client_message(connection_info clients[], int sender) {
    int read_size;
    message msgGet;

    //ssize_t recv(int s, void *buf, size_t len, int flags);
    if ((read_size = recv(clients[sender].socket,(void*)&msgGet, sizeof(message), 0)) == 0) {
        close(clients[sender].socket);
        clients[sender].socket = 0;
    }
    else {
        std::cout <<msgGet.request<< std::endl;
        message msgBack;
        std::string str;
        str = check_request(msgGet);
        strcpy(msgBack.request, str.c_str());
        if(str == "HTTP/1.0 200 OK") {
            strcpy(msgBack.request, process_request(msgGet).c_str());
        }
        //strcpy(msgBack.request, "HTTP/1.0 200 OK");
        if (send(clients[sender].socket, &msgBack, sizeof(message), 0) == -1) {
            perror("Send failed");
            exit(1);
        }
    }
}

int construct_fd_set(fd_set *set, connection_info *server_info,
                     connection_info clients[])
{
    FD_ZERO(set); //Clear all entries from the set.
    FD_SET(STDIN_FILENO, set);
    FD_SET(server_info->socket, set); //Add fd to the set.

    int max_fd = server_info->socket;
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket > 0)
        {
            FD_SET(clients[i].socket, set);
            if(clients[i].socket > max_fd)
            {
                max_fd = clients[i].socket;
            }
        }
    }
    return max_fd;
}

void send_too_full_message(int socket)
{
    std::string output;
    output = "Server too full";

    if(send(socket, &output, sizeof(output), 0) == -1)
    {
        perror("Send failed");
        exit(1);
    }

    close(socket);
}

void handle_new_connection(connection_info *server_info, connection_info clients[])
{
    socklen_t address_len;
    int new_socket = accept(server_info->socket, (struct sockaddr*)&server_info->address, &address_len);

    if (new_socket == -1)
    {
        perror("Accept Failed");
        exit(1);
    }

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clients[i].socket == 0) {
            clients[i].socket = new_socket;
            break;

        } else if (i == MAX_CLIENTS -1) // if we can accept no more clients
        {
            send_too_full_message(new_socket);
        }
    }
}

int main(int argc, char *argv[]) {
    puts("Starting server.");

    fd_set file_descriptors;

    connection_info server_info;
    connection_info clients[MAX_CLIENTS];

    struct sigaction sa;

    sa.sa_flags = 0; // or SA_RESTART
    sigemptyset(&sa.sa_mask);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
    }

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    initialize_server(&server_info, atoi(argv[1]));

    while(keepRunning)
    {
        int max_fd = construct_fd_set(&file_descriptors, &server_info, clients);

        //int select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

        if(select(max_fd+1, &file_descriptors, NULL, NULL, NULL) == -1)
        {
            perror("Select Failed");
            stop_server(clients);
        }

        if(FD_ISSET(server_info.socket, &file_descriptors))
        {
            handle_new_connection(&server_info, clients);
        }

        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(clients[i].socket > 0 && FD_ISSET(clients[i].socket, &file_descriptors))
            {
                handle_client_message(clients, i);
                close(clients[i].socket);
                clients[i].socket = 0;
            }
        }
    }
    stop_server(clients);
    return 0;
}