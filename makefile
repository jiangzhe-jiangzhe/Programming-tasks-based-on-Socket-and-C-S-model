CXX ?= g++
all: server client_multicast client_unicast client_tcp 

server: ./servers/main.cpp  ./connect/connect.cpp  ./servers/servers.cpp ./config/config.cpp  ./lock/locker.cpp ./handle_data/tcp_data.cpp ./handle_data/udp_data.cpp 
	$(CXX) -o server  $^ -lpthread -lz

client_multicast: ./clients/client_multicast.cpp ./config/config.cpp ./clients/client.cpp  ./handle_data/tcp_data.cpp ./handle_data/udp_data.cpp 
	$(CXX) -o client_multicast $^ -lpthread -lz

client_tcp: ./clients/client_tcp.cpp ./config/config.cpp ./clients/client.cpp  ./handle_data/tcp_data.cpp ./handle_data/udp_data.cpp 
	$(CXX) -o client_tcp $^ -lz -lpthread

client_unicast: ./clients/client_unicast.cpp ./config/config.cpp ./clients/client.cpp  ./handle_data/tcp_data.cpp ./handle_data/udp_data.cpp 
	$(CXX) -o client_unicast $^ -lpthread -lz

clean:
	rm  -r server
	rm  -r client_multicast
	rm  -r client_tcp
	rm  -r client_unicast
