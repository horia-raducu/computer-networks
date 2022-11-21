/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <strings.h>


#define SERVER_TCP_PORT 3000	/* well-known port */
#define BUFLEN		256	/* buffer length */

int echod(int);
void reaper(int);

int get_filesize(FILE *fptr){
	int pos = ftell(fptr);
	fseek(fptr, 0, SEEK_END);
	int length = ftell(fptr);
	fseek(fptr, pos, SEEK_SET);
	
	return(length);
}

int get_linesize(char *l){
	
	
	int c, count;
	count = 0;
	
	while(l[count] != EOF || l[count] != '\n'){
		count++;
	}

	return count;
}

int main(int argc, char **argv)
{
	FILE *fptr;
	FILE *tfptr;
	char buff[BUFLEN];
	char inbuff[BUFLEN];
	char *filename;
	int filesize = 0;
	int buffpos = 0;
	
	int n;
	
	int 	sd, new_sd, client_len, port;
	struct	sockaddr_in server, client;


	switch(argc){
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %d [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/	
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1){
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void) signal(SIGCHLD, reaper);

	while(1) {
	  client_len = sizeof(client);
	  new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
	  if(new_sd < 0){
	    fprintf(stderr, "Can't accept client \n");
	    exit(1);
	  }
	  switch (fork()){
	  case 0:		/* child */
		(void) close(sd);
		
		n = read(new_sd, inbuff, BUFLEN);
		filename = inbuff;
		filename[strcspn(filename, "\n")] = 0;

		
		if((tfptr = fopen(filename, "rb")) == NULL){
			fprintf(stderr, filename);
			fprintf(stderr, " : cannot find file\n");
			write(new_sd, " : cannot find file\n", BUFLEN);
			close(new_sd);
			exit(0);
		}

		//-----HERE-----
		
		filesize = get_filesize(tfptr);
		
		while( fgets(buff, sizeof(buff), tfptr) ){
			//write(1, buff, strlen(buff));
			write(new_sd, buff, strlen(buff));
		}
		
		
		exit(0);
	  default:		/* parent */
		(void) close(new_sd);
		break;
	  case -1:
		fprintf(stderr, "fork: error\n");
	  }
	}
}

/*	echod program	*/
int echod(int sd)
{
	char	*bp, buf[BUFLEN];
	int 	n, bytes_to_read;

	while(n = read(sd, buf, BUFLEN)) 
 
		//if((fptr = fopen()) == NULL){
		//	write(sd, buf, n);
		//	close(sd);
		//}

	return(0);
}

/*	reaper		*/
void	reaper(int sig)
{
	int	status;
	while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}
