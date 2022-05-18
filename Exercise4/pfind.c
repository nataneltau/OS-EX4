#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>


#define SUCCESS 0

/* credits:
 * help with the_search_thread() function: https://www.youtube.com/watch?v=j9yL30R6npk
*/

typedef struct node{
    //DIR *data;
    char path_name[PATH_MAX];
    struct node *link;
}node;

char *search_term;
int threads_can_start = 0;
int counter_of_finds = 0;
int max_threads_waiting;
int curr_threads_waiting = 0;
mtx_t lock_for_beginning;
mtx_t lock_for_queue;
mtx_t lock_for_counter_of_finds;
cnd_t cv;
node *start, *end;


int is_queue_empty(){

    return end == NULL || start == NULL;

}

//TODO - need to make cnd_signal inside this func
int atomic_insert(const char *path){//this func got also DIR *wanted_dir

    int rc;
    node *temp;

    rc = mtx_lock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    temp = (node*)malloc(sizeof(node));

    //temp->data = wanted_dir;
    temp->link = NULL;
    strncpy(temp->path_name, path, PATH_MAX);

    if(is_queue_empty()){//queue is empty
        start = end = temp;
        rc = mtx_unlock(&lock_for_queue);
        if(rc != thrd_success){
            //TODO - Print a suitable message
        }//end of if
        return SUCCESS;
    }//end of if

    //if we are here than the queue isn't empty
    end->link = temp;
    end = temp;

    rc = mtx_unlock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    return SUCCESS;

}//end of function atomic_insert


//need to do cnd_wait inside this func, and maintain curr_threads_waiting and  check if the queue will stay empty forever, if it will then back to main thread?
node *atomic_dequeue(){

    int rc;
    node *temp;
    node *next;


    //need to check that queue isn't empty, if empty then cnd_wait and increase some counter
    rc = mtx_lock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    temp = (node*)malloc(sizeof(node));

    //temp->data = start->data;
    strncpy(temp->path_name, start->path_name, PATH_MAX);
    temp->link = NULL;

    next = start;

    if(next->link == NULL){//last element in queue

        free(next);
        start = NULL;
        end = NULL;

        rc = mtx_unlock(&lock_for_queue);
        if(rc != thrd_success){
            //TODO - Print a suitable message
        }//end of if

        return temp;

    }

    start = start->link;
    free(next);

    rc = mtx_unlock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    return temp;

}//end of function atomic_dequeue

int fit_search(const char *name){

    char *result;

    result = strstr(name, search_term);

    if(result != NULL){
        return SUCCESS;
    }

    //if we are here than name doesn't conatin search_term
    return !SUCCESS;

}

int directory_can_be_search(const char *path){
    DIR *dir;
    dir = opendir(current->path_name);
    if(dir == NULL){//error in opendir so directory cann't be searched
        return 0;
    }//end of if

    closedir(dir);

    return 1;//directory can be searched


}//end of function directory_can_be_search

int the_search_thread(){

    int rc;
    node *current;
    DIR *dir;
    struct dirent *entry;
    char pathi[PATH_MAX] = {0};


    rc = mtx_lock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    if(!threads_can_start){
        cnd_wait(&cv, &lock_for_beginning);
    }//end of if

    rc = mtx_unlock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    current = atomic_dequeue();//it seems that this func is infinit recursion, but it should stop here
    //when we searched all the directories

    dir = opendir(current->path_name);
    if(dir == NULL){//error in opendir
        //TODO - Print a suitable message
    }//end of if

    entry = readdir(dir);
    while(entry != NULL){

        strcat(pathi, current->path_name);
        strcat(pathi, "/");
        strcat(pathi, entry->d_name);

        //this if check if entry is directory different than "." or ".."
        if(entry->d_type == DT_DIR && strcmp(entry->d_name, ".")!= 0 && strcmp(entry->d_name, "..")!= 0){

            if(directory_can_be_search(pathi)){

                if(atomic_insert(pathi) != SUCCESS){
                    //TODO - Print a suitable message
                }//end of inner if

            }//end of if
            else{
                printf("Directory %s: Permission denied.\n", pathi);
            }//end of else

        }//end of if
        else{

            if(fit_search(entry->d_name) == SUCCESS){//dirent contain the search term
                
                rc = mtx_lock(&lock_for_counter_of_finds);
                if(rc != thrd_success){
                    //TODO - Print a suitable message
                }//end of if

                counter_of_finds++;

                rc = mtx_unlock(&lock_for_counter_of_finds);
                if(rc != thrd_success){
                    //TODO - Print a suitable message
                }//end of if

                printf("%s\n", pathi);
                
            }//end of if

        }//end of else

        memset(pathi, 0, sizeof(pathi));
    }//end of while

    


    //TODO - dont busy wait

    closedir(dir);

    the_search_thread();

}//end of function the_search_thread

int main(int argc, char *argv[]){

    if(argc != 4){
        //TODO - print what what wanted in the assignment
    }//end of if

    int rc;
    int threads_num;

    if(!directory_can_be_search(argv[1])){//enter if the directory cann't be searched
        //TODO - print what what wanted in the assignment
    }

    mtx_init(&lock_for_beginning, mtx_plain);
    mtx_init(&lock_for_queue, mtx_plain);
    mtx_init(&lock_for_counter_of_finds, mtx_plain);


    search_term = argv[2];
    threads_num  = atoi(argv[3]);
    max_threads_waiting = threads_num;
    thrd_t thread[threads_num];
    start = NULL;
    end = NULL;

    cnd_init(&cv);
    if(atomic_insert(argv[1]) != SUCCESS){
        //TODO - Print a suitable message
    }//end of inner if

    

    for (int t = 0; t < threads_num; ++t) {//create n searching threads
        rc = thrd_create(&thread[t], the_search_thread, NULL);
        if (rc != thrd_success) {
          //TODO - Print a suitable message
        }//end of if
    }//end of for

    rc = mtx_lock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if
    
    threads_can_start = 1;

    rc = mtx_unlock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    cnd_broadcast(&cv);//all threads can start, we signal them to start




    //TODO - at the end need destroy locks and maybe also all cnd_t

}//end of main 


