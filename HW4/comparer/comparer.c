#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	ssize_t rval;
	ssize_t len1 = 0;
	ssize_t len2 = 0;
	char *line1 = NULL;
	char *line2 = NULL;
	FILE *F1, *F2;
	F1 = fopen(argv[1], "r");
	F2 = fopen(argv[2], "r");
	float count = 0;
	float equal = 0;
	while (((rval = getline(&line1, &len1, F1)) != -1) && 
		((rval = getline(&line2, &len2, F2)) != -1)) {
		if (strcmp(line1, line2) == 0)
			equal = equal + 1.0;
		count = count + 1.0;
	}
	float res = (equal*100)/count;
	fprintf(stdout, "%f", res);
	fclose(F1);
	fclose(F2);
	if (line1)
		free(line1);
	if (line2)
		free(line2);
	return 0;
}