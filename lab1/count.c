#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 2048

void search(char *file_name, char *str, FILE *in, FILE *out) {
    int matches = 0;
    char buffer[BUFF_SIZE];
    int test=0;
    // today is jan 21st 
    //go through each lines to find matches
    while(fgets(buffer, BUFF_SIZE, in) != NULL) {
        const char *temp = buffer;
        //update pointer after a match found
        while(temp = strstr(temp, str)) {
            //fprintf(out, "\nA match found on line -> %d: ", line_num);
            //fprintf(out, "%s", temp);
            matches++;
            temp++;
        }
    }
    printf("Number of matches = %d", matches);
    fprintf(out, "Number of matches = %d", matches);
}

void get_file_size(FILE *in, FILE *out) {
    fseek(in, 0, SEEK_END);
    int size = ftell(in);
    printf("Size of file is %d\n", size);
    fprintf(out, "Size of file is %d\n", size);
    fseek(in, 0, SEEK_SET);
}

int main(int argc, char *argv[]) {
    //write(1,"\E[H\E[2J",7);
    if(argc != 4) {
        printf("Usage: %s <input-filename> <search-string> <output-filename>\n", argv[0]);
    }
    FILE *in;
    FILE *out;
    if((in = fopen(argv[1], "r")) == NULL || (out = fopen(argv[3], "w+")) == NULL) {
        perror("Error");
        exit(1);
    }
    get_file_size(in, out);
    search(argv[1], argv[2], in, out);
    if(in) {
        fclose(in);
    }
    if(out) {
        fclose(out);
    }
    return 0;
}
