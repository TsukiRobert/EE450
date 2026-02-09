#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

struct Space {
    string id;  
    int slots[12];  
};

// get rid of white space 
static string trim(const string &s) {
    size_t start = 0;
    while (start < s.size() && isspace((unsigned char)s[start])) {
        start++;
    }
    size_t end = s.size();
    while (end > start && isspace((unsigned char)s[end - 1])) {
        end--;
    }
    return s.substr(start, end - start);
}

static Space* findSpace(vector<Space> &spaces, const string &id) {
    for (auto &sp : spaces) {
        if (sp.id == id) return &sp;
    }
    return nullptr;
}


int main() {
    int xxx = 273;                   
    int SERVER_R_PORT = 22000 + xxx;

    // load spaces from spaces.txt
    vector<Space> spaces;
    {
        ifstream infile("spaces.txt");
        if (!infile.is_open()) {
            cerr << "Failed to open spaces.txt" << endl;
            return 1;
        }
        while (true) {
            Space s;
            infile >> s.id;
            if (!infile.good()) break;
            for (int i = 0; i < 12; ++i) {
                infile >> s.slots[i];
            }
            spaces.push_back(s);
        }
    }

    // create UDP socket and bind
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in myAddr{};
    myAddr.sin_family = AF_INET;
    myAddr.sin_port   = htons(SERVER_R_PORT);
    myAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(myAddr.sin_zero, 0, sizeof(myAddr.sin_zero));

    if (bind(sockfd, (struct sockaddr*)&myAddr, sizeof(myAddr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }

    cout << "Server R is up and running using UDP on port "
         << SERVER_R_PORT << "." << endl;


    // handle requests from Server M
    while (true) {
        char buf[4096];
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);

        ssize_t n = recvfrom(sockfd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr*)&fromAddr, &fromLen);
        if (n <= 0) continue;
        buf[n] = '\0';
        string msg = trim(string(buf));

        stringstream ss(msg);
        string cmd;
        ss >> cmd;

        // Search function
        if (cmd == "SEARCH") {
            string lot;
            ss >> lot;  // HPC or UPC or empty or All

            if (lot.empty()) lot = "ALL";

            cout << "Server R received an availability request from the main server."
                 << endl;

            string response;
            string lotNameForEmpty = "ALL";

            for (const auto &sp : spaces) {
                char lotChar = sp.id.empty() ? '?' : sp.id[0];

                if (lot == "UPC" && lotChar != 'U') continue;
                if (lot == "HSC" && lotChar != 'H') continue;

                if (lot == "UPC") lotNameForEmpty = "UPC";
                else if (lot == "HSC") lotNameForEmpty = "HSC";

                vector<int> freeSlots;
                for (int i = 0; i < 12; ++i) {
                    if (sp.slots[i] == 0) freeSlots.push_back(i + 1);
                }
                if (freeSlots.empty()) continue;

                response += sp.id + ": Time slot(s) ";
                for (int s : freeSlots) {
                    response += to_string(s) + " ";
                }
                response += "\n";
            }

            if (response.empty()) {
                response = "No available spaces in " + lotNameForEmpty + "\n ---Start a new request---";
            }

            cout << "Server R finished sending the response to the main server."
                 << endl;

            sendto(sockfd, response.c_str(), response.size(), 0,
                   (struct sockaddr*)&fromAddr, fromLen);
        }

        // Lookup function
        else if (cmd == "LOOKUP") {
            int userID;
            ss >> userID;

            cout << "Server R received a lookup request from the main server."
                 << endl;

            string response;
            bool any = false;

            for (const auto &sp : spaces) {
                vector<int> mine;
                for (int i = 0; i < 12; ++i) {
                    if (sp.slots[i] == userID) mine.push_back(i + 1);
                }
                if (!mine.empty()) {
                    any = true;
                    response += sp.id + ": Time slot(s) ";
                    for (int s : mine) response += to_string(s) + " ";
                    response += "\n";
                }
            }

            if (!any) response = "You have no current reservations\n ---Start a new request---";

            cout << "Server R finished sending the reservation information to the main server."
                 << endl;

            sendto(sockfd, response.c_str(), response.size(), 0,
                   (struct sockaddr*)&fromAddr, fromLen);
        }

        // Reserve function
        else if (cmd == "RESERVE") {
            int userID;
            string username;
            string spaceID;

            ss >> userID >> username >> spaceID;

            vector<int> requested;
            int slot;
            while (ss >> slot) requested.push_back(slot);

            cout << "Server R received a reservation request from the main server."
                 << endl;

            Space *sp = findSpace(spaces, spaceID);
            if (!sp) {
                string resp = "RESERVE_NONE " + spaceID + " " + username + " 0";
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
                continue;
            }

            vector<int> available, unavailable;
            for (int s : requested) {
                if (s < 1 || s > 12) {
                    unavailable.push_back(s);
                } else if (sp->slots[s-1] == 0) {
                    available.push_back(s);
                } else {
                    unavailable.push_back(s);
                }
            }

            // Case: all requested slots are available
            if (!requested.empty() && unavailable.empty() && available.size() == requested.size()) {

                cout << "All requested time slots are available." << endl;

                for (int s : available) {
                    sp->slots[s-1] = userID;
                }

                cout << "Successfully reserved " << spaceID
                     << " at time slot(s) ";
                for (int s : available) cout << s << " ";
                cout << "for " << username << "." << endl;

                stringstream out;
                out << "RESERVE_OK " << spaceID << " " << username
                    << " " << available.size();
                for (int s : available) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            }
            // Case: no remaining available slots
            else if (available.empty()) {
                cout << "Time slot(s) ";
                for (int s : unavailable) cout << s << " ";
                cout << "not available for " << spaceID << "." << endl;

                stringstream out;
                out << "RESERVE_NONE " << spaceID << " " << username
                    << " " << unavailable.size();
                for (int s : unavailable) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            }
            // Case: some available, some unavailable 
            else {
                cout << "Time slot(s) ";
                for (int s : unavailable) cout << s << " ";
                cout << "not available for " << spaceID << "." << endl;
                cout << "Requesting to reserve rest available slots (Y/N)."
                     << endl;

                stringstream out;
                out << "RESERVE_PARTIAL " << spaceID << " " << username
                    << " " << userID
                    << " " << available.size();
                for (int s : available) out << " " << s;
                out << " " << unavailable.size();
                for (int s : unavailable) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            }
        }

        //Confirmation for the reservation
        else if (cmd == "CONFIRM") {
            char answer;
            ss >> answer;
            int userID;
            string username, spaceID;
            ss >> userID >> username >> spaceID;

            if (answer == 'Y' || answer == 'y') {
                cout << "User confirmed partial reservation." << endl;

                Space *sp = findSpace(spaces, spaceID);
                vector<int> toReserve;
                int slot;
                while (ss >> slot) toReserve.push_back(slot);

                if (sp) {
                    for (int s : toReserve) {
                        if (s >= 1 && s <= 12) {
                            sp->slots[s-1] = userID;
                        }
                    }
                }

                cout << "Successfully reserved " << spaceID
                     << " at time slot(s) ";
                for (int s : toReserve) cout << s << " ";
                cout << "for " << username << "." << endl;

                stringstream out;
                out << "RESERVE_OK " << spaceID << " " << username
                    << " " << toReserve.size();
                for (int s : toReserve) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            } else {
                cout << "Reservation cancelled." << endl;

                stringstream out;
                out << "RESERVE_FAIL " << spaceID << " " << username;
                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            }
        }

        // Cancel Function
        else if (cmd == "CANCEL") {
            int userID;
            string username;
            string spaceID;
            ss >> userID >> username >> spaceID;

            vector<int> requested;
            int slot;
            while (ss >> slot) requested.push_back(slot);

            cout << "Server R received a cancellation request from the main server."
                 << endl;

            Space *sp = findSpace(spaces, spaceID);
            if (!sp) {
                // No such space, treat as failure
                cout << "No reservation found for " << username
                     << " at " << spaceID << " time slot(s) ";
                for (int s : requested) cout << s << " ";
                cout << "." << endl;

                stringstream out;
                out << "CANCEL_FAIL " << spaceID << " " << username
                    << " " << requested.size();
                for (int s : requested) out << " " << s;
                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
                continue;
            }

            vector<int> badSlots;
            for (int s : requested) {
                if (s < 1 || s > 12 || sp->slots[s-1] != userID) {
                    badSlots.push_back(s);
                }
            }

            if (!badSlots.empty()) {
                cout << "No reservation found for " << username
                     << " at " << spaceID << " time slot(s) ";
                for (int s : badSlots) cout << s << " ";
                cout << "." << endl;

                stringstream out;
                out << "CANCEL_FAIL " << spaceID << " " << username
                    << " " << badSlots.size();
                for (int s : badSlots) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            } else {
                for (int s : requested) {
                    if (s >= 1 && s <= 12) sp->slots[s-1] = 0;
                }

                cout << "Successfully cancelled reservation for " << spaceID
                     << " at time slot(s) ";
                for (int s : requested) cout << s << " ";
                cout << "for " << username << "." << endl;

                stringstream out;
                out << "CANCEL_OK " << spaceID << " " << username
                    << " " << requested.size();
                for (int s : requested) out << " " << s;

                string resp = out.str();
                sendto(sockfd, resp.c_str(), resp.size(), 0,
                       (struct sockaddr*)&fromAddr, fromLen);
            }
        }
    }
    close(sockfd);
    return 0;
}
