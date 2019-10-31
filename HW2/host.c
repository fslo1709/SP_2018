#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

typedef struct {
	int money;
	int key;
	int id;
}decide;

typedef struct {
	int rank;
	int id;
	int score;
}playinfo;

int x, maxfd;

void readline(int fd, char * buffer) {
    char c;
    int counter = 0;
    while (read(fd, &c, 1) != 0) {
        if (c == '\n') {
            break;
        }
        buffer[counter++] = c;
    }
    buffer[counter] = '\0';
    return;
}

int Fifopen(char *name, int per) {
	int fd;
	if (per == 1) {
		fd = open(name, O_RDONLY);
		if (fd < 0)
			fprintf(stderr, "Error opening %s\n", name);
	}
	else {
		fd = open(name, O_WRONLY | O_APPEND);
		if (fd < 0)
			fprintf(stderr, "Error opening %s\n", name);	
	}
	return fd;
}

int comp(const void *a, const void *b) {
	decide *A = (decide *)a;
	decide *B = (decide *)b;
	return B->money - A->money;
}

int cid(const void *a, const void *b) {
	playinfo *A = (playinfo *)a;
	playinfo *B = (playinfo *)b;
	return A->id - B->id;
}

int cscore(const void *a, const void *b) {
	playinfo *A = (playinfo *)a;
	playinfo *B = (playinfo *)b;
	return B->score - A->score;
}

int main(int argc, char const *argv[])
{
	int d = umask(0000);
	char buf[200];
	char cmp[20];
	char fifoM[50], fifoR[4][50];
	memset(fifoM, '\0', sizeof(fifoM));
	for (int i = 0; i < 4; i++)
		memset(fifoR[i], '\0', sizeof(fifoR[i]));
	int ID = atoi(argv[1]);

	sprintf(fifoM, "host%d.FIFO", ID);
	sprintf(fifoR[0], "host%d_A.FIFO", ID);
	sprintf(fifoR[1], "host%d_B.FIFO", ID);
	sprintf(fifoR[2], "host%d_C.FIFO", ID);
	sprintf(fifoR[3], "host%d_D.FIFO", ID);
	strcpy(cmp, "-1 -1 -1 -1");
	mkfifo(fifoM, 0777);
	for (int i = 0; i < 4; i++)
		mkfifo(fifoR[i], 0777);

	while (1) {
		playinfo players[4];
		readline(STDIN_FILENO, buf);
		if (strcmp(buf, cmp) == 0) {
			break;
		}

		sscanf(buf, "%d %d %d %d", &players[0].id, &players[1].id, &players[2].id, &players[3].id);
		players[0].score = players[1].score = players[2].score = players[3].score = 0;
		players[0].rank = players[1].rank = players[2].rank = players[3].rank = 0;
		
		pid_t pid[4];
		int fdM, fdn[4];
		int pmoney[4];
		int playk[4];
		int rounds = 0;

		playk[0] = rand() % 65536;
		playk[1] = rand() % 65536;
		while (playk[1] == playk[0])
			playk[1] = rand() % 65536;
		playk[2] = rand() % 65536;
		while (playk[2] == playk[1] || playk[2] == playk[0])
			playk[2] = rand() % 65536;
		playk[3] = rand() % 65536;
		while (playk[3] == playk[2] || playk[3] == playk[1] || playk[3] == playk[0])
			playk[3] = rand() % 65536;
		
		char hID[3], pID[4][2], rKey[4][10];
		sprintf(hID, "%d", ID);
		for (int i = 0; i < 4; i++) {
			sprintf(pID[i], "%c", 'A'+i);
			sprintf(rKey[i], "%d", playk[i]);
		}
		
		for (int i = 0; i < 4; i++) {
			pmoney[i] = 0;
			pid[i] = fork();
			if (pid[i] < 0) {
				perror("Could not fork: ");
				exit(EXIT_FAILURE);
			}
			else if (pid[i] == 0) {
				execv("player", (char *[]){ "./player", hID, pID[i], rKey[i], NULL });
			}
			else {
				fdn[i] = Fifopen(fifoR[i], 2);
				if (fdn[i] < 0) {
					fprintf(stderr, "Error opening fifo for writing, %d\n", i);
					exit(-1);
				}
			}
		}
		fdM = Fifopen(fifoM, 1);
		if (fdM < 0) {
			fprintf(stderr, "Error opening fifo for reading\n");
			exit(-1);
		}

		while (rounds < 10) {
			for (int i = 0; i < 4; i++)
				pmoney[i] += 1000;
			char moneymes[200];
			sprintf(moneymes, "%d %d %d %d\n", pmoney[0], pmoney[1], pmoney[2], pmoney[3]);
			int len = strlen(moneymes);
			for (int i = 0; i < 4; i++) {
				write(fdn[i], moneymes, len);
				fsync(fdn[i]);
			}

			decide mon[4];
			char t;
			for (int i = 0; i < 4; i++) {
				char message[100];
				readline(fdM, message);
				sscanf(message, "%c %d %d", &t, &mon[i].key, &mon[i].money);
				mon[i].id = t-'A'+1;
			}
			qsort(mon, 4, sizeof(decide), comp);
			int winner = 0;
			if (mon[0].money == mon[1].money) {
				winner = 1;
				while (mon[winner].money == mon[winner-1].money && winner < 3) {
					winner++;
				}
			}
			pmoney[mon[winner].id-1] -= mon[winner].money;
			players[mon[winner].id-1].score++;
			rounds++;
		}
		for (int i = 0; i < 4; i++) {
			int x = waitpid(pid[i], NULL, 0);
			if (x < 0) {
				perror("Error waiting for a child: ");
				exit(EXIT_FAILURE);
			}
		}
		close(fdM);
		for (int i = 0; i < 4; i++)
			close(fdn[i]);
		qsort(players, 4, sizeof(playinfo), cscore);
		players[0].rank = 1;
		for (int i = 1; i < 4; i++) {
			if (players[i].score == players[i-1].score)
				players[i].rank = players[i-1].rank;
			else
				players[i].rank = i;
		}
		qsort(players, 4, sizeof(playinfo), cid);			
		char temp[200];
		sprintf(temp, "%d %d\n%d %d\n%d %d\n%d %d\n\0", players[0].id, players[0].rank,
													players[1].id, players[1].rank,
													players[2].id, players[2].rank,
													players[3].id, players[3].rank);
		write(STDOUT_FILENO, temp, strlen(temp));
		fsync(STDOUT_FILENO);
	}
	x = remove(fifoM);
	if (x < 0) {
		fprintf(stderr, "Error unlinking\n");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < 4; i++) {
		x = remove(fifoR[i]);
		if (x < 0) {
			fprintf(stderr, "Error unlinking\n");
			exit(EXIT_FAILURE);
		}
	}
	exit(0);
}