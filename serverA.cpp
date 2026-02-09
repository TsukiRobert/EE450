#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

struct Member {
    int userID;
    string username;
    string encryptedPassword;
};

string encryptPassword(const string &plain) {
    string res = plain;
    for (char &c : res) {
        if (c >= '0' && c <= '9') {
            c = '0' + ( (c - '0' + 3) % 10 );
        } else if (c >= 'A' && c <= 'Z') {
            c = 'A' + ( (c - 'A' + 3) % 26 );
        } else if (c >= 'a' && c <= 'z') {
            c = 'a' + ( (c - 'a' + 3) % 26 );
        } 
    }
    return res;
}

int main() {
    int xxx = 273;                       
    int SERVER_A_PORT = 21000 + xxx;

    // create the UDP socket and bind
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_A_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));

    cout << "[Server A] Booting up using UDP on port "
         << SERVER_A_PORT << "." << endl;

    // load members.txt into the membersDB
    vector<Member> membersDB;
    ifstream infile("members.txt");
    int id;
    string user, pass;
    while (infile >> id >> user >> pass) {
        membersDB.push_back({id, user, pass});
    }

    // main loop: handle requests from Server M
    while (true) {
        char buf[1024];
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);

        ssize_t n = recvfrom(sockfd, buf, sizeof(buf)-1, 0, (struct sockaddr*)&clientAddr, &addrLen);
        if (n <= 0) continue;
        buf[n] = '\0';

        // Parse username and password
        string username, password;
        {
            string msg(buf);
            size_t pos = msg.find(' ');
            if (pos != string::npos) {
                username = msg.substr(0, pos);
                password = msg.substr(pos+1);
            }
        }

        cout << "[Server A] Received username " << username
             << " and password ******." << endl;

        string response;

        // Guest login
        if (username == "guest" && password == "123456") {
            cout << "[Server A] Guest has been authenticated." << endl;
            response = "OK GUEST";
        } else {
            // member login
            string enc = encryptPassword(password);
            int foundID = -1;
            for (const auto &m : membersDB) {
                if (m.username == username && m.encryptedPassword == enc) {
                    foundID = m.userID;
                    break;
                }
            }

            if (foundID != -1) {
                cout << "[Server A] Member " << username
                     << " has been authenticated." << endl;
                response = "OK MEMBER " + to_string(foundID);
            } else {
                cout << "[Server A] The username " << username
                     << " or password ****** is incorrect." << endl;
                response = "FAIL";
            }
        }

        sendto(sockfd, response.c_str(), response.size(), 0,
               (struct sockaddr*)&clientAddr, addrLen);
    }

    close(sockfd);
    return 0;
}
