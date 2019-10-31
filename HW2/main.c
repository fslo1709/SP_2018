#include <stdio.h>
#include <stdlib.h>

typedef struct {
	int id;
	int key;
	int money;
}decide;

int comp(const void *a, const void *b) {
	decide *A = (decide *)a;
	decide *B = (decide *)b;
	return B->money - A->money;
}

int main(int argc, char const *argv[])
{
	decide mon[4];
	for (int i = 0; i < 4; i++)
		scanf("%d %d %d", &mon[i].id, &mon[i].key, &mon[i].money);
	qsort(mon, 4, sizeof(decide), comp);
	int winner = 0;
	if (mon[0].money == mon[1].money) {
		winner = 1;
		while (mon[winner].money == mon[winner-1].money && winner < 3) {
			winner++;
		}
	}
	printf("%d\n", mon[winner].money);
	return 0;
}