all: server client

server:  whatsappio.cpp whatsappio.h whatsappServer.cpp whatsappServer.hpp
	g++ -Wall -std=c++11 -g whatsappio.cpp whatsappServer.cpp -o whatsappServer

client:  whatsappio.cpp whatsappio.h whatsappClient.cpp whatsappClient.hpp
	g++ -Wall -std=c++11 -g whatsappio.cpp whatsappClient.cpp -o whatsappClient

clean:
	-rm -f *.o whatsappClient whatsappServer

tar:
	tar cfv ex4.tar Makefile whatsappServer.cpp whatsappServer.hpp whatsappClient.cpp whatsappClient.hpp whatsappio.cpp whatsappio.h README

.PHONY: all clean