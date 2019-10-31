#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>

typedef struct {
	int score;
	int id;
	int rank;
}ssc;

ssc scores[20];

int cmp1(const void *a, const void *b) {
	ssc *A = (ssc *)a;
	ssc *B = (ssc *)b;
	return (B->score - A->score);
}

int cmp2(const void *a, const void *b) {
	ssc *A = (ssc *)a;
	ssc *B = (ssc *)b;
	return (A->id - B->id);
}

void ArrangeScores(int size) {
	qsort(scores, size, sizeof(ssc), cmp1);
	scores[0].rank = 1;
	for (int i = 1; i < size; i++) {
		if (scores[i].score == scores[i-1].score)
			scores[i].rank = scores[i-1].rank;
		else
			scores[i].rank = i+1;
	}
	qsort(scores, size, sizeof(ssc), cmp2);
	return;
}

int main(int argc, char const *argv[])
{
	int BtoH[12][2], HtoB[12][2];
	int nhosts = atoi(argv[1]), nplayers = atoi(argv[2]);

	int cnt = 0;
	int sequence[4846][4]; 
	for (int i = 0; i < nplayers-3; i++) {
		for (int j = i+1; j < nplayers-2; j++) {
			for (int k = j+1; k < nplayers-1; k++) {
				for (int l = k+1; l < nplayers; l++) {
					sequence[cnt][0] = i+1;
					sequence[cnt][1] = j+1;
					sequence[cnt][2] = k+1;
					sequence[cnt][3] = l+1;
					cnt++;
				}
			}
		}
	}
	for (int i = 0; i < nplayers; i++)
		scores[i].id = i+1;

	int count = 0;
	pid_t pid;
	while (count < nhosts) {
		if (pipe(BtoH[count]) == -1) {
			fprintf(stderr, "Could not pipe 1\n");
			exit(-1);
		}
		if (pipe(HtoB[count]) == -1) {
			fprintf(stderr, "Could not pipe 2\n");
			exit(-1);
		}
		pid = fork();
		if (pid < 0) {
			fprintf(stderr, "Could not fork child\n");
			exit(0);
		}
		else if (pid == 0) {
			pid = fork();
			if (pid < 0) {
				fprintf(stderr, "Could not fork grandchild\n");
				exit(0);		
			}
			if (pid > 0) {
				exit(0);
			}
			char sendval[5];
			memset(sendval, '\0', sizeof(sendval));
			if (dup2(BtoH[count][0], STDIN_FILENO) == -1) {
				fprintf(stderr, "Error duplicating stdin\n");
				exit(-1);
			}
			if (dup2(HtoB[count][1], STDOUT_FILENO) == -1) {
				fprintf(stderr, "Error duplicating stdout\n");
				exit(-1);
			}
			close(BtoH[count][0]);
			close(BtoH[count][1]);
			close(HtoB[count][0]);
			close(HtoB[count][1]);
			sprintf(sendval, "%d", count+1);
			execv("host", (char *[]){ "./host", sendval, NULL });
		}
		else {
			close(BtoH[count][0]);
			close(HtoB[count][1]);
		}
		count++;
	}
	

	int jobs = 0, jobsdone = 0, maxfd = 0, minfd = 10000;

	fd_set MASTER;
    FD_ZERO(&MASTER);
    
    for (int i = 0; i < nhosts; i++) {
    	FD_SET(HtoB[i][0], &MASTER);
    	if (HtoB[i][0] > maxfd)
    		maxfd = HtoB[i][0];
    	if (HtoB[i][0] < minfd)
    		minfd = HtoB[i][0];
    }
	
	for (int i = 0; i < nhosts; i++) {
		if (jobs == cnt)
			break;
		char sendm[70];
		memset(sendm, '\0', sizeof(sendm));
		sprintf(sendm, "%d %d %d %d\n", 	sequence[i][0]
										,	sequence[i][1]
										,	sequence[i][2]
										,	sequence[i][3]);
		write(BtoH[i][1], sendm, strlen(sendm));
		fsync(BtoH[i][1]);
		jobs++;
	}

	while (jobsdone < cnt) {
		fd_set TEMPSET;
    	FD_ZERO(&TEMPSET);
    	TEMPSET = MASTER;
    	struct timeval tout;
		tout.tv_sec = 0;
		tout.tv_usec = 10;		
    	int rt = select(maxfd + 1, &TEMPSET, NULL, NULL, &tout);
    	for (int CFD = minfd; CFD <= maxfd && rt > 0; CFD++) {
    		if (FD_ISSET(CFD, &TEMPSET)) {
    			char readbuf[70];
    			memset(readbuf, '\0', sizeof(readbuf));
    			int ids[4], ranks[4], sendnum = 0;
    			read(CFD, readbuf, sizeof(readbuf));
    			sscanf(readbuf, "%d %d %d %d %d %d %d %d", 	&ids[0], &ranks[0]
    													,	&ids[1], &ranks[1]
    													,	&ids[2], &ranks[2]
    													,	&ids[3], &ranks[3]);
    			for (int i = 0; i < 4; i++) {
    				if (ranks[i] == 1)
    					scores[ids[i]-1].score += 3;
    				if (ranks[i] == 2)
    					scores[ids[i]-1].score += 2;
    				if (ranks[i] == 3)
    					scores[ids[i]-1].score += 1;
					if (ranks[i] == 4)
						scores[ids[i]-1].score += 0;
    			}
    			for (int i = 0; i < nhosts; i++) {
    				if (CFD == HtoB[i][0]) {
    					sendnum = i;
    					break;
    				}
    			}
    			if (jobs < cnt) {
    				char sendm[70];
    				sprintf(sendm, "%d %d %d %d\n", 	sequence[jobs][0]
													,	sequence[jobs][1]
													,	sequence[jobs][2]
													,	sequence[jobs][3]);
    				write(BtoH[sendnum][1], sendm, strlen(sendm));
    				fsync(BtoH[sendnum][1]);
    				jobs++;
    			}
    			else {
    				char sendm[20];
    				memset(sendm, '\0', sizeof(sendm));
    				strcpy(sendm, "-1 -1 -1 -1\n");
    				write(BtoH[sendnum][1], sendm, strlen(sendm)+1);
    				fsync(BtoH[sendnum][1]);
    				FD_CLR(CFD, &MASTER);
    			}
    			rt--;
    			jobsdone++;
    		}
    	}
	}
	char temp[20];
	ArrangeScores(nplayers);
	for (int i = 0; i < nplayers; i++) {
		sprintf(temp, "%d %d\n", scores[i].id, scores[i].rank);
		write(STDOUT_FILENO, temp, strlen(temp));
		fsync(STDOUT_FILENO);
	}
	
	exit(0);
}