all: PoolServer MultiConnnectClient

PoolServer : PoolServer.o errexit.o
	gcc -o $@ $^ -lpthread

MultiConnnectClient : MultiConnnectClient.o errexit.o clientsock.o
	gcc -o $@ $^

PoolServer.o : PoolServer.c
	gcc -c -g -o $@ $< -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast

%.o : %.c
	gcc -c -g -o $@ $<

clean :
	rm -f *.o PoolServer MultiConnnectClient
