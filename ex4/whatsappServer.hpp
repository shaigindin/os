
#ifndef EX4_WHATSAPPSERVER_HPP
#define EX4_WHATSAPPSERVER_HPP

//#include <sys/time.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include <iostream>
//#include <netinet/in.h>
//#include <sys/select.h>
//#include <sys/param.h>
//#include <netdb.h>
//#include <set>
//#include <map>
//#include <vector>
//#include <zconf.h>
//#include <errno.h>

#include <iostream>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/param.h>
#include <netdb.h>
#include <unistd.h>
#include <map>
#include <set>
#include "whatsappio.h"
#define MAX_GROUP_NAME 30
#define MAX_CLIENT_NAME 30
#define MAX_MESSAGE_LENGTH 256
#define MAX_GROUP_SIZE 50


/**
 * a class representing a server.
 */
class whatsappServer {
public:

    std::map<char*, int > clients_list;
    std::map<char*, std::set<char*>> groups;
    int serverWaitingSocket;

    whatsappServer(int portNumber);

    int establish(unsigned short portnumber);

    int get_connection(int s);

    int read_data(int s, char *buf, int n);

    int write_data(int socket, char *buf, int n);

    void handle_message(std::string line, int current_communicating_socket);

    void main_run();

};


#endif //EX4_WHATSAPPSERVER_HPP
