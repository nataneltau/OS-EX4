#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


//when i stopped: need to check in insert_th_queue and continue to build th_queue implemantion 
#define SUCCESS 0

/* credits:
 * help with the_search_thread() function: https://www.youtube.com/watch?v=j9yL30R6npk
 * help with using stat() function: https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
*/

typedef struct node{
    //DIR *data;
    char path_name[PATH_MAX+1];
    struct node *link;
}node;

char *search_term;
int was_error = 0;
int threads_can_start = 0;
int counter_of_finds = 0;
int max_threads_waiting;
mtx_t lock_for_beginning;
mtx_t lock_for_queue;
mtx_t lock_for_array_threads;
mtx_t lock_for_counter_of_finds;
mtx_t lock_for_error;
cnd_t let_begin;
cnd_t *array_threads_queue;//the size of the array should be number_of_threads+1
node *start, *end;
int start_th_queue, end_th_queue;

int is_th_queue_empty(){

    return start_th_queue == -1;

}//end of function is_th_queue_empty

int is_queue_empty(){

    return end == NULL || start == NULL;

}//end of function is_queue_empty

void delete_th_queue(){//when I am inside I hold lock_for_queue || inside i also hold another lock

    int rc;

    if(is_th_queue_empty()){
        return;
    }//end of if

    rc = mtx_lock(&lock_for_array_threads);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    cnd_signal(&array_threads_queue[start_th_queue]);

    if(start_th_queue == end_th_queue){//queue has only one element
        start_th_queue = -1;
        end_th_queue = -1;
    }//end of if
    else{
        if(start_th_queue == max_threads_waiting){
            start_th_queue = 0;
        }//end of if
        else{
            start_th_queue++;
        }//end of else

    }//end of else

    rc = mtx_unlock(&lock_for_array_threads);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

}//end of function delete_th_queue

void search_over_wake_everyone(){

    //int check;

    while(!is_th_queue_empty()){
        delete_th_queue();
    }//end of while

}//end of function search_over_wake_everyone

void insert_th_queue(){//when I am inside I hold lock_for_queue

    int rc;
    //int check;

    if( (start_th_queue == 0 && end_th_queue == max_threads_waiting-1) || (start_th_queue == end_th_queue +2)){//queue will be full after this inseration

        //check = is_queue_empty;

        if(is_queue_empty()){//the search is over, I am the last that is awake

            rc = mtx_unlock(&lock_for_queue);
            if(rc != thrd_success){
                //TODO - Print a suitable message
            }//end of if


           search_over_wake_everyone();

           thrd_exit(SUCCESS);

        }//end of if


    }//end of if

    //if we are here and we entered the last if then the theards that should wake didn't, wait they will wake

    if(is_th_queue_empty()){
        start_th_queue = 0;
        end_th_queue = 0;
    }//end of if
    else{//th_queue isn't empty
        if(end_th_queue == max_threads_waiting){
            end_th_queue = 0;
        }
        else end_th_queue++;
    }//end of else


    cnd_wait(&array_threads_queue[end_th_queue], &lock_for_queue);

    //check = is_queue_empty;

    if(is_queue_empty()){//check if wake up cause queue has something or cause the search is over

        rc = mtx_unlock(&lock_for_queue);
        if(rc != thrd_success){
            //TODO - Print a suitable message
        }//end of if


        //search_over_wake_everyone();//don't think need this cause if we are here than someone already called this func

        thrd_exit(SUCCESS);

    }//end of if
    

}//end of function insert_th_queue

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
        delete_th_queue();
        rc = mtx_unlock(&lock_for_queue);
        if(rc != thrd_success){
            //TODO - Print a suitable message
        }//end of if
        return SUCCESS;
    }//end of if

    //if we are here than the queue isn't empty
    end->link = temp;
    end = temp;

    delete_th_queue();
    rc = mtx_unlock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    return SUCCESS;

}//end of function atomic_insert

char *atomic_dequeue(){

    int rc;
    char *temp;
    node *next;


    //need to check that queue isn't empty, if empty then cnd_wait and increase some counter
    rc = mtx_lock(&lock_for_queue);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }

    if(!is_th_queue_empty() || is_queue_empty()){//there is threads waiting or the queue of dir is empty
        insert_th_queue();
    }

    temp = (char *)malloc(sizeof(char) * (PATH_MAX+1));
    //temp->data = start->data;
    strncpy(temp, start->path_name, PATH_MAX+1);

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
    dir = opendir(path);
    if(dir == NULL){//error in opendir so directory cann't be searched
        return 0;
    }//end of if

    closedir(dir);

    return 1;//directory can be searched


}//end of function directory_can_be_search

void the_search_thread(){

    int rc;
    char *curr_path;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char pathi[PATH_MAX] = {0};


    rc = mtx_lock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    if(!threads_can_start){
        cnd_wait(&let_begin, &lock_for_beginning);
    }//end of if

    rc = mtx_unlock(&lock_for_beginning);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    curr_path = atomic_dequeue();//it seems that this func is infinit recursion, but it should stop here
    //when we searched all the directories

    dir = opendir(curr_path);
    if(dir == NULL){//error in opendir
        //TODO - Print a suitable message
    }//end of if

    entry = readdir(dir);
    while(entry != NULL){

        strcat(pathi, curr_path);
        strcat(pathi, "/");
        strcat(pathi, entry->d_name);

        if(stat(pathi, &statbuf) != 0 ){//stat failed, check if symlink as written in the forum
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
        }
        //this if check if entry is directory different than "." or ".."
        else if( S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".")!= 0 && strcmp(entry->d_name, "..")!= 0){/*entry->d_type == DT_DIR &&*/
            /*CHECK ABOUT SYM LINK*/
            if(directory_can_be_search(pathi)){

                if(atomic_insert(pathi) != SUCCESS){
                    //TODO - Print a suitable message
                }//end of inner if

            }//end of if
            else{
                printf("Directory %s: Permission denied.\n", pathi);
            }//end of else

        }//end of if
        else{//pathi isn't directory

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
    free(curr_path);

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
    mtx_init(&lock_for_error, mtx_plain);
    mtx_init(&lock_for_array_threads, mtx_plain);


    search_term = argv[2];
    threads_num  = atoi(argv[3]);
    max_threads_waiting = threads_num;
    thrd_t thread[threads_num];
    start = NULL;
    end = NULL;
    start_th_queue = -1;
    end_th_queue = -1;

    cnd_init(&let_begin);

    array_threads_queue = (cnd_t *)malloc(sizeof(cnd_t) * (threads_num+1));


    for(int j=0; j<threads_num+1; j++){
        cnd_init(&array_threads_queue[j]);
    }

    if(atomic_insert(argv[1]) != SUCCESS){
        //TODO - Print a suitable message
    }//end of inner if

    

    for (int t = 0; t < threads_num; t++) {//create n searching threads
        rc = thrd_create(&thread[t], (void *)the_search_thread, NULL);
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

    cnd_broadcast(&let_begin);//all threads can start, we signal them to start



    for(int i = 0; i<threads_num; i++){//wait for all threads to finish
        thrd_join(thread[i], NULL);
    }


    rc = mtx_lock(&lock_for_counter_of_finds);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if

    printf("Done searching, found %d files\n", counter_of_finds);//

    rc = mtx_unlock(&lock_for_counter_of_finds);
    if(rc != thrd_success){
        //TODO - Print a suitable message
    }//end of if


    mtx_destroy(&lock_for_beginning);
    mtx_destroy(&lock_for_queue);
    mtx_destroy(&lock_for_counter_of_finds);
    mtx_destroy(&lock_for_error);
    mtx_destroy(&lock_for_array_threads);
    cnd_destroy(&let_begin);

    for(int j=0; j<threads_num+1; j++){
        cnd_destroy(&array_threads_queue[j]);
    }
    free(array_threads_queue);
    //TODO - at the end need destroy locks and maybe also all cnd_t

    if(was_error){//an error occur in at least one threads
        //TODO - WHAT NEEDED AND EXIT PROPERLY
    }

    return 0;

}//end of main 


