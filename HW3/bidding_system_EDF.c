#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#define SIGUSR3 SIGWINCH

int cur_prior, bid_sys, cpid;
int ser_num[3];
struct timeval deadlines[3];
struct timespec process[3], remtim[3];
int blocked[3];
int processing[3];

void sigusr1_handler(int sig) {
	char mssgout[100];
	char mssgin[100];
	char errmes[40];
	gettimeofday(&deadlines[0], NULL);
	deadlines[0].tv_sec += 2;
	if (processing[1] == 1) {
		long long int cmp0, cmp1;
		cmp0 = deadlines[0].tv_sec;
		cmp0 *= 1000000;
		cmp0 += deadlines[0].tv_usec;
		cmp1 = deadlines[1].tv_sec;
		cmp1 *= 1000000;
		cmp1 += deadlines[1].tv_usec;
		if (cmp0 > cmp1) {
			/*Finishes after sigusr2, needs to return to sigusr2 
			then come back here*/
			blocked[0] = 1;
			return;
		}
	}
	//Finishes before sigusr2, done first
	processing[0] = 1;
	sprintf(mssgin, "receive 0 %d\n", ser_num[0]);
	if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	while (1) {
		if (clock_nanosleep(CLOCK_REALTIME, 0, &process[0], &remtim[0]) != 0) 
			process[0] = remtim[0];
		else
			break;
	}
	kill(cpid, SIGUSR1);
	sprintf(mssgout, "finish 0 %d\n", ser_num[0]);
	processing[0] = 0;
	if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	process[0].tv_nsec = 500000000;
	ser_num[0]++;
	blocked[0] = 0;
	if (blocked[2] == 1) {
		blocked[2] = 0;
		kill(getpid(), SIGUSR3);
	}
	return;
}

void sigusr2_handler(int sig) {
	char mssgout[100];
	char mssgin[100];
	char errmes[40];
	gettimeofday(&deadlines[1], NULL);
	deadlines[1].tv_sec += 3;
	processing[1] = 1;
	sprintf(mssgin, "receive 1 %d\n", ser_num[1]);
	if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	while (1) {
		if (clock_nanosleep(CLOCK_REALTIME, 0, &process[1], &remtim[1]) != 0) 
			process[1] = remtim[1];
		else
			break;
	}
	kill(cpid, SIGUSR2);
	sprintf(mssgout, "finish 1 %d\n", ser_num[1]);
	process[1].tv_sec = 1;
	if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	process[1].tv_nsec = 0;
	processing[1] = 0;
	ser_num[1]++;
	if (blocked[2] == 1) {
		blocked[2] = 0;
		kill(getpid(), SIGUSR3);
	}
	else if (blocked[0] == 1) {
		blocked[0] = 0;
		kill(getpid(), SIGUSR1);
	}
}

void sigusr3_handler(int sig) {
	char mssgout[100];
	char mssgin[100];
	char errmes[40];
	gettimeofday(&deadlines[2], NULL);
	if (deadlines[2].tv_usec >= 700000) {
		deadlines[2].tv_sec++;
		deadlines[2].tv_usec = deadlines[2].tv_usec - 700000;
	}
	else
		deadlines[2].tv_usec += 300000;
	if (processing[0] == 1 || processing[1] == 1) {
		long long int cmp0, cmp1, cmp2;
		cmp2 = deadlines[2].tv_sec;
		cmp2 *= 1000000;
		cmp2 += deadlines[2].tv_usec;
		if (processing[0] == 1) {
			cmp0 = deadlines[0].tv_sec;
			cmp0 *= 1000000;
			cmp0 += deadlines[0].tv_usec;
		}
		else
			cmp0 = cmp2 + 1;
		if (processing[1] == 1) {
			cmp1 = deadlines[1].tv_sec;
			cmp1 *= 1000000;
			cmp1 += deadlines[1].tv_usec;
		}
		else
			cmp1 = cmp2 + 1;
		if (cmp2 > cmp1 || cmp2 > cmp0) {
			blocked[2] = 1;
			return;
		}
	}
	processing[2] = 1;
	sprintf(mssgin, "receive 2 %d\n", ser_num[2]);
	if (write(bid_sys, mssgin, strlen(mssgin)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	while (1) {
		if (clock_nanosleep(CLOCK_REALTIME, 0, &process[2], &remtim[2]) != 0) 
			process[2] = remtim[2];
		else
			break;
	}
	kill(cpid, SIGUSR3);
	sprintf(mssgout, "finish 2 %d\n", ser_num[2]);
	processing[2] = 0;
	if (write(bid_sys, mssgout, strlen(mssgout)) < 0) {
		sprintf(errmes, "Error writing to bidding_system_log\n");
		write(2, errmes, strlen(errmes));
		exit(-1);
	}
	process[2].tv_nsec = 200000000;
	ser_num[2]++;
	blocked[2] = 0;
}

int main(int argc, char const *argv[]) {
	int fdstdin[2], fdstdout[2];
	char test_[50];
	strcpy(test_, argv[1]);
	process[0].tv_sec = 0;
	process[0].tv_nsec = 500000000;
	process[1].tv_sec = 1;
	process[1].tv_nsec = 0;
	process[2].tv_sec = 0;
	process[2].tv_nsec = 200000000;
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
		execv("./customer_EDF", (char *[]){ "./customer_EDF", argum, NULL });
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
	struct sigaction sigh3;
	sigh1.sa_handler = &sigusr1_handler;
	sigh1.sa_flags = SA_RESTART;
	sigemptyset(&sigh1.sa_mask);
	sigaddset(&sigh1.sa_mask, SIGUSR2);
	sigh2.sa_handler = &sigusr2_handler;
	sigh2.sa_flags = SA_RESTART;
	sigemptyset(&sigh2.sa_mask);
	sigh3.sa_handler = &sigusr3_handler;
	sigh3.sa_flags = SA_RESTART;
	sigemptyset(&sigh3.sa_mask);
	sigaddset(&sigh3.sa_mask, SIGUSR1);
	sigaddset(&sigh3.sa_mask, SIGUSR2);

	if (sigaction(SIGUSR1, &sigh1, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR1\n");
		exit(-1);
	}
	if (sigaction(SIGUSR2, &sigh2, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR2\n");
		exit(-1);
	}
	if (sigaction(SIGUSR3, &sigh3, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR3\n");
		exit(-1);
	}

	while (1) {
		fd_set TEMP;
		FD_ZERO(&TEMP);
		TEMP = MASTER;
		struct timeval tout;
		tout.tv_sec = 0;
		tout.tv_usec = 10;
		int rt = select(maxfd + 1, &TEMP, NULL, NULL, &tout);
		if (FD_ISSET(fdstdout[0], &TEMP)) {
			char buf[11];
			memset(buf, '\0', sizeof(buf));
			if (read(fdstdout[0], buf, 10) < 0) {
				perror("");
				exit(-1);
			}
			if (strcmp(buf, "terminate\n") == 0)
				break;
		}
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