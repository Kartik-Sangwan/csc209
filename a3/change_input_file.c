# include <stdio.h>
# include <stdlib.h>

int main(int argc, char *argv[]){

	FILE *fp;
	int n1 = 29, n2 = 29, n3 = 30, n4 = 30;
	fp = fopen("copy_test", "w");
	fwrite(&n1, 1, sizeof(int), fp);
	fwrite(&n2, 1, sizeof(int), fp);
	fseek(fp, 160, SEEK_SET);
	fwrite(&n3, 1, sizeof(int), fp);
	fwrite(&n4, 1, sizeof(int), fp);
}
