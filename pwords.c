#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include<omp.h>
#define Num_Of_Threads 20
typedef struct dict {
  char *word;
  int count;
  struct dict *next;
} dict_t;

char *
make_word( char *word ) {
  return strcpy( malloc( strlen( word )+1 ), word );
}

dict_t *
make_dict(char *word) {
  dict_t *nd = (dict_t *) malloc( sizeof(dict_t) );
  nd->word = make_word( word );
  nd->count = 1;
  nd->next = NULL;
  return nd;
}

dict_t *
insert_word( dict_t *d, char *word ) {
  
  //   Insert word into dict or increment count if already there
  //   return pointer to the updated dict
  
  dict_t *nd;
  dict_t *pd = NULL;		// prior to insertion point 
  dict_t *di = d;		// following insertion point
  // Search down list to find if present or point of insertion
  #pragma omp parallel num_threads(Num_Of_Threads){
	  while(di && ( strcmp(word, di->word ) >= 0) ) { 
			if( strcmp( word, di->word ) == 0 ) { 
				di->count++;		// increment count 
				return d;			// return head 
			}
			pd = di;			// advance ptr pair
			di = di->next;
		}
		nd = make_dict(word);		// not found, make entry 
		nd->next = di;		// entry bigger than word or tail 
		if (pd) {
			pd->next = nd;
			return d;			// insert beond head 
		}
  
  }
  
  return nd;
}

void print_dict(dict_t *d) {
  #pragma omp parallel num_threads(Num_Of_Threads){
	while (d) {
		printf("[%d] %s\n", d->count, d->word);
		#pragma omp critical
			d = d->next;
	}
  }
}

int
get_word( char *buf, int n, FILE *infile) {
  int inword = 0;
  int c; 
  #pragma omp parallel num_threads(Num_Of_Threads) lastprivate(inword){
		while( (c = fgetc(infile)) != EOF ) {
			if (inword && !isalpha(c)) {
				
				buf[inword] = '\0';	// terminate the word string
				return 1;
			} 
			if (isalpha(c)) {
				#pragma omp atomic
					buf[inword++] = c;
			}
		}
  }
  return 0;			// no more words
}

#define MAXWORD 1024
dict_t *
words( FILE *infile ) {
  dict_t *wd = NULL;
  char wordbuf[MAXWORD];
  // we parallelised how we red data from file
  
  #pragma omp parallel num_threads(Num_Of_Threads)
  {
	  int i = omp_get_thread_num();
	  
	  while( get_word( wordbuf, MAXWORD, infile ) ) {
		  #pragma omp barrier
			  wd = insert_word(wd, wordbuf); // add to dict
	  }
  }
  
  return wd;
}

int main( int argc, char *argv[] ) {
  dict_t *d = NULL;
  FILE *infile = stdin;
  if (argc >= 2) {
    infile = fopen (argv[1],"r");
  }
  if( !infile ) {
    printf("Unable to open %s\n",argv[1]);
    exit( EXIT_FAILURE );
  }
  d = words( infile );
  print_dict( d );
  fclose( infile );
}

