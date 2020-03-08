
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "point.h"
#include "serial_closest.h"
#include "utilities_closest.h"


/*
 * Multi-process (parallel) implementation of the recursive divide-and-conquer
 * algorithm to find the minimal distance between any two pair of points in p[].
 * Assumes that the array p[] is sorted according to x coordinate.
 */
double closest_parallel(struct Point *p, int n, int pdmax, int *pcount) {

 	if (n < 4 || pdmax == 0){
		return closest_serial(p, n);
 	} else {

		int mid = n / 2;
		struct Point *left = p;
		struct Point *right = p + mid;
		struct Point mid_point = p[mid];
		int pipe_fd[2][2];
		for (int i = 0; i < 2; i++){
			if (pipe(pipe_fd[i]) == -1){
				perror("Pipe child 1.");
				exit(1);
			}
			int child = fork();
			if (child < 0){
				perror("Child fork.");
				exit(1);
			}
			else if (child == 0){
				if (close(pipe_fd[i][0]) == -1){
					perror("Close reading end of child.");
					exit(1);
				}
				for (int childno = 0; childno < i; childno++){
					if (close(pipe_fd[childno][0]) == -1){
						perror("Close reading ends of previusly forked childrens.");
						exit(1);
					}
				}
				double min_dist;
				if (i == 0){
					min_dist = closest_parallel(left, mid, --pdmax, pcount);
				} else {
					min_dist = closest_parallel(right, n - mid, --pdmax, pcount);
				}
				if (write(pipe_fd[i][1], &min_dist, sizeof(double)) != sizeof(double)){
					perror("Writing to pipe from child.");
					exit(1);
				}
				if (close(pipe_fd[i][1]) == -1){
					perror("Closing child writing end of pipe i.e. done writing.");
					exit(1);
				}

				//REMOVE THIS PRINTF STATEMENT WHEN DONE..
				//printf("%d     %ld      \n", (*pcount), (long) getpid());
				exit(++(*pcount));

			} else {

				if (close(pipe_fd[i][1]) == -1){
					perror("Close writing end of the parent.");
					exit(1);
				}

			}
		}
		//Only the parent reaches here after collecting the information of distances from the childrens.
		int status;

		for (int i = 0; i < 2; i++){
			if (wait(&status) == -1){
				perror("Parent waiting for children error.");
				exit(1);
			}

			if (WIFEXITED(status)){
				(*pcount) += WEXITSTATUS(status);
			}
		}

		double min_dist = 2147483647.0;
		for (int i = 0; i < 2; i++){
			double temp_min;
			if (read(pipe_fd[i][0], &temp_min, sizeof(double)) != sizeof(double)){
				perror("Error reading from child.");
				exit(1);
			}
			if (temp_min < min_dist) min_dist = temp_min;
		}

		double delta = min_dist;

		struct Point *strip = malloc(sizeof(struct Point) * n);
		if (strip == NULL){
			perror("Malloc.");
			exit(1);
		}

		int j = 0;
		for (int i = 0; i < n; i++){
			if (abs(p[i].x - mid_point.x) < delta){
				strip[j] = p[i], j++;
			}
		}

		double final_min = min(delta, strip_closest(strip, j ,delta));
		free(strip);
    	return final_min;
    }
}


