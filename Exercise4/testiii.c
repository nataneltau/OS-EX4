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

    /*FILE *fptr_in, *fptr_out;
    char str[100];
    fptr_in = fopen("stam.txt", "r");
    fptr_out = fopen("outi.txt", "w");

    while(fscanf(fptr_in, "%s", str) == 1){
        fprintf(fptr_out, "%s ", str);
    }

    fclose(fptr_in);
    fclose(fptr_out);*/

    return 0;
}