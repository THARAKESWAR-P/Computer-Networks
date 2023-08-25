// A Simple Client Implementation
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/poll.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 
  
int main() { 
    int sockfd; 
    char buf[100];
    struct sockaddr_in servaddr; 
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if ( sockfd < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
  
    memset(&servaddr, 0, sizeof(servaddr)); 
     
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(8181); 
    inet_aton("127.0.0.1", &servaddr.sin_addr); 
      
    int n;
    socklen_t len; 
    char *hello = "CLIENT:HELLO"; 
    
    int  counter = 5;
    struct pollfd p;
    memset(&p, 0, sizeof(p));
    p.events = POLLIN;
    
    while(counter--)
    {
    	sendto(sockfd, (const char *)hello, strlen(hello), 0, 
			(const struct sockaddr *) &servaddr, sizeof(servaddr)); 
		p.fd = sockfd;
		if(poll(&p, 1, 3000) > 0)
		{
			n = recvfrom(sockfd, (char *)buf, 100, 0, 
				( struct sockaddr *) &servaddr, &len); 
			buf[n] = '\0'; 
    		printf("%s\n", buf);
    		break;
		}
    }
    if(counter < 0) printf("Timeout exceeded\n"); 
           
    close(sockfd); 
    return 0; 
}
