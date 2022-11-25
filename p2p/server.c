/* time_server.c - main */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <stdio.h>
#include <time.h>

#define MAX_CONTENT 50 //max # of contents
//Content list 0 = address ; 1 = address 2 = peer name
#define MAX_PEERS 10
#define DATA 50
#define BUFLEN 99

pthread_mutex_t mutex;// = PTHREAD_MUTEX_INITIALIZER;

int content_count;

struct content{
	int exist;
	int port;
	char type;
	char content_name[DATA];
	char peer_name[20];
	char ipAddr[INET_ADDRSTRLEN];
	//struct sockaddr_in addr;	/* the from address of a client	*/
};
struct content content_list[MAX_CONTENT];
struct content content_pdu;

struct generald{
	char type;
	char data[BUFLEN];
};
struct generald data_pdu;

/*------------------------------------------------------------------------
 * main - Iterative UDP server for TIME service
 *------------------------------------------------------------------------
 */
int main(int argc, char *argv[])
{
	struct sockaddr_in fsin;	/* the from address of a client	*/
	char	*service = "3000";	/* service name or port number	*/
	char	buf[100];		/* "input" buffer; any size > 0	*/
	char    *pts;
	int	sock,n;			/* server socket		*/
	time_t	now;			/* current time			*/
	int	alen;			/* from-address length		*/
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
	u_short	portbase = 0;	/* port base, for non-root servers	*/

	int m;
	FILE *fptr;
	int filesize;
	char errormsg[] = "Error File Not Found";

	char temp[INET_ADDRSTRLEN];



	switch (argc) {
	case	1:
		break;
	case	2:
		service = argv[1];
		break;
	default:
		fprintf(stderr, "usage: time_server [host [port]]\n");

	}
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;

   /* Map service name to port number */
        sin.sin_port = htons((u_short)atoi(service));

    /* Allocate a socket */
        s = socket(AF_INET, SOCK_DGRAM, 0);
        if (s < 0){
		fprintf(stderr, "can't creat socket\n");
		exit(1);
	}

		listen(s, 5);

    /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		fprintf(stderr, "can't bind to %s port\n", service);

		clearContentList();



		//get message
		//register or get content?
		//register contents: save content name, location
		//get content location
		//allow up to MAX_CONTENT files
		int i = 0;

		while(content_count < MAX_CONTENT){
				alen = sizeof(fsin);
			//get data
			if ((n = recvfrom(s, &content_pdu, sizeof(content_pdu), 0,
				(struct sockaddr *)&fsin, &alen)) < 0)
			fprintf(stderr, "recvfrom error\n");


			//register the content with the username of owner and address
			if(content_pdu.type == 'R'){
				register_content(s, fsin);
			}
			if(content_pdu.type == 'D'){
				download_content(s, fsin);
			}
			if(content_pdu.type == 'T'){
				deregister_content(s, fsin);
			}
			if(content_pdu.type == 'O'){
				get_content_list(s, fsin);
			}


		}


}

int register_content(int s, struct sockaddr_in fsin){
	char myIP[INET_ADDRSTRLEN];

	content_list[content_count] = content_pdu;
	printf("Content registration: \n");
	printf("content name: %s \n", content_list[content_count].content_name);
	printf("Peer name: %s \n", content_list[content_count].peer_name);
	printf("Port= %d \n", content_list[content_count].port);

	//register address of content server
	inet_ntop(AF_INET, &fsin.sin_addr, myIP, sizeof(myIP));
	strcpy(content_list[content_count].ipAddr, myIP);
	//printing address for test.
	printf( "Server address: %s \n", content_list[content_count].ipAddr);

	content_list[content_count].exist = 1;

	//send back acknowledgement
	content_pdu.type = 'A';
	(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
				(struct sockaddr *)&fsin, sizeof(fsin));

	content_count++;

}

int deregister_content(int s, struct sockaddr_in fsin){
	char myIP[INET_ADDRSTRLEN];
	int i = 0;

	struct content content_dont_exist;			// Error PDU
	content_dont_exist.exist = 0;
	content_dont_exist.type = 'E';
	strcpy(content_dont_exist.content_name, "File does not exist\n");

	int index = find_content();
	printf("Content Deregistration \n");
	printf("content name: %s \n", content_pdu.content_name);
	printf("Peer name: %s \n", content_pdu.peer_name);
	if(index == -1){
		printf("Content does not exist in index server \n");
		(void) sendto(s, &content_dont_exist, sizeof(content_dont_exist), 0,
					(struct sockaddr *)&fsin, sizeof(fsin));
	}else{
		content_list[index].exist = 0;
		content_list[index].content_name[0] = '\0';
		content_pdu.type = 'A';
		(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
					(struct sockaddr *)&fsin, sizeof(fsin));
		content_count--;
	}


	return 0;
}

int download_content(int s, struct sockaddr_in fsin){
	int i = 0;
	struct content content_dont_exist;
	content_dont_exist.exist = 0;
	content_dont_exist.type = 'E';
	strcpy(content_dont_exist.content_name, "File does not exist\n");

	i = find_content();
	if(i == -1){
		printf("\n %s content NOT found \n", content_pdu.content_name);


		(void) sendto(s, &content_dont_exist, sizeof(content_dont_exist), 0,
					(struct sockaddr *)&fsin, sizeof(fsin));
	}else{
		printf("\n %s content Found \n", content_pdu.content_name);
		printf("port: %d \n\n", content_list[i].port);
		(void) sendto(s, &content_list[i], sizeof(content_list[i]), 0,
					(struct sockaddr *)&fsin, sizeof(fsin));
	}
}

int get_content_list(int s, struct sockaddr_in fsin){
	int i = 0;
	struct content message_pdu;
	message_pdu.type = 'E';
	strcpy(message_pdu.content_name, "List end\n");

	for(i = 0; i < MAX_CONTENT; i++){
		if(content_list[i].exist == 1){
			content_pdu = content_list[i];
			content_pdu.type = 'O';
			(void) sendto(s, &content_pdu, sizeof(content_pdu), 0,
						(struct sockaddr *)&fsin, sizeof(fsin));
		}
	}
	(void) sendto(s, &message_pdu, sizeof(message_pdu), 0,
				(struct sockaddr *)&fsin, sizeof(fsin));

}


int find_content(){
	int i = 0;
	for(i = 0; i < MAX_CONTENT; i++){
		if(strcmp(content_pdu.content_name, content_list[i].content_name) == 0)
			return i;
	}
	return -1;
}

void print_socket(int sockfd){
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

}

void clearContentList(){

	int i = 0;

	for(i = 0; i < MAX_CONTENT; i++){
		content_list[i].content_name[0] = '\0';
		//content_list[i].addr[0] = '\0';
		content_list[i].peer_name[0] = '\0';
	}
}
