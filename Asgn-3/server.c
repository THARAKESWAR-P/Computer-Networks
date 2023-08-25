#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char* argv[])
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	time_t tick;

	int i;
	char buf[100];		
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	
	serv_addr.sin_family		= AF_INET;
	// inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(atoi(argv[1]));

	
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); 
	srand(time(0));
	
	while (1) {

		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}

		recv(newsockfd, buf, 100, 0);
		if(strcmp(buf, "Send Load") == 0)
		{
			int num = rand()%100 + 1;
			sprintf(buf, "%d", num);
			send(newsockfd, buf, strlen(buf) + 1, 0);
			printf("Load sent: %s\n", buf);
		}
		else if(strcmp(buf, "Send Time") == 0)
		{
			tick=time(NULL);
			snprintf(buf,sizeof(buf),"%s",ctime(&tick));
			send(newsockfd, buf, strlen(buf) + 1, 0);
			//printf("%s\n", buf);
		}

		close(newsockfd);
	}
	return 0;
}
			


