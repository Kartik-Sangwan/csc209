#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAXLINE 256

#define SUCCESS "Password verified\n"
#define INVALID "Invalid password\n"
#define NO_USER "No such user\n"

int main(void) {
  char user_id[MAXLINE];
  char password[MAXLINE];

  /* The user will type in a user name on one line followed by a password 
     on the next.
     DO NOT add any prompts.  The only output of this program will be one 
	 of the messages defined above.
   */

  if(fgets(user_id, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  if(fgets(password, MAXLINE, stdin) == NULL) {
      perror("fgets");
      exit(1);
  }
  
  // TODO
  int n, status;
  int fd[2];
  n = fork();

  if (pipe(fd) == -1) {
        perror("pipe");
        exit(1);
  }

  if (n < 0){
    perror("fork");
    exit(1);
  } else if (n == 0){

    if (write(fd[1], user_id, 10) == -1) {
        perror("write to pipe");
    } 
    if (write(fd[1], password, 10) == -1) {
        perror("write to pipe");
    } 

    if (dup2(fd[0], fileno(stdin)) == -1) {
      perror("dup2");
    }

    close(fd[0]);
    close(fd[1]);
    execl("./validate", "validate", NULL);

  } else {

    close(fd[0]);
    close(fd[1]);

    if (wait(&status) == -1){
      perror("wait");
      exit(1);
    }

    int exitval;

    if(WIFEXITED(status)){

      exitval = WEXITSTATUS(status);
      
      if (exitval == 0) printf("%s", SUCCESS);
      else if (exitval == 2) printf("%s", INVALID);
      else if (exitval == 3) printf("%s", NO_USER);
      else{
        perror("error in parent");
        exit(1);
      }
    }

  }

  return 0;
}
