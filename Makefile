CXX=g++
CXXFLAGS=-Wall -Wextra -std=c++17

all: serverM serverA serverR serverP client

serverM: serverM.cpp
	$(CXX) $(CXXFLAGS) -o serverM serverM.cpp

serverA: serverA.cpp
	$(CXX) $(CXXFLAGS) -o serverA serverA.cpp

serverR: serverR.cpp
	$(CXX) $(CXXFLAGS) -o serverR serverR.cpp

serverP: serverP.cpp
	$(CXX) $(CXXFLAGS) -o serverP serverP.cpp

client: client.cpp
	$(CXX) $(CXXFLAGS) -o client client.cpp

clean:
	rm -f serverM serverA serverR serverP client
