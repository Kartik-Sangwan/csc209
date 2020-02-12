#include <stdio.h>
// Add your system includes here.
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "ftree.h"
#include <string.h>
#include <stdlib.h>

struct TreeNode *build_tree(const char *fname, char *path){
    struct stat sb;
    struct TreeNode *tn = malloc(sizeof(struct TreeNode));
    if (tn == NULL){
	perror("malloc");
	exit(1);
    }
    tn->fname = malloc(strlen(fname) + 1);
    if (tn->fname == NULL){
	perror("malloc");
	exit(1);
    }
    strcpy(tn->fname, fname);
    strcat(tn->fname, "\0");
    if (lstat(path, &sb) == -1){
	perror("lstat");
	exit(1);
    }
    if (S_ISDIR(sb.st_mode)) tn->type = 'd';
    else if (S_ISREG(sb.st_mode)) tn->type = '-';
    else if (S_ISLNK(sb.st_mode)) tn->type = 'l';
    tn->permissions = sb.st_mode & 0777;
    if ((tn->type == '-' || tn->type == 'l')){
        tn->next = NULL;
    	tn->contents = NULL;
    	return tn;
    } else {
	tn->next = NULL;
	tn->contents = NULL;
	DIR *dr_ptr = opendir(path);
	if (dr_ptr == NULL){
	    perror("opendir");
	    exit(1);
	}
	struct TreeNode *curr, *prev;
	struct dirent *entry_ptr;
	entry_ptr = readdir(dr_ptr);
	while(entry_ptr != NULL){
	    if (entry_ptr->d_name[0] != '.'){
	    	char new_path[strlen(path) + strlen(entry_ptr->d_name) + 2];
	    	strcpy(new_path, path);
	    	strcat(new_path, "/");
      	    	strcat(new_path, entry_ptr->d_name);
	    	strcat(new_path, "\0");
	    	if (tn->contents == NULL){
	            tn->contents = build_tree(entry_ptr->d_name, new_path);
		    curr = tn->contents;
	    	} else {
		    prev = curr;
		    curr = build_tree(entry_ptr->d_name, new_path);
	            prev->next = curr;
	    	}
	    }
	    entry_ptr = readdir(dr_ptr);
	}
	if (closedir(dr_ptr) == -1){
	    perror("closedir");
	    exit(1);
	}
	return tn;
    }

}

/*
 * Returns the FTree rooted at the path fname.
 *
 * Use the following if the file fname doesn't exist and return NULL:
 * fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
 *
 */
struct TreeNode *generate_ftree(const char *fname) {

    // Your implementation here.
    // Hint: consider implementing a recursive helper function that
    // takes fname and a path.  For the initial call on the 
    // helper function, the path would be "", since fname is the root
    // of the FTree.  For files at other depths, the path would be the
    // file path from the root to that file.
    struct stat sb;
    if(lstat(fname, &sb) == -1){
	fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
	return NULL;
    }
    char path[strlen(fname)+1];
    strcpy(path, fname);
    strcat(path, "\0");
    struct TreeNode *tn = build_tree(fname, path);
    return tn;
}

/*
 * Prints the TreeNodes encountered on a preorder traversal of an FTree.
 *
 * The only print statements that you may use in this function are:
 * printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions)
 * printf("%s (%c%o)\n", root->fname, root->type, root->permissions) 
*/
void print_ftree(struct TreeNode *root) {
    // Here's a trick for remembering what depth (in the tree) you're at
    // and printing 2 * that many spaces at the beginning of the line.
    static int depth = 0;
    printf("%*s", depth * 2, "");

    // Your implementation here.
    //SHOULD USE A TEMPORARY VARIABLE HERE RATHER THAT ROOT MODIFICATION.

    if (root->type == 'd'){
	   printf("===== %s (%c%o) =====\n", root->fname, root->type, root->permissions);
	   depth++;
	   struct TreeNode *curr = root->contents;
	   while(curr != NULL){
		print_ftree(curr);
		curr = curr->next;
	   }
	   depth--;
    } else {
    	printf("%s (%c%o)\n", root->fname, root->type, root->permissions);
    }
}

/*
 * Deallocate all dynamically-allocated memory in the FTree rooted at node.
 *
 */
void deallocate_ftree (struct TreeNode *node) {
    // Your implementation here.
    if(node != NULL){
	if (node->type == 'd'){
	    struct TreeNode *curr1 = node->contents;
	    struct TreeNode *curr2 = node->contents;
	    while(curr1 != NULL){
	        curr1 = curr1->next;
	        deallocate_ftree(curr2);
	        curr2 = curr1;
	    }
        }
        free(node->fname);
        free(node);
    }
}




