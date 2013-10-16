all: TCPservice TCPclient

TCPservice: TCPservice.o errexit.o
	gcc -o $@ $^ -lpthread

TCPclient: TCPclient.o errexit.o
	gcc -o $@ $^

%.o: %.c
	gcc -c -g -o $@ $<

clean:
	rm -f *.o TCPservice TCPclient
