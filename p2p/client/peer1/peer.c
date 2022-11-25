/* time_client.c - main */

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>

#define	BUFSIZE 64

#define SERVER_TCP_PORT 3014	/* well-known port */

#define PACKET 101
#define DATA 50

#define BUFLEN 99


void	reaper(int sig);
int tcpServerPort;
struct content{
	int exist;
	int port;
	char type;
	char content_name[DATA];
	char peer_name[20];
	char ipAddr[INET_ADDRSTRLEN];
	//struct sockaddr_in addr;	/* the from address of a client	*/
};
struct content content_pdu;

struct generald{
	char type;
	char data[BUFLEN];
};
struct generald data_pdu;


/*------------------------------------------------------------------------
 * main - UDP client for TIME service that prints the resulting time
 *------------------------------------------------------------------------
 */

void server_Request(int s, struct sockaddr_in sin){
	FILE *fptr;
	int n = 0;
	int	alen;			/* from-address length		*/
	char filename[20];

	while(1){

		write(1, "Enter 'R' to register content\n 'D' to download content: ", 30);

		read(0, &content_pdu.type, 2);		/* client request file name */

		switch(content_pdu.type){
			case 'R':		//Register content

				register_content(s, sin);

				break;
			case 'D':			//Request content
				request_download(s, sin);
				break;
			case 'T':
				deregister_content(s, sin);
				break;
			case 'O':
				get_content_list(s, sin);
		}
		//bzero(spdu.data, DATA);
	}
}


int main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*service = "3000";
	char	now[100];		/* 32-bit integer to hold time	*/
	struct hostent	*phe;	/* pointer to host information entry	*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	int	s, n, type;	/* socket descriptor and socket type	*/

	struct	sockaddr_in server;
	int sd;

	char buf[PACKET];


	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		service = argv[2];
		/* FALL THROUGH */
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: UDPtime [host [port]]\n");
		exit(1);
	}

	memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

    /* Map service name to port number */
        sin.sin_port = htons((u_short)atoi(service));

        if ( phe = gethostbyname(host) ){
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
                }
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE ){
		fprintf(stderr, "Can't get host entry \n");
		exit(1);
	}


    /* Allocate a socket */
        s = socket(PF_INET, SOCK_DGRAM, 0);
        if (s < 0){
		fprintf(stderr, "Can't create socket \n");
		exit(1);
	}


	//get username from this peer
	write(1, "Enter Username: ", 16);
	n = read(0, content_pdu.peer_name, DATA);		/* client request file name */
	content_pdu.peer_name[n-1] = '\0';


	//Set up TCP here so that the info can be stored in parent
	/* Create a stream socket	*/
		if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			fprintf(stderr, "Can't creat a socket\n");
			exit(1);
		}

	// /* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(0);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) < 0){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	//get port for content server
	tcpServerPort = print_socket(sd);

	//parent will serve user, child will act as TCP server in background
	switch(fork()){
		case 0:
			tcp_server(sd);

			break;

		default:		/* parent */
			usleep(250000); //wait 0.25 seconds for SERVER to initialize
			server_Request(s, sin);

			break;
	  case -1:
			fprintf(stderr, "fork: error\n");
	}

}



//Content Server Handler
void tcp_server(int sd)
{
	struct	sockaddr_in server, client;
	int new_sd, client_len;

	int n;
	char inbuff[BUFLEN];
	char *filename;

	int timeout = 0;

	write(1, "TCP server is set up: Address, port\n", 30);
	print_socket(sd);

	/* queue up to 5 connect requests  */
	listen(sd, 5);
	(void) signal(SIGCHLD, reaper);


	while(1){
		client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client %d\n", new_sd);
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
			(void) close(sd);

			n = read(new_sd, inbuff, BUFLEN);
			filename = inbuff;
			filename[strcspn(filename, "\n")] = 0;

			write(1, filename, sizeof(filename));
			write(new_sd, "test123", 7);

			(void) close(new_sd);
			exit(0);
		default:
			(void) close(new_sd);
			break;
	  case -1:
			fprintf(stderr, "fork: error\n");
		}
	}


}


void tcp_client(){
	int 	sd, port;
	struct	hostent		*hp;
	struct	sockaddr_in server;
	char	*bp, rbuf[BUFLEN], sbuf[BUFLEN];

	int n;

	//creating stream socket
	sd = socket(AF_INET, SOCK_STREAM, 0);
	if(sd < 0){
		fprintf(stderr, "Can't create stream socket\n");
		exit(1);
	}

	//bzero((char *)&server, sizeof(struct sockaddr_in));
	memset(&server, '\0', sizeof(server));
	server.sin_family = AF_INET;


	//LETS MAKE SURE PORT IS BEING SET PROPERLY

	printf("\ntcp client, port = %d ", content_pdu.port);
	//write(1, port, sizeof(port));
	server.sin_port = htons(content_pdu.port);

	//get ip address
	if (hp = gethostbyname(content_pdu.ipAddr))
	  bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if ( inet_aton(content_pdu.ipAddr, (struct in_addr *) &server.sin_addr) ){
	  fprintf(stderr, "Can't get server's address\n");
	  exit(1);
	}

	//server.sin_addr.s_addr = content_pdu.peer_server.sin_addr.s_addr;
	//{TEST
	char temp[INET_ADDRSTRLEN];
	int i = inet_ntop( AF_INET, &server.sin_addr.s_addr,
		temp, sizeof(temp));
	printf( "tcp client, Server address:%s\n", temp);
	//}TEST


	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
	  fprintf(stderr, "Can't connect to content server \n");
		return;
	}

	write(1, "File Name: ", 10);
	n = read(0, sbuf, BUFLEN);		/* get user message */

	write(sd, sbuf, n);			/* send it out */
	n = read(sd, sbuf, BUFLEN);
	write(1, sbuf, n);

}

//returns port number
int print_socket(int sockfd){
	char myIP[16];
  unsigned int myPort;
  struct sockaddr_in my_addr;

	// Get my ip address and port
	bzero(&my_addr, sizeof(my_addr));
	socklen_t len = sizeof(my_addr);
	getsockname(sockfd, (struct sockaddr *) &my_addr, &len);
	inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
	myPort = ntohs(my_addr.sin_port);

	printf("", myIP);
	write(1, "\nServer ip address: ", 20);
	write(1, myIP, sizeof(myIP));
	printf("Local port : %u\n", myPort);

	return myPort;
}


int register_content(int s, struct sockaddr_in sin){
	int	alen;			/* from-address length		*/
	int n;
	write(1, "Enter File Name: ", 17);

	n = read(0, content_pdu.content_name, DATA);		/* client request file name */

	content_pdu.content_name[n-1] = '\0';

	content_pdu.port = tcpServerPort;
	//write(s, &spdu, sizeof(spdu));	/* send out request pdu */
	do{
		(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
					(struct sockaddr *)&sin, sizeof(sin));

		alen = sizeof(sin);
		recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
			(struct sockaddr *)&sin, &alen);

		if(content_pdu.type == 'A')
			printf("Acknowledgement recieved= %s \n", content_pdu.content_name);

	}while(content_pdu.type != 'A');

}

int deregister_content(int s, struct sockaddr_in sin){
	int	alen;			/* from-address length		*/
	int n;
	char msg[] = "Enter File to De-register: \n";
	write(1, msg, sizeof(msg));

	n = read(0, content_pdu.content_name, DATA);		/* client request file name */

	content_pdu.content_name[n-1] = '\0';

	content_pdu.port = tcpServerPort;
	//write(s, &spdu, sizeof(spdu));	/* send out request pdu */
		(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
					(struct sockaddr *)&sin, sizeof(sin));

		alen = sizeof(sin);
		recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
			(struct sockaddr *)&sin, &alen);

		if(content_pdu.type == 'A')
			printf("Acknowledgement recieved= %s \n", content_pdu.content_name);
		else if(content_pdu.type == 'E'){
			printf("%s \n", content_pdu.content_name);
		}else{
			printf("Something went wrong\n");
		}

}


int request_download(int s, struct sockaddr_in sin){
	int n;
	int	alen;			/* from-address length		*/
	write(1, "Enter File Name: ", 17);

	n = read(0, content_pdu.content_name, DATA);  /* client request file name */
	content_pdu.content_name[n-1] = '\0';
	//write(s, &spdu, sizeof(spdu));	/* send out request pdu */
	(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
				(struct sockaddr *)&sin, sizeof(sin));

	//read(s, &content_pdu, sizeof(content_pdu));
	alen = sizeof(sin);
	recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
		(struct sockaddr *)&sin, &alen);

	if(content_pdu.type != 'E'){
		printf("\n content Found peer: %s ", content_pdu.peer_name);
		printf("\n content name: %s ", content_pdu.content_name);
		//{TEST
		printf( "Server Req: Ip= %s\n", content_pdu.ipAddr);
		printf( "Server Req: Port= %d\n", content_pdu.port);
		//}TEST
		tcp_client();
	}else{
		printf( "%s\n", content_pdu.content_name);
	}

}

int get_content_list(int s, struct sockaddr_in sin){
	int n;
	int	alen = sizeof(sin);;			/* from-address length		*/

	content_pdu.type = 'O';

	(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
				(struct sockaddr *)&sin, sizeof(sin));

	recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
		(struct sockaddr *)&sin, &alen);
	printf("----Content List---\n");
	while(content_pdu.type == 'O'){	//while the server is listing content, print
		printf("File: %s User: %s \n", content_pdu.content_name, content_pdu.peer_name);
		recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
			(struct sockaddr *)&sin, &alen);
	}
	if(content_pdu.type == 'E'){
		printf("%s \n", content_pdu.content_name);
	}

}


/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
