#define BACKLOG (10)
#define FOURKILOBYTE 4096

/*
 *   Primary function to be passed off to a pthread in order to allow concurrent handling of clients.
 *   Identifies what is in the client's requested path and then responds accordingly
 *
 *   @param socket:   file descriptor for an accepted client
 */
void *handleClient(void* socket);

/*
 *   Serves the client a file based on the file extension at the end of path
 *
 *   @param sock:     file descriptor for an accepted client
 *   @param path:     path to the specific resource that the client wants to access
 */
void clientResponse(int sock, char* path);

/*
 *   Serves the client a directory listing
 *
 *   @param sock:     file descriptor for an accepted client
 *   @param path:     path to the specific resource that the client wants to access
 */
void listingResponse(int sock, char* path);

/*
 *   Serves the client an error 404 page
 *
 *   @param sock:     file descriptor for an accepted client
 */
void errorResponse(int sock);

/*
 *   Constructs and sends an HTTP 1.0 message HTTP to an accepted client
 *
 *   @param sock:          file descriptor for an accepted client
 *   @param responseCode:  response status code to send the client for the request resource
 *   @param filetype:      file type of client's requested resource, used to set HTTP Content-Typer header
 */
void sendHeader(int sock, char* responseCode, char* filetype);

/*
 *   Helper function to send a given ASCII string to the client
 *
 *   @param string:        ASCII string to be sent
 *   @param sock:          file descriptor for an accepted client
 */
void sendHelper(char* string, int sock);