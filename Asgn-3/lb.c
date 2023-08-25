#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

int main(int argc, char* argv[])
{
	int			sockfd, newsockfd;  /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr, serv_addr1, serv_addr2;
	time_t prev_sec, curr_sec;

	int i;
	char buf[100];		
	
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}
	
	serv_addr.sin_family		= AF_INET;
	// inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(atoi(argv[1]));
	
	serv_addr1.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr1.sin_addr);
	serv_addr1.sin_port	= htons(atoi(argv[2]));
	
	serv_addr2.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr2.sin_addr);
	serv_addr2.sin_port	= htons(atoi(argv[3]));

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); 
	
	struct pollfd p;
    memset(&p, 0, sizeof(p));
    p.events = POLLIN;
	
	int s1_load=0, s2_load=0, T=5000, flag = 0;
	
	while (1) {
    	p.fd = sockfd;
    	
    	//printf("Time: %d\n", T);
    	prev_sec=time(NULL);
		if(poll(&p, 1, T) > 0 && flag)
		{
			
			newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
								&clilen) ;
			if (newsockfd < 0) {
				printf("Accept error\n");
				exit(0);
			}
			
			if (fork() == 0) {
				close(sockfd);	
				int csockfd;
				if ((csockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("Cannot create socket\n");
					exit(0);
				}
				// printf("Load:   s1-%d s2-%d", s1_load , s2_load);
				if(s1_load <= s2_load)
				{
					if ((connect(csockfd, (struct sockaddr *) &serv_addr1,
							sizeof(serv_addr1))) < 0) {
						perror("Unable to connect to server\n");
						exit(0);
					}
					send(csockfd, "Send Time", strlen("Send Time") + 1, 0);
					printf("Sending client request to %u(s1)\n", serv_addr1.sin_addr.s_addr);
					recv(csockfd, buf, 100, 0);
					
				
				}else
				{
					if ((connect(csockfd, (struct sockaddr *) &serv_addr2,
							sizeof(serv_addr2))) < 0) {
						perror("Unable to connect to server\n");
						exit(0);
					}
					send(csockfd, "Send Time", strlen("Send Time") + 1, 0);
					printf("Sending client request to %u(s2)\n", serv_addr2.sin_addr.s_addr);
					recv(csockfd, buf, 100, 0);
										
				}
				close(csockfd);
				send(newsockfd, buf, strlen(buf) + 1, 0);
				close(newsockfd);
				exit(0);
			}

			curr_sec = time(NULL);
			T -= (curr_sec-prev_sec)*1000;
		}else
		{
			int csockfd1, csockfd2;
			if ((csockfd1 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("Cannot create socket\n");
				exit(0);
			}
			if ((connect(csockfd1, (struct sockaddr *) &serv_addr1,
						sizeof(serv_addr1))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}
			send(csockfd1, "Send Load", strlen("Send Load") + 1, 0);
			recv(csockfd1, buf, 100, 0);
			s1_load = atoi(buf);
			printf( "Load received from %u(s1) %d\n", serv_addr1.sin_addr.s_addr, s1_load);
			close(csockfd1);
			
			if ((csockfd2 = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
				printf("Cannot create socket\n");
				exit(0);
			}
			if ((connect(csockfd2, (struct sockaddr *) &serv_addr2,
						sizeof(serv_addr2))) < 0) {
				perror("Unable to connect to server\n");
				exit(0);
			}
			send(csockfd2, "Send Load", strlen("Send Load") + 1, 0);
			recv(csockfd2, buf, 100, 0);
			s2_load = atoi(buf);
			printf( "Load received from %u(s2) %d\n", serv_addr2.sin_addr.s_addr, s2_load);
			close(csockfd2);
			flag =1;
			T = 5000;
		}
		
		
		close(newsockfd);
	}
	return 0;
}
			


