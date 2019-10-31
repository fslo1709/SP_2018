#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void Print_error(char *er) { //Error handler for main
	perror(er); 
	exit(1);
}

struct Node{
	char val;
	int dim;
	float thr;
	struct Node *left, *right;
};

struct Node* Node_Create(float thr, int dim, short val) {
	struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
	newNode->val = val, newNode->dim = dim, newNode->thr = thr;
	newNode->left = newNode->right = NULL;
	return newNode;
}

struct Info{
	int ID;
	int Label;
	float ft[33];
};

void Pointer_Swap(struct Info **a, struct Info **b) {
	struct Info *temp = *a; 
	*a = *b; 
	*b = temp;
	return;
}

int C_ONES(struct Info *entries[], int L, int R) {
	int count = 0;
	for (int i = L; i <= R; ++i) 
		if (entries[i]->Label == 1) 
			count++;
	if (count == (R-L+1)) 
		return -1;
	return count;
}

float GET_GINI(struct Info *entries[], int id, int ST, int ED){ //ST->start, ED->end
	float L1 = 0, L0 = 0, R1 = 0, R0 = 0;
	for (int i = ST; i <= id; ++i) {
		if(entries[i]->Label == 1) 
			L1++;
		if(entries[i]->Label == 0) 
			L0++;
	}
	L1 /= (id - ST + 1), L0 /= (id - ST + 1);
	for (int i = id+1; i <= ED; ++i) {
		if(entries[i]->Label == 1) 
			R1++;
		if(entries[i]->Label == 0) 
			R0++;
	}
	R1 /= (ED - id), R0 /= (ED - id);
	return (L1 * (1-L1) + L0 * (1-L0) + R1 * (1-R1) + R0 * (1-R0));
}

void traverse(struct Node *root) { //LDR traversal
	if (root) {
		traverse(root->left);
		if (root->dim == -2) 
			fprintf(stdout, "Leaf: value = %d\n", root->val);
		else 
			fprintf(stdout, "Node: dimension = %d, threshold = %f\n", root->dim, root->thr);
		traverse(root->right);
	}
	return;
}

int Divide(struct Info *entries[], int L, int R, int Dim) { //Find pivot
	int mid = R, split = L;
	for (int i = L; i <= R; i++)
		if (entries[i]->ft[Dim] < entries[mid]->ft[Dim]) 
			Pointer_Swap(&entries[i], &entries[split++]);
	Pointer_Swap(&entries[mid], &entries[split]);
	return split;
}

void Qsorting(struct Info *entries[], int L, int R, int Dim) { //Quicksort
	if (L < R) {
		int mid = Divide(entries, L, R, Dim);
		Qsorting(entries, L, mid-1, Dim);
		Qsorting(entries, mid+1, R, Dim);
	}
	return;
}

volatile int nTrees; volatile int nThreads; volatile int nJobs; volatile int CZ_Count;

int EXAMPLE_n;
struct Info *EXAMPLE, *CZ_; 
struct Node *Trees[30000];

void Initialize_data(int argc, char *argv[], FILE **FP_TR, FILE **FP_TST) {
	if (argc != 9) {
		puts("Format: ./hw4 -data [data_dir] -output submission.csv -tree [tree_number] -thread [thread_number]");
		exit(0);
	}
	srand(time(NULL));
	EXAMPLE = (struct Info *)malloc(30000*sizeof(struct Info));
	if (!EXAMPLE) 
		Print_error("MLE");
	nTrees = atoi(argv[6]), nThreads = atoi(argv[8]);
	char Training[50], Testing[50]; 
	sprintf(Training, "%s/training_data", argv[2]);
	sprintf(Testing, "%s/testing_data", argv[2]);
	FILE *train = fopen(Training, "r");
	if (!train) 
		Print_error("Training data file error");
	*FP_TR = train;
	FILE *test = fopen(Testing, "r");
	if (!test) 
		Print_error("Testing data file error");
	*FP_TST = test;
}

void Read_data(FILE *FP_TR) {
	while (~fscanf(FP_TR, "%d", &EXAMPLE[EXAMPLE_n].ID)) { 
		for (int i = 0; i < 33; i++) 
			fscanf(FP_TR, "%f", &EXAMPLE[EXAMPLE_n].ft[i]);
		fscanf(FP_TR, "%d", &EXAMPLE[EXAMPLE_n++].Label);
	}
}
/*TRAINING*/
pthread_mutex_t train1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t train2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t traincond = PTHREAD_COND_INITIALIZER;
/*TESTING*/
pthread_mutex_t test1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t test2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t testcond = PTHREAD_COND_INITIALIZER;


struct Node* New_Root(struct Info *entries[], int LEFT, int RIGHT) { 
	int zeroes = C_ONES(entries, LEFT, RIGHT);
	if (zeroes <= 0) { 
		struct Node *leaf = Node_Create(-2, -2, -zeroes);
		return leaf;
	}
	int ones = (RIGHT - LEFT + 1) - zeroes;
	float M_GINI = 1000.02; int Target_Cut = -1, High_Dim = -1;
	for (int i = 0; i < 33; ++i) { 
		Qsorting(entries, LEFT, RIGHT, i); 
		float L_GINI = 532.0; 
		int Local_Cut = -1;
		for (int j = LEFT; j < RIGHT; ++j) { 
			float CUR_GINI = GET_GINI(entries, j, LEFT, RIGHT);
			if (CUR_GINI < L_GINI) 
				L_GINI = CUR_GINI, Local_Cut = j;
		}
		if (L_GINI < M_GINI) {
			M_GINI = L_GINI, Target_Cut = Local_Cut;
			High_Dim = i;
		}
	}
	assert(Target_Cut != -1 && High_Dim != -1);
	Qsorting(entries, LEFT, RIGHT, High_Dim);
	float THR = (entries[Target_Cut]->ft[High_Dim] + entries[Target_Cut+1]->ft[High_Dim]) / 2.0;
	struct Node *newNode = Node_Create(THR, High_Dim, -1);
	newNode->left = New_Root(entries, LEFT, Target_Cut);
	newNode->right = New_Root(entries, Target_Cut+1, RIGHT);
	return newNode;
}

volatile int dead_threads = 0;

void* Thread_RTree(void *arg) {
	while (1) {
		pthread_mutex_lock(&train1);
		if (nJobs == nTrees) {
			pthread_mutex_lock(&train2);
			dead_threads++;
			pthread_cond_signal(&traincond);
			pthread_mutex_unlock(&train1);
			pthread_mutex_unlock(&train2);
			pthread_exit((void*)0);
		}
		int curPos = nJobs++;
		pthread_mutex_unlock(&train1);
		struct Info *entries[120] = {}; 
		for (int i = 0; i < 120; i++) 
			entries[i] = &EXAMPLE[rand() % EXAMPLE_n];
		struct Node *root = New_Root(entries, 0, 120-1);
		Trees[curPos] = root;
		assert(root != NULL);
	}
	return (void *)0;
}

void create_random_forest(void) {
	pthread_t tid[nThreads];
	for (int i = 0; i < nThreads; ++i) {
		pthread_create(&tid[i], NULL, Thread_RTree, (void *)0);
	}
	pthread_mutex_lock(&train2);
	while (dead_threads != nThreads) {
		pthread_cond_wait(&traincond, &train2);
    }
	pthread_mutex_unlock(&train2);
	for (int i = 0; i < nThreads; ++i) {
		pthread_join(tid[i], NULL);
	}
}

volatile int q_dead_threads, q_nJobs;
int person_prediction[30000];

int Tree_score(struct Node* root, struct Info* entry) {
	if (root->dim == -2) 
		return root->val;
	if (root->thr > entry->ft[root->dim]) 
		return Tree_score(root->left, entry);
	return Tree_score(root->right, entry);
}

int evaluate_all_trees(struct Info *entry) {
	int score[2] = {};
	for (int i = 0; i < nTrees; ++i) 
		score[Tree_score(Trees[i], entry)]++;
	return score[0] > score[1] ? 0 : 1;
}

void* run_trees(void *arg) {
	while (1) {
		pthread_mutex_lock(&test1);
		if (q_nJobs == CZ_Count) {
			pthread_mutex_lock(&test2);
			q_dead_threads++;
			pthread_cond_signal(&testcond);
			pthread_mutex_unlock(&test1);
			pthread_mutex_unlock(&test2);
			pthread_exit((void*)0); 
		}
		int pos = q_nJobs++; 
		pthread_mutex_unlock(&test1);
		struct Info *query = &CZ_[pos];
		person_prediction[query->ID] = evaluate_all_trees(query);
	}
	return (void *)0;
}

void Many_Queries(void) {
	pthread_t tid2[nThreads];
	for (int i = 0; i < nThreads; ++i) 
		pthread_create(&tid2[i], NULL, run_trees, (void *)0);
	pthread_mutex_lock(&test2);
	while (q_dead_threads != nThreads) 
		pthread_cond_wait(&testcond, &test2);
	pthread_mutex_unlock(&test2);
	for (int i = 0; i < nThreads; ++i) 
		pthread_join(tid2[i], NULL);		
}

int Predictions(FILE *FP_TST) {
	assert(FP_TST); 
	int max = -1;
	CZ_ = (struct Info *)malloc(30000 * sizeof(struct Info));
	while (~fscanf(FP_TST, "%d", &CZ_[CZ_Count].ID)) {
		for (int i = 0; i < 33; i++) 
			fscanf(FP_TST, "%f", &CZ_[CZ_Count].ft[i]);
		max = CZ_[CZ_Count].ID > max ? CZ_[CZ_Count].ID: max;
		CZ_Count++;
	}
	for (int i = 0; i < max; i++) 
		person_prediction[i] = -1;
	Many_Queries();
	return max;
}

void FinalResult(char o_fname[], int max_id) {
	FILE *o_file = fopen(o_fname, "w");
	assert(o_file != NULL);
	fprintf(o_file, "id,label\n");
	for (int i = 0; i < max_id; ++i) 
		if (person_prediction[i] != -1) 
			fprintf(o_file, "%d,%d\n", i, person_prediction[i]);
}

int main(int argc, char *argv[]) {
	FILE *FP_TR, *FP_TST;
	Initialize_data(argc, argv, &FP_TR, &FP_TST);
	Read_data(FP_TR);
	create_random_forest();
	free(EXAMPLE); 
	fclose(FP_TR);
	int max_id = Predictions(FP_TST);
	FinalResult(argv[4], max_id);
	fclose(FP_TST);
	exit(0);
}
