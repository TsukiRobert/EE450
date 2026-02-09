#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main() {
    int xxx = 273;  

    int SERVER_M_UDP = 24000 + xxx; 
    int SERVER_M_TCP = 25000 + xxx;  
    int SERVER_A_PORT = 21000 + xxx; 
    int SERVER_R_PORT = 22000 + xxx; 
    int SERVER_P_PORT = 23000 + xxx; 

    // Create UDP socket
    int udpSock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSock < 0) {
        perror("udp socket");
        return 1;
    }

    sockaddr_in udpAddr{};
    udpAddr.sin_family = AF_INET;
    udpAddr.sin_port   = htons(SERVER_M_UDP);
    udpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(udpAddr.sin_zero, 0, sizeof(udpAddr.sin_zero));

    if (bind(udpSock, (struct sockaddr*)&udpAddr, sizeof(udpAddr)) < 0) {
        perror("bind udp");
        close(udpSock);
        return 1;
    }

    cout << "[Server M] Booting up using UDP on port "
         << SERVER_M_UDP << "." << endl;

    // Address of Server A
    sockaddr_in serverAAddr{};
    serverAAddr.sin_family = AF_INET;
    serverAAddr.sin_port   = htons(SERVER_A_PORT);
    serverAAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t serverAAddrLen = sizeof(serverAAddr);

    // Address of Server R
    sockaddr_in serverRAddr{};
    serverRAddr.sin_family = AF_INET;
    serverRAddr.sin_port   = htons(SERVER_R_PORT);
    serverRAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t serverRAddrLen = sizeof(serverRAddr);

    // Address of Server P
    sockaddr_in serverPAddr{};
    serverPAddr.sin_family = AF_INET;
    serverPAddr.sin_port   = htons(SERVER_P_PORT);
    serverPAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t serverPAddrLen = sizeof(serverPAddr);

    // Create TCP socket for clients
    int tcpSock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSock < 0) {
        perror("tcp socket");
        close(udpSock);
        return 1;
    }

    sockaddr_in tcpAddr{};
    tcpAddr.sin_family = AF_INET;
    tcpAddr.sin_port   = htons(SERVER_M_TCP);
    tcpAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(tcpAddr.sin_zero, 0, sizeof(tcpAddr.sin_zero));

    if (bind(tcpSock, (struct sockaddr*)&tcpAddr, sizeof(tcpAddr)) < 0) {
        perror("bind tcp");
        close(udpSock);
        close(tcpSock);
        return 1;
    }

    if (listen(tcpSock, 5) < 0) {
        perror("listen");
        close(udpSock);
        close(tcpSock);
        return 1;
    }

    while (true) {
        int clientSock = accept(tcpSock, nullptr, nullptr);
        if (clientSock < 0) continue;

        // Authentication 
        char buf[2048];
        ssize_t n = recv(clientSock, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            close(clientSock);
            continue;
        }
        buf[n] = '\0';
        string authMsg(buf);

        string username, password;
        {
            size_t pos = authMsg.find(' ');
            if (pos != string::npos) {
                username = authMsg.substr(0, pos);
                password = authMsg.substr(pos+1);
            } else {
                username = authMsg;
            }
        }

        cout << "Server M received username " << username
             << " and password ******." << endl;

        // UDP to server A
        cout << "Server M sent the authentication request to Server A."
             << endl;

        sendto(udpSock, authMsg.c_str(), authMsg.size(), 0,
               (struct sockaddr*)&serverAAddr, serverAAddrLen);

        // Receive reply from A
        char udpBuf[1024];
        sockaddr_in fromAddr{};
        socklen_t fromLen = sizeof(fromAddr);
        ssize_t rn = recvfrom(udpSock, udpBuf, sizeof(udpBuf)-1, 0,
                              (struct sockaddr*)&fromAddr, &fromLen);
        if (rn <= 0) {
            close(clientSock);
            continue;
        }
        udpBuf[rn] = '\0';
        string authResp(udpBuf);

        cout << "Server M received the response from Server A using UDP over port "
             << SERVER_M_UDP << "." << endl;

        bool isGuest  = false;
        bool isMember = false;
        int  userID   = -1;
        string toClient;

        if (authResp.rfind("OK GUEST", 0) == 0) {
            isGuest = true;
            toClient = "OK GUEST";
        } else if (authResp.rfind("OK MEMBER", 0) == 0) {
            isMember = true;
            toClient = "OK MEMBER";
            size_t p = authResp.find(' ', 9);
            if (p != string::npos) {
                userID = stoi(authResp.substr(p+1));
            }
        } else {
            toClient = "FAIL";
        }

        cout << "Server M sent the response to the client using TCP over port "
             << SERVER_M_TCP << "." << endl;

        send(clientSock, toClient.c_str(), toClient.size(), 0);

        // Authentication failed
        if (!isGuest && !isMember) {
            close(clientSock);
            continue;
        }

        while (true) {
            char cmdBuf[4096];
            ssize_t cn = recv(clientSock, cmdBuf, sizeof(cmdBuf)-1, 0);
            if (cn <= 0) break;
            cmdBuf[cn] = '\0';

            string command(cmdBuf);
            if (command == "quit") {
                break;
            }

            // search function
            if (command.rfind("search", 0) == 0) {
                stringstream ss(command);
                string cmd, lotToken;
                ss >> cmd >> lotToken;
                string lotArg = "ALL";
                if (lotToken == "UPC") lotArg = "UPC";
                else if (lotToken == "HSC") lotArg = "HSC";

                cout << "Server M received an availability request from "
                     << (isGuest ? "Guest" : username)
                     << " using TCP over port " << SERVER_M_TCP << "."
                     << endl;

                // Send search to server R
                string msgToR = "SEARCH " + lotArg;

                cout << "Server M sent the availability request to Server R."
                     << endl;

                sendto(udpSock, msgToR.c_str(), msgToR.size(), 0,
                       (struct sockaddr*)&serverRAddr, serverRAddrLen);

                char rBuf[65536];
                sockaddr_in rAddr{};
                socklen_t rLen = sizeof(rAddr);
                ssize_t sr = recvfrom(udpSock, rBuf, sizeof(rBuf)-1, 0,
                                      (struct sockaddr*)&rAddr, &rLen);
                if (sr <= 0) continue;
                rBuf[sr] = '\0';
                string rResp(rBuf);

                cout << "Server M received the response from Server R using UDP over port "
                     << SERVER_M_UDP << "." << endl;
                cout << "Server M sent the availability information to the client."
                     << endl;

                send(clientSock, rBuf, sr, 0);
                continue;
            }

            // Look up function
            if (command == "lookup") {
                if (!isMember) {
                    string msg = "Guest does not have lookup privilege. Please log in as a member.";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                    continue;
                }

                cout << "Server M received a lookup request from "
                     << username << " using TCP over port "
                     << SERVER_M_TCP << "." << endl;

                // forward to R
                string msgToR = "LOOKUP " + to_string(userID);

                cout << "Server M sent the lookup request to Server R."
                     << endl;

                sendto(udpSock, msgToR.c_str(), msgToR.size(), 0,
                       (struct sockaddr*)&serverRAddr, serverRAddrLen);

                char rBuf[4096];
                sockaddr_in rAddr{};
                socklen_t rLen = sizeof(rAddr);
                ssize_t sr = recvfrom(udpSock, rBuf, sizeof(rBuf)-1, 0,
                                      (struct sockaddr*)&rAddr, &rLen);
                if (sr <= 0) continue;
                rBuf[sr] = '\0';

                cout << "Server M received the response from Server R using UDP over port "
                     << SERVER_M_UDP << "." << endl;
                cout << "Server M sent the lookup result to the client."
                     << endl;

                send(clientSock, rBuf, sr, 0);
                continue;
            }

            // Reserve function
            if (command.rfind("reserve ", 0) == 0) {
                if (!isMember) {
                    string msg = "Guests cannot reserve. Please log in as a member.";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                    continue;
                }

                stringstream ss(command);
                string cmd, spaceID;
                ss >> cmd >> spaceID;
                vector<int> reqSlots;
                int s;
                while (ss >> s) reqSlots.push_back(s);
                if (spaceID.empty() || reqSlots.empty()) {
                    string msg = "Please provide valid space ID and time slots to reserve.";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                    continue;
                }

                cout << "Server M received a reservation request from "
                     << username << " using TCP over port "
                     << SERVER_M_TCP << "." << endl;

                // Formulate the message to R
                stringstream out;
                out << "RESERVE " << userID << " " << username << " "
                    << spaceID;
                for (int x : reqSlots) out << " " << x;
                string msgToR = out.str();

                cout << "Server M sent the reservation request to Server R."
                     << endl;

                sendto(udpSock, msgToR.c_str(), msgToR.size(), 0,
                       (struct sockaddr*)&serverRAddr, serverRAddrLen);

                char rBuf[4096];
                sockaddr_in rAddr{};
                socklen_t rLen = sizeof(rAddr);
                ssize_t sr = recvfrom(udpSock, rBuf, sizeof(rBuf)-1, 0,
                                      (struct sockaddr*)&rAddr, &rLen);
                if (sr <= 0) continue;
                rBuf[sr] = '\0';
                string rResp(rBuf);

                cout << "Server M received the response from Server R using UDP over port "
                     << SERVER_M_UDP << "." << endl;

                stringstream rs(rResp);
                string tag;
                rs >> tag;

                // Reservation successful
                if (tag == "RESERVE_OK") {
                    string space, uname;
                    int k;
                    rs >> space >> uname >> k;
                    vector<int> finalSlots;
                    int t;
                    while (rs >> t) finalSlots.push_back(t);

                    // Ask server P for price
                    stringstream pReq;
                    pReq << "PRICE_RESERVE " << username << " " << space
                         << " " << k;
                    for (int x : finalSlots) pReq << " " << x;

                    sendto(udpSock, pReq.str().c_str(), pReq.str().size(), 0,
                           (struct sockaddr*)&serverPAddr, serverPAddrLen);

                    char pBuf[1024];
                    sockaddr_in pAddr{};
                    socklen_t pLen = sizeof(pAddr);
                    ssize_t pr = recvfrom(udpSock, pBuf, sizeof(pBuf)-1, 0,
                                          (struct sockaddr*)&pAddr, &pLen);
                    if (pr <= 0) continue;
                    pBuf[pr] = '\0';
                    string pResp(pBuf); 

                    cout << "Server M received the pricing information from Server P using UDP over port "
                         << SERVER_M_UDP << "." << endl;
                    cout << "Server M sent the reservation result to the client."
                         << endl;

                    // Reply to the client with the final message
                    stringstream toCli;
                    toCli << "Reservation successful for " << space
                          << " at time slot(s) ";
                    for (int x : finalSlots) toCli << x << " ";
                    toCli << "\n" << pResp << "\n"
                          << "---Start a new request---\n";

                    string finalMsg = toCli.str();
                    send(clientSock, finalMsg.c_str(), finalMsg.size(), 0);
                }
                // Reservation partial success (the special case)
                else if (tag == "RESERVE_PARTIAL") {
                    string space, uname;
                    int user, nAvail, nUnavail;
                    rs >> space >> uname >> user >> nAvail;

                    vector<int> availSlots(nAvail);
                    for (int i = 0; i < nAvail; ++i) rs >> availSlots[i];
                    rs >> nUnavail;
                    vector<int> unavailSlots(nUnavail);
                    for (int i = 0; i < nUnavail; ++i) rs >> unavailSlots[i];

                    stringstream prompt;
                    prompt << "Time slot(s) ";
                    for (int x : unavailSlots) prompt << x << " ";
                    prompt << "not available. Do you want to reserve the remaining slots? (Y/N):";

                    string promptStr = prompt.str();
                    cout << "Server M sent the partial reservation confirmation request to the client."
                         << endl;
                    send(clientSock, promptStr.c_str(), promptStr.size(), 0);

                    char ynBuf[32];
                    ssize_t ynr = recv(clientSock, ynBuf, sizeof(ynBuf)-1, 0);
                    if (ynr <= 0) break;
                    ynBuf[ynr] = '\0';
                    char answer = ynBuf[0];

                    // Forward confirmation to R
                    stringstream cf;
                    cf << "CONFIRM " << answer << " " << userID << " "
                       << username << " " << space;
                    if (answer == 'Y' || answer == 'y') {
                        for (int x : availSlots) cf << " " << x;
                    }

                    string cfMsg = cf.str();
                    cout << "Server M sent the confirmation response to Server R."
                         << endl;
                    sendto(udpSock, cfMsg.c_str(), cfMsg.size(), 0,
                           (struct sockaddr*)&serverRAddr, serverRAddrLen);

                    // Get final outcome from R
                    char r2Buf[4096];
                    sockaddr_in r2Addr{};
                    socklen_t r2Len = sizeof(r2Addr);
                    ssize_t sr2 = recvfrom(udpSock, r2Buf, sizeof(r2Buf)-1, 0,
                                           (struct sockaddr*)&r2Addr, &r2Len);
                    if (sr2 <= 0) continue;
                    r2Buf[sr2] = '\0';
                    string r2Resp(r2Buf);

                    stringstream rs2(r2Resp);
                    string tag2;
                    rs2 >> tag2;

                    // User choose N or R 
                    if (tag2 == "RESERVE_FAIL") {
                        string fail = "Reservation failed. No slots were reserved.\n"
                                      "---Start a new request---\n";
                        send(clientSock, fail.c_str(), fail.size(), 0);
                    } else if (tag2 == "RESERVE_OK") {
                        string space2, uname2;
                        int k;
                        rs2 >> space2 >> uname2 >> k;
                        vector<int> finalSlots(k);
                        for (int i = 0; i < k; ++i) rs2 >> finalSlots[i];

                        stringstream pReq;
                        pReq << "PRICE_RESERVE " << username << " " << space2
                             << " " << k;
                        for (int x : finalSlots) pReq << " " << x;

                        sendto(udpSock, pReq.str().c_str(), pReq.str().size(), 0,
                               (struct sockaddr*)&serverPAddr, serverPAddrLen);

                        char pBuf[1024];
                        sockaddr_in pAddr{};
                        socklen_t pLen = sizeof(pAddr);
                        ssize_t pr = recvfrom(udpSock, pBuf, sizeof(pBuf)-1, 0,
                                              (struct sockaddr*)&pAddr, &pLen);
                        if (pr <= 0) continue;
                        pBuf[pr] = '\0';
                        string pResp(pBuf); 

                        cout << "Server M received the pricing information from Server P using UDP over port "
                             << SERVER_M_UDP << "." << endl;
                        cout << "Server M sent the reservation result to the client."
                             << endl;

                        stringstream toCli;
                        toCli << "Reservation successful for " << space2
                              << " at time slot(s) ";
                        for (int x : finalSlots) toCli << x << " ";
                        toCli << "\n" << pResp << "\n"
                              << "---Start a new request---\n";

                        string finalMsg = toCli.str();
                        send(clientSock, finalMsg.c_str(), finalMsg.size(), 0);
                    }
                }
                // Direct reservation failure
                else if (tag == "RESERVE_NONE") {
                    string space, uname;
                    int nBad;
                    rs >> space >> uname >> nBad;
                    vector<int> badSlots(nBad);
                    for (int i = 0; i < nBad; ++i) rs >> badSlots[i];

                    stringstream toCli;
                    toCli << "Time slot(s) ";
                    for (int x : badSlots) toCli << x << " ";
                    toCli << "not available. Reservation failed. No slots were reserved.\n"
                          << "---Start a new request---\n";
                    string msg = toCli.str();
                    send(clientSock, msg.c_str(), msg.size(), 0);
                }
                else {
                    string msg = "Reservation failed. No slots were reserved.\n"
                                 "---Start a new request---\n";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                }

                continue;
            }

            // Cancel function
            if (command.rfind("cancel ", 0) == 0) {
                if (!isMember) {
                    string msg = "Guests cannot cancel. Please log in as a member.";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                    continue;
                }

                stringstream ss(command);
                string cmd, spaceID;
                ss >> cmd >> spaceID;
                vector<int> cancelSlots;
                int s;
                while (ss >> s) cancelSlots.push_back(s);

                if (spaceID.empty() || cancelSlots.empty()) {
                    string msg = "Cancel failed. Please provide valid space ID and time slots to cancel.";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                    continue;
                }

                cout << "Server M received a cancellation request from "
                     << username << " using TCP over port "
                     << SERVER_M_TCP << "." << endl;

                // formulate cancel message to R
                stringstream out;
                out << "CANCEL " << userID << " " << username << " "
                    << spaceID;
                for (int x : cancelSlots) out << " " << x;
                string msgToR = out.str();

                cout << "Server M sent the cancellation request to Server R."
                     << endl;

                sendto(udpSock, msgToR.c_str(), msgToR.size(), 0,
                       (struct sockaddr*)&serverRAddr, serverRAddrLen);

                char rBuf[4096];
                sockaddr_in rAddr{};
                socklen_t rLen = sizeof(rAddr);
                ssize_t sr = recvfrom(udpSock, rBuf, sizeof(rBuf)-1, 0,
                                      (struct sockaddr*)&rAddr, &rLen);
                if (sr <= 0) continue;
                rBuf[sr] = '\0';
                string rResp(rBuf);

                cout << "Server M received the response from Server R using UDP over port "
                     << SERVER_M_UDP << "." << endl;

                stringstream rs(rResp);
                string tag;
                rs >> tag;

                if (tag == "CANCEL_FAIL") {
                    string msg = "Cancellation failed. You do not own all requested slots.\n"
                                 "---Start a new request---\n";
                    send(clientSock, msg.c_str(), msg.size(), 0);
                } else if (tag == "CANCEL_OK") {
                    string space, uname;
                    int k;
                    rs >> space >> uname >> k;
                    vector<int> freed(k);
                    for (int i = 0; i < k; ++i) rs >> freed[i];

                    // Ask server P for refund
                    stringstream pReq;
                    pReq << "PRICE_CANCEL " << username << " " << space
                         << " " << k;
                    for (int x : freed) pReq << " " << x;

                    sendto(udpSock, pReq.str().c_str(), pReq.str().size(), 0,
                           (struct sockaddr*)&serverPAddr, serverPAddrLen);

                    char pBuf[1024];
                    sockaddr_in pAddr{};
                    socklen_t pLen = sizeof(pAddr);
                    ssize_t pr = recvfrom(udpSock, pBuf, sizeof(pBuf)-1, 0,
                                          (struct sockaddr*)&pAddr, &pLen);
                    if (pr <= 0) continue;
                    pBuf[pr] = '\0';
                    string pResp(pBuf); 

                    cout << "Server M received the refund information from Server P using UDP over port "
                         << SERVER_M_UDP << "." << endl;

                    stringstream toCli;
                    toCli << "Cancellation successful for " << space
                          << " at time slot(s) ";
                    for (int x : freed) toCli << x << " ";
                    toCli << "\n" << pResp << "\n"
                          << "---Start a new request---\n";

                    string msg = toCli.str();
                    send(clientSock, msg.c_str(), msg.size(), 0);
                }

                continue;
            }

            string unknown = "Unknown command.\n";
            send(clientSock, unknown.c_str(), unknown.size(), 0);
        }

        close(clientSock);
    }

    close(udpSock);
    close(tcpSock);
    return 0;
}
