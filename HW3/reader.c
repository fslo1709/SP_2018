#include <stdio.h>
#include <string.h>

int main(int argc, char const *argv[])
{
	char buf[40];
	memset(buf, '\0', sizeof(buf));
	FILE *fd = fopen("my_test_data_EDF", "w");
	sprintf(buf, "1 0.4\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 0.5\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 0.8\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 1.1\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 1.4\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 1.7\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 2.0\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 2.3\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 2.6\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 2.9\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 3.2\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 3.5\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 3.8\n");
	fwrite(buf, 1, strlen(buf), fd);
	sprintf(buf, "2 4.1\n");
	fwrite(buf, 1, strlen(buf), fd);
	fclose(fd);
	return 0;
}