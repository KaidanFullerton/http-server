#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>

#include "server.h"

void *handleClient(void* socket){

    int *dynSock = (int *) socket;
    int sock = *dynSock;

    free(dynSock);

    char path[100];
    char buf[FOURKILOBYTE];
    memset(buf, 0, sizeof(buf));
    /* Receive HTTP 1.0 GET request from the client */
    int data_recieved = 0; int recv_count = 1;

    while(strstr(buf, "\r\n\r\n") == NULL){
        recv_count = recv(sock, buf + data_recieved, FOURKILOBYTE - data_recieved, 0);
        if(recv_count < 0){
            perror("recv");
            exit(1);
        }
        else{
            data_recieved += recv_count;
        }
    }
    /* Parse requested path from GET request */
    char* endPath = strchr(buf + 5, ' ');
    memset(path, 0, sizeof(path));
    memcpy(path, buf+5, endPath - (buf+5));
    if(strlen(path) == 0){
      strcpy(path, "/");
    }

    /* Check if path is valid and send appropriately to the client */
    struct stat statresult;
    if (stat(path, &statresult)) {
        errorResponse(sock);
    }
    else{
        if(S_ISDIR(statresult.st_mode)){ /* is a directory */
            /* check for index.html file */
            char indexFinder[200]; 
            memset(indexFinder,0,sizeof(indexFinder));
            if(strcmp(path, "/") == 0){
              sprintf(indexFinder, "%sindex.html", path+1);
            }
            else{
              sprintf(indexFinder, "%sindex.html", path);
            }
            if(stat(indexFinder, &statresult)) { /* index.html doesn't exist, serve directory listing */
                listingResponse(sock, path);
            }
            else{ /* index.html exists, serve it to client */
                clientResponse(sock,indexFinder);
            }
        }
        else{ /* not a directory, must be a file */
            clientResponse(sock,path);
        }
    }
    close(sock);
    return NULL;
}

void clientResponse(int sock, char* path)
{
    char buf[FOURKILOBYTE];
    memset(buf, 0, sizeof(buf));

    /* find file extension */
    char* extension = strrchr(path,'.') + 1;
    char* filetype;
    if      (strcmp(extension,"html") == 0) filetype = "text/html";
    else if (strcmp(extension,"txt")  == 0) filetype = "text/html";
    else if (strcmp(extension,"jpeg") == 0) filetype = "image/jpeg";
    else if (strcmp(extension,"jpg")  == 0) filetype = "image/jpg";
    else if (strcmp(extension,"gif")  == 0) filetype = "image/gif";
    else if (strcmp(extension,"png")  == 0) filetype = "image/png";
    else if (strcmp(extension,"pdf")  == 0) filetype = "application/pdf";
    else if (strcmp(extension,"ico")  == 0) filetype = "image/x-icon";
    else filetype = "";
    sendHeader(sock, "200 OK", filetype);
    
    /* read in file and send to client */
    FILE *fp = NULL;
    fp = fopen(path, "r");
    int data_to_send = 0;
    int data_sent = 0; int send_count = 0;
    while (data_to_send += fread(buf+data_to_send,1,FOURKILOBYTE-data_to_send,fp)) {
        while(data_sent != data_to_send){
            send_count = send(sock, buf + data_sent, data_to_send - data_sent, 0);
            if(send_count < 0){
                perror("send");
                exit(1);
            }
            else{
                data_sent += send_count;
            }
        }
        data_to_send = 0;
        data_sent = 0;
    }
    fclose(fp);
}

void errorResponse(int sock){
    char buf[FOURKILOBYTE];
    memset(buf, 0, sizeof(buf));

    sendHeader(sock, "404 NOT FOUND", "text/html");

    FILE *fp;
    fp = fopen("error.html", "r");
    fread(buf, 1, FOURKILOBYTE, fp);
    fclose(fp);
    int data_to_send = strlen(buf);
    int data_sent = 0; int send_count = 0;

    while(data_sent != data_to_send){
        send_count = send(sock, buf + data_sent, data_to_send - data_sent, 0);

        if(send_count < 0){
            perror("send");
            exit(1);
        }
        else{
            data_sent += send_count;
        }
    }
}

void sendHeader(int sock, char* responseCode, char* filetype){
    char buf[FOURKILOBYTE];
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n", responseCode, filetype);
    sendHelper(buf,sock);
}

void listingResponse(int sock, char* path){
    char placeholder[FOURKILOBYTE];
    memset(placeholder, 0, sizeof(placeholder));

    struct dirent *val;
    DIR *d = opendir(path);
    if(d == NULL){
        perror("directory");
        exit(1);
    }

    /* send open tags of html page */
    sendHeader(sock,"200 OK","text/html");
    sprintf(placeholder, "<html>Directory listing for: %s <br/><ul>", path);
    sendHelper(placeholder, sock);

    while((val = readdir(d)) != NULL ){
        /* send each list item */
        memset(placeholder, 0, sizeof(placeholder));
        sprintf(placeholder, "<li><a href=\"%s/\">%s</a></li>", val->d_name, val->d_name);
        sendHelper(placeholder, sock);
    }

    /* send close tags of html page */
    sendHelper("</ul></html>", sock);

    if(closedir(d) != 0){
        perror("Closing directory");
        exit(1);
    }
}


void sendHelper(char* string, int sock){

    int data_to_send = strlen(string);
    int data_sent = 0; int send_count = 0;

    while(data_sent != data_to_send){
        send_count = send(sock, string + data_sent, data_to_send - data_sent, 0);

        if(send_count < 0){
            perror("send");
            exit(1);
        }
        else{
            data_sent += send_count;
        }
    }


}

