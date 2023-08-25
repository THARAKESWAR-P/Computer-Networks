#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

int findUser(char *filename, char *username) 
{
	FILE *fp;
	int line_no = 1;
	int cnt = 0;
	char temp[250];

	if((fp = fopen(filename, "r")) == NULL) return(-1);

	while(fgets(temp, 250, fp) != NULL) 
	{
		// printf("str: %s temp: %s\n", username, temp);
		temp[strlen(temp)-1] = '\0';
		if((strcmp(temp, username)) == 0) 
		{
			printf("Found line: %d\n", line_no);
			printf("\n%s\n", temp);
			cnt++;
		}
		line_no++;
	}
	
	if(fp) fclose(fp);

   	return cnt;
}


int main()
{
	int			sockfd, newsockfd; 
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;

	int i;

	char buf[300], str[50];

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Cannot create socket\n");
		exit(0);
	}

	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);

	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	{
		printf("Unable to bind local address\n");
		exit(0);
	}

	listen(sockfd, 5);
	
	while (1) {
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen) ;

		if (newsockfd < 0) {
			printf("Accept error\n");
			exit(0);
		}

		if (fork() == 0) {
			close(sockfd);

			strcpy(buf, "LOGIN:");
			send(newsockfd, buf, strlen(buf) + 1, 0);

			for(i=0; i < 300; i++) buf[i] = '\0';

			recv(newsockfd, str, 50, 0);
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
				recv(newsockfd, str, 50, 0);
			}
			printf("%s\n", buf);

			if(findUser("users.txt", buf) > 0)
			{
				strcpy(buf,"FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);

				while(strcmp(buf, "exit"))
				{
					recv(newsockfd, str, 50, 0);
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
						recv(newsockfd, str, 50, 0);
					}
					if(strcmp(buf, "exit")==0) break;
					char cwd[300];
					// char *ptr = strtok(buf, " ");
					// char *arg = strtok(NULL, "\n");
					char ptr[200], arg[200];
					len = 0;
					while(buf[len] == ' ') len++;
					int j=0, k=0;
					while(buf[len] != ' ' && buf[len] != '\0') ptr[j++] = buf[len++];
					ptr[j] = '\0';
					//printf("ptr: %s arg: %s\n", ptr, arg);
					while(buf[len] == ' ') len++;
					while(buf[len] != ' ' && buf[len] != '\0') arg[k++] = buf[len++];
					arg[k] = '\0';
					//printf("ptr: %s arg: %s\n", ptr, arg);
					while(buf[len] == ' ') len++;
					printf("ptr: %s arg: %s\n", ptr, arg);

					if(strcmp(ptr, "pwd")==0)
					{
						// printf("pwd\n");
    					if (getcwd(cwd, sizeof(cwd)) == NULL)
    					{
      						perror("Error-getcwd()");
      						strcpy(buf, "####");
      						// send(newsockfd, "####", strlen("####") + 1, 0);
      					}	
    					else
    					{
      						printf("Current directory is: %s\n", cwd);
      						strcpy(buf, cwd);
							// send(newsockfd, cwd, strlen(cwd) + 1, 0);
						}
					}
					else if(strcmp(ptr, "dir")==0)
					{
						// printf("dir\n");
						DIR *directory; 
						if(strcmp(arg, "")!=0) directory = opendir(arg);
						else directory = opendir(".");
    					struct dirent *dr;
    					if (directory == NULL) {
        					perror ("Error-opendir()");
        					strcpy(buf, "####");
    					}else
    					{
    						buf[0] = '\0';
    						while ((dr = readdir(directory)) != NULL) {
    							printf("%s\n", dr->d_name); 
    							strcat(buf, dr->d_name);
    							strcat(buf, "\n");
    						}
    						// send(newsockfd, buf, strlen(buf) + 1, 0);
    						closedir(directory);
    					}
					}
					else if(strcmp(ptr, "cd")==0)
					{
						// printf("cd\n");
						if(strcmp(arg, "")==0) strcpy(arg, "/home");
						if (chdir(arg) != 0)
						{
    						perror("Error-chdir()");
    						strcpy(buf, "####");
    						// send(newsockfd, "####", strlen("####") + 1, 0);
    					}	
  						else 
  						{
    						if (getcwd(cwd, sizeof(cwd)) == NULL)
    						{
      							perror("Error-getcwd()");
      							strcpy(buf, "####");
      							// send(newsockfd, "####", strlen("####") + 1, 0);
      						}
    						else
    						{
      							printf("Current directory is: %s\n", cwd);
      							strcpy(buf, cwd);
      							// send(newsockfd, cwd, strlen(cwd) + 1, 0);
      						}
  						}
					}
					else strcpy(buf, "$$$$"); // send(newsockfd, "$$$$", strlen("$$$$") + 1, 0);
					
					// send(newsockfd, buf, strlen(buf) + 1, 0);
					len = 0; flag = 0;
					while(!flag)
					{
						for(int i=0; i<50; i++, len++)
						{
							if(buf[len] == '\0') 
							{
								flag = 1;
								str[i] = buf[len];
								break;
							}
							str[i] = buf[len];
						}
						send(newsockfd, str, 50, 0);
					}				
				}
			}
			else
			{
				strcpy(buf,"NOT_FOUND");
				send(newsockfd, buf, strlen(buf) + 1, 0);	
			}
			close(newsockfd);
			exit(0);
		}
		close(newsockfd);
	}
	return 0;
}
