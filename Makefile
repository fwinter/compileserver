CXX=g++
CPPFLAGS=-I/usr/include/happycoders/socket
LDFLAGS=-L/usr/lib/happycoders


all: server

clean:
	-rm -f *.o
	-rm -f server

server: server.o  
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) -lpthread -lsocket


%.o : %.cc
	$(CXX) $(CPPFLAGS) -c $<
