#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include <sys/time.h>

#define LG_INFO 262144 

int send_graphite(char *hostname,int port,char *name,char *value,int stamp) {

	int	graphite_sock;
	struct	sockaddr_in server;

	struct  hostent *host;


	char *message;


	// Opening graphite_socket
	graphite_sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if (graphite_sock < 0) {
		nagios_graphite_logger(LG_INFO, "error opening datagram graphite_socket");     
		return 0;
	};

	/* initialize server addr */
	memset((char *) &server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

	/* get ip address */
	struct hostent *sp;
	sp = gethostbyname(hostname);
	memcpy(&server.sin_addr, sp->h_addr, sp->h_length);

	/* convert timestamp to string */
	char buf[100];
	sprintf(buf,"%d",stamp);

	/* calculate message size */
	int message_size = sizeof(name)+sizeof(value)+sizeof(buf)+3*sizeof(char);
	message = malloc(message_size);

	sprintf(message,"%s %s %d\n",name,value,stamp);
	nagios_graphite_logger(LG_INFO, message);     

	/* send message */
	if (sendto(graphite_sock, message, strlen(message), 0, (struct sockaddr *) &server, sizeof(server)) == -1) {
		nagios_graphite_logger(LG_INFO, "error sending packet");     
		return 0;
	}

	close(graphite_sock);
	free(message);
	return 0;
}

