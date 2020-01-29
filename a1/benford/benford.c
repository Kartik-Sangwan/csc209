#include <stdio.h>
#include <stdlib.h>

#include "benford_helpers.h"

/*
 * The only print statement that you may use in your main function is the following:
 * - printf("%ds: %d\n")
 *
 */
int main(int argc, char **argv) {

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "benford position [datafile]\n");
        return 1;
    }

    // TODO: Implement.
    if (argc == 2){ 
	int *arr = malloc(sizeof(int)*BASE); 
	int num = 0;
	for(int j = 0; j < BASE; j++){ 
		arr[j] = 0; 
	} 
	int i = strtol(argv[1], NULL, 10);
   	while(scanf("%d", &num) == 1){
		add_to_tally(num, i, arr);
	}
	for(int i = 0; i < BASE; i++){
		printf("%ds: %d\n",i ,arr[i]);
	}
	free(arr);
    }  else {
	int num = 0;
	int *arr = malloc(sizeof(int) * BASE);
	for(int j = 0; j < BASE; j++){
		arr[j] = 0;	
	}
	int i = strtol(argv[1], NULL, 10);
	FILE *file = fopen(argv[2], "r");
	while(fscanf(file, "%d", &num) == 1){
		add_to_tally(num, i, arr);
	}
	for(int i = 0; i < BASE; i++){
		printf("%ds: %d\n", i, arr[i]);
	}
	free(arr);
    }
    return 0;
}
