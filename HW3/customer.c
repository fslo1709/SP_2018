#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>

int cust_log, unfinished;
int ranks[3];

void signal_handler(int sig) {
	char mess[100];
	int c_code;
	char errmess1[20], errmess2[36];
	memset(mess, '\0', sizeof(mess));
	switch (sig) {	
		case SIGUSR1:
			c_code = 1;
			struct itimerval inter;
			inter.it_interval.tv_sec = 0;
			inter.it_interval.tv_usec = 0;
			inter.it_value.tv_sec = 0;
			inter.it_value.tv_usec = 0;
			if (setitimer(ITIMER_REAL, &inter, NULL) < 0) {
				perror("");
				exit(-1);
			}
			break;
		case SIGUSR2:
			c_code = 2;
			break;
		case SIGINT:
			c_code = 0;
			break;
		default:
			sprintf(errmess1, "Wrong signal sent\n");
			write(2, errmess1, sizeof(errmess1));
			exit(-1);
			return;
	}
	sprintf(mess, "finish %d %d\n", c_code, ranks[c_code]);
	if (write(cust_log, mess, strlen(mess)) < 0) {
		sprintf(errmess2, "Error writing data to customer_log\n");
		write(2, errmess2, strlen(errmess2));
	}
	ranks[c_code]++;
	unfinished--;
	return;
}

void sig_alarm_handler(int sig) {
	char mess[100];
	sprintf(mess, "timeout 1 %d\n", ranks[1]);
	if (write(cust_log, mess, strlen(mess)) < 0) {
		char errmess[38];
		sprintf(errmess, "Error writing data to costumer log\n");
		write(cust_log, errmess, strlen(errmess));
		exit(-1);
	}
	exit(0);
}

int main(int argc, char const *argv[]) {
	unfinished = 0;
	cust_log = open("customer_log", O_WRONLY | O_CREAT | O_TRUNC |O_APPEND, 0777);
	if (cust_log < 0) {
		fprintf(stderr, "Error opening customer_log\n");
		exit(-1);
	}
	int test_data = open(argv[1], O_RDONLY);
	if (test_data < 0) {
		fprintf(stderr, "Error opening test_data\n");
		exit(-1);
	}
	int er = 1;
	pid_t parentpid = getppid();
	struct sigaction sigact, sigalrm;
	sigact.sa_handler = &signal_handler;
	sigalrm.sa_handler = &sig_alarm_handler;
	sigact.sa_flags = SA_RESTART;
	sigemptyset(&sigact.sa_mask);
	sigaddset(&sigact.sa_mask, SIGALRM);
	sigemptyset(&sigalrm.sa_mask);
	sigaddset(&sigalrm.sa_mask, SIGUSR1);
	sigaddset(&sigalrm.sa_mask, SIGUSR2);
	sigaddset(&sigalrm.sa_mask, SIGINT);

	if (sigaction(SIGUSR1, &sigact, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR1\n");
		exit(-1);
	}
	if (sigaction(SIGUSR2, &sigact, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGUSR2\n");
		exit(-1);
	}
	if (sigaction(SIGINT, &sigact, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGINT\n");
		exit(-1);
	}
	if (sigaction(SIGALRM, &sigalrm, NULL) < 0) {
		fprintf(stderr, "Could not handle SIGALARM\n");
		exit(-1);
	}

	for (int i = 0; i < 3; i++)
		ranks[i] = 1;
	int prev = 0, prev_nsecs = 0, next, next_nsecs;
	while (er > 0) {
		int code, size_ = 0;
		char buf;
		char buf_num[3];
		char buf_int[20];
		char buf_dec[3];
		memset(buf_num, '\0', sizeof(buf_num));
		memset(buf_int, '\0', sizeof(buf_int));
		memset(buf_dec, '\0', sizeof(buf_dec));
		er = read(test_data, buf_num, 2);
		if (er <= 0)
			break;
		code = atoi(buf_num);
		size_ = 0;
		while ((er = read(test_data, &buf, 1)) > 0) {
			if (buf == '\n' || buf == '.')
				break;
			buf_int[size_++] = buf;
		}
		if (er < 0)
			break;
		next = atoi(buf_int);
		next_nsecs = 0;
		if (buf == '.') {
			er = read(test_data, buf_dec, 2);
			if (er < 0) 
				break;
			next_nsecs = buf_dec[0] - '0';
		}
		struct timespec tim, remtim;
		if (next == prev) {
			tim.tv_sec = 0;
			tim.tv_nsec = (next_nsecs - prev_nsecs) * 100000000;
		}
		else {
			if (next_nsecs > prev_nsecs) {
				tim.tv_sec = next - prev;
				tim.tv_nsec = (next_nsecs - prev_nsecs) * 100000000;
			}
			else {
				tim.tv_sec = next - prev - 1;
				tim.tv_nsec = (next_nsecs + 10 - prev_nsecs) * 100000000;
			}
		}

		while (1) {
			if (clock_nanosleep(CLOCK_REALTIME, 0, &tim, &remtim) != 0)
				tim = remtim;
			else
				break;
		}

		prev = next;
		prev_nsecs = next_nsecs;
		char mess[100];
		switch (code) {	
			case 0:
				sprintf(mess, "ordinary\n");
				if (write(STDOUT_FILENO, mess, strlen(mess)) < 0) {
					fprintf(stderr, "Error writing to stdout\n");
					exit(-1);
				}
				fsync(STDOUT_FILENO);
				break;
			case 1:
				kill(parentpid, SIGUSR1);
				struct itimerval inter;
				inter.it_interval.tv_sec = 0;
				inter.it_interval.tv_usec = 0;
				inter.it_value.tv_sec = 1;
				inter.it_value.tv_usec = 0;
				if (setitimer(ITIMER_REAL, &inter, NULL) < 0) {
					perror("");
					exit(-1);
				}
				break;
			case 2:
				kill(parentpid, SIGUSR2);
				/*Since no VIP customer would be sent
				before receiving the previous one, no
				need to set an alarm for this one:
				Guaranteed that a VIP will always be
				processed on time, only member may cause
				timeout*/
				break;
			default:
				fprintf(stderr, "Read data is: %d\n", code);
				exit(-1);
		}
		unfinished++;
		memset(mess, '\0', sizeof(mess));
		sprintf(mess, "send %d %d\n", code, ranks[code]);
		if (write(cust_log, mess, strlen(mess)) < 0) {
			fprintf(stderr, "Error writing to customer_log\n");
			exit(-1);
		}
	}
	while (unfinished > 0) {
		;
	}
	if (er < 0) {
		perror("");
		exit(EXIT_FAILURE);
	}
	close(cust_log);
	close(test_data);
	exit(0);
}