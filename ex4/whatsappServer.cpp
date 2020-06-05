
#include <cstring>
#include <algorithm>
#include "whatsappServer.hpp"

#define SUCCESS 0
#define FAILURE (-1)
#define EXIT_FAILURE 1
#define CORRECT_NUM_OF_ARGS 2
#define PORT_NUMBER 1

/**
 * establishes a socket that listens to given port number (waiting for clients to connect to this port).
 * @param portnumber the number of the port that through it clients request to join.
 * @return the number of the socket that will receive connection requests, or -1 if an error occurred.
 */
int whatsappServer::establish(unsigned short portnumber) {
    char myname[MAXHOSTNAMELEN + 1];
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;
//hostnet initialization
    gethostname(myname, MAXHOSTNAMELEN);
    hp = gethostbyname(myname);
    if (hp == nullptr) {
        print_error("gethostbyname", errno);
        return FAILURE;
    }
//sockaddrr_in initlization
    memset(&sa, 0, sizeof(struct sockaddr_in));
    sa.sin_family = hp->h_addrtype;
/* this is our host address */

    memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
/* this is our port number */
    sa.sin_addr.s_addr = INADDR_ANY;
    sa.sin_port = htons(portnumber);

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        print_error("socket", errno);
        return FAILURE;
    }
    if (bind(s, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
        print_error("bind", errno);
        if (close(s) < 0){
            print_error("close", errno);
        }
        return FAILURE;
    }
    if (listen(s, 10) < 0){ /* max # of queued connects */
        print_error("listen", errno);
        if (close(s) < 0){
            print_error("close", errno);
        }
        return FAILURE;
    }
    return (s);
}

/**
 * reads data from fd (file descriptor) s into given parameter buf for at most n characters.
 * @param s the fd to read from.
 * @param buf this is where the data that was read is saved.
 * @param n the maximal number of characters to read.
 * @return the number of characters that were read or -1 if an error occurred.
 */
int whatsappServer::read_data(int s, char *buf, int n) {
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
 * writes data given in parameter buf to socket, for n characters.
 * @param socket the socket to write to.
 * @param buf the message to write.
 * @param n the number of characters.
 * @return number of characters that were written or -1 if an error occurred.
 */
int whatsappServer::write_data(int socket, char *buf, int n) {

    int bcount = 0;
    int br = 0;
    while (bcount < n) {             /* loop until full buffer */
        if ((br = write(socket, buf, n - bcount)) > 0) {
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
 * connects a client to the server.
 * @param socket_number_server the socket through which was the connection request.
 * @return the socket number connecting the client to the server or -1 if an error occurred.
 */
int whatsappServer::get_connection(int socket_number_server) {
    int server_to_client_socket; /* socket of connection */
    std::string msg_to_send_back = "CONNECTED\n";
    if ((server_to_client_socket = accept(socket_number_server, NULL, NULL)) < 0) {
        print_error("accept", errno);
        return FAILURE;
    }

    //getting client name as the first message from the client.
    char *first_message = new char[WA_MAX_MESSAGE]; //first message from client is its name
    if (read_data(server_to_client_socket, first_message, WA_MAX_MESSAGE) < 0) {
        print_error("read_data", errno);
        return FAILURE;
    }

    for (auto client: clients_list){
        if (!strcmp(client.first, first_message)){
            msg_to_send_back = "dup\n";
            if (write_data(server_to_client_socket, &msg_to_send_back[0], (int) msg_to_send_back.size()) < 0) {
                print_error("write_data", errno);
                return FAILURE;
            }
            return SUCCESS;
        }
    }

    for (auto group: groups){
        if (!strcmp(group.first, first_message)){
            msg_to_send_back = "dup\n";
            if (write_data(server_to_client_socket, &msg_to_send_back[0], (int) msg_to_send_back.size()) < 0) {
                print_error("write_data", errno);
                return FAILURE;
            }
            return SUCCESS;
        }
    }

    clients_list.insert(std::pair<char *, int>(first_message, server_to_client_socket)); // adding new socket to clients map.

    if (write_data(server_to_client_socket, &msg_to_send_back[0], (int) msg_to_send_back.size()) < 0) {
        print_error("write_data", errno);
        return FAILURE;
    }

    print_connection_server(first_message);
    return server_to_client_socket;
}

/**
 * a constructor for the server.
 * @param portNumber the port number of the server.
 */
whatsappServer::whatsappServer(int portNumber) {

    unsigned short portNum = (unsigned short) portNumber;

    serverWaitingSocket = establish(portNum);
    if (serverWaitingSocket < 0) {
        exit(EXIT_FAILURE);//bad port num or problem with establish
    }
}


/**
 * comparator for char*, used to sort vector for the WHO case.
 * @param a first char* to compare.
 * @param b second char* to compare.
 * @return
 */
bool cmp(const char * a,const char * b){
    return strcmp(a, b) < 0;
}

/**
 * handles a message from a client.
 * @param line the message received from a client.
 * @param current_communicating_socket the connection socket with the client.
 */
void whatsappServer::handle_message(std::string line, int current_communicating_socket) {

    std::string name, message, client_name;
    command_type type;
    std::vector<std::string> group_members;

    for (std::pair<std::string, int> client : clients_list) {
        if (client.second == current_communicating_socket) {
            client_name = client.first;
        }
    }

    std::string command = line.substr(0, line.size()-1);
    parse_command(command, type, name, message, group_members);

    char *received_name = new char[client_name.length() + 1];
    strcpy(received_name, client_name.c_str());
    char *name_as_char = new char[name.length() + 1];
    strcpy(name_as_char, name.c_str());

    switch (type) {
        case CREATE_GROUP: {

            //case: name is not unique.
            for (auto client: clients_list){
                if (!strcmp(client.first, name_as_char)){
                    print_create_group(true, false, client_name, name);
                    std::string to_send = "ERROR: failed to create group \"";
                    to_send.append(name);
                    to_send.append("\".\n");
                    if (write_data(current_communicating_socket, &to_send[0], (int) to_send.length()) < 0) {
                        print_error("write_data", errno);
                        return;
                    }
                    return;
                }
            }
            for (auto group: groups){
                if (!strcmp(group.first, name_as_char)){
                    print_create_group(true, false, client_name, name);
                    std::string to_send = "ERROR: failed to create group \"";
                    to_send.append(name);
                    to_send.append("\".\n");
                    if (write_data(current_communicating_socket, &to_send[0], (int) to_send.length()) < 0) {
                        print_error("write_data", errno);
                        return;
                    }
                    return;
                }
            }

            //case : group name is unique but not all given group members are actual clients.
            bool all_members_are_valid = true;
            for (auto member: group_members){
                for (auto client: clients_list){
                    char *mem_as_char = new char[member.length() + 1];
                    strcpy(mem_as_char, member.c_str());
                    if (!strcmp(client.first, mem_as_char)){
                        all_members_are_valid = true;
                        break;
                    }
                    all_members_are_valid = false;
                    delete mem_as_char;
                }
            }
            if (!all_members_are_valid){
                print_create_group(true, false, client_name, name);
                std::string to_send = "ERROR: failed to create group \"";
                to_send.append(name);
                to_send.append("\".\n");
                if (write_data(current_communicating_socket, &to_send[0], (int) to_send.length()) < 0) {
                    print_error("write_data", errno);
                    return;
                }
                return;
            }


            //case : group name is unique and all given group members are actual clients.
            bool flag = true;
            std::set<char*> members_set = {};
            members_set.insert(received_name);

            for (std::string element:group_members) {
                char *member = new char[element.length() + 1];
                strcpy(member, element.c_str());
                for (auto client: members_set){
                    if (!strcmp(client, member)){
                        flag = false;
                        break;
                    }
                }
                if (flag) {
                    members_set.insert(member);
                }
                flag = true;
            }

            groups.insert(std::pair<char*, std::set<char*>>(name_as_char, members_set)); //add to groups map

            print_create_group(true, true, client_name, name);

            std::string to_send = "Group \"";
            to_send.append(name);
            to_send.append("\" was created successfully.\n");

            if (write_data(current_communicating_socket, &to_send[0], (int) to_send.length()) < 0) { //sending client what to print
                print_error("write_data", errno);
                return;
            }
            break;
        }

        case SEND: {

            int id;
            std::set<char*> selected_group;
            bool is_client = false, is_group = false;
            for (auto client : clients_list) {
                if (!strcmp(client.first, name_as_char)) {
                    is_client = true;
                    id = client.second;
                }
            }
            for (auto group : groups) { //include checking if sender is in group
                if (!strcmp(group.first, name_as_char)) {
                    for (auto member: group.second){
                        if (!strcmp(received_name, member)){
                            is_group = true;
                            selected_group = group.second;
                            break;
                        }
                    }
                    break;
                }
            }

            //case : sending message to a client
            if (is_client) {
                std::string message_to_send_client = client_name;
                message_to_send_client.append(": ");
                message_to_send_client.append(message);
                message_to_send_client.append("\n");

                if (write_data(id, &message_to_send_client[0], (int) message_to_send_client.length()) < 0) { //send message to actual client
                    print_error("write_data", errno);
                    return;
                }

                print_send(true, true, client_name, name, message); //print server success message

                std::string message_to_send_back = "Sent successfully.\n"; //sending back to client that the send succeeded
                if (write_data(current_communicating_socket, &message_to_send_back[0], (int) message_to_send_back.length()) < 0) { //sending back to client that the send succeeded
                    print_error("write_data", errno);
                    return;
                }
                return;
            }

            //case: sending a message to a group
            if (is_group) {

                std::string message_to_send_client = client_name;
                message_to_send_client.append(": ");
                message_to_send_client.append(message);
                message_to_send_client.append("\n");

                for (auto member: selected_group){
                    if (!strcmp(received_name, member)){
                        continue;
                    }
                    for (auto client : clients_list) {
                        if (!strcmp(client.first, member)) {
                            id = client.second;
                            if (write_data(id, &message_to_send_client[0], (int) message_to_send_client.length()) < 0) { //send message to actual client
                                print_error("write_data", errno);
                                return;
                            }
                            break;
                        }
                    }
                }

                print_send(true, true, client_name, name, message); //print server success message

                std::string message_to_send_back = "Sent successfully.\n"; //sending back to client that the send succeeded
                if (write_data(current_communicating_socket, &message_to_send_back[0], (int) message_to_send_back.length()) < 0) { //sending back to client that the send succeeded
                    print_error("write_data", errno);
                    return;
                }
                return;
            }

            //case: name is not group nor client name or sender is not in group
            print_send(true, false, client_name, name, message); //print server error message

            std::string message_to_send_back = "ERROR: failed to send.\n"; //sending back to client that send did not succeeded
            if (write_data(current_communicating_socket, &message_to_send_back[0], (int) message_to_send_back.length()) < 0) { //sending back to client that there is an error
                print_error("write_data", errno);
                return;
            }
            break;
        }

        case WHO: {

            std::vector<char*> sorted_clients;
            std::map<char *, int>::iterator sorted_clients_iterator;
            std::string who_msg_to_send = "";

            for (sorted_clients_iterator = clients_list.begin(); sorted_clients_iterator != clients_list.end(); ++sorted_clients_iterator) {
                sorted_clients.emplace_back((*sorted_clients_iterator).first);
            }

            std::sort(sorted_clients.begin(), sorted_clients.end(), cmp);

            bool first = true;
            for (auto client: sorted_clients) {
                if (!first) {
                    who_msg_to_send.append(",");
                }
                who_msg_to_send.append(client);
                first = false;
            }
            who_msg_to_send.append("\n");

            if (write_data(current_communicating_socket, &who_msg_to_send[0], (int) who_msg_to_send.length()) < 0) { //sending back to client
                print_error("write_data", errno);
                return;
            }

            print_who_server(client_name);
            break;
        }

        case EXIT: {

            //removing client from all groups
            for (auto &group: groups) {
                //for each group check if client in group
                for (auto member: group.second){
                    if (!strcmp(member, received_name)){
                        group.second.erase(member);
                        break;
                    }
                }
            }

            //removing client from clients list
            for (std::pair<char*, int> client : clients_list) {
                if (client.second == current_communicating_socket) {
                    clients_list.erase(client.first);
                    break;
                }
            }

            std::string exit_message = "can_exit\n"; //telling the client to exit
            if (write_data(current_communicating_socket, &exit_message[0], (int) exit_message.length()) < 0) {
                print_error("write_data", errno);
            }

            print_exit(true, client_name);
            break;
        }

        case INVALID: {
            print_invalid_input(); // should never happen
        }
    }
}

/**
 * runs the server continuously, handling connection requests and client requests.
 */
void whatsappServer::main_run() {
    fd_set read_fds;
    int max_fd;
    while (true) {

        FD_ZERO(&read_fds);
        FD_SET(serverWaitingSocket, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        max_fd = serverWaitingSocket;
        for (auto client : clients_list) {
            FD_SET(client.second, &read_fds);
            if (client.second > max_fd) {
                max_fd = client.second;
            }
        }

        int sel_res = select(max_fd + 1, &read_fds, nullptr, nullptr, nullptr);
        if (sel_res < 0) {
            print_error("select", errno);
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(serverWaitingSocket, &read_fds)) {
            if (get_connection(serverWaitingSocket) < 0) {
                print_error("get_connection", errno);
                continue;
            }
            continue;
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char *sent_message = new char[WA_MAX_MESSAGE];
            if (read_data(STDIN_FILENO, sent_message, WA_MAX_MESSAGE) < 0) {
                print_error("read_data", errno);
                continue;
            }

            if (!strcmp(sent_message, "EXIT\n")) {
                std::string exit_message = "can_exit\n"; //telling the client to exit
                for (auto client: clients_list) {
                    print_exit(true, client.first);
                    if (write_data(client.second, &exit_message[0], (int) exit_message.length()) < 0) {
                        print_error("write_data", errno);
                    }
                }
                print_exit();
                exit(SUCCESS);
            }
            else {
                print_invalid_input();
            }
            continue;
        }

        for (auto client: clients_list) {
            if (FD_ISSET(client.second, &read_fds)) {
                char *sent_message = new char[WA_MAX_MESSAGE];
                if (read_data(client.second, sent_message, WA_MAX_MESSAGE) < 0) {
                    print_error("read_data", errno);
                    continue;
                }

                handle_message(sent_message, client.second);
                break;
            }
        }
    }
}

/**
 * constructing and running the server.
 * @param argc number of given arguments.
 * @param argv given arguments as array of char*.
 * @return 0 on success, -1 on failure,
 */
int main(int argc, char **argv) {

    if (argc != CORRECT_NUM_OF_ARGS) {
        print_server_usage();
        return FAILURE;
    }

    whatsappServer *server = new whatsappServer(atoi(argv[PORT_NUMBER]));
    server->main_run();
    return SUCCESS;
}
