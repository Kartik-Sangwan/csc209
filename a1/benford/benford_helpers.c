#include <stdio.h>
#include "benford_helpers.h"

int count_digits(int num) {
    // TODO: Implement.
    int count = 0;
    while(num != 0){
	count++;
	num = (num/BASE);
    }
    return count;
}

int get_ith_from_right(int num, int i) {
    // TODO: Implement.
    int count = -1, rem = 0;
    while(count != i){
	rem = num%BASE;
	num = num/BASE;
	count++;
    }
    return rem;
}

int get_ith_from_left(int num, int i) {
    // TODO: Implement.
    return get_ith_from_right(num, count_digits(num) - i - 1);
}

void add_to_tally(int num, int i, int *tally) {
    // TODO: Implement.
    int digit = get_ith_from_left(num, i);
    tally[digit] += 1; 
    return;
}
