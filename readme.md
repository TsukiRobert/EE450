1, Xinsong (Robert) Fan
2, 7072371273
3, In this term project, there are multiple stages. Before the assignment start, I went over the socket programming posted on the brightspace and did some small practices. After reading the assignment guideline, I planned how information flows through each server and what are some logic requirements needed. And with the help from gpt, I planned the skeleton of the server and how the server communicates at first. Then, I filled in the code for each function and tune the logic for each of them to handle the edge cases and output onscreen messages. After the lab, since the project is a huge project, there are plenties of codes written and it was messy at first. I fed my code into the GPT 5 and let it see how I should structure my code to make my code look more compact and brief. Finally, for the testing stages, I asked GPT 5 to gave me some edge cases to test my code and found some small bugs that were noticed. 
4, I formulates the required cpp files naming as exactly the guideline asks for: 
    ServerA.cpp: the authenticating server. it handles the authentication through username and that divides into two parts: guests and members login. It matches the user input with the members.txt to verify the credentials through encrypted passwords. 
    ServerM.cpp: the main server that bridges the connection. The main addresses have the port of all servers and manage to send UPD sockets to ARP servers and listens to the client server for commands. It also handles the commands that are divided into members and guests. 
    ServerR.cpp: Appearntly serverR handles the reservtion system that includes lookup, search, reserve and cancel. It communicates with the main server waiting for the request from the clients and send the response back to the main then to the client.
    ServerP.cpp: It calculates the price of the reservation; Reservation plus cancellation fees are calculated by the price then sent to main and then reply to clients. 
    Client.cpp: the client establishes the TCP connection with the main server and sending requrest to the server. It also has to login: guest 123456 and member_name password.  
5, Message exchange 
    Authentication: username and password are separated by a space
                    server side: receive OK MEMBER | OK GUEST | FAIL as reply 
    Reservation:
        Search: as the guideline addressed
            return: <spaceID_1> <Time_slots> eg: 1 2 3 (first, second, and third time slot)
        reserve: as the guideline and help funcction addressed 
            return: success <ID> <slots>
            Partial: partial <ID> <slots>
    Pricing:
        reserve: price_reserve <username> ID <Slots>
        return total price
        cancellation: PRICE_CANCEL <username> ID <slots>
    The other function and commands are exactly as the onscreen message and requirement addressed
6, Code reused: 
    In this assignment, I mainly used AI for discussion, structure, and learning. I did not use the code from the GPT directly. As described in number 3, I planned the structure with gpt and wrote the code myself. And after I'm done with the code, I asked GPT to provide me restructure advice.
7, Ubuntu version: I believe it's 20.04.06 version 