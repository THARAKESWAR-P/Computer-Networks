#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main()
{
	int	sockfd;
	struct sockaddr_in	serv_addr;

	int i;
	char buf[300], str[50];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Unable to create socket\n");
		exit(0);
	}

	serv_addr.sin_family	= AF_INET;
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);

	if ((connect(sockfd, (struct sockaddr *) &serv_addr,
						sizeof(serv_addr))) < 0) {
		perror("Unable to connect to server\n");
		exit(0);
	}

	for(i=0; i < 300; i++) buf[i] = '\0';
	recv(sockfd, str, 50, 0);
	int len=0, flag=0;
	while(!flag)
	{
		for(int i=0; i<50; i++)
		{
			if(str[i] == '\0')
			{
				flag = 1;
				break;
			}
			*(buf+len) = str[i];
			*(buf+len+1) = '\0';
			len++;
		}
		if(flag) break;
		recv(sockfd, str, 50, 0);
	}
	printf("%s", buf);
	
	scanf("%s", buf);
	send(sockfd, buf, strlen(buf) + 1, 0);
	
	recv(sockfd, str, 50, 0);
	len=0; flag=0;	
	while(!flag)
	{
		for(int i=0; i<50; i++)
		{
			if(str[i] == '\0')
			{
				flag = 1;
				break;
			}
			*(buf+len) = str[i];
			*(buf+len+1) = '\0';
			len++;
		}
		if(flag) break;
		recv(sockfd, str, 50, 0);
	}
	if(strcmp(buf, "FOUND") == 0)
	{
		char command[300];
		printf("enter command: ");
		scanf("\n");
		scanf("%[^\n]%*c", command);
		while(strcmp(command, "exit"))
		{
			//send(sockfd, command, strlen(command) + 1, 0);
			len = 0; flag = 0;
		while(!flag)
		{
			for(int i=0; i<50; i++, len++)
			{
				if(command[len] == '\0') 
				{
					flag = 1;
					str[i] = command[len];
					break;
				}
				str[i] = command[len];
			}
			send(sockfd, str, 50, 0);
		}
			for(i=0; i < 300; i++) buf[i] = '\0';
			
			recv(sockfd, str, 50, 0);
			len=0, flag=0;
			while(!flag)
			{
				for(int i=0; i<50; i++)
				{
					if(str[i] == '\0')
					{
						flag = 1;
						break;
					}
					*(buf+len) = str[i];
					*(buf+len+1) = '\0';
					len++;
				}
				if(flag) break;
				recv(sockfd, str, 50, 0);
			}
			
			if(strcmp(buf, "$$$$") == 0)
			{
				printf("Invalid command\n");
			}
			else if(strcmp(buf, "####") == 0)
			{
				printf("Error in running command\n");
			}
			else printf("%s\n", buf);
			printf("enter command: ");
			scanf("\n");
			scanf("%[^\n]%*c", command);
		}
		send(sockfd, command, strlen(command) + 1, 0);
			
	}
	else if(strcmp(buf, "NOT_FOUND") == 0)
	{
		printf("Invalid username\n");
		close(sockfd);
		return 0;
	}

	close(sockfd);
	return 0;

}


