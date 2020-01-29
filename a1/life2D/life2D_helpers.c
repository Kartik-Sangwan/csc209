#include <stdio.h>
#include <stdlib.h>


void print_state(int *board, int num_rows, int num_cols) {
    for (int i = 0; i < num_rows * num_cols; i++) {
        printf("%d", board[i]);
        if (((i + 1) % num_cols) == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

int count_neighbours(int i, int cols, int *arr){
	int count = 0;
	if(arr[i - cols - 1] == 1) count++;
	if(arr[i - cols] == 1) count++;
	if(arr[i - cols + 1] == 1) count++;
	if(arr[i - 1] == 1) count++;
	if(arr[i + 1] == 1) count++;
	if(arr[i + cols - 1] == 1) count++;
	if(arr[i + cols] == 1) count++;
	if(arr[i + cols + 1] == 1) count++;
	return count;
}
void update_state(int *board, int num_rows, int num_cols) {
    // TODO: Implement.
    int arr[num_cols * num_rows];
    for(int i = 0; i < num_cols*num_rows; i++){
	arr[i] = board[i];
        if (!(i>=0 && i<=num_cols-1) && !((i % num_cols) == 0) && !(((i+1) % num_cols) == 0) && !(i>=num_rows*num_cols-num_cols && i<=num_rows*num_cols-1)){
		int val = board[i];
		int count = count_neighbours(i, num_cols, board);
		if (val == 1 && (count < 2 || count > 3)) arr[i] = 0;
		else if (val == 0 && (count == 2 || count == 3)) arr[i] = 1;
	}
    }
    
    for(int i = 0; i < num_cols * num_rows; i++) board[i] = arr[i];
    return;
}
