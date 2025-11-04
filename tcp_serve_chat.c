#include "header.h"

int	main()
{
	printf("Configuring local address...\n");
	struct addrinfo	hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_socktype = SOCK_STREAM;

	struct addrinfo	*bind_address;
	getaddrinfo(0, "8080", &hints, &bind_address);

	printf("Creating socket...\n");
	int	socket_listen = socket(bind_address->ai_family,
		bind_address->ai_socktype, bind_address->ai_protocol);

	printf("Binding socket to local address...\n");
	if (bind(socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
	{
		fprintf(stderr, "listen() failed. (%d)\n", errno);
		return 1;
	}
	freeaddrinfo(bind_address);
	printf("Listening...\n");
	if (listen(socket_listen, 10) < 0)
	{
		fprintf(stderr, "listen() failed. (%d)\n", errno);
		return 1;
	}

	fd_set	master;
	FD_ZERO(&master);
	FD_SET(socket_listen, &master);
	int	max_socket = socket_listen;

	printf("Waiting for connections...\n");
	while (1)
	{
		fd_set	reads;
		reads = master;
		if (select(max_socket + 1, &reads, 0, 0, 0) < 0)
		{
			fprintf(stderr, "select() failed. (%d)\n", errno);
			return 1;
		}
		for (int i = 0 ; i <= max_socket ; i++)
		{
			if (FD_ISSET(i, &reads))
			{
				if (i == socket_listen)
				{
					struct sockaddr_storage	client_address;
					socklen_t	client_len = sizeof(client_address);
					int	socket_client = accept(socket_listen,
						(struct sockaddr*)&client_address,
						&client_len);
					if (socket_client < 0)
					{
						fprintf(stderr, "accept() failed. (%d)\n",
							errno);
						return 1;
					}
					FD_SET(socket_client, &master);
					if (socket_client > max_socket)
						max_socket = socket_client;
					char	address_buffer[100];
					getnameinfo((struct sockaddr*)&client_address,
						client_len, address_buffer, 100, 0, 0,
						NI_NUMERICHOST);
					printf("New connection from %s\n", address_buffer);
				}
				else
				{
					char	red[1024];
					int	bytes_received = read(i, red, 1024);
					if (bytes_received <= 0)
					{
						FD_CLR(i, &master);
						close(i);
						continue ;
					}
					for (int j = 0 ; j <= max_socket ; j++)
					{
						if (FD_ISSET(j, &master))
						{
							if (j == socket_listen || j == i)
								continue ;
							else
								write(j, red, bytes_received);
						}
					}
				}
			} // FD_ISSET
		} // for i to max_socket
	} // while (1)
	printf("Closing listening socket...\n");
	close(socket_listen);
	printf("Finished\n");
	return (0);
}
