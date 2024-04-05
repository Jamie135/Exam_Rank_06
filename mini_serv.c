#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>

typedef	struct client
{
	int	fd;
	int	id;
} client;

void	err_exit(char *str)
{
	write(2, str, strlen(str));
	exit(1);
}

int	main(int argc, char **argv)
{
	const int	MAX = 128;
	const int	BUF = 200000;
	client	user[MAX];
	char	buffer[BUF];
	char	message[BUF];
	int	newid = 0;
	int	server;
	fd_set	active, ready;

	if (argc != 2)
		err_exit("Wrong number of arguments\n");

	if ((server = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err_exit("Fatal error\n");

	struct sockaddr_in	addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons(atoi(argv[1]));
	
	if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		err_exit("Fatal error\n");
	if (listen(server, MAX) < 0)
		err_exit("Fatal error\n");
	bzero(user, sizeof(client) * MAX);
	FD_ZERO(&active);
	FD_SET(server, &active);
	int	max_user = server;
	while (42)
	{
		ready = active;
		if (select(max_user + 1, &ready, NULL, NULL, NULL) < 0)
			err_exit("Fatal error\n");
		for (int socket = 0; socket <= max_user; socket++)
		{
			if (!FD_ISSET(socket, &ready))
				continue;
			bzero(buffer, BUF);
			if (socket == server)
			{
				int	newfd;
				if ((newfd = accept(server, NULL, NULL)) < 0)
					err_exit("Fatal error\n");
				FD_SET(newfd, &active);
				max_user = (newfd > max_user) ? newfd : max_user;
				user[newfd].fd = newfd;
				user[newfd].id = newid++;
				sprintf(buffer, "server: client %d just arrived\n", user[newfd].id);
				for (int i = 0; i < MAX; i++)
				{
					if (user[i].fd != 0 && user[i].fd != newfd)
						send(user[i].fd, buffer, strlen(buffer), 0);
				}
			}
			else
			{
				int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);

				if (bytes <= 0)
				{
					bzero(message, BUF);
					sprintf(message, "server: client %d just left\n", user[socket].id);
					for (int i = 0; i < MAX; i++)
					{
						if (user[i].fd != 0 && user[i].fd != socket)
							send(user[i].fd, message, strlen(message), 0);
					}
					close(socket);
					FD_CLR(socket, &active);
				}
				else
				{
					bzero(message, BUF);
					sprintf(message, "client %d: %s\n", user[socket].id, buffer);
					for (int i = 0; i < MAX; i++)
					{
						if (user[i].fd != socket)
							send(user[i].fd, message, strlen(message), 0);
					}
				}
			}
		}
	}
	return (0);
}
