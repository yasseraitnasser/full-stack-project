#include "header.h"

int	main()
{
	printf("Configuring local address...\n");
	struct addrinfo	hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	struct addrinfo	*bind_addresss;
	getaddrinfo(0, "8080", &hints, &bind_addresss);

	printf("Creating socket...\n");
	int	socket_fd = socket(bind_addresss->ai_family,
		bind_addresss->ai_socktype, bind_addresss->ai_protocol);

	printf("Binding socket to local address...\n");
	if (bind(socket_fd, bind_addresss->ai_addr,
		bind_addresss->ai_addrlen))
	{
		fprintf(stderr, "bind() failed. (%d)\n", errno);
		return 1;	
	}
	freeaddrinfo(bind_addresss);

	printf("Listenning...\n");
	if (listen(socket_fd, 10) < 0)
	{
		fprintf(stderr, "listen() failed. (%d)\n", errno);
		return 1;	
	}

	printf("Waiting for connection...\n");
	struct sockaddr_storage	client_address;
	socklen_t	client_len = sizeof(client_address);
	int	client_fd = accept(socket_fd,
		(struct sockaddr*)&client_address, &client_len);
	if (client_fd < 0)
	{
		fprintf(stderr, "accept() failed. (%d)\n", errno);
		return 1;
	}

	printf("Client is connected... ");
	char address_buffer[100];
	getnameinfo((struct sockaddr*)&client_address,
	client_len, address_buffer, sizeof(address_buffer), 0, 0,
	NI_NUMERICHOST);
	printf("%s\n", address_buffer);

	printf("Reading request...\n");
	char	request[1024];
	int	bytes_received = read(client_fd, request, 1024);
	printf("Reaceived %d bytes\n", bytes_received);

	printf("%.*s", bytes_received, request);

	printf("Sending response...\n");
	const char	*response =
		"HTTP/1.1 200 OK\r\n"
		"Connection: close\r\n"
		"Content-Type: text/plain\r\n\r\n"
		"Local time is: ";
	int	bytes_sent = write(client_fd, response, strlen(response));
	printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(response));


	time_t	timer;
	time(&timer);
	char	*time_msg = ctime(&timer);
	bytes_sent = write(client_fd, time_msg, strlen(time_msg));
	printf("Sent %d of %d bytes.\n", bytes_sent, (int)strlen(time_msg));

	printf("Closing connection...\n");
	close(client_fd);
	printf("Closing listenning socket...\n");
	close(socket_fd);
	printf("Finished.\n");

	return (0);
}
