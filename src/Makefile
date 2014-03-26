# ddserver Makefile
all: ddserver

ddserver: main.o communicator.o
	$(CXX) $(LDFLAGS) main.o communicator.o -o ddserver $(LIBS)

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

communicator.o: communicator.cpp
	$(CXX) $(CXXFLAGS) -c communicator.cpp

clean:
	rm *.o ddserver

