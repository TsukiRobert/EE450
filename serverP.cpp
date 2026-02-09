#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

// Create a vector for each UPC and HSC since they track time separately
struct UserPricing {
    vector<pair<string,int>> upcReservations;
    vector<pair<string,int>> hscReservations;
};

// Username map to pricing state
static map<string, UserPricing> userPricingState;

static string trim(const string &s) {
    size_t a = 0, b = s.size();
    while (a < b && isspace((unsigned char)s[a])) ++a;
    while (b > a && isspace((unsigned char)s[b - 1])) --b;
    return s.substr(a, b - a);
}

static bool isPeakSlot(int s) {
    return (s == 4 || s == 9);
}

// Compute total price
static double computeTotal(char lot, const vector<pair<string,int>> &v) {
    double firstRate = (lot == 'H') ? 30.0 : 20.0;
    double laterRate = (lot == 'H') ? 20.0 : 14.0;

    double total = 0;
    for (int i = 0; i < (int)v.size(); ++i) {
        int slot = v[i].second;
        double rate = (i == 0) ? firstRate : laterRate;
        if (isPeakSlot(slot)) rate *= 1.5;
        total += rate;
    }
    return total;
}

// refund computation
static double computeRefund(char lot, int k) {
    return (lot == 'H' ? 20.0 : 14.0) * k;
}

// add slots to the vector
static void addSlots(vector<pair<string,int>> &vec,
                     const string &spaceID,
                     const vector<int> &slots)
{
    for (int s : slots) {
        if (s < 1 || s > 12) continue;
        auto p = make_pair(spaceID, s);
        if (find(vec.begin(), vec.end(), p) == vec.end())
            vec.push_back(p);
    }
}

// remove slots from vector
static void removeSlots(vector<pair<string,int>> &vec,
                        const string &spaceID,
                        const vector<int> &slots)
{
    for (int s : slots) {
        auto it = find(vec.begin(), vec.end(),
                       make_pair(spaceID, s));
        if (it != vec.end()) vec.erase(it);
    }
}

int main() {
    int SERVER_P_PORT = 23000 + 273;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_P_PORT);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    cout << "Server P is up and running using UDP on port "
         << SERVER_P_PORT << "." << endl;

    while (true) {
        char buf[4096];
        sockaddr_in from{};
        socklen_t fromLen = sizeof(from);
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf)-1, 0,
                             (sockaddr*)&from, &fromLen);
        if (n <= 0) continue;

        buf[n] = '\0';
        string msg = trim(buf);

        stringstream ss(msg);
        string cmd, username, spaceID;
        int k;
        ss >> cmd;

        // Price for reservation
        if (cmd == "PRICE_RESERVE") {
            ss >> username >> spaceID >> k;

            vector<int> slots;
            for (int x; ss >> x;) slots.push_back(x);
            if ((int)slots.size() != k) continue;

            cout << "Server P received a pricing request from the main server.\n";

            char lot = spaceID[0];
            auto &state = userPricingState[username];
            auto &vec = (lot == 'H') ? state.hscReservations : state.upcReservations;

            addSlots(vec, spaceID, slots);

            double total = computeTotal('U', state.upcReservations) +
                           computeTotal('H', state.hscReservations);

            cout << "Calculated total price of $"
                 << fixed << setprecision(2) << total
                 << " for " << username << ".\n";

            string resp = "TOTAL " + 
                (static_cast<ostringstream&&>(
                    ostringstream() << fixed << setprecision(2) << total
                )).str();

            sendto(sockfd, resp.c_str(), resp.size(), 0, (sockaddr*)&from, fromLen);
            cout << "Server P finished sending the price to the main server.\n";
        }

        // Price for cancellation
        else if (cmd == "PRICE_CANCEL") {
            ss >> username >> spaceID >> k;

            vector<int> slots;
            for (int x; ss >> x;) slots.push_back(x);
            if ((int)slots.size() != k) continue;

            cout << "Server P received a refund request from the main server.\n";

            char lot = spaceID[0];

            auto it = userPricingState.find(username);
            if (it != userPricingState.end()) {
                auto &vec = (lot == 'H') ? it->second.hscReservations
                                        : it->second.upcReservations;
                removeSlots(vec, spaceID, slots);
            }

            double refund = computeRefund(lot, k);

            cout << "Calculated refund of $"
                 << fixed << setprecision(2) << refund
                 << " for " << username << ".\n";

            string resp = "REFUND " + 
                (static_cast<ostringstream&&>(
                    ostringstream() << fixed << setprecision(2) << refund
                )).str();

            sendto(sockfd, resp.c_str(), resp.size(), 0, (sockaddr*)&from, fromLen);
            cout << "Server P finished sending the refund amount to the main server.\n";
        }
    }
}
