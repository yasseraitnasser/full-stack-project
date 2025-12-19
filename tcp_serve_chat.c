#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdbool.h>


typedef struct	s_list
{
	int	fd;
	int	id;
	bool	flag;
	char	*msg;
	struct s_list	*next;
}	t_list;

fd_set	master, rfds, wfds;
t_list	*head = NULL;
int	max = 0;
char	buffer[1001];
int	last_id = 0;

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	ft_erro_exit(char *msg)
{
	write(2, msg, strlen(msg));
	exit(1);
}

int	list_size(t_list *head)
{
	int	size = 0;

	while (head)
	{
		size++;
		head = head->next;
	}
	return (size);
}

void	broadcast(int c, char *str)
{
	for (int i = 0 ; i <= max ; i++)
	{
		if (FD_ISSET(i, &wfds) && i != c)
			write(i, str, strlen(str));
	}
}

void	send_msg(int fd, t_list *client)
{
	char	*msg;

	while (extract_message(&(client->msg), &msg))
	{
		sprintf(buffer, "client %d: ", client->id);
		broadcast(fd, buffer);
		broadcast(fd, msg);
		free(msg);
	}
}

void	push_back(t_list *new, t_list **head)
{
	if (*head == NULL)
	{
		*head = new;
		return ;
	}
	t_list	*tmp = *head;
	while (tmp->next)
	{
		tmp = tmp->next;
	}
	tmp->next = new;
}

void	erase(int fd, t_list **head)
{
	t_list	*tmp = *head;

	if (!tmp)
		return ;
	if (tmp->fd == fd)
	{
		*head = (*head)->next;
		free(tmp);
		return ;
	}
	while (tmp->next)
	{
		if (tmp->next->fd == fd)
		{
			t_list	*tmp2 = tmp->next;
			tmp->next = tmp2->next;
			free(tmp2);
			return ;
		}
		tmp = tmp->next;
	}
}

t_list	*find_client(int fd, t_list *head)
{
	while (head)
	{
		if (head->fd == fd)
			return head;
		head = head->next;
	}
	return (NULL);
}

void	register_client(int client_socket)
{
	FD_SET(client_socket, &master);
	t_list	*new = malloc(sizeof(t_list));
	if (!new)
		ft_erro_exit("Fatal error\n");
	new->fd = client_socket;
	new->id = last_id;
	new->next = NULL;
	new->flag = true;
	new->msg = NULL;
	push_back(new, &head);
	if (client_socket > max)
		max = client_socket;
	sprintf(buffer, "server: client %d just arrived\n", last_id++);
	broadcast(client_socket, buffer);
}

void	remove_client(int s, int client_id)
{
	close(s);
	FD_CLR(s, &master);
	sprintf(buffer, "server: client %d just left\n", client_id);
	broadcast(s, buffer);
	erase(s, &head);
}

int	main(int argc, char **argv)
{
	if (argc != 2)
		ft_erro_exit("Wrong number of arguments\n");
	int	server_socket;
	struct sockaddr_in	addr;
	socklen_t		addrlen = sizeof addr;

	// socket create and verification
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
		ft_erro_exit("Fatal error\n");
	bzero(&addr, sizeof(addr));

	// assign IP, PORT
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	addr.sin_port = htons(atoi(argv[1]));

	// Binding newly created socket to given IP and verification
	if ((bind(server_socket, (const struct sockaddr *)&addr, addrlen)) != 0)
		ft_erro_exit("Fatal error\n");
	if (listen(server_socket, 100) != 0)
		ft_erro_exit("Fatal error\n");

	FD_ZERO(&master);
	FD_SET(server_socket, &master);
	max = server_socket;
	while (1)
	{
		rfds = wfds = master;
		if (select(max + 1, &rfds, &wfds, 0, 0) < 0)
			ft_erro_exit("Fatal error\n");
		for (int s = 0 ; s <= max ; s++)
		{
			if (!FD_ISSET(s, &rfds))
				continue ;
			if (s == server_socket)
			{
				int	client_socket = accept(s, 0, 0);
				if (client_socket >= 0)
				{
					register_client(client_socket);
					break ;
				}
			}
			else
			{
				t_list	*client = find_client(s, head);
				int	bytes = recv(s, buffer, 1000, 0);
				if (bytes <= 0)
				{
					remove_client(s, client->id);
					break ;
				}
				buffer[bytes] = 0;
				client->msg = str_join(client->msg, buffer);
				send_msg(s, client);
			}
		}
	}
	close(server_socket);
	return (0);
}
