EE450 SOCKET PROGRAMMING PROJECT
DISTRIBUTED PARKING RESERVATION SYSTEM
============================================================

Student Information
------------------------------------------------------------
Name:        Xinsong (Robert) Fan
Student ID:  7072371273
Course:      EE450
Platform:    Ubuntu 20.04.06


1. INTRODUCTION
------------------------------------------------------------
This project implements a distributed parking reservation system
using C/C++ and UNIX socket programming. The system consists of a
TCP-based client and multiple backend servers that communicate
through UDP. Each server is responsible for a specific task,
including authentication, reservation management, and pricing.

The goal of this project is to gain hands-on experience with
network programming, inter-process communication, protocol
design, and distributed system architecture in a Linux
environment.


2. PROJECT DEVELOPMENT PROCESS
------------------------------------------------------------
The project was completed in multiple stages:

(1) Preparation
    Before starting the assignment, I reviewed the socket
    programming materials provided on Brightspace and completed
    small practice programs to become familiar with TCP and UDP
    communication.

(2) System Design
    After studying the project specification, I planned the
    overall architecture of the system, focusing on how
    information flows between the client, the main server, and
    backend servers. I also identified logic requirements and
    potential edge cases.

(3) Implementation
    With the help of GPT as a learning and planning tool, I
    designed the initial server skeletons and communication
    structure. I then implemented each server independently and
    handled edge cases while ensuring all on-screen messages
    strictly matched the project specification.

(4) Code Refinement
    As the project grew in size, the code became complex. I
    reorganized and refactored the code to improve readability,
    modularity, and maintainability.

(5) Testing and Debugging
    I tested the system using various edge cases involving
    authentication failures, partial reservations, cancellations,
    and pricing calculations. This helped identify and fix subtle
    bugs in logic and communication.


3. SOURCE CODE FILES
------------------------------------------------------------
All source files follow the naming conventions specified in the
assignment.

serverA.cpp
    Authentication Server. Handles guest and member login by
    verifying encrypted passwords against members.txt.

serverM.cpp
    Main Server. Coordinates all system operations, communicates
    with backend servers via UDP, and handles client requests
    over TCP.

serverR.cpp
    Reservation Server. Manages parking availability, reservation,
    lookup, and cancellation using data loaded from spaces.txt.

serverP.cpp
    Pricing Server. Calculates parking fees and refunds based on
    parking location, duration, and peak-hour rules.

client.cpp
    Client program. Establishes a TCP connection with the main
    server, performs authentication, sends commands, and displays
    responses.


4. MESSAGE EXCHANGE FORMAT
------------------------------------------------------------

Authentication:
    Request:
        <username> <password>

    Response:
        OK MEMBER
        OK GUEST
        FAIL


Reservation (Server R):
    Search:
        <spaceID>: Time slot(s) <slot1> <slot2> ...

    Reserve:
        Success:
            RESERVE_OK <spaceID> <slots>
        Partial:
            RESERVE_PARTIAL <spaceID> <available slots>


Pricing (Server P):
    Reserve:
        PRICE_RESERVE <username> <spaceID> <slots>
        Returns total parking cost.

    Cancel:
        PRICE_CANCEL <username> <spaceID> <slots>
        Returns refund amount.

All other commands and responses strictly follow the on-screen
message formats defined in the project specification.


5. CODE REUSE AND AI USAGE
------------------------------------------------------------
AI tools were used strictly as learning and planning assistance.
They were used to:
    - Understand socket programming concepts
    - Design server architecture
    - Improve code organization and readability
    - Generate edge-case testing scenarios

6. HOW TO COMPILE AND RUN
------------------------------------------------------------

Step 1: Compile the project
    make all

Step 2: Start the servers (in separate terminals)
    ./serverM
    ./serverA
    ./serverR
    ./serverP

Step 3: Run the client
    ./client <username> <password>

Example:
    ./client guest 123456

Step 4: Clean executables
    make clean


7. NOTES
------------------------------------------------------------
- All programs remain running until manually terminated.
- The project was developed and tested on Ubuntu 20.04.06.
- All on-screen messages strictly follow the assignment
  specification.

============================================================

