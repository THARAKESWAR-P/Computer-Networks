/*    THE CLIENT PROCESS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_LEN 1024
#define MAXLIST 100

int main()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    
    time_t tick;
    struct tm *ts;
    struct pollfd p;
    memset(&p, 0, sizeof(p));
    p.events = POLLIN;

    while (1)
    {
        char input[MAX_LEN], buf[MAX_LEN];

        int NoOfOwnCmds = 3, switchOwnArg = 0, NoOfParsed = 0, i, isPort = 0;
        char *ListOfOwnCmds[NoOfOwnCmds];
        char parsed[MAXLIST][MAX_LEN];
        char filename[100];

        ListOfOwnCmds[0] = "QUIT";
        ListOfOwnCmds[1] = "GET";
        ListOfOwnCmds[2] = "PUT";

        printf("MyOwnBrowser> ");
        if (fgets(input, MAX_LEN, stdin) == NULL)
        {
            continue;
        }
        // printf("punk..\n");

        strcpy(buf, input);

        char *token = strtok(buf, " \n");
        while (token != NULL)
        {
            strcpy(parsed[NoOfParsed++], token);
            token = strtok(NULL, " \n");
        }

        for (i = 0; i < NoOfOwnCmds; i++)
        {
            if (strcmp(parsed[0], ListOfOwnCmds[i]) == 0)
            {
                switchOwnArg = i + 1;
                break;
            }
        }
        if (switchOwnArg == 3)
        {
            strcpy(filename, parsed[NoOfParsed - 1]);
            NoOfParsed--;
        }
        else if (switchOwnArg == 1)
        {
            printf("\nGoodbye\n");
            exit(0);
        }
        else if (switchOwnArg == 2)
        {
            /* code */
        }
        else
        {
            printf("Invalid command\n");
            continue;
        }

        token = strtok(parsed[NoOfParsed - 1], "/");
        if ((token = strtok(NULL, "/")) != NULL)
        {
            strcpy(parsed[NoOfParsed++], token);
        }

        // strcpy(parsed[NoOfParsed], "/");
        token = strtok(NULL, "\n");
        strcpy(parsed[NoOfParsed++], token);

        // port number stored in NoOfParsed-1 index
        token = strtok(parsed[NoOfParsed - 1], ":");
        if ((token = strtok(NULL, ":")) != NULL)
        {
            strcpy(parsed[NoOfParsed++], token);
            isPort = 1;
        }

        /*for (i = 0; i < NoOfParsed; i++) {
            printf("%s\n", parsed[i]);
        }*/
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Unable to create socket\n");
            exit(0);
        }

        serv_addr.sin_family = AF_INET;
        printf("%s\n", parsed[2]);
        inet_aton(parsed[2], &serv_addr.sin_addr);
        serv_addr.sin_port = htons(80);
        if(isPort) serv_addr.sin_port = htons(atoi(parsed[NoOfParsed-1]));
        if ((connect(sockfd, (struct sockaddr *)&serv_addr,
                     sizeof(serv_addr))) < 0)
        {
            perror("Unable to connect to server\n");
            exit(0);
        }

        char request[MAX_LEN];
        strcpy(request, parsed[0]);
        strcat(request, " ");

        char accept[100], extension[10], key[100];
        accept[0] = '\0';
        extension[0] = '\0';
        key[0] = '\0';
        if (isPort)
            strcpy(accept, parsed[NoOfParsed - 2]);
        else
            strcpy(accept, parsed[NoOfParsed - 1]);
        strcat(request, accept);
        if (switchOwnArg == 3)
        {
            strcat(request, "/");
            strcat(request, filename);
        }

        strcat(request, " HTTP/1.1");

        strcat(request, "\r\nHost: ");
        strcat(request, parsed[2]);

        tick = time(NULL);
        strcat(request, "\r\nDate: ");
        strcat(request, ctime(&tick));

        strcat(request, "Accept: ");
        token = strtok(accept, ".");
        if ((token = strtok(NULL, ".")) != NULL)
        {
            strcpy(extension, token);
        }
        if (strcmp(extension, "html") == 0)
        {
            strcpy(key, "text/html");
        }
        else if (strcmp(extension, "pdf") == 0)
        {
            strcpy(key, "application/pdf");
        }
        else if (strcmp(extension, "jpg") == 0)
        {
            strcpy(key, "image/jpeg");
        }
        else
        {
            strcpy(key, "text/*");
        }
        strcat(request, key);

        strcat(request, "\r\nAccept-Language: en-us,en;q=0.7");

        strcat(request, "\r\nConnection: close"); // Remove /r/n

        int timeout=0;
        switch (switchOwnArg)
        {
        case 1:
            printf("\nGoodbye\n");
            close(sockfd);
            exit(0);
        case 2:
            strcat(request, "\r\nIf-Modified-Since: ");
            tick -= (2 * 24 * 60 * 60);
            ts = localtime(&tick);
            char temp[100];
            strftime(temp, sizeof(temp), "%A, %d %m %Y %X GMT", ts);
            strcat(request, temp);
            strcat(request, "\r\n\r\n");
            printf("Request Headers:\n%s", request);
            send(sockfd, request, strlen(request) + 1, 0);
            char temp1[100];
            int len;
            strcat(accept, ".");

            strcat(accept, extension);
            accept[strlen(accept)] = '\0';
            p.fd = sockfd;
            if (poll(&p, 1, 3000) <= 0)
            {
                printf("\nClient connection closed\n\n");
                close(sockfd);
                timeout = 1;
                break;
            }
            
            if (strcmp(extension, "html") == 0)
            {
                FILE *fp = fopen("output.html", "w+");
                int flg = 0;
                char pc = '\0', cc = '\0';
                int i = 0, j=0;
                while (1)
                {
                    char buf[300];
                    int k = recv(sockfd, buf, 299, 0);
                    if (k <= 0)
                        break;

                    buf[k] = '\0';
                    if (flg == 0)
                    {

                        printf("%s", buf);
                        for (i = 0; i < strlen(buf); i++)
                        {
                            if (buf[i] == '\r' && pc == '\n')
                            {
                                i = i + 2;
                                flg = 1;
                                break;
                            }
                            pc = buf[i];
                        }
                    }
                    if(j==0 && strncmp(buf+9, "200", 3) != 0)
                    {
                        flg = 0;
                        close(sockfd);
                        break;
                    }
                    if (flg == 1)
                    {
                        for (int j = i; j < strlen(buf); j++)
                        {
                            fprintf(fp, "%c", *(buf + j));
                        }
                    }
                    j++;
                }
                fclose(fp);
                int p1 = fork();
                if (p1 == 0)
                {
                    char *a[] = {"firefox", "output.html", NULL};
                    execvp(a[0], a);
                }
            }
            if (strcmp(extension, "pdf") == 0)
            {
                FILE *fp = fopen("output.pdf", "w+");
                int flg = 0;
                char pc = '\0', cc = '\0';
                int i = 0, j=0;
                while (1)
                {
                    char buf[300];
                    int k = recv(sockfd, buf, 299, 0);
                    if (k <= 0)
                        break;
                    buf[k] = '\0';
                    i = 0;
                    if (flg == 0)
                    {
                        printf("%s", buf);
                        for (; i < k; i++)
                        {
                            if (buf[i] == '\r' && pc == '\n')
                            {
                                i = i + 2;
                                flg = 1;
                                break;
                            }
                            pc = buf[i];
                        }
                    }
                    if(j==0 && strncmp(buf+9, "200", 3) != 0)
                    {
                        flg = 0;
                        close(sockfd);
                        break;
                    }
                    if (flg == 1)
                    {
                        for (int j = i; j < k; j++)
                        {
                            fprintf(fp, "%c", *(buf + j));
                        }
                    }
                    j++;
                }
                fclose(fp);
                int p1 = fork();
                if (p1 == 0)
                {
                    char *a[] = {"open", "output.pdf", NULL};
                    execvp(a[0], a);
                }
            }
            if (strcmp(extension, "jpg") == 0)
            {
                FILE *fp = fopen("output.jpg", "w+");
                int flg = 0;
                char pc = '\0', cc = '\0';
                int i = 0, j=0;
                while (1)
                {
                    char buf[300];
                    int k = recv(sockfd, buf, 299, 0);
                    if (k <= 0)
                        break;
                    buf[k] = '\0';
                    i = 0;
                    if (flg == 0)
                    {
                        printf("%s", buf);
                        for (; i < k; i++)
                        {
                            if (buf[i] == '\r' && pc == '\n')
                            {
                                i = i + 2;
                                flg = 1;
                                break;
                            }
                            pc = buf[i];
                        }
                    }
                    if(j==0 && strncmp(buf+9, "200", 3) != 0)
                    {
                        flg = 0;
                        close(sockfd);
                        break;
                    }
                    if (flg == 1)
                    {
                        for (int j = i; j < k; j++)
                        {
                            fprintf(fp, "%c", *(buf + j));
                        }
                    }
                    j++;
                }
                fclose(fp);
                int p1 = fork();
                if (p1 == 0)
                {
                    char *a[] = {"open", "output.jpg", NULL};
                    execvp(a[0], a);
                }
            }
            close(sockfd);
            break;
        case 3:
            strcat(request, "\r\nContent-language: en-us;q=0.7");
            int nc = 0;
            FILE *fp1;
            fp1 = fopen(filename, "r");
            fseek(fp1, 0, SEEK_END);
            nc = ftell(fp1);
            fclose(fp1);
            strcat(request, "\r\nContent-length: ");
            char result[50];
            sprintf(result, "%d", nc);
            strcat(request, result);
            strcat(request, "\r\n\r\n");
            printf("Request Headers:\n%s", request);
            send(sockfd, request, strlen(request) + 1, 0);
            FILE *fp = fopen(filename, "r");

            char buf1[2];
            recv(sockfd, buf1, 2, 0);
            if (fp == NULL)
            {
                perror("FILE NOT-FOUND\n");
                close(sockfd);
            }
            else
            {
                char sendo[50];
                int y = 0;
                while (!feof(fp))
                {
                    char c;
                    c = fgetc(fp);
                    if (y == 50)
                    {
                        y = 0;
                        send(sockfd, sendo, 50, 0);
                        sendo[y++] = c;
                    }
                    else
                    {
                        sendo[y++] = c;
                    }
                }
                if (1)
                {
                    if (y != 50)
                        sendo[y] = '\0';
                    send(sockfd, sendo, y, 0);
                }
            }
            fclose(fp);
            p.fd = sockfd;
            if (poll(&p, 1, 3000) <= 0)
            {
                printf("\nClient connection closed\n\n");
                close(sockfd);
                timeout = 1;
                break;
            }
            char s[5000];
            len = recv(sockfd, s, 5000, 0); // can change
            s[len] = '\0';
            printf("%s\n\n", s);
            close(sockfd);
            break;
        default:
            break;
        }
        // free(input);
        // free(buf);
        // free(parsed);
        // free(ListOfOwnCmds);
        // free(filename);
    }
    return 0;
}