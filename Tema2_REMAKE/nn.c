#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(){
    char ff[]="./files/files1/f.txt";
    char buff[4096];
    char fff[30];
    scanf("%s",fff);
    fff[strlen(fff)]='\0';
    FILE*f=fopen(fff,"r");
    fread(buff,1,4096,f);
    printf("%s\n",buff);
    fclose(f);
    return 0;
}