#include <stdio.h>
#include <stdlib.h>

#include "life2D_helpers.h"


int main(int argc, char **argv) {

    if (argc != 4) {
        fprintf(stderr, "Usage: life2D rows cols states\n");
        return 1;
    }

    // TODO: Implement.
    int rows = strtol(argv[1], NULL, 10);
    int cols = strtol(argv[2], NULL, 10);
    int states = strtol(argv[3], NULL, 10);
    int i = 0;
    int *board = malloc(sizeof(int) * rows * cols);
    while(fscanf(stdin, "%d", &board[i]) != EOF){
	i++;
    }
    for(int i = 0; i < states; i++){
	print_state(board, rows, cols);
	update_state(board, rows, cols);
    }
    free(board);
    return 0;
}

