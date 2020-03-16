
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

	//Base case that calls closest_serial to get the closest pair.

 	if (n < 4 || pdmax == 0){
		return closest_serial(p, n);
 	} else {

		int mid = n / 2;
		// The left and right half of the array p of n points.
		struct Point *left = p;
		struct Point *right = p + mid;
		struct Point mid_point = p[mid];

		//Initialisation of a pipe for the parent that can hold 2 children atmmost.
		int pipe_fd[2][2];
		for (int i = 0; i < 2; i++){

			// Creating a pipe to the i_th child.
			if (pipe(pipe_fd[i]) == -1){
				perror("Pipe child 1.");
				exit(1);
			}

			//Creating the child process through fork.
			int child = fork();

			if (child < 0){
				perror("Child fork.");
				exit(1);
			}

			else if (child == 0){

				// Closing the reading end of the child process as the child will only write to the parent.
				if (close(pipe_fd[i][0]) == -1){
					perror("Close reading end of child.");
					exit(1);
				}

				// Closing all the reading ends of any previously forked childeren that
				// this child inherited from its parent.
				for (int childno = 0; childno < i; childno++){
					if (close(pipe_fd[childno][0]) == -1){
						perror("Close reading ends of previusly forked children.");
						exit(1);
					}
				}
				double min_dist;
				if (i == 0){

					// Recursive call to find minimum distnace from the left half of the array p.
					min_dist = closest_parallel(left, mid, --pdmax, pcount);
				} else {

					// Recursive call to find minimum distance from the right half of the array p.
					min_dist = closest_parallel(right, n - mid, --pdmax, pcount);
				}

				// Writing the minimum distance that was calculated into the pipe for parent to read.
				if (write(pipe_fd[i][1], &min_dist, sizeof(double)) != sizeof(double)){
					perror("Writing to pipe from child.");
					exit(1);
				}

				if (close(pipe_fd[i][1]) == -1){
					perror("Closing child writing end of pipe i.e. done writing.");
					exit(1);
				}

				// Exiting from the child so that the child doesn't create anymore child processes.
				// Exit code represents number of worker procceses instantiated.
				exit((*pcount));

			} else {
				// Parent Process.
				// Close the writing end of the parent as parent is only interested in reading from child.
				if (close(pipe_fd[i][1]) == -1){
					perror("Close writing end of the parent.");
					exit(1);
				}

			}
		}

		// Only the parent reaches here after collecting the information of distances from the childrens.
		int status;

		// Increase pcount for parent as it has instantiated two new child.
		(*pcount) += 2;
		// Wait for all the child processes to complete.
		for (int i = 0; i < 2; i++){
			if (wait(&status) == -1){
				perror("Parent waiting for children error.");
				exit(1);
			}

			// Sum the exit codes of all the children.
			if (WIFEXITED(status)){
				(*pcount) += WEXITSTATUS(status);
			}
		}

		// Finding the minimum distance between the two left and right half of the array p.

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

		// Creating a array for the points near the center line.
		struct Point *strip = malloc(sizeof(struct Point) * n);
		if (strip == NULL){
			perror("Malloc.");
			exit(1);
		}

		// Populating the with points that are closer than delta.
		int j = 0;
		for (int i = 0; i < n; i++){
			if (abs(p[i].x - mid_point.x) < delta){
				strip[j] = p[i], j++;
			}
		}

		// Finding the min of all the three distances to obtain the least distance
		// between any two points in array p.
		double final_min = min(delta, strip_closest(strip, j ,delta));
		free(strip);
    	return final_min;
    }
}


