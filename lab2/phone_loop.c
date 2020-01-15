#include<stdio.h>
#include<stdlib.h>
int main(){
	
	int a, val;
	int errors = 0;
	char phone[11];
	scanf("%s", phone);
	val = scanf("%d", &a);
	while(val != EOF){
		if (a == -1){
        	        printf("%s\n", phone);
       		 } else if (a >= 0 && a <= 9){
              		  printf("%c\n", phone[a]);
      		 } else {
                	printf("ERROR\n");
			errors += 1;
		 }
		val = scanf("%d", &a);
	}
	if (errors) {
		return 1;
	}else {
		return 0;
	}
}
