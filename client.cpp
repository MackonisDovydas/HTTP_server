#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include "HTTP_utils.h"

void connect_to_server(connection_info *connection, char *address, char *port)
{
        if ((connection->socket = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
            perror("Could not create socket");
        }
        connection->address.sin_addr.s_addr = inet_addr(address);
        connection->address.sin_family = AF_INET;
        connection->address.sin_port = htons(atoi(port));
        if (connect(connection->socket, (struct sockaddr *) &connection->address, sizeof(connection->address)) == -1) {
            perror("Connect failed.");
            exit(1);
        }
    puts("Connected to server.");
}

void user_input(connection_info *connection)
{
    message msg;
    std::cin.getline(msg.request,255);
    if(send(connection->socket,&msg, sizeof(message), 0) == -1)
    {
        perror("Send failed");
        exit(1);
    }
}


void server_message(connection_info *connection)
{
    message msg;

    //Receive a reply from the server
    ssize_t recv_val = recv(connection->socket, (void*)&msg, sizeof(message), 0);
    if(recv_val == -1)
    {
        perror("recv failed");
        exit(1);

    }
    else if(recv_val == 0)
    {
        close(connection->socket);
        puts("Server disconnected.");
        exit(0);
    }
    std::system ("clear");
    std::cout<<msg.request<<std::endl;
}

int main(int argc, char *argv[])
{
    connection_info connection;
    fd_set file_descriptors;

    if (argc !=3) {
        fprintf(stderr,"Usage: %s <IP> <port>(Optional)\n", argv[0]);
        exit(1);
    }

    connect_to_server(&connection, argv[1], argv[2]);
    std::cout<<"Example: GET /docs/index.html HTTP/1.0"<<std::endl;
    //keep communicating with server
    while(true)
    {
        FD_ZERO(&file_descriptors); //Clear all entries from the set
        FD_SET(STDIN_FILENO, &file_descriptors); // Add fd to the set.
        FD_SET(connection.socket, &file_descriptors);
        fflush(stdin);

        if(select(connection.socket+1, &file_descriptors, NULL, NULL, NULL) == -1)
        {
            perror("Select failed.");
            exit(1);
        }

        // Return true if fd is in the set.
        if(FD_ISSET(STDIN_FILENO, &file_descriptors))
        {
            user_input(&connection);
        }

        if(FD_ISSET(connection.socket, &file_descriptors))
        {
            server_message(&connection);
        }
    }
    return 0;
}
