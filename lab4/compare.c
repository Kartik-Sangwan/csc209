#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
  Write the main() function of a program that takes exactly two arguments,
  and prints one of the following to standard output:
    - "Same\n" if the arguments are the same.
    - "Different\n" if the arguments are different.
    - "Invalid\n" if the program is called with an incorrect number of
      arguments.
	  NOTE: Cut and paste these strings into your printf statements to 
	  ensure that they will be exactly the strings above.

  Your main function should return 0, regardless of what is printed.
*/
int main(int argc, char **argv){
	if (argc == 3){
		char *str1 = argv[1];
		char *str2 = argv[2];
		int equal = 1;
		if (strlen(str1) == strlen(str2)){
			for(int i = 0; i < strlen(str1) && equal = 1; i++){
				if (str1[i] != str2[i]) equal = 0;
			}
			if (equal) printf("Same\n");
			else printf("Different\n");
		} else {
			printf("Different\n");
		}
	} else {
		printf("Invalid\n");
	}
	return 0;
}
