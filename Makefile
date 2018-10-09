client: server
	g++ -std=c++11 -o client client.cpp
server:
	g++ -std=c++11 -o server server.cpp
clean:
	rm -f client server
