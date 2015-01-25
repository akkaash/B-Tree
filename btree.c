#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"
#include "sys/queue.h"

typedef int bool;
#define true 1
#define false 0

TAILQ_HEAD(tailhead, entry) head;
struct tailqhead *headp;
struct entry
{
	long offset;
	TAILQ_ENTRY(entry) entries;
} *n1, *n2, *np;


typedef struct{ /* B-Tree node */ 
	int n; /* Number of keys in node */ 
	int *key; /* Node's keys */ 
	long *child; /* Node's child subtree offsets */ 
} btree_node;

typedef struct 
{
	btree_node* node;
	long offset;
}package;

btree_node *root;
char *indexFileName = NULL;
FILE *indexFileP = NULL;
int order = -1;
long rootOffset = -1L;

btree_node* createNewNode(){
	btree_node* node = (btree_node*) malloc(sizeof(btree_node));
	node->n = 0;
	node->key = (int *) calloc(order-1, sizeof(int));
	node->child = (long*) calloc(order, sizeof(long));

	memset(node->key, 0, order-1 * sizeof(int));
	memset(node->child, 0, order * sizeof(long));

	return node;
}

btree_node* createTempNode(){
	btree_node* node = (btree_node*) malloc(sizeof(btree_node));
	node->n = 0;
	node->key = (int *) calloc(order, sizeof(int));
	node->child = (long*) calloc(order+1, sizeof(long));

	memset(node->key, 0, order * sizeof(int));
	memset(node->child, 0, (order+1) * sizeof(long));

	return node;	
}

void displayNode(btree_node* node){
	if (node == NULL)
	{
		printf("displayNode: node is NULL\n");
	}
	printf("displayNode: n=%d\t", node->n);
	printf("keys= ");
	int i;
	for (i = 0; i < order-1; ++i)
	{
		printf("%d ", node->key[i]);
	}
	printf("\toffsets= ");
	for (i = 0; i < order; ++i)
	{
		printf("%ld ", node->child[i]);
	}
	printf("\n");


}

void displayTempNode(btree_node* node){
	if (node == NULL)
	{
		printf("displayTempNode: node is NULL\n");
	}


	printf("displayTempNode: n=%d\t", node->n);
	printf("keys= ");
	int i;
	for (i = 0; i < order; ++i)
	{
		printf("%d ", node->key[i]);
	}
	printf("\toffsets= ");
	for (i = 0; i < order+1; ++i)
	{
		printf("%ld ", node->child[i]);
	}
	printf("\n");


}

long writeNodeToFile(btree_node* node){
	long backupOffset = ftell(indexFileP);

	// fseek(indexFileP, offset, SEEK_SET);
	fwrite( &node->n, sizeof( int ), 1, indexFileP ); 
	fwrite( node->key, sizeof( int ), order - 1, indexFileP ); 
	fwrite( node->child, sizeof( long ), order, indexFileP );

	fflush(indexFileP);
	
	return backupOffset;
}

btree_node *readNodeFromOffset(long offset){

	if(offset <= 0){
		return NULL;
	}

	long backupOffset = ftell(indexFileP);
	int n;
	int *keys = calloc(order-1, sizeof(int));
	long *offsets = calloc(order, sizeof(long));

	fseek(indexFileP, offset, SEEK_SET);
	fread(&n, sizeof(int), 1, indexFileP);
	fread(keys, sizeof(int), order-1, indexFileP);
	fread(offsets, sizeof(long), order, indexFileP);

	// printf("readNodeFromOffset: n=%d\t", n);
	// printf("keys= ");
	int i;
	for (i = 0; i < order-1; ++i)
	{
		// printf("%d ", keys[i]);
	}
	// printf("\toffsets= ");
	for (i = 0; i < order; ++i)
	{
		// printf("%ld ", offsets[i]);
	}
	// printf("\n");

	btree_node *returnNode = createNewNode();
	returnNode->n = n;
	// returnNode->key = (int*) calloc(order-1, sizeof(int));
	// memcpy(returnNode->key, keys, order-1*sizeof(int));
	// returnNode->child = (long *) calloc(order, sizeof(long));
	// memcpy(returnNode->child, offsets, order*sizeof(long));

	for(i = 0; i < order-1; i++){
		returnNode->key[i] = keys[i];
	}
	for(i = 0; i < order; i++){
		returnNode->child[i] = offsets[i];
	}

	//reset the file pointer
	fseek(indexFileP, backupOffset, SEEK_SET);
	// free(&n);
	// free(keys);
	// free(offsets);
	return returnNode;
}

void buildFromIndexFile(){
	long backupOffset = ftell(indexFileP);
	fseek(indexFileP, 0, SEEK_SET);
	fread(&rootOffset, sizeof(long), 1, indexFileP);
	root = readNodeFromOffset(rootOffset);
	
	fseek(indexFileP, backupOffset, SEEK_SET);
}

void init(int argc, char **argv)
{
	root = NULL;
	if(argc < 3)
	{
		printf("Insufficient arguments. \n");
		exit(-1);
	} 
	else if (argc > 3)
	{
		printf("Too many arguments.\n");
		exit(-1);
	}
	else
	{
		indexFileName = argv[1];
		order = atoi(argv[2]);
		if (order < 3)
		{
			printf("order should be greater than or equal to 3\n");
			exit(-1);
		}

		indexFileP = fopen(indexFileName, "r+b");
		if(indexFileP == NULL){
			rootOffset = -1L;
			indexFileP = fopen(indexFileName, "w+b");
			fwrite(&rootOffset, sizeof(long), 1, indexFileP);
			fflush(indexFileP);
		}
		else{
			// fseek(indexFileP, 0, SEEK_SET);
			// fread(&rootOffset, sizeof(long), 1, indexFileP);

			buildFromIndexFile();
			fseek(indexFileP, 0, SEEK_END);
		}
	}


}

int getCommandType(char *s)
{
	char *command = (char *)strtok(s, " ");
	if(strcmp(command, "add") == 0)
	{
		return 1;
	}
	else if (strcmp(command, "find") == 0)
	{
		return 2;
	}
	else if (strcmp(command, "print") == 0)
	{
		return 3;
	}
	else if (strcmp(command, "end") == 0)
	{
		return 4;
	}
	return -1;
}


int searchNode(btree_node* node, int value){
	int i = -1;
	if (node == NULL)
	{
		// printf("searchNode: node is NULL\n");
	}
	// printf("%d\n", node->n);
	for(i=0; i<node->n;i++){
		if(value < node->key[i]){
			return i;
		}
	}

	return i;
}

void copyNodeToChild(btree_node* node, btree_node* child, int bottom, int top){
	int i;
	int j = 0;
	for(i = bottom; i <= top; i++){
		if (i != top){
			child->key[j] = node->key[i];
			child->n++;
		}
		// child->key[j] = node->key[i];
		child->child[j] = node->child[i];
		
		j++;
	}
}

btree_node* mPromoteNode = NULL;
void setPromoteNode(btree_node* node){
	mPromoteNode = node;
}
btree_node* getPromoteNode(){
	return mPromoteNode;
}

void shiftAndFill(btree_node* node, int value, int pos){
	int i;
	for(i=node->n; i > pos; i--){
		// printf("shiftAndFill: in loop\n");
		node->key[i] = node->key[i-1];
		node->child[i] = node->child[i-1];
	}

	node->key[pos] = value;
	node->child[pos] = 0;
	node->n++;
	// printf("shiftAndFill: \n");
	// displayNode(node);
}

void promotedShiftAndFill(btree_node* node, btree_node* promoteNode, int pos){
	int i;

	for(i = node->n; i > pos; i--){
		node->key[i] = node->key[i-1];
	}
	for(i = node->n + 1; i > pos; i--){
		node->child[i] = node->child[i-1];
	}
	node->key[pos] = promoteNode->key[0];
	node->child[pos] = promoteNode->child[0];
	node->child[pos+1] = promoteNode->child[1];
	node->n++;

}



btree_node* splitNode(btree_node* node, int value, int pos){
	// int median = (int)ceil((order-1)/2) + 1;
	// btree_node* left = createNewNode();
	// btree_node* right = createNewNode();

	// copyNodeToChild(node, left, 0, median);
	// copyNodeToChild(node, right, median+1, node->n);

	// long newOffset = -1L;
	// if(pos <= median){
	// 	int t = searchNode(left, value);
	// 	shiftAndFill(left, value, t);
	// 	// newOffset = writeNodeToFile(left);
	// } else if (pos > median){
	// 	int t = searchNode(right, value);
	// 	// shiftAndFill(right, value, pos-median);
	// 	shiftAndFill(right, value, t);
	// 	// newOffset = writeNodeToFile(right);
	// }

	// // if (newOffset == -1L){
	// // 	printf("splitNode: newOffset= -1. Exiting!\n");
	// // 	exit(-1);
	// // } else{

	// // }


	// long leftOffset = writeNodeToFile(left);
	// long rightOffset = writeNodeToFile(right);

	// btree_node* promoteNode = createNewNode();
	// promoteNode->key[0] = node->key[median];
	// promoteNode->child[0] = leftOffset;
	// promoteNode->child[1] = rightOffset;
	// promoteNode->n++;

	// return promoteNode;
	// // setPromoteNode(promoteNode);

	btree_node* tempNode = createTempNode();

	// printf("pos = %d\n", pos);
	// getchar();

	int i;
	int j = 0;
	for(i = 0; i<=node->n; i++){
		if (i != node->n){
			tempNode->key[j] = node->key[i]; 
		}
		tempNode->child[j] = node->child[i];
		j++;
	}
	j = order;
	for(i = order-1; i >= pos; i--){
		if (i != order-1){
			tempNode->key[j] = tempNode->key[i];
		}
		tempNode->child[j] = tempNode->child[i];
		j--;
	}
	tempNode->key[j] = value;
	tempNode->child[j] = 0;

	// displayTempNode(tempNode);

	int median = (int) floor(order/2);

	btree_node* left = createNewNode();
	btree_node* right = createNewNode();
	j = 0;
	for(i = 0; i <= median; i++){
		if(i != median){
			left->key[j] = tempNode->key[i];
			left->n++;
		}

		left->child[j] = tempNode->child[i];
		j++;
	}

	j = 0;
	for(i = median+1; i <= order; i++){
		if (i != order){
			right->key[j] = tempNode->key[i];
			right->n++;
		}

		right->child[j] = tempNode->child[i];
		j++;
	}

	long leftOffset = writeNodeToFile(left);
	long rightOffset = writeNodeToFile(right);

	btree_node* promoteNode = createNewNode();
	promoteNode->key[0] = tempNode->key[median];
	promoteNode->child[0] = leftOffset;
	promoteNode->child[1] = rightOffset;
	promoteNode->n++;

	return promoteNode;

}

btree_node* promotedSplitNode(btree_node* node, btree_node* promoteNode, int pos){

	// int median = (int)ceil((order-1)/2) + 1;
	// btree_node* left = createNewNode();
	// btree_node* right = createNewNode();

	// copyNodeToChild(node, left, 0, median);
	// //fix for child value on left node...
	// // left->child[median] = node->child[median];


	// copyNodeToChild(node, right, median+1, node->n);
	// // right->child[node->n] = node->child[node->n];

	// if(pos <= median){
	// 	int t = searchNode(left, promoteNode->key[0]);
	// 	promotedShiftAndFill(left, promoteNode, t);

	// } else if(pos > median){
	// 	right->child[node->n] = node->child[node->n];
	// 	int t = searchNode(right, promoteNode->key[0]);
	// 	promotedShiftAndFill(right, promoteNode, t);
	// }

	// long leftOffset = writeNodeToFile(left);
	// long rightOffset = writeNodeToFile(right);

	// btree_node* nextPromoteNode = createNewNode();
	// nextPromoteNode->key[0] = node->key[median];
	// nextPromoteNode->child[0] = leftOffset;
	// nextPromoteNode->child[1] = rightOffset;
	// nextPromoteNode->n++;

	// return nextPromoteNode;

	btree_node* tempNode = createTempNode();
	// printf("pos = %d\n", pos);

	int i, j = 0;
		for(i = 0; i<=node->n; i++){
		if (i != node->n){
			tempNode->key[j] = node->key[i]; 
		}
		tempNode->child[j] = node->child[i];
		j++;
	}
	j = order;
	for(i = order-1; i >= pos; i--){
		if (i != order-1){
			tempNode->key[j] = tempNode->key[i];
		}
		tempNode->child[j] = tempNode->child[i];
		j--;
	}

	tempNode->key[j] = promoteNode->key[0];
	tempNode->child[j] = promoteNode->child[0];
	tempNode->child[j+1] = promoteNode->child[1];

	// displayTempNode(tempNode);

	int median = (int) floor(order/2);

	btree_node* left = createNewNode();
	btree_node* right = createNewNode();
	j = 0;
	for(i = 0; i <= median; i++){
		if(i != median){
			left->key[j] = tempNode->key[i];
			left->n++;
		}

		left->child[j] = tempNode->child[i];
		j++;
	}

	j = 0;
	for(i = median+1; i <= order; i++){
		if (i != order){
			right->key[j] = tempNode->key[i];
			right->n++;
		}

		right->child[j] = tempNode->child[i];
		j++;
	}

	long leftOffset = writeNodeToFile(left);
	long rightOffset = writeNodeToFile(right);

	btree_node* returnNode = createNewNode();
	returnNode->key[0] = tempNode->key[median];
	returnNode->child[0] = leftOffset;
	returnNode->child[1] = rightOffset;
	returnNode->n++;

	return returnNode;



}

btree_node* insert(int value, btree_node* root, int *promoteValue){
	// printf("insert called\n");
	if(root == NULL && rootOffset == -1){
		root = createNewNode();
		root->key[root->n] = value;
		root->n++;
		
		long newOffset = writeNodeToFile(root);

		btree_node* returnNode = createNewNode();
		returnNode->n = -1;
		returnNode->child[0] = newOffset;

		return returnNode;		
		
	} 


	else if(root == NULL && rootOffset != -1){
		// printf("insert: Inside second if\n");
		// promoteValue = (int*)malloc(sizeof(int));
		*promoteValue = value;
		// printf("insert: second if: %d\n", *promoteValue);
		return NULL;
	} 

	else if(root != NULL && rootOffset != -1){
		// printf("WTF: \n");
		// displayNode(root);
		// printf("==============\n");
		int i = searchNode(root, value);
		// printf("i=%d\n", i);
		if(i == -1){
			// printf("insert: searchNode Failed!\n");
			// printf("insert: value=%d\n", value);
			exit(-1);
		} else{
			int pval;
			btree_node* newRoot = readNodeFromOffset(root->child[i]);
			btree_node* r = insert(value, newRoot, &pval);
			// printf("insert: back in!\n");
			 if(r == NULL){
				// printf("back in insert:%d\n", pval);
				if(root->n < order-1){
					// printf("insert: r == NULL: calling shiftAndFill\n");
					shiftAndFill(root, pval, i);
					long newOffset = writeNodeToFile(root);

					btree_node* returnNode = createNewNode();
					returnNode->n = -1;
					returnNode->child[0] = newOffset;

					return returnNode;
				} else{
					 // printf("*************\n");
					 // printf("insert: following node is full\n");
					 // displayNode(root);
					 // printf("*************\n");
					 // printf("insert: Calling Split Node\n");
					btree_node* newRoot = splitNode(root, pval, i);
					// rootOffset = writeNodeToFile(newRoot);
					return newRoot;

				}
				// getchar();
			} else if (r != NULL){
				// printf("********* r != NULL**********\n");
				// displayNode(r);

				if (r->n == -1){
					root->child[i] = r->child[0];
					long returnOffset = writeNodeToFile(root);

					btree_node* returnNode = createNewNode();
					returnNode->n = -1;
					returnNode->child[0] = returnOffset;

					return returnNode;

				} else{
					// printf("insert: r != NULL: r->n != -1: \n");
					if (root->n < order-1){
						// printf("insert: r != NULL: r->n != -1: Calling promotedShiftAndFill\n");
						promotedShiftAndFill(root, r, i);

						long newOffset = writeNodeToFile(root);

						btree_node* returnNode = createNewNode();
						returnNode->n = -1;
						returnNode->child[0] = newOffset;

						return returnNode;

					} else{
						// // printf("insert: r != NULL: r->n != -1: Calling promotedSplitNode\n");
						// displayNode(root);
						// displayNode(r);
						btree_node* newRoot = promotedSplitNode(root, r, i);

						return newRoot;
					}
				}
			}
		}

	} 

	printf("insert: Unknown error occurred. Returning NULL!\n");
	return NULL;

}

bool search(btree_node* node, int value){
	// printf("search called\n");
	if(node == NULL){
		// printf("node == NULL\n");
		return false;
	}
	
	int i;
	for(i=0; i<node->n; i++){
		if(value == node->key[i]){
			return true;
		} else if(value < node->key[i]){
			btree_node* newNode = readNodeFromOffset(node->child[i]);
			return search(newNode, value);
		}
	}

	btree_node* newNode = readNodeFromOffset(node->child[node->n]);
	return search(newNode, value);
}



void add(char *inputLine){
	// printf("Inside add()\n");
	int firstEverFlag = 0;
	if(root == NULL)
		firstEverFlag = 1;


	strtok(inputLine, " ");
	int value = atoi(strtok(NULL, " "));
	// printf("add: adding value = %d\n", value);
	btree_node* tmp_root = root;

	if (search(root, value)){
		printf("Entry with key=%d already exists\n", value);
		return;
	}

	root = insert(value, root, NULL);
	// printf("add: after insert\n");
	
	if (root->n == -1){
		// displayNode(root);
		rootOffset = root->child[0];

		if (firstEverFlag == 1){
			// printf("add: firstEverFlag = 1\n");

			root = readNodeFromOffset(root->child[0]);
			// displayNode(root);
			

		} else if(tmp_root != NULL){
			root = tmp_root;
		}
			

		

		
	}else{
		rootOffset = writeNodeToFile(root);
		// printf("add: else!!!\n");
	}

	// printf("rootOffset = %ld \n", rootOffset);
	// displayNode(root);
}


void find(char *inputLine){
	strtok(inputLine, " ");
	int value = atoi(strtok(NULL, " "));
	// printf("find: find value = %d\n", value);

	if (root == NULL){
		printf("Entry with key=%d does not exist\n", value);
		return;
	}

	if(search(root, value)){
		printf("Entry with key=%d exists\n", value);
		return;
	} else{
		printf("Entry with key=%d does not exist\n", value);
		return;	
	}
}

void print(char *inputLine){



	n1 = malloc(sizeof(struct entry));
	n1->offset = rootOffset;
	// printf("print: rootOffset = %ld\n", rootOffset);
	TAILQ_INSERT_HEAD(&head, n1, entries);
	long tmp, nextChange = rootOffset;
	int level = 1;

	printf(" %d:", level);

	if (root == NULL){
		printf("\n");
		return;
	}

	while(head.tqh_first != NULL){
		printf(" ");
		// printf("inside while\n");
		np = head.tqh_first;
		btree_node* node = readNodeFromOffset(np->offset);
		
		if(node != NULL){
			// printf("node->offset = %d\n", node->offset);
			int i;
			for(i=0; i < order; i++){
				if(node->child[i] != 0){
					n2 = malloc(sizeof(struct entry));
					n2->offset = node->child[i];
					TAILQ_INSERT_TAIL(&head, n2, entries);
					tmp = node->child[i];	
				}
				
			}
			// for(i = 0; i < node->n - 1; i++){
			// 	printf("print-> %d \n", node->key[i]);
			// }

			for( i = 0; i < node->n - 1; i++ ) { 
				printf( "%d,", node->key[ i ] ); 
			} 
			printf( "%d", node->key[ node->n - 1 ] );


			
		} else{
			// printf("print: node = NULL\n");
		}
		
		if ((np=head.tqh_first)->offset == nextChange){
			level++;
			nextChange = tmp;
			TAILQ_REMOVE(&head, head.tqh_first, entries);
			if (head.tqh_first != NULL){
				printf(" \n %d:", level);
			}
			
		} else
			TAILQ_REMOVE(&head, head.tqh_first, entries);
		// printf(" ");

		// printf("print: after TAILQ_REMOVE: \n");
		// for(np = head.tqh_first; np!= NULL; np = np->entries.tqe_next){
		// 	printf("%ld ", np->offset);
		// }
		// printf("\n");
	}

	// printf("outside while\n");
	printf(" \n");

}

void end(char *inputLine){

	fseek(indexFileP, 0, SEEK_SET);
	fwrite(&rootOffset, sizeof(long), 1, indexFileP);
	fflush(indexFileP);
	fclose(indexFileP);
	
}

int main(int argc, char **argv)
{
	init(argc, argv);

	char *inputLine = NULL;
	size_t len = 0;
	getline(&inputLine, &len, stdin);
	char *pos;
	if ((pos=strchr(inputLine, '\n')) != NULL)
	    *pos = '\0';

	while(strcmp(inputLine, "end") != 0)
	{
		char *s = malloc(strlen(inputLine) * sizeof(char));
		sprintf(s, "%s", inputLine);

		int commandType = getCommandType(s);

		switch(commandType)
		{
			case 1: add(inputLine);
					break;
			case 2: find(inputLine);
					break;
			case 3: print(inputLine);
					break;
			case 4: end(inputLine);
					break;
			default:
					printf("Incorrect Command. \n");
		}

		getline(&inputLine, &len, stdin);
		
		if ((pos=strchr(inputLine, '\n')) != NULL)
	    	*pos = '\0';
		
	}

	end(inputLine);

	return 0;
}

