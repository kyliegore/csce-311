cc = g++

flags = -std=c++17
flags += -Wall
flags += -g



#
text-server: text-server.cc
	g++ -g -o text-server.cc


#
text-client: text-client.cc
	g++ -g -o text-client.cc


#
clean:
	$(RM) text-server text-client
