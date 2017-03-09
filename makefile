tcpproxy: tcpproxy.o sockutils.o
	gcc -o tcpproxy tcpproxy.o sockutils.o -I.

clean:
	rm *.o tcpproxy
