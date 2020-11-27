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


/* This program takes two arguments:
 * 1) The port number on which to bind and listen for connections, and
 * 2) The directory out of which to serve files.
 */

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

    /* Change directory to server_documents */
    chdir("server_documents");
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