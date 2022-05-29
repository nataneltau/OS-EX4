#include <threads.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>


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
int counter_curr_th_waiting = 0;
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

int is_th_queue_empty(){//return true is array_threads_queue is empty

    return start_th_queue == -1;

}//end of function is_th_queue_empty 

int is_queue_empty(){//return true if the queue for dir is empty

    return end == NULL || start == NULL;

}//end of function is_queue_empty

//this func remove thread from array_threads_queue
void delete_th_queue(){//when I am inside I hold lock_for_queue || inside i also hold another lock

    

    if(is_th_queue_empty()){//array_threads_queue don't signal any one
        return;
    }//end of if

    mtx_lock(&lock_for_array_threads);

    if((counter_curr_th_waiting == max_threads_waiting) && is_queue_empty()){//it's over :(
        //do nothing 
    }
    else{
        counter_curr_th_waiting--;
    }

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

    mtx_unlock(&lock_for_array_threads);

}//end of function delete_th_queue

void search_over_wake_everyone(){//this func called when the search is over and it exit all threads

    //int check;

    while(!is_th_queue_empty()){
        delete_th_queue();
    }//end of while

}//end of function search_over_wake_everyone

void insert_th_queue(){//when I am inside I hold lock_for_queue

    mtx_lock(&lock_for_array_threads);

    counter_curr_th_waiting++;

    mtx_unlock(&lock_for_array_threads);
    //int check;
    if(counter_curr_th_waiting == max_threads_waiting ){//queue full

        //check = is_queue_empty;


        if(is_queue_empty()){//the search is over, I am the last that is awake

            mtx_unlock(&lock_for_queue);

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

    if(is_queue_empty() && counter_curr_th_waiting == max_threads_waiting){//check if wake up cause queue has something or cause the search is over

        mtx_unlock(&lock_for_queue);
        //search_over_wake_everyone();//don't think need this cause if we are here than someone already called this func

        thrd_exit(SUCCESS);

    }//end of if
    

}//end of function insert_th_queue

int atomic_insert(const char path[]){//this func got also DIR *wanted_dir

    
    node *temp;

    mtx_lock(&lock_for_queue);

    temp = (node*)malloc(sizeof(node));

    if(temp == NULL){//malloc failed
        mtx_unlock(&lock_for_queue);
        was_error = 1;
        fprintf( stderr, "%s\n", strerror(errno));
        thrd_exit(1);        
    }//end of if

    temp->link = NULL;
    strncpy(temp->path_name, path, PATH_MAX);

    if(is_queue_empty()){//queue is empty
        start = end = temp;
        delete_th_queue();
        mtx_unlock(&lock_for_queue);
        
        return SUCCESS;
    }//end of if

    //if we are here than the queue isn't empty
    end->link = temp;
    end = temp;

    delete_th_queue();
    mtx_unlock(&lock_for_queue);

    return SUCCESS;

}//end of function atomic_insert

node *atomic_dequeue(){

    
    node *temp;
    //node *next;

    //need to check that queue isn't empty, if empty then cnd_wait and increase some counter
    mtx_lock(&lock_for_queue);

    if(!is_th_queue_empty() || is_queue_empty()){//there is threads waiting or the queue of dir is empty
        insert_th_queue();
    }

    temp = (node *)malloc(sizeof(node));
    if(temp == NULL){//malloc failed
        mtx_unlock(&lock_for_queue);
        was_error = 1;
        fprintf( stderr, "%s\n", strerror(errno));
        thrd_exit(1);        
    }//end of if
    //strncpy(temp, start->path_name, PATH_MAX+1);

    temp = start;

    if(start->link == NULL){//last element in queue

        start = NULL;
        end = NULL;
        //free(next);

        mtx_unlock(&lock_for_queue);

        return temp;

    }

    start = start->link;
    temp->link = NULL;
    //free(next);

    mtx_unlock(&lock_for_queue);

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

    
    //char curr_path[PATH_MAX+2];
    node *curr_node;
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char pathi[PATH_MAX+2] = {0};


    mtx_lock(&lock_for_beginning);

    if(!threads_can_start){
        cnd_wait(&let_begin, &lock_for_beginning);
    }//end of if

    mtx_unlock(&lock_for_beginning);

    curr_node = atomic_dequeue();//it seems that this func is infinit recursion, but it should stop here
    //when we searched all the directories

    dir = opendir(curr_node->path_name);
    if(dir == NULL){//error in opendir
        was_error = 1;
        fprintf( stderr, "%s\n", strerror(errno));
        thrd_exit(1);
    }//end of if

    entry = readdir(dir);
    while(entry != NULL){

        strcat(pathi, curr_node->path_name);
        strcat(pathi, "/");
        strcat(pathi, entry->d_name);

        if(stat(pathi, &statbuf) != 0 ){//stat failed, check if symlink as written in the forum
            if(fit_search(entry->d_name) == SUCCESS){//dirent contain the search term
                
                mtx_lock(&lock_for_counter_of_finds);

                counter_of_finds++;

                mtx_unlock(&lock_for_counter_of_finds);

                printf("%s\n", pathi);
                
            }//end of if
        }
        //this if check if entry is directory different than "." or ".."
        else if( S_ISDIR(statbuf.st_mode) && strcmp(entry->d_name, ".")!= 0 && strcmp(entry->d_name, "..")!= 0){/*entry->d_type == DT_DIR &&*/
            /*CHECK ABOUT SYM LINK*/
            if(directory_can_be_search(pathi)){

                if(atomic_insert(pathi) != SUCCESS){
                    was_error = 1;
                    fprintf( stderr, "%s\n", strerror(errno));
                    thrd_exit(1);
                }//end of inner if

            }//end of if
            else{
                printf("Directory %s: Permission denied.\n", pathi);
            }//end of else

        }//end of if
        else{//pathi isn't directory

            if(fit_search(entry->d_name) == SUCCESS){//dirent contain the search term
                
                mtx_lock(&lock_for_counter_of_finds);

                counter_of_finds++;

                mtx_unlock(&lock_for_counter_of_finds);

                printf("%s\n", pathi);
                
            }//end of if

        }//end of else

        memset(pathi, 0, sizeof(pathi));
        entry = readdir(dir);

    }//end of while

    
    closedir(dir);
    //free(curr_path);

    //the_search_thread();

}//end of function the_search_thread

void the_search_th(){

    while(1){
        the_search_thread();
    }

}//end of function the_search_th

int main(int argc, char *argv[]){


    if(argc != 4){
        errno = EINVAL;
        fprintf( stderr, "%s\n", strerror(errno));
        exit(1);
    }//end of if

    
    int threads_num;
    char *rooti = "./";

    if(!directory_can_be_search(argv[1])){//enter if the directory cann't be searched
        strcat(rooti, argv[1]);
        if(!directory_can_be_search(rooti)){
            errno = EINVAL;
            fprintf( stderr, "%s\n", strerror(errno));
            exit(1);
        }
    }
    else{
        rooti = argv[1];
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

    if(atomic_insert(rooti) != SUCCESS){
        fprintf( stderr, "%s\n", strerror(errno));
        exit(1);
    }//end of inner if

    

    for (int t = 0; t < threads_num; t++) {//create n searching threads
        thrd_create(&thread[t], (void *)the_search_th, NULL);
        
    }//end of for

    mtx_lock(&lock_for_beginning);
    
    threads_can_start = 1;

    mtx_unlock(&lock_for_beginning);

    cnd_broadcast(&let_begin);//all threads can start, we signal them to start



    for(int i = 0; i<threads_num; i++){//wait for all threads to finish
        thrd_join(thread[i], NULL);
    }


    mtx_lock(&lock_for_counter_of_finds);

    printf("Done searching, found %d files\n", counter_of_finds);//

    mtx_unlock(&lock_for_counter_of_finds);


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

    if(was_error){//an error occur in at least one threads
        exit(1);
    }

    return 0;

}//end of main 


