#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SUCCESS 0

typedef struct node{
    DIR *dirp;
    struct node *link;
}node;

int threads_can_start = 0;
int counter_of_finds = 0;
int threads_waiting;
mtx_t lock_for_beginning;
mtx_t lock_for_queue;
cnd_t cv;
node *start, *end;


int is_queue_empty(){

    return end == NULL || start == NULL;

}



int atomic_insert(DIR *wanted_dirp){

    int rc;
    node *temp;

    rc = mtx_lock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    temp = (node*)malloc(sizeof(node));

    temp->dirp = wanted_dirp;
    temp->link = NULL;

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

//need to check that queue isn't empty before
DIR *atomic_dequeue(){

    int rc;
    DIR *temp;
    node *next;

    rc = mtx_lock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    temp = start->dirp;

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

int the_search_thread(){

    int rc;

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


    //TODO - dont busy wait



}




int main(int argc, char *argv[]){

    if(argc != 4){
        //TODO - print what what wanted in the assignment
    }//end of if

    int rc;
    int threads_num;

    rc = mtx_init(&lock_for_beginning, mtx_plain);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    rc = mtx_init(&lock_for_queue, mtx_plain);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if


    threads_num  = atoi(argv[3]);
    threads_waiting = threads_num;
    thrd_t thread[threads_num];
    start = NULL;
    end = NULL;

    cnd_init(&cv);

    //TODO - check if argv[1] can be searched
    //TODO - insert argv[1] info the queue (remmember argv[1] is string maybe need casting)


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




    //TODO - at the end need destroy locks

}//end of main 


