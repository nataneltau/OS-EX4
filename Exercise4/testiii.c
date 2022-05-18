#include <string.h>
#include <stdio.h>

int main(){

    char path[100];

    char *abc = "Hello World ";

    //memset(path, 0, sizeof(path));

    strncpy(path, abc, 100);

    printf("%s\n", path);


    strcat(path, "hello");

    printf("%s\n", path);


    return 0;
}