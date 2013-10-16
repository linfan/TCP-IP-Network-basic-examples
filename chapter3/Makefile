all: TCPdaytimed TCPdaytime UDPtimed UDPtime PrintAllServByPort

TCPdaytimed : TCPdaytimed.o passivesock.o errexit.o
	gcc -o $@ $<

TCPdaytime : TCPdaytime.o clientsock.o errexit.o
	gcc -o $@ $<

UDPtimed : UDPtimed.o passivesock.o errexit.o
	gcc -o $@ $<

UDPtime : UDPtime.o clientsock.o errexit.o
	gcc -o $@ $<

PrintAllServByPort: PrintAllServByPort.o
	gcc -o $@ $<

%.o : %.c
	gcc -c -g -o $@ $<

clean :
	rm -f *.o TCPdaytimed TCPdaytime UDPtimed UDPtime PrintAllServByPort
