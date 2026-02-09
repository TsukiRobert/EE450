#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <cctype>      
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

// to make sure the client input is a real valid space
static bool isValidSpaceId(const string &id) {
    if (id.size() != 4) return false;
    if (id[0] != 'U' && id[0] != 'H') return false;
    for (int i = 1; i < 4; ++i) {
        if (!isdigit(static_cast<unsigned char>(id[i]))) return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    int xxx = 273;                    
    int SERVER_M_TCP = 25000 + xxx;   

    if (argc != 3) {
        cerr << "Usage: ./client <username> <password>" << endl;
        return 1;
    }

    string username = argv[1];
    string password = argv[2];

    cout << "Client is up and running." << endl;

    // Create TCP socket and connect to Server M
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_M_TCP);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    // Authentication Part (Part 2A)
    cout << username << " sent an authentication request to the main server."
         << endl;

    string authMsg = username + " " + password;
    send(sockfd, authMsg.c_str(), authMsg.size(), 0);

    char buf[2048];
    ssize_t n = recv(sockfd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) {
        cerr << "Connection closed by server during authentication." << endl;
        close(sockfd);
        return 1;
    }
    buf[n] = '\0';
    string authResp(buf);

    bool isGuest  = false;
    bool isMember = false;

    if (authResp == "OK MEMBER") {
        cout << username << " received the authentication result." << endl;
        cout << "Authentication successful." << endl;
        isMember = true;
    } else if (authResp == "OK GUEST") {
        cout << "You have been granted guest access." << endl;
        isGuest = true;
    } else {
        cout << "Authentication failed: username or password is incorrect."
             << endl;
        close(sockfd);
        return 0;
    }

    while (true) {
        string line;
        cout << "> ";
        getline(cin, line);
        if (line.empty()) continue;

        // help function
        if (line == "help") {
            if (isGuest) {
                cout
                    << "Please enter the command: <search <parking lot>>, <quit>"
                    << endl;
            } else {
                cout
                    << "Please enter the command: <search <parking lot>>, "
                    << "<reserve <space> <timeslots>>, <lookup>, "
                    << "<cancel <space> <timeslots>>, <quit>"
                    << endl;
            }
            continue;
        }

        // quit function
        if (line == "quit") {
            send(sockfd, line.c_str(), line.size(), 0);
            break;
        }

        // Input validation for all commands before sending to server M
        bool isSearch   = false;
        bool isLookup   = false;
        bool isReserve  = false;
        bool isCancel   = false;
        bool shouldSend = true;   // this one to help me process some errors before sending to the server M 

        stringstream ss(line);
        string cmd;
        ss >> cmd;

        if (cmd == "search") {
            isSearch = true;
            string lot;
            ss >> lot;

            if (!lot.empty() && lot != "UPC" && lot != "HSC") {
                cout
                    << "Error: invalid parking lot name. Please enter \"UPC\" or \"HSC\"."
                    << endl;
                cout << "---Start a new request---" << endl;
                shouldSend = false;
            }
        } else if (cmd == "lookup") {
            isLookup = true;
        } else if (cmd == "reserve") {
            isReserve = true;

            bool hasSpace = false;
            bool hasTime  = false;

            string space;
            int ts;

            if (ss >> space) {
                hasSpace = true;
            }

            if (ss >> ts) {
                hasTime = true;
            }

            // First validation checks: no time slots or space ID in the command
            if (!hasSpace || !hasTime) {
                cout
                    << "Error: Space code and timeslot(s) are required. "
                    << "Please specify a space code and at least one timeslot."
                    << endl;
                cout << "---Start a new request---" << endl;
                shouldSend = false;
            }
            // second validation check: the id is not valid
            else if (!isValidSpaceId(space)) {
                cout
                    << "Error: invalid space ID. Please enter a valid space ID "
                    << "U/H plus 3 digits (check by search function)"
                    << endl;
                cout << "---Start a new request---" << endl;
                shouldSend = false;
            }
        } else if (cmd == "cancel") {
            isCancel = true;
        } else {
        }

        // member only function (lookup)
        if (isLookup && shouldSend) {
            if (isGuest) {
                cout
                    << "Guests can only check availability. Please log in as a member for full access."
                    << endl;
                cout << "---Start a new request---" << endl;
                shouldSend = false;
            }
        }

        // reserve / cancel: member only functions
        if ((isReserve || isCancel) && shouldSend && isGuest) {
            cout
                << "Guests cannot reserve/cancel. Please log in as a member."
                << endl;
            cout << "---Start a new request---" << endl;
            shouldSend = false;
        }

        if (!shouldSend) {
            continue;
        }

        // print onscreen message
        if (isSearch) {
            cout << username
                 << " sent an availability request to the main server."
                 << endl;
        } else if (isLookup) {
            cout << username
                 << " sent a lookup request to the main server."
                 << endl;
        } else if (isReserve) {
            cout << username
                 << " sent a reservation request to the main server."
                 << endl;
        } else if (isCancel) {
            cout << username
                 << " sent a cancellation request to the main server."
                 << endl;
        }

        // Sending command to server M
        send(sockfd, line.c_str(), line.size(), 0);

        //Response from server M
        char rBuf[8192];
        ssize_t rn = recv(sockfd, rBuf, sizeof(rBuf) - 1, 0);
        if (rn <= 0) {
            cout << "Connection closed by server." << endl;
            break;
        }
        rBuf[rn] = '\0';
        string serverReply(rBuf);

        cout << "The client received the response from the main server "
             << "using TCP over port " << SERVER_M_TCP << "."
             << endl;

        // When parking lot is empty (no avaiable case)
        if (serverReply.rfind("EMPTY ", 0) == 0) {
            stringstream es(serverReply);
            string tag, lotName;
            es >> tag >> lotName;  

            cout << "No available spaces in " << lotName << "." << endl;
            cout << "---Start a new request---" << endl;
        } else {
            cout << serverReply << endl;
        }

        // Partial reserveration case
        if (serverReply.find("Do you want to reserve the remaining slots? (Y/N):")
            != string::npos) {

            string yn;
            while (true) {
                cout << "Enter Y or N: ";
                getline(cin, yn);
                if (yn.empty()) continue;
                char c = yn[0];
                if (c == 'Y' || c == 'y' || c == 'N' || c == 'n') break;
                cout << "Please enter Y or N." << endl;
            }

            send(sockfd, yn.c_str(), yn.size(), 0);

            rn = recv(sockfd, rBuf, sizeof(rBuf) - 1, 0);
            if (rn <= 0) {
                cout << "Connection closed by server." << endl;
                break;
            }
            rBuf[rn] = '\0';
            string finalReply(rBuf);

            cout << "The client received the response from the main server "
                 << "using TCP over port " << SERVER_M_TCP << "."
                 << endl;

            cout << finalReply << endl;
        }

    }
    close(sockfd);
    return 0;
}
