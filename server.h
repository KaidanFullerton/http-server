#define BACKLOG (10)
#define FOURKILOBYTE 4096


/* primary function to serve an accepted client the data they requested */
void *handleClient(void* socket);

void clientResponse(int sock, char* path);

void listingResponse(int sock, char* path);

void errorResponse(int sock);

void sendHeader(int sock, char* responseCode, char* filetype);

void sendHelper(char* string, int sock);