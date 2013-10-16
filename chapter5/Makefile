all: TCPservice TCPclient

TCPservice: TCPservice.o errexit.o
	gcc -o $@ $^

TCPclient: TCPclient.o errexit.o
	gcc -o $@ $^

%.o: %.c
	gcc -c -g -o $@ $<

clean:
	rm *.o TCPservice TCPclient
