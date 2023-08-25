#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h> 

#define MAX_LEN 1024
void log_File(char* s,int pn,int gop,char *p){
    FILE* fp;
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    char date_time[20];
    strftime(date_time, sizeof(date_time), "%Y%m%d%H%M%S", tm);
    char date[10];
    int d=0;
    char time[10];
    int ti=0;
    for(int i=0;i<14;i++){
        if(i>1 && i<8){
            date[d++]=date_time[i];
        }
        else if(i>=8){
            time[ti++]=date_time[i];
        }
    }
    time[6] = '\0';
    date[6] = '\0';
    fp = fopen("AccessLog.txt","a");
    if(gop == 1){
        fprintf(fp,"%s:%s:%s:%d:GET:%s\n",date,time,s,pn,p);
    }
    else{
        fprintf(fp,"%s:%s:%s:%d:PUT:%s\n",date,time,s,pn,p);
    }
    fclose(fp);
    return ;
}
int main(){
    int sockfd, newsockfd;
    int clilen;
    struct sockaddr_in cli_addr, serv_addr;
    int i;
    char buf[100];
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Cannot create socket\n");
        exit(0);
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080);
    if(bind(sockfd,(struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0){
        perror("unable to bind local addr\n");
        exit(0);
    }
    listen(sockfd,5);
    while(1){
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
        if(newsockfd < 0){
            perror("Accept Error\n");
            exit(0);
        }
        int p = fork();
        char request[MAX_LEN], response[MAX_LEN];
        struct stat att;
        if(p == 0){
            close(sockfd);
            char s[5000];
            int len = recv(newsockfd, s, 5000, 0);//can change
            s[len] = '\0';
            printf("%s\n\n", s);
            //FILE* fp3 = fopen("Access_log.txt","w+");

            time_t tick;
            struct tm *ts;
            time(&tick);
            tick += (3 * 24 * 60 * 60);
            ts = localtime(&tick);
            char temp[100];
            strftime(temp, sizeof(temp), "%A, %d %m %Y %X GMT", ts);
            request[0] = '\0';

            strcat(request, "\r\nExpires: "); // find curr+3 days
            strcat(request, temp);
            strcat(request, "\r\nCache control: no-store");
            strcat(request, "\r\nContent-language: en-us;q=0.7");
            strcat(request, "\r\nContent-length: ");
            //printf("%s\n", request);
            //request[strlen(request)] = '\0';

            //printf("%s\n", request);
            if(s[0] == 'G' && s[1] == 'E' && s[2] == 'T'){
                int i = 4;
                char path[1000];
                int y = 0;
                path[y++] = '/';
                while(1){
                    if(s[i] == ' ') break;
                    else path[y++] = s[i++];
                }
                path[y] = '\0';
                log_File(inet_ntoa(cli_addr.sin_addr),80,1,path);
                stat(path, &att);
                if((time(NULL) - att.st_mtime) > (2 * 24 * 60 * 60))
                {
                    response[0] = '\0';
                    strcat(response, "HTTP/1.1 403 FORBIDDEN");
                    strcat(response, request);
                    strcat(response, "0\r\n\r\n");
                    perror("HTTP/1.1 403 FORBIDDEN\n\n");
                    send(newsockfd,response,strlen(response),0);
                    close(newsockfd);
                    exit(0);
                    continue;
                }

                FILE* fp1;
                fp1 = fopen(path,"r");
                fseek(fp1,0,SEEK_END);
                int nc = ftell(fp1);
                fclose(fp1);
                char result[50];
                //result[0] = '\0';
                
                FILE* fp = fopen(path,"r");
                if(fp == NULL){
                    printf("FOPEN ERROR\n");
                    if(strcmp(strerror(errno), "EACCES") == 0)
                    {
                        response[0] = '\0';
                        strcat(response, "HTTP/1.1 403 FORBIDDEN");
                        strcat(response, request);
                        strcat(response, "0\r\n\r\n");
                        perror("HTTP/1.1 403 FORBIDDEN\n\n");
                    }

                    if(strcmp(strerror(errno), "ENOENT") == 0)
                    {
                        response[0] = '\0';
                        strcat(response, "HTTP/1.1 404 NOT-FOUND");
                        strcat(response, request);   
                        strcat(response, "0\r\n\r\n");  
                        perror("HTTP/1.1 404 NOT-FOUND\n\n");                   
                    }
                    send(newsockfd,response,strlen(response),0);
                }
                else{
                    response[0] = '\0';
                    strcat(response, "HTTP/1.1 200 OK");
                    strcat(response, request);
                    sprintf(result, "%d\r\n\r\n", nc);
                    strcat(response,result);
                    send(newsockfd,response,strlen(response),0);
                    //printf("%s\n", response);
                    /*char *h = "HTTP/1.1 200 OK\r\n\r\n";
                    send(newsockfd, h, strlen(h),0);*/
                    char sendo[50];
                    int y = 0;
                    while (!feof(fp)) // to read file
                    {
                        char c;
                        c = fgetc(fp);
                        if(y == 50){
                            y = 0;
                            send(newsockfd,sendo,50,0);
                            sendo[y++] = c;
                        }
                        else{
                            sendo[y++] = c;
                        }
                    }
                    if(1){
                        if(y != 50) sendo[y] = '\0';
                        send(newsockfd,sendo,y,0);
                    }
                }
                fclose(fp);
            }
            if(s[0] == 'P' && s[1] == 'U' && s[2] == 'T'){
                int i = 4;
                char path[1000];
                int y = 0, flg=0;
                path[y++] = '/';
                while(1)
                {
                    if(s[i] == ' ') break;
                    else{
                        path[y++] = s[i++];
                    }
                }
                path[y] = '\0';

                log_File(inet_ntoa(cli_addr.sin_addr),80,0,path);
                FILE* fp = fopen(path,"w+");
                int nd = 0;
                char pc = '\0';
                if(fp == NULL)
                {
                    printf("FOPEN ERROR\n");
                	if(strcmp(strerror(errno), "EACCES") == 0)
                    {
                        response[0] = '\0';
                        strcat(response, "HTTP/1.1 403 FORBIDDEN");
                        strcat(response, request);
                        strcat(response, "0\r\n\r\n");
                        perror("HTTP/1.1 403 FORBIDDEN\n\n");
                    }

                    if(strcmp(strerror(errno), "ENOENT") == 0)
                    {
                        response[0] = '\0';
                        strcat(response, "HTTP/1.1 404 NOT-FOUND");
                        strcat(response, request);   
                        strcat(response, "0\r\n\r\n"); 
                        perror("HTTP/1.1 404 NOT-FOUND\n\n");                   
                    }
                    send(newsockfd,response,strlen(response),0);
                }
                else
                {
                    i++;
                    int ne = 7;
                    while(1){
                        if(s[i] == '\n') ne--;
                        if(ne == 0) break;
                        i++;
                    }
                    i = i+17;
                    char num[20];
                    int fn = 0;
                    while(i < strlen(s)){
                        if(s[i] == '\r') break;
                        nd = (nd*10)+(s[i]-'0');
                        i++;
                    }
                    ne = 2;
                    while(i < strlen(s)){
                        if(s[i] == '\n') ne--;
                        if(ne == 0){
                            break;
                        }
                        i++;
                    }
                    char buf1[2];
                    buf1[0] = 'u';
                    buf1[1] = '\0';

                    send(newsockfd, buf1, 2, 0);
                    if(nd > 0){
                        while(1){
                            char buf[6];
                            int k = recv(newsockfd,buf,5,0);
                            //printf("%d\n%s\n", k, buf);
                            for(int i = 0;i < k;i++){
                                fprintf(fp,"%c",buf[i]);
                            }
                            nd = nd-k;
                            if(nd <= 0) break;
                        }
                    }
                    response[0] = '\0';
                    strcat(response, "HTTP/1.1 200 OK");
                    strcat(response, request);
                    strcat(response, "0\r\n\r\n");
                    send(newsockfd,response,strlen(response),0);
                }
                fclose(fp);   
            }
            else
            {
                response[0] = '\0';
                strcat(response, "HTTP/1.1 400 BAD-REQUEST");
                strcat(response, request);
                strcat(response, "0\r\n\r\n");
                perror("HTTP/1.1 400 BAD-REQUEST\n\n");                   

                send(newsockfd,response,strlen(response),0);
            }
            close(newsockfd);
            exit(0);
        }
		close(newsockfd);
    }
    return 0;
}
