#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

char *readline(int fd, char * buffer) {
    char c;
    int counter = 0;
    while (read(fd, &c, 1) != 0) {
        if (c == '\n') {
            break;
        }
        buffer[counter++] = c;
    }
    return buffer;
}

int main(int argc, char const *argv[])
{
	int hostID = atoi(argv[1]), rkey = atoi(argv[3]);
	char playerID = argv[2][0];
	int play = playerID - 'A';


	char fifoname1[50];
	sprintf(fifoname1, "host%d_%c.FIFO", hostID, playerID);
	int fd1 = open(fifoname1, O_RDONLY);

	char fifoname2[50];
	sprintf(fifoname2, "host%d.FIFO", hostID);
	int fd2 = open(fifoname2, O_WRONLY | O_APPEND);

	int responses = 0;

	while (responses < 10) {
		int money = 0;
		int am[4];
		char buf[15];
		memset(buf, '\0', sizeof(buf));
		sscanf(readline(fd1, buf), "%d %d %d %d", &am[0], &am[1], &am[2], &am[3]);

		if (responses%4 == play)
			money = am[play];
		else
			money = 0;

		char ans[40];
		memset(ans, '\0', sizeof(ans));
		sprintf(ans, "%c %d %d\n", playerID, rkey, money);
		write(fd2, ans, strlen(ans));
		fsync(fd2);
		responses++;
	}
	close(fd1);
	close(fd2);
	exit(0);
}