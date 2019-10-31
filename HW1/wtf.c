#include <stdio.h>
#include <stdlib.h>
typedef struct {
	int id;
	int amount;
	int price;
} Item;
int main(int argc, char const *argv[])
{
	FILE *infile;
	Item bid;
	infile = fopen("item_list", "r");
	if (infile==NULL) {
		printf("ERROR");
		exit(1);
	}
	while (fread(&bid, sizeof(Item), 1, infile))
		printf("id %d amount %d price %d \n", bid.id, bid.amount, bid.price);
	fclose(infile);
	return 0;
}