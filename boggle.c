#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "stdint.h" /* Replace with <stdint.h> if appropriate */

#define MAX_LINE 100
#define ALPHA_SIZE 26
#define MAX_BOARD_SIZE 10

char *dictfile = "/usr/share/dict/words";

typedef struct trie_node *TrieNodePtr;

typedef struct trie_node {
  int terminal;
  TrieNodePtr children[ALPHA_SIZE];
} TrieNode;

typedef struct board *BoardPtr;

typedef struct board {
  int size;
  char data[MAX_BOARD_SIZE][MAX_BOARD_SIZE];
} Board;

struct search_result {
  int found;
  int valid_prefix;
};

/* remove newlines */
void chomp(char *s) {
    while(*s && *s != '\n' && *s != '\r') s++;
    *s = 0;
}

int alpha_position(char c) {
  return c - 'a';
}

void initialize_trie_node(TrieNodePtr t) {
  int i;
  t->terminal = 0;
  for (i=0;i<ALPHA_SIZE;i++) t->children[i] = NULL;  
}

TrieNodePtr talloc(void) {
  return (TrieNodePtr) malloc(sizeof(TrieNode));
}

void insert_into_trie(TrieNodePtr root, char *string) {
  int position;
  int i;
  TrieNodePtr child;
  for (i=0; i<strlen(string); i++) {
    position = alpha_position(string[i]);
    child = root->children[position];
    if (child == NULL) {
      child = talloc();
      initialize_trie_node(child);
      root->children[position] = child;
    } 
    root = child;
    if (i == strlen(string) - 1) {
      root->terminal = 1;
    }
  }
}

void display_trie_children(TrieNodePtr t) {
  int i;
  for (i=0; i<ALPHA_SIZE;i++) {
    if (t->children[i] != NULL) {
      printf("%c ",i + 'a');
    }
  }
  printf("\n");
}

int has_children(TrieNodePtr t) {
  int i;
  for (i=0; i<ALPHA_SIZE; i++) {
    if (t->children[i] != NULL) {
      return 1;
    }
  }
  return 0;
}

struct search_result search_trie(TrieNodePtr root, char *string) {
  int position, i;
  struct search_result result;

  for (i=0; i<strlen(string); i++) {
    position = alpha_position(string[i]);
    result.valid_prefix = root->children[position] != NULL;
    if (root->children[position] == NULL) {
      result.found = 0;
      return result;
    } else {
      root = root->children[position];
    }
  }
  result.found = root->terminal;
  result.valid_prefix = has_children(root);
  return result;
}



/**
 * removes newlines, lowercases
 * returns false if any non-alpha characters
 * are found in the string
 * this is to remove words with apostrophes, dashes,
 * accents, etc.
 */

int normalizeWord(char *s) {
  int c = 0;
  chomp(s);
  if (strlen(s) < 3) return 0;
  while(*s) {
    c = tolower(*s);
    if (!(c >= 'a' && (c <= 'z'))) return 0;
    *s++ = c;
  }
  return 1;
}

TrieNodePtr readWordList() {
  FILE *fp;
  char line[MAX_LINE];
  int lcount;
  int legal = 0;
  TrieNodePtr root = talloc();
  initialize_trie_node(root);

  fp = fopen(dictfile,"r");
  if (fp == NULL) {
      puts("cannot open file");
      return NULL;
    }
  while( fgets(line, sizeof(line), fp) != NULL ) {
    lcount++;
    legal = normalizeWord(line);
    if (legal) {
      insert_into_trie(root,line);
    } 
  }
 
  fclose(fp);  /* Close the file */
  return root;
}


BoardPtr readBoard(char *filename) {
  BoardPtr board = (BoardPtr)malloc(sizeof(Board));

  FILE *fp;
  int row,col;
  char line[MAX_LINE];
  char c;

  fp = fopen(filename,"r");
  if (fp == NULL) {
    puts("cannot read board");
    return NULL;
  }
  row = 0;
  while( fgets(line, sizeof(line), fp) != NULL ) {
    for(col=0; col<strlen(line); col++) {
      c = line[col];
      if (c >= 'a' && c <= 'z') {
	board->data[row][col] = c;
      }
    }
    row++;
  }  
  fclose(fp);
  board->size = row;
  return board;
}

void check(BoardPtr board, TrieNodePtr trie, int row, int col, int seen[], char *word) {
  struct search_result result;
  int i,j,nrow,ncol;
  char newword[board->size * board->size];
  int nseen[board->size * board->size];
  result = search_trie(trie,word);
  if (result.found) {
    printf("%s\n",word); /* found one! */
  }
  if (!result.valid_prefix) {
    return;  /* this is a dead end */
  }

  /* copy seen array */
  for (i=0;i<board->size * board->size; i++) nseen[i] = seen[i];

  /* ok. next letter. */
  /* neighbors (-1,-1),(-1,0),(-1,1), etc. */
  for (i=-1;i<2;i++) {
    for (j=-1; j<2; j++) {
      nrow = row + i;
      ncol = col + j;
      if (nrow >= 0 && 
	  nrow < board->size && 
	  ncol >= 0 && 
	  ncol < board->size &&
	  seen[nrow * board->size + ncol] == 0) {

	/* append letter, mark position in seen, and check */
	sprintf(newword,"%s%c",word,board->data[nrow][ncol]);
	seen[nrow * board->size + ncol] = 1;
	check(board,trie,nrow,ncol,nseen,newword);
      }
    }
  }

}

/* really just a driver function for check() to kick it off.
   gets called at each position on the board */
void checkPaths(BoardPtr board, TrieNodePtr trie, int row, int col) {
  char word[board->size * board->size];
  int seen[board->size * board->size];
  int i;
  for (i=0;i<board->size * board->size; i++) seen[i] = 0;
  seen[(row * board->size) + col] = 1;
  sprintf(word,"%c",board->data[row][col]);
  check(board,trie,row,col,seen,word);
}

void walkBoard(BoardPtr board, TrieNodePtr trie) {
  int row,col;
  for (row=0; row < board->size; row++) {
    for (col=0; col < board->size; col++) {
      checkPaths(board,trie,row,col);
    }
  }
}

int main(int argc, char **argv) {
  TrieNodePtr root = readWordList();
  BoardPtr board = readBoard(argv[1]);

  if (root == NULL) {
    return 1;
  } 
  walkBoard(board,root); 
  return 0;
}
