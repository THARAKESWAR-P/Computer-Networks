/*
			NETWORK PROGRAMMING WITH SOCKETS

In this program we illustrate the use of Berkeley sockets for interprocess
communication across the network. We show the communication between a server
process and a client process.


*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

double applyOp(double a, double b, char op){
    switch(op){
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default : return a;
    }
}

			/* THE SERVER PROCESS */

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	time_t tick;

	int i;
	char str[100];		/* We will use this buffer for communication */

	/* The following system call opens a socket. The first parameter
	   indicates the family of the protocol to be followed. For internet
	   protocols we use AF_INET. For TCP sockets the second parameter
	   is SOCK_STREAM. The third parameter is set to 0 for user
	   applications.
	*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Cannot create socket\n");
		exit(0);
	}

	/* The structure "sockaddr_in" is defined in <netinet/in.h> for the
	   internet family of protocols. This has three main fields. The
 	   field "sin_family" specifies the family and is therefore AF_INET
	   for the internet family. The field "sin_addr" specifies the
	   internet address of the server. This field is set to INADDR_ANY
	   for machines having a single IP address. The field "sin_port"
	   specifies the port number of the server.
	*/
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	/* With the information provided in serv_addr, we associate the server
	   with its port using the bind() system call. 
	*/
	if (bind(sockfd, (struct sockaddr *) &serv_addr,
					sizeof(serv_addr)) < 0) {
		perror("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5); /* This specifies that up to 5 concurrent client
			      requests will be queued up while the system is
			      executing the "accept" system call below.
			   */

	/* In this program we are illustrating an iterative server -- one
	   which handles client connections one by one.i.e., no concurrency.
	   The accept() system call returns a new socket descriptor
	   which is used for communication with the server. After the
	   communication is over, the process comes back to wait again on
	   the original socket descriptor.
	*/
	int t=1;
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
					&clilen) ;

		if (newsockfd < 0) {
			perror("Accept error\n");
			exit(0);
		}
		for(i=0; i < 100; i++) str[i] = '\0';
		
		while(1)
		{
			char *buf = calloc(1, sizeof(char));
			recv(newsockfd, str, 5, 0);
			int len, flag=0;
		
			while(!flag)
			{
				// printf("buf: %s\n", buf);
				// printf("str: %s\n", str);
				for(int i=0; i<5; i++)
				{
					if(str[i] == '\0')
					{
						flag = 1;
						break;
					}
					len = strlen(buf);
					buf = realloc(buf, len+1);
					*(buf+len) = str[i];
					*(buf+len+1) = '\0';
				}
				if(flag) break;
				recv(newsockfd, str, 5, 0);
			} 
			// printf("%s\n", buf);
			if(strcmp(buf, "-1") == 0) break;
			double v1 = 0.0, v2 = 0.0, ans = 0.0;
			char op = ' ', bop = ' ';
			bool isFrac = false, isBrac = false;
			double fpl = 0.1, frac = 0.0;
			
			int i=0;
			for(; buf[i] != '\0'; i++)
			{
				if(buf[i]<='9' && buf[i]>='0')
				{
					if(isFrac)
					{
						v1 += fpl * (buf[i]-'0');
						fpl *= 0.1;
					}else
					{
						v1 *= 10;
						v1 += (buf[i]-'0');
					}
				}else if(buf[i] == '.') isFrac = true, fpl = 0.1;
				else
				{
					break;
				}
			}
			ans = v1;
			// printf("%f\n", ans);
			if(buf[i] != '\0')
			{
				while(buf[i] != '\0')
				{
					if(buf[i]<='9' && buf[i]>='0')
					{
						if(isFrac)
						{
							v2 += fpl * (buf[i]-'0');
							fpl *= 0.1;
						}else
						{
							v2 *= 10;
							v2 += (buf[i]-'0');
						}
					}else if(buf[i] == '.') isFrac = true, fpl = 0.1;
					else if(buf[i] == '(')
					{
						isBrac = true;
						bop=' ';
						v1 = 0.0;
						v2 = 0.0;
					}
					else if(buf[i] == ')')
					{
						isBrac = false;
						if(bop == ' ') v1 = v2;
						else{
							v1 = applyOp(v1, v2, bop);
						}
						if(op != ' ') ans = applyOp(ans, v1, op);
						else ans = v1;
						op = ' ';
					}
					else if(buf[i] == ' ')
					{
						i++;
						continue;
					}
					else
					{
						if(isBrac)
						{
							if(bop!=' ') v1 = applyOp(v1, v2, bop);
							else v1 = v2;
							bop = buf[i];
							isFrac = false;
							v2 = 0.0;
						}
						else{
							ans = applyOp(ans, v2, op);
							// printf("%f\n", ans);
							op = buf[i];
							isFrac = false;
							v2 = 0.0;
						}
					}
					i++;
				}
			}
		
			if(op != ' ')
			{
				ans = applyOp(ans, v2, op);
			}
			
			sprintf(buf, "%f", ans);
			send(newsockfd, buf, strlen(buf) + 1, 0);
			//recv(newsockfd, buf, 100, 0);				
		}

		close(newsockfd);
	}
	return 0;
}
			


