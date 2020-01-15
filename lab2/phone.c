#include<stdlib.h>
#include<stdio.h>
int main(){
	char phone[11];
	int a;
	scanf("%s" ,phone);
	scanf("%d" ,&a);
	if (a == -1){
		printf("%s", phone);
		return 0;
	} else if (a >= 0 && a <= 9){
		printf("%c", phone[a]);
		return 0;
	} else {
		printf("ERROR");
		return 1;
	}
}
