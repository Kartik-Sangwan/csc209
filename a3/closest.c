#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

#include "point.h"
#include "utilities_closest.h"
#include "serial_closest.h"
#include "parallel_closest.h"


//print_usage is called when there is an error in the command line arguements.

void print_usage() {
    fprintf(stderr, "Usage: closest -f filename -d pdepth\n\n");
    fprintf(stderr, "    -d Maximum process tree depth\n");
    fprintf(stderr, "    -f File that contains the input points\n");

    exit(1);
}

int main(int argc, char **argv) {
    int n = -1;
    long pdepth = -1;
    char *filename = NULL;

	//pcount is the number of worker processes that will be instantiated.
    int pcount = 0;
	int ch = -1;

    // Parsing the command line arguments with flags using getopt.
	if (argc != 5){
		print_usage();
	}
	// Parsing the flags and storing the parameters.
	while ((ch = getopt(argc, argv, "f:d:")) != -1){

		switch(ch) {

			case('f'):
				if (optarg != NULL)
					filename = optarg;
				break;
			case('d'):
				if (optarg != NULL && optarg >= 0)
				    pdepth = atoi(optarg);
				break;
			default:
				print_usage();
		}

	}

    // Read the points
    n = total_points(filename);
    struct Point points_arr[n];
    read_points(filename, points_arr);

    // Sort the points
    qsort(points_arr, n, sizeof(struct Point), compare_x);

    // Calculate the result using the parallel algorithm.
    double result_p = closest_parallel(points_arr, n, pdepth, &pcount);
    printf("The smallest distance: is %.2f (total worker processes: %d)\n", result_p, pcount);
	double brute_result = brute_force(points_arr, n);
	printf("The smallest distance: is %.2f \n", brute_result);

    exit(0);
}
