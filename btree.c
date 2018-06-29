#include <stdio.h>
#include <stdlib.h>

#define M 4

typedef enum {FALSE, TRUE} boolean;

typedef int keytype;

boolean done, deleted, undersize;

typedef struct leaf {
    int n;
    keytype key[2 * M];
    struct leaf *branch[2 * M + 1];
} *leafptr;

leafptr root = NULL;

keytype key;

leafptr newl;

leafptr newleaf(void) {
	leafptr l;
	
	if ((l = malloc(sizeof *l)) == NULL ){
		printf("cannot allocate memory for index\n");
		exit(EXIT_FAILURE);
	}
	return l;
}

leafptr newl;

void printtree(leafptr p)
{
	static int depth = 0;
	int k;
	
	if (p == NULL) {
	    printf(":");
	    return;
	}
	printf("("); 
	depth++;
	for (k = 0; k < p->n; k++) {
	    printtree(p->branch[k]); 
	    printf("%d", p->key[k]);
	}
	printtree(p->branch[p->n]); 
	printf(")"); depth--;
}

void insertitem(leafptr l, int k)
{
	int i;
	
	for (i = l->n; i > k; i--) {
		l->key[i] = l->key[i - 1];
		l->branch[i + 1] = l->branch[i];
	}
	l->key[k] = key;
	l->branch[k + 1] = newl;
	l->n++;
}

void insertsub(leafptr l) {
	int k;

	if ( l == NULL){
		done = FALSE;
		newl = NULL;
		return;
	}
	k = 0;
	while (k < l->n && l->key[k] < key) {
		printf("num %d : key %d\n", k, l->key[k]);
		k++;
	}
	if (l->key[k] == key) {
		printf("登録済み\n");
		done = TRUE;
		return;
	}
	insertsub(l->branch[k]);
	if (done) {
		return;
	}
	if (l->n < 2 * M) {
		insertitem(l, k);
		done = TRUE;
	} else {
		split(l, k);
		done = FALSE;
	}
}

void insert(void) {
	leafptr l;
	
	insertsub(root);
	if (done) {
	    return;
	}
	printf("newleaf\n");
	l = newleaf();
	l->n = 1;
	l->key[0] = key;
	l->branch[0] = root;
	l->branch[1] = newl;
	root = l;
}

int main()
{
	char s[2];
	for (;;) {
		printf("挿入 In, 検索 Sn, 削除 Dn (n:整数) ? ");
		if (scanf("%1s%d", s, &key) != 2) {
			break;
		}
		switch (s[0]) {
		case 'I':
		case 'i':
			insert();
			break;
		/*case 'S':
		case 's':
			search();
			break;
		case 'D':
		case 'd':
			delete();
			break;*/
		default:
			printf("???\n");
			break;
		}
		printtree(root);
		printf("\n\n");
	}
	return EXIT_SUCCESS;
}

