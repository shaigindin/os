
#ifndef EX4_WHATSAPPCLIENT_HPP
#define EX4_WHATSAPPCLIENT_HPP


#include <string>
#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/param.h>
#include <netdb.h>
#include <unistd.h>
#include "whatsappio.h"
#define MAX_GROUP_NAME 30
#define MAX_CLIENT_NAME 30
#define MAX_MESSAGE_LENGTH 256
#define MAX_GROUP_SIZE 50

/**
 * a class representing a client.
 */
class whatsappClient {

public:
    char *clientName;
    char *serverIPAddress;
    unsigned short serverPort;
    int clientSocket;

    whatsappClient(char *clientName, char* serverIP, unsigned short port);

    int call_socket(char *hostname, unsigned short portnumber);

    void main_run();

    void validate_command(std::string line);

    int read_data(int s, char *buf, int n);

    int write_data(int s, char *buf, int n);
};


#endif //EX4_WHATSAPPCLIENT_HPP
