
#include <cstring>
#include "whatsappClient.hpp"
#include <algorithm>
#include <iterator>

#define SUCCESS 0
#define FAILURE (-1)
#define EXIT_FAILURE 1
#define CORRECT_NUM_OF_ARGS 4
#define CLIENT_NAME 1
#define SERVER_IP 2
#define PORT_NUMBER 3



/**
 * a constructor for the client.
 * @param name the client's name.
 * @param serverIP the IP of the server the client requests to connect to.
 * @param port the number of the port to connect to.
 */
whatsappClient::whatsappClient(char *name, char *serverIP, unsigned short port) {

    struct hostent *h;
    clientName = name;
    serverPort = port;

    if ((h = gethostbyname(serverIP)) == nullptr) {
        print_error("gethostbyname", errno);
        exit(1);
    }

    serverIPAddress = inet_ntoa(*((struct in_addr *) h->h_addr));

    if ((clientSocket = call_socket(serverIP, serverPort)) < 0) {  //creating client socket and connecting to server.
        print_error("call_socket", errno);
        exit(EXIT_FAILURE);
    }

    std::string tmp = clientName;
    tmp.append("\n");

    if (write_data(clientSocket, clientName, (int) tmp.size()) < 0) { //sending server client name- first message.
        print_error("write_data", errno);
        exit(EXIT_FAILURE);
    }
}

/**
 * reads data from fd (file descriptor) s into given parameter buf for at most n characters.
 * @param s the fd to read from.
 * @param buf this is where the data that was read is saved.
 * @param n the maximal number of characters to read.
 * @return the number of characters that were read or -1 if an error occurred.
 */
int whatsappClient::read_data(int s, char *buf, int n) {
    int bcount; /* counts bytes read */
    int br; /* bytes read this pass */
    bcount = 0;
    br = 0;
    while (bcount < n) { /* loop until full buffer */
        br = read(s, buf, n - bcount);
        if ((br > 0)) {
            bcount += br;
            if (buf[bcount - 1] == '\0' || buf[bcount - 1] == '\n') {
                break;
            }
            buf += br;
        }
        if (br < 1) {
            return FAILURE;
        }
    }
    return (bcount);
}

/**
 * creates a connection socket between the client and the server.
 * @param hostname the IP of the server the client requests to connect to.
 * @param portNumber the number of the port to connect to.
 * @return the created connection socket number on success, -1 on failure.
 */
int whatsappClient::call_socket(char *hostname, unsigned short portNumber) {
    struct sockaddr_in sa;
    struct hostent *hp;
    int clientSocket;
    if ((hp = gethostbyname(hostname)) == nullptr) {
        print_error("gethostbyname", errno);
        exit(EXIT_FAILURE);
    }
    memset(&sa, 0, sizeof(sa));
    memcpy((char *) &sa.sin_addr, hp->h_addr, (size_t) hp->h_length);
    sa.sin_family = (sa_family_t) hp->h_addrtype;
    sa.sin_port = htons((u_short) portNumber);

    if ((clientSocket = socket(hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        print_error("socket", errno);
        exit(EXIT_FAILURE);
    }

    int res = connect(clientSocket, (struct sockaddr *) &sa, sizeof(sa));
    if (res < 0) {
        close(clientSocket);
        print_error("connect", errno);
        exit(EXIT_FAILURE);
    }

    return clientSocket;
}

/**
 * validates given string is composed only of letters and digits.
 * @param name given string to validate.
 * @return true if string is composed only of letters and digits, false otherwise.
 */
bool validate_name(char* name){
    int i = 0;
    while (name[i]){
        if (!isalpha(name[i]) && !isdigit(name[i])){
            return false;
        }
        i++;
    }
    return true;
}

/**
 * handles a message from the client's stdin.
 * @param line the request given.
 */
void whatsappClient::validate_command(std::string line) {

    std::string name, message;
    command_type type;
    std::vector<std::string> group_members;

    std::string command = line.substr(0, line.size()-1);
    parse_command(command, type, name, message, group_members);

    switch (type) {
        case INVALID: {
            print_invalid_input();
            break;
        }

        case CREATE_GROUP: {

            bool flag = false;
            char * first_member = new char[group_members[0].length() + 1];
            strcpy(first_member, group_members[0].c_str());
            char * name_as_char = new char[name.length() + 1];
            strcpy(name_as_char, name.c_str());

            if (!validate_name(name_as_char)){
                std::cout << "Group name must consist only of letters and digits." << std::endl;
                flag = true;
            }

            for(auto member: group_members){ // if group name is equal to one of clients
                char * member_as_char = new char[member.length() + 1];
                strcpy(member_as_char, member.c_str());
                if (!strcmp(name_as_char, member_as_char)){
                    flag = true;
                }
            }

            if (group_members.empty() || (group_members.size() == 1 && (!strcmp(first_member, clientName))) || flag) { //not according to demands?????
                print_create_group(false, false, clientName, name);
            } else {
                if (write_data(clientSocket, &line[0], (int) line.length()) < 0) {
                    print_error("write_data", errno);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }

        case SEND: {
            char *name_as_char = new char[name.length() + 1];
            strcpy(name_as_char, name.c_str());
            if (name.empty() || (!strcmp(name_as_char, clientName))) { //not according to demands
                print_send(false, false, clientName, name, message);
            } else {
                if (write_data(clientSocket, &line[0], (int) line.length()) < 0) {
                    print_error("write_data", errno);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        }

        case WHO: {
            if (write_data(clientSocket, &line[0], (int) line.length()) < 0) {
                print_error("write_data", errno);
                exit(EXIT_FAILURE);
            }
            break;
        }

        case EXIT: {
            if (write_data(clientSocket, &line[0], (int) line.length()) < 0) {
                print_error("write_data", errno);
                exit(EXIT_FAILURE);
            }
            break;
        }
    }
}

/**
 * runs the client continuously, handling requests.
 */
void whatsappClient::main_run() {

    //after client sent its name in ctor
    char *message = new char[WA_MAX_MESSAGE];
    if (read_data(clientSocket, message, WA_MAX_MESSAGE) < 0) {
        print_error("read_data", errno);
        exit(EXIT_FAILURE);
    }

    if (!strcmp(message, "dup\n")) { //client name already exists
        print_dup_connection();
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    if (strcmp(message, "CONNECTED\n")) { //after sending name server answers something different from connected
        print_fail_connection();
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    print_connection(); //prints connected successfully

    //set of socket descriptors
    int max_fd = std::max(clientSocket, STDIN_FILENO);  //for select

    while (true) {

        fd_set readfds;

        //clear the socket set
        FD_ZERO(&readfds);

        //add master socket to set
        FD_SET(clientSocket, &readfds); //adding client socket to the set
        FD_SET(STDIN_FILENO, &readfds); // adding stdin to the set

        if (select(max_fd + 1, &readfds, nullptr, nullptr, nullptr) < 0) {
            print_error("select", errno);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char *sent_message = new char[WA_MAX_MESSAGE];
            if (read_data(STDIN_FILENO, sent_message, WA_MAX_MESSAGE) < 0) {
                print_error("read_data", errno);
                exit(EXIT_FAILURE);
            }

            validate_command(sent_message);
            continue;
        }

        if (FD_ISSET(clientSocket, &readfds)) {
            char *sent_message = new char[WA_MAX_MESSAGE];
            if (read_data(clientSocket, sent_message, WA_MAX_MESSAGE) < 0) {
                print_error("read_data", errno);
                exit(EXIT_FAILURE);
            }
            if (!strcmp(sent_message, "can_exit\n")) {
                print_exit(false, clientName);
                if (close(clientSocket) < 0){ //closing communicating socket with client.
                    print_error("close", errno);
                    exit(EXIT_FAILURE);
                }
                exit(SUCCESS);
            } else {
                printf(sent_message); //server tells client exactly what to print. std::cout??
            }
        }
    }
}

/**
 * writes data given in parameter buf to socket, for n characters.
 * @param client_socket the socket to write to.
 * @param buf the message to write.
 * @param n the number of characters.
 * @return number of characters that were written or -1 if an error occurred.
 */
int whatsappClient::write_data(int client_socket, char *buf, int n) {

    int bcount = 0;
    int br = 0;

    while (bcount < n) {             /* loop until full buffer */
        if ((br = write(client_socket, buf, n - bcount)) > 0) {
            bcount += br;                /* increment byte counter */
            buf += br;                   /* move buffer ptr for next read */
        }
        if (br < 0) {
            return FAILURE; /* signal an error to the caller */
        }
    }
    return (bcount);
}

/**
 * constructing and running the client.
 * @param argc number of given arguments.
 * @param argv given arguments as array of char*.
 * @return 0 on success, -1 on failure,
 */
int main(int argc, char **argv) {
    if (argc != CORRECT_NUM_OF_ARGS) {
        print_client_usage();
        return FAILURE;
    }

    char *name = argv[CLIENT_NAME];
    char *ipNum = argv[SERVER_IP];

    if (!validate_name(name)){
        std::cout << "Client name must consist only of letters and digits." << std::endl;
        return FAILURE;
    }

    unsigned short portNum = (unsigned short) atoi(argv[PORT_NUMBER]);
    whatsappClient *client = new whatsappClient(name, ipNum, portNum);
    client->main_run();
    return SUCCESS;
}
