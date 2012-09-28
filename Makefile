nagios2graphite.o: nagios-handler.o send-graphite.o
	gcc -shared -o nagios2graphite.o send-graphite.o nagios-handler.o  -fPIC

nagios-handler.o: nagios-handler.c
	gcc -shared -c nagios-handler.c -fPIC

send-graphite.o: send-graphite.c
	gcc -shared -c send-graphite.c -fPIC

clean:
	rm -f *.o

install:
	install nagios2graphite.o /var/lib/nagios3/

