#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

int cur_prior, bid_sys, cpid;
int ser_num[3];

void sigusr1_handler(int sig) {
	char mssgout[100];
	char mssgin[100];
	char errmes[40];
	sprintf(mssgin, "receive 1 %d\n", ser_num[1]);
	if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
	}
	struct timespec tim, remtim;
	tim.tv_sec = 0;
	tim.tv_nsec = 500000000;
	while (1) {
		if (clock_nanosleep(CLOCK_REALTIME, 0, &tim, &remtim) != 0) 
			tim = remtim;
		else
			break;
	}
	kill(cpid, SIGUSR1);
	sprintf(mssgout, "finish 1 %d\n", ser_num[1]);
	if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
	}
	ser_num[1]++;
	return;
}
void sigusr2_handler(int sig) {
	char mssgout[100];
	char mssgin[100];
	char errmes[40];
	sprintf(mssgin, "receive 2 %d\n", ser_num[2]);
	if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
	}
	struct timespec tim, remtim;
	tim.tv_sec = 0;
	tim.tv_nsec = 200000000;
	while (1) {
		if (clock_nanosleep(CLOCK_REALTIME, 0, &tim, &remtim) != 0) 
			tim = remtim;
		else
			break;
	}
	kill(cpid, SIGUSR2);
	sprintf(mssgout, "finish 2 %d\n", ser_num[2]);
	if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
	}
	ser_num[2]++;
}

int main(int argc, char const *argv[]) {
	int fdstdin[2], fdstdout[2];
	char test_[50];
	strcpy(test_, argv[1]);
	for (int i = 0; i < 3; i++)
		ser_num[i] = 1;

	if (pipe(fdstdin) < 0) {
		fprintf(stderr, "Error making pipe\n");
		exit(-1);
	}
	if (pipe(fdstdout) < 0) {
		fprintf(stderr, "Error making pipe\n");
		exit(-1);
	}
	cpid = fork();
	if (cpid == -1) {
		fprintf(stderr, "Error forking\n");
		exit(-1);
	}
	else if (cpid == 0) {
		if (dup2(fdstdin[0], STDIN_FILENO) == -1) {
			fprintf(stderr, "Error duplicating stdin\n");
			exit(-1);
		}
		if (dup2(fdstdout[1], STDOUT_FILENO) == -1) {
			fprintf(stderr, "Error duplicating stdout\n");
			exit(-1);
		}
		close(fdstdin[0]);
		close(fdstdin[1]);
		close(fdstdout[0]);
		close(fdstdout[1]);
		char argum[30];
		memset(argum, '\0', sizeof(argum));
		strcpy(argum, argv[1]);
		execv("./customer", (char *[]){ "./customer", argum, NULL });
		perror("");
		exit(-1);
	}
	else {
		close(fdstdin[0]);
		close(fdstdout[1]);
	}
	int maxfd = fdstdout[0] + 1, flag = 1;
	char receive[9];
	char finish[8];

	fd_set MASTER;
    FD_ZERO(&MASTER);
    FD_SET(fdstdout[0], &MASTER);

	strcpy(receive, "receive");
	strcpy(finish, "finish");
	bid_sys = open("bidding_system_log", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0777);
	if (bid_sys < 0) {
		perror("");
		exit(-1);
	}

	struct sigaction sigh1;
	struct sigaction sigh2;
	sigh1.sa_handler = &sigusr1_handler;
	sigh1.sa_flags = SA_RESTART;
	sigemptyset(&sigh1.sa_mask);
	sigh2.sa_handler = &sigusr2_handler;
	sigh2.sa_flags = SA_RESTART;
	sigemptyset(&sigh2.sa_mask);
	sigaddset(&sigh2.sa_mask, SIGUSR1);

	if (sigaction(SIGUSR1, &sigh1, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR1\n");
	}
	if (sigaction(SIGUSR2, &sigh2, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR2\n");
	}

	while (flag > 0) {
		fd_set TEMP;
		FD_ZERO(&TEMP);
		TEMP = MASTER;
		struct timeval tout;
		tout.tv_sec = 0;
		tout.tv_usec = 10;
		int rt = select(maxfd + 1, &TEMP, NULL, NULL, &tout);
		if (FD_ISSET(fdstdout[0], &TEMP)) {
			char mssgin[100];
			char mssgout[100];
			char errmes[40];
			memset(mssgin, '\0', sizeof(mssgin));
			memset(mssgout, '\0', sizeof(mssgout));
			char buf[10];
			int x = read(fdstdout[0], buf, 9);
			if (x == 0) {
				flag = 0;
				break;
			}
			if (x < 0) {
				perror("");
				exit(-1);
			}
			sprintf(mssgin, "receive 0 %d\n", ser_num[0]);
			if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
				sprintf(errmes, "Error writing to bidding_system_log\n");
				write(2, errmes, strlen(errmes));
			}
			struct timespec tim, remtim;
			tim.tv_sec = 1;
			tim.tv_nsec = 0;
			while (1) {
				if (clock_nanosleep(CLOCK_REALTIME, 0, &tim, &remtim) != 0) 
					tim = remtim;
				else
					break;
			}
			kill(cpid, SIGINT);
			sprintf(mssgout, "finish 0 %d\n", ser_num[0]);
			if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
				sprintf(errmes, "Error writing to bidding_system_log\n");
				write(2, errmes, strlen(errmes));
			}
			ser_num[0]++;
		}
	}
	if (flag < 0) {
		fprintf(stderr, "Error\n");
		exit(-1);
	}
	wait(NULL);
	char finalmes[100];
	sprintf(finalmes, "terminate\n");
	if (write(bid_sys, finalmes, strlen(finalmes)) < 0) {
		char errmes[40];
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
	}
	close(bid_sys);
	exit(0);
}