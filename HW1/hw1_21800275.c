#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFFER_SIZE 1024
#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[])
{
	// char write_msg[BUFFER_SIZE] = "HIHIHI";
	char read_msg[BUFFER_SIZE];
	pid_t pid;
	int ret;
	int fd[2];
	char **new_argv;
	char *ptr = argv[1];
	int count = 0;
	int max_strlen = 0;
	int num = 0;
	char color = '1';
	int total_flag = 0;
	int total_n = 0;
	int total_l = 0;
	if (argc < 3)
	{
		fprintf(stderr, "usage: cpu <string>\n");
		exit(1);
	}
	else if (argc >= 4)
	{
		switch (toupper(argv[3][0]))
		{
		case 'K':
			color = '0';
			break;
		case 'G':
			color = '2';
			break;
		case 'Y':
			color = '3';
			break;
		case 'B':
			color = '4';
			break;
		case 'P':
			color = '5';
			break;
		case 'C':
			color = '6';
			break;
		default:
			color = '1';
			break;
		}
	}

	if (argc == 5)
	{
		switch (toupper(argv[4][0]))
		{
		case 'N':
			total_flag = 1;
			break;
		case 'L':
			total_flag = 2;
			break;
		default:
			total_flag = 0;
			break;
		}
	}
	while (*ptr != 0)
	{
		if (*ptr == ' ')
		{
			count++;
			if (num > max_strlen)
				max_strlen = num;
			num = 0;
		}
		else
		{
			num++;
		}
		ptr++;
	}
	if (num > max_strlen)
		max_strlen = num;

	new_argv = (char **)malloc(sizeof(char *) * (count + 2));
	for (int i = 0; i <= count; i++)
		*(new_argv + i) = (char *)malloc(sizeof(char) * (max_strlen + 1));

	ptr = argv[1];
	int i = 0;
	int j = 0;
	while (*ptr != 0)
	{
		if (*ptr != ' ')
		{
			new_argv[i][j] = *ptr;
			j++;
		}
		else
		{
			new_argv[i][j] = 0;
			i++;
			j = 0;
		}
		ptr++;
	}
	new_argv[i][j] = 0;
	new_argv[i + 1] = 0;

	/* create the pipe */
	if (pipe(fd) == -1)
	{
		fprintf(stderr, "Pipe failed");
		return 1;
	}

	/* now fork a child process */
	if ((pid = fork()) == 0)
	{ /* child process */
		close(fd[READ_END]);
		ret = dup2(fd[WRITE_END], STDOUT_FILENO);
		/* close the unused end of the pipe */
		close(fd[WRITE_END]);

		/* close the write end of the pipe */
		ret = execvp(new_argv[0], new_argv);
		if (ret < 0)
		{
			perror("Error: execvp failed");
			exit(EXIT_FAILURE);
		}
	}
	else if (pid > 0)
	{ /* parent process */

		wait(NULL);
		/* close the unused end of the pipe */

		close(fd[WRITE_END]);

		FILE *fd_read = fdopen(fd[READ_END], "r");

		int i = 1;

		int argv2_len = strlen(argv[2]);

		while (fgets(read_msg, BUFFER_SIZE, fd_read) != 0)
		{
			int flag = 0;
			int grep_point[128] = {0};
			int l = 0;
			int msg_len = strlen(read_msg);
			if (msg_len >= argv2_len)
				for (int j = 0; j <= (msg_len - argv2_len); j++)
				{
					for (int k = 0; k < argv2_len; k++)
					{
						if (read_msg[j + k] != argv[2][k])
						{
							break;
						}
						else if (k == argv2_len - 1)
						{
							flag = 1;
							if (l < 128 - 1)
							{
								grep_point[l++] = j;
								grep_point[l] = -1;
							}

							j += argv2_len - 1;
						}
					}
				}
			if (flag)
			{
				l = 0;
				printf("\033[0;32m[%d] \033[0m", i);
				for (int k = 0; k < msg_len; k++)
				{
					if (grep_point[l] != -1 && grep_point[l] == k)
					{
						printf("\033[0;3%cm", color);
						total_n++;
					}
					printf("%c", read_msg[k]);
					if (grep_point[l] != -1 && grep_point[l] + argv2_len - 1 == k)
					{
						printf("\033[0m");
						if (l < 128 - 1)
							l++;
					}
				}
				total_l++;
			}
			i++;
		}
		printf("\033[0m");

		/* close the READ end of the pipe */
		for (int i = 0; i <= count; i++)
		{
			free(*(new_argv + i));
		}
		free(new_argv);
		close(fd[READ_END]);
		fclose(fd_read);
		if (total_flag == 1)
			printf("\n Found word: %d\n", total_n);
		else if ((total_flag == 2))
			printf("\n Found line: %d\n", total_l);
	}
	else
	{

		fprintf(stderr, "Fork failed");
		return 1;
	}

	return 0;
}