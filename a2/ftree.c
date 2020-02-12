#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include "ftree.h"
#include <string.h>
#include <stdlib.h>


/*
 * Returns the FTree rooted at path with name fname.
 * It recursively creates the Tree using the path parameter passed to 
 * to it initially from generate_ftree. Then it initialises the contents of the struct and processes the 
 * contents of the directory/file.
*/

struct TreeNode *build_tree(const char *fname, char *path){

    struct stat sb;

    // Creating space for the current TreeNode.
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
    // Error checking for wrong path.
    if (lstat(path, &sb) == -1){
	perror("lstat");
	exit(1);
    }

    // Inserting values into the TreeNode tn;

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

  	// Opening the directory.
	DIR *dr_ptr = opendir(path);

	// Error checking.
	if (dr_ptr == NULL){
	    perror("opendir");
	    exit(1);
	}

	// Initialising the pointers needed for creating the Tree from TreeNode tn.
        struct TreeNode *curr, *prev;
	struct dirent *entry_ptr;
	entry_ptr = readdir(dr_ptr);

	while(entry_ptr != NULL){
	    if (entry_ptr->d_name[0] != '.'){

		// Creating the new path for the current node in the Tree.
		char new_path[strlen(path) + strlen(entry_ptr->d_name) + 2];
	    	strcpy(new_path, path);
	    	strcat(new_path, "/");
      	    	strcat(new_path, entry_ptr->d_name);
	    	strcat(new_path, "\0");

		if (tn->contents == NULL){

		    // Creating the contents of the current directory using a recursive call.
	            tn->contents = build_tree(entry_ptr->d_name, new_path);
		    curr = tn->contents;

	    	} else {

		    // Creating the linked list for the current directory that is attached to tn->contents.
		    prev = curr;
		    curr = build_tree(entry_ptr->d_name, new_path);
	            prev->next = curr;
	    	}
	    }

	    // Reading the next item in the directory.
	    entry_ptr = readdir(dr_ptr);
	}

	// Closing the directory.
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

    // Error checking for incorrect name of the file/directory to be opened.
    if(lstat(fname, &sb) == -1){
	fprintf(stderr, "The path (%s) does not point to an existing entry!\n", fname);
	return NULL;
    }

    char path[strlen(fname)+1];
    strcpy(path, fname);
    strcat(path, "\0");

    // Calling a helper function to create the Tree rooted at fname.
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
    // Deallocates the memory in the heap for the TreeNodes recursively using two pointers.
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




