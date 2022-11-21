/* A simple echo client using TCP */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <strings.h>



#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		256	/* buffer length */

int main(int argc, char **argv)
{	
	FILE *fptr;
	int 	n, i, bytes_to_read;
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*host, *bp, rbuf[BUFLEN], sbuf[BUFLEN];
	char 	*filename;

	switch(argc){
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host)) 
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(host, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect \n");
	  exit(1);
	}
	
	
	write(1, "File Name: ", 10);
	n=read(0, sbuf, BUFLEN);		/* get user message */
	  write(sd, sbuf, n);			/* send it out */
	
	filename = sbuf;
	filename[strcspn(filename, "\n")] = 0;
	
	
	read(sd, rbuf, BUFLEN);
	char error[BUFLEN] = " : cannot find file\n";
	if(strcmp(rbuf, error) == 0){
		write(1, rbuf, BUFLEN);
		
		close(sd);
		exit(0);
	}
	
	
	fptr = fopen (filename, "w");
	
	  while ((i = read(sd, rbuf, strlen(rbuf))) > 0){
		write(1, rbuf, strlen(rbuf));
		fputs(rbuf, fptr);
	  }
	  
	  //write(1, rbuf, strlen(rbuf));
	  //printf("\n");

	close(sd);
	return(0);
}
