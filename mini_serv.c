#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

typedef	struct	s_client
{
	int	id;
	char	msg[100000];
}	t_client;

void	err(char *str)
{
	if (str)
		write(2, str, strlen(str));
	else
		write(2, "Fatal error", 11);
	write(2, "\n", 1);
	exit(1);
}

t_client	client[1024];
char	buffread[120000];
char	buffwrite[120000];
int	recentfd = 0;
int	newid = 0;
fd_set	activefd, readyfd, writefd;

void	sendall(int	fd)
{
	for (int i = 0; i <= recentfd; i++)
	{
		if (FD_ISSET(i, &writefd) && i != fd)
			if (send(i, buffwrite, strlen(buffwrite), 0) == -1)
				err(NULL);
	}
}

int	main(int argc, char **argv)
{
	int	serverfd;

	if (argc != 2)
		err("Wrong number of arguments");
	if ((serverfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		err(NULL);
	
	socklen_t	len;
	struct	sockaddr_in	addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(atoi(argv[1]));

	if (bind(serverfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		err(NULL);
	if (listen(serverfd, 100) < 0)
		err(NULL);
	
	bzero(client, sizeof(client));
	FD_ZERO(&activefd);
	FD_SET(serverfd, &activefd);
	recentfd = serverfd;

	while (42)
	{
		readyfd = writefd = activefd;
		if (select(recentfd + 1, &readyfd, &writefd, NULL, NULL) < 0)
			continue;
		for (int socket = 0; socket <= recentfd; socket++)
		{
			if (!FD_ISSET(socket, &readyfd))
				continue;
			if (socket == serverfd)
			{
				int	newfd;

				if ((newfd = accept(serverfd, (struct sockaddr *)&addr, &len)) < 0)
					continue;
				FD_SET(newfd, &activefd);
				recentfd = (newfd > recentfd) ? newfd : recentfd;
				client[recentfd].id = newid++;
				sprintf(buffwrite, "server: client %d just arrived\n", client[newfd].id);
				sendall(newfd);
				break;
			}
			else
			{
				int	bytes = recv(socket, buffread, sizeof(buffread) - 1, 0);

				if (bytes <= 0)
				{
					sprintf(buffwrite, "server: client %d just left\n", client[socket].id);
					sendall(socket);
					FD_CLR(socket, &activefd);
					close(socket);
					break;
				}
				else
				{
					for (int i = 0, j = strlen(client[socket].msg); i < bytes; i++, j++)
					{
						client[socket].msg[j] = buffread[i];
						if (client[socket].msg[j] == '\n')
						{
							client[socket].msg[j] = '\0';
							sprintf(buffwrite, "client %d: %s\n", client[socket].id, client[socket].msg);
							sendall(socket);
							bzero(client[socket].msg, strlen(client[socket].msg));
							j = -1;
						}
					}
					break;
				}
			}
		}
	}
	return (0);
}
