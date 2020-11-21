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

#define BACKLOG (10)
#define FOURKILOBYTE 4096

/* This program takes two arguments:
 * 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */

/* primary function to serve an accepted client the data they requested */
void *handleClient(void* socket);

void clientResponse(int sock, char* path);

void listingResponse(int sock, char* path);

void errorResponse(int sock);

void sendHeader(int sock, char* responseCode, char* filetype);

void sendHelper(char* string, int sock);

int main(int argc, char **argv) {
    /* Used for checking return values. */
    int retval;

    /* Ensure proper command line input. */
    if (argc != 3) {
        printf("Usage: %s <port> <document root>\n", argv[0]);
        exit(1);
    }

    /* Read the port number from the first command line argument. */
    int port = atoi(argv[1]);

    printf("Webserver configuration:\n");
    printf("\tPort: %d\n", port);
    printf("\tDocument root: %s\n", argv[2]);

    /* Create a socket to which clients will connect. */
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) {
        perror("Creating socket failed");
        exit(1);
    }

    /* Immediately frees the socket for reuse after the server shuts down. */
    int reuse_true = 1;
    retval = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &reuse_true,
                        sizeof(reuse_true));
    if (retval < 0) {
        perror("Setting socket option failed");
        exit(1);
    }

    /* Create an address structure which tells the OS which port and address
     * to bind to in order to receive incoming connections. */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    /* This system call asks the OS to bind the socket to address and port
     * specified above. */
    retval = bind(server_sock, (struct sockaddr *)&addr, sizeof(addr));
    if(retval < 0) {
        perror("Error binding to port");
        exit(1);
    }

    /* Now that we've bound to an address and port, we tell the OS that we're
     * ready to start listening for client connections.  This effectively
     * activates the server socket.  BACKLOG (#defined above) tells the OS how
     * many clients to queue and reserve for incoming connections that have not 
     * yet been accepted. */
    retval = listen(server_sock, BACKLOG);
    if(retval < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    /* Change directory to test_documents */
    chdir("test_documents");
    while(1) {
        /* Declare a socket for the client connection. */
        int sock;

        /* Another address structure. This time, the system will automatically
         * fill it in, when we accept a connection, to tell us where the
         * connection came from. */
        struct sockaddr_in remote_addr;
        unsigned int socklen = sizeof(remote_addr);

        /* Accept the first waiting connection from the server socket and
         * populate the address information.  The result (sock) is a socket
         * descriptor for the newly connected client. If there are no pending
         * connections in the back log, this function will block indefinitely
         * while waiting for a client connection to be made.
         * */
        sock = accept(server_sock, (struct sockaddr *) &remote_addr, &socklen);
        if(sock < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        int *argument = malloc(sizeof(int));
        *argument = sock;

        pthread_t new_thread;

        int retval = pthread_create(&new_thread, NULL,
                                    handleClient, argument);
        if (retval) {
            printf("pthread_create() failed\n");
            exit(1);
        }

        retval = pthread_detach(new_thread);
        if (retval) {
            printf("pthread_detach() failed\n");
            exit(1);
        }
    }
    close(server_sock);
}


void *handleClient(void* socket){

    int *dynSock = (int *) socket;
    int sock = *dynSock;

    free(dynSock);

    char path[100];
    char buf[FOURKILOBYTE];
    memset(buf, 0, sizeof(buf));
    /* Receive from the client */
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
        if(S_ISDIR(statresult.st_mode)){ // is directory
            // check for index.html file,
            char indexFinder[200]; 
            memset(indexFinder,0,sizeof(indexFinder));
            if(strcmp(path, "/") == 0){
              sprintf(indexFinder, "%sindex.html", path+1);
            }
            else{
              sprintf(indexFinder, "%sindex.html", path);
            }
            if(stat(indexFinder, &statresult)) {
                listingResponse(sock, path);
            }
            else{
                clientResponse(sock,indexFinder);
            }
        }
        else{ // not a directory, must be a file
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
    // find file extension
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
    sendHeader(sock,"200 OK",filetype);
    // read in file
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
    // fill in HTTP header to buffer
    sendHeader(sock,"404 NOT FOUND","text/html");
    FILE *fp;
    fp = fopen("error.html", "r");
    fread(buf, 1, FOURKILOBYTE,fp);
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

// fill the buffer with the appropriate header
void sendHeader(int sock, char* responseCode, char* filetype)
{
    char buf[FOURKILOBYTE];
    memset(buf,0,sizeof(buf));
    sprintf(buf,"HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n",responseCode,filetype);
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
    sendHeader(sock,"200 OK","text/html");
    sprintf(placeholder, "<html>Directory listing for: %s <br/><ul>", path);
    sendHelper(placeholder, sock);

    while((val = readdir(d)) != NULL ){
        // helper function to send list item
        memset(placeholder, 0, sizeof(placeholder));
        sprintf(placeholder, "<li><a href=\"%s/\">%s</a></li>", val->d_name, val->d_name);
        sendHelper(placeholder, sock);
    }

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

