/*
 * Requests types 
 */
#define REQUEST_GET 1
#define REQUEST_LISTFILES 2
#define REQUEST_NOT_IMPLEMENTED -1

#define MAX_PATH 1024
#define LENGTH_BUFFER 1024
#define MAX_CLIENTS 10
#define SERVER_PORT 8989

/*
 * Errors
 */

#define DIRECTORY_NO_EXISTS -1

typedef struct 
{
	int sock;
	struct sockaddr_in sin;	
} server_t;

typedef struct 
{
	int sock;
} client_t;

client_t clients[MAX_CLIENTS];
int index_client = 0;

int get_request_type(char *input, char *type);
char *removeSpaces(char *input);
int request_list_files(int sock, char *input);
unsigned int request_get_files(int sock, char *input);
unsigned int request_execute(int sock, char *input);
unsigned int get_header_request(int sock);
void sig_handler(int sig __attribute__((unused)));
void setnonblocking(int sock);
unsigned int waitClients(server_t *server);
unsigned int bindServer(server_t *server);
unsigned int create_sock(server_t *server);
unsigned int create_server();
