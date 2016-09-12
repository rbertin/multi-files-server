#include "server.h"


int get_request_type(char *input, char *type)
{
	unsigned int i = 0;

	type = malloc(sizeof(char) * 124);
	if (type == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);		
	}

	while(input[i] != '\0' && i < 124)
	{
		if (input[i] == ' ' || input[i] == '\n') {
			break;
		}
		type[i] = input[i];
		i++;
	}
	type[i] = '\0';
	
	if (strcmp(type, "GET") == 0) {
		free(type);
		return (REQUEST_GET);
	}
	if (strcmp(type, "LISTFILES") == 0) {
		free(type);
		return (REQUEST_LISTFILES);
	}

	free(type);
	return (REQUEST_NOT_IMPLEMENTED);
}

char *removeSpaces(char *input)
{
	char *ptr = input;
	char *tmp = NULL;

	while(*ptr && *ptr == ' ' && *ptr != '\0')
		ptr++;

	tmp = ptr;
	while(*tmp && *tmp != '\0') {
		if (*tmp == '\n') {
			*tmp = '\0';
		}
		tmp++;	
	}

	return ptr;
}

int request_list_files(int sock, char *input)
{
	char filename[MAX_PATH];
	struct dirent *pdir = NULL;
	DIR *dir = NULL;
	char *ptr = NULL;

	input = &input[strlen("LISTFILES")];
	ptr = removeSpaces(input);
	dir = opendir(ptr);
	if (dir == NULL) {
		send(sock, "Directory error\n", 16, 0);
	   	return (DIRECTORY_NO_EXISTS);	
	}

	while((pdir = readdir(dir))) {
		snprintf(filename, MAX_PATH, "%s\n", pdir->d_name);
		send(sock, filename, strlen(filename), 0);
	}

	closedir(dir);

	return (1);
}

unsigned int request_get_files(int sock, char *input)
{
	char *ptr = NULL;
	char **list = NULL;
	struct stat stats;
	unsigned int count = 0;
	unsigned int offset = 0;
	char trame[124] = {0};
	int fd;
	unsigned char characters;

	input = &input[strlen("GET")];
	input = removeSpaces(input);
	
	list = (char**)malloc(sizeof(char*) + 1);
	
	ptr = strtok(input, ";");
	if (ptr == NULL) {
		send(sock, "Bad syntax for GET\n", 19, 0);
		return (0);
	}

	do {
		list[offset] = malloc(sizeof(char) * strlen(ptr) + 1);
		strncpy(list[offset], ptr, strlen(ptr));
		list[offset++][strlen(ptr)] = '\0';
		ptr = strtok(NULL, ";");
	} while(ptr != NULL);
	
	snprintf(trame, sizeof(trame) - 1, "[%d]\r\n", offset);
	send(sock, trame, strlen(trame), 0);
	
	while(count < offset)
	{
		if (stat(list[count], &stats) == -1) {
			switch(errno)
			{
				case EACCES:
					snprintf(trame, sizeof(trame) - 1, "File: %s\nError: Denied\r\n", list[count]);
				break;	
				case ENOENT:
					snprintf(trame, sizeof(trame) - 1, "File: %s\nError: Not Found\r\n", list[count]);
				break;
				default:
					snprintf(trame, sizeof(trame) - 1, "File: %s\nError: Unknown\r\n", list[count]);
				break;
			}
			send(sock, trame, strlen(trame), 0);
		}
		else {
			snprintf(trame, sizeof(trame) - 1, "File: %s\nSize: %lu\r\n", list[count], stats.st_size);
			send(sock, trame, strlen(trame), 0);
			fd = open(list[count], O_RDONLY);
			if (fd == -1) {
				perror("open");
			}
			while(read(fd, &characters, 1) > 0) 
			{
				send(sock, &characters, 1, 0);
			}
		}
		free(list[count++]);
	}
	free(list);
	return (0);
}

unsigned int request_execute(int sock, char *input)
{
	char *type = NULL;
	int result;

	result = get_request_type(input, type);
	switch(result)
	{
		case REQUEST_GET:
			request_get_files(sock, input);
			break;
		case REQUEST_LISTFILES:
			if (request_list_files(sock, input) == DIRECTORY_NO_EXISTS) {
				
			}
			break;
		case REQUEST_NOT_IMPLEMENTED:
			return (0);
		default:
			break;
	}
	return (1);
}

unsigned int get_header_request(int sock)
{
	char *output = NULL;
	int length;
	
	output = malloc(sizeof(char) * LENGTH_BUFFER + 1);
	if (output == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);	
	}

	length = recv(sock, output, LENGTH_BUFFER - 1, 0);
	if (length < 0) {
		perror("recv");
		exit(EXIT_FAILURE);
	}

	if (length == 0)
		return 0;

	output[LENGTH_BUFFER] = '\0';
	request_execute(sock, output);
	free(output);
	return (length);
}

void sig_handler(int sig __attribute__((unused)))
{
	int i = 0;

	while(i < index_client)
	{
		if (fcntl(clients[i].sock, F_GETFL) != -1) 
			close(clients[i].sock);
		i++;
	}

	exit(EXIT_FAILURE);
}

void setnonblocking(int sock)
{
	int result;
	result = fcntl(sock, F_GETFL);
	if (result < 0) {
		perror("fcntl");
		exit(EXIT_FAILURE);
	}
	result = (result | O_NONBLOCK);
	if (fcntl(sock, F_SETFL, result) < 0) {
		perror("fcntl");
		exit(EXIT_FAILURE);
	}
}

unsigned int waitClients(server_t *server)
{
	int csock;
	struct sockaddr_in csin;
	socklen_t recv_size = sizeof(csin);
	fd_set exceptfs;
	fd_set readfs;
	int i = 0;
	int max = server->sock;
	struct timeval timeout;
	int result;

	if (signal(SIGINT, sig_handler) == SIG_ERR) {
		perror("signal");
		exit(EXIT_FAILURE);
	}

	if (listen(server->sock, MAX_CLIENTS) == -1)
	{
		perror("listen");
		exit(EXIT_FAILURE);
	}
	
	while(1)
	{
		FD_ZERO(&readfs);
		FD_SET(server->sock, &readfs);
		
		for (i = 0; i < index_client; i++) 
			FD_SET(clients[i].sock, &readfs);

		timeout.tv_sec = 2;
		timeout.tv_usec = 0;

		result = select(max + 1, &readfs, NULL, &exceptfs, &timeout);
	   	if (result < 0) {	
			perror("select");
			exit(EXIT_FAILURE);
		}

		if (result == 0)
			continue;

		if (FD_ISSET(server->sock, &exceptfs)) 
		{
			exit(EXIT_FAILURE);
		}
		else if (FD_ISSET(server->sock, &readfs)) 
		{
			csock = accept(server->sock, (struct sockaddr*)&csin, &recv_size);
			if (csock < 0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
			/*
			 * Set the socket non blockable
			 */
			setnonblocking(csock);
				
			max = csock > max ? csock : max;
			FD_SET(csock, &readfs);
			clients[index_client].sock = csock;
			index_client++;
		}
		else {

			for (i = 0; i < index_client; i++)
			{
				if (FD_ISSET(clients[i].sock, &readfs)) 
				{
					if (get_header_request(clients[i].sock) == 0)
					{
						memmove(
							clients + i, 
							clients + i + 1, 
							(index_client - i) * sizeof(clients)
						);
						index_client--;
						continue;				
					}
				}
			}
		}
	}
}

unsigned int bindServer(server_t *server)
{
	int err;
	
	err = bind(
		server->sock, 
		(struct sockaddr*)&server->sin, 
		sizeof(server->sin)
	);
	
	if (err == -1) {
		perror("bind");
		return (0);
	}
	printf("Server listen on *:%d\n", SERVER_PORT);
	waitClients(server);
	return (1);
}

unsigned int create_sock(server_t *server)
{
	int reuse_sock = 1;
	int result; 

	server->sock = socket(AF_INET, SOCK_STREAM, 0);
	if (server->sock == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	
	/*
	 * If the server crash it's possible 
	 * that some connetions rests in TIME_WAIT.
	 * In this case we reuse the this socket.
	 */
	result = setsockopt(
		server->sock, 
		SOL_SOCKET, 
		SO_REUSEADDR, 
		&reuse_sock, 
		sizeof(reuse_sock)
	);
	if (result == -1) {
		perror("setsockopt");
		return (0);
	}
	/*
	 * Set the socket non-blocking 
	 */

	server->sin.sin_addr.s_addr = htonl(INADDR_ANY);
	server->sin.sin_family = AF_INET;
	server->sin.sin_port = htons(SERVER_PORT);
	
	return (1);
}

unsigned int create_server()
{
	server_t server;

	if (create_sock(&server) == 0) {
		fprintf(stderr, "Error during the booting of server\n");
		close(server.sock);	
		exit(EXIT_FAILURE);
	}
	
	if (bindServer(&server) == 0) {
		fprintf(stderr, "The server can't bind()\n");
		close(server.sock);
		exit(EXIT_FAILURE);
	}
	
	return (1);
}

int main(void)
{
	create_server();

	return (0);
}

