#include <stdio.h>
extern int __he_entry_main();

int main(){
    int num = __he_entry_main();
    printf("so cool!helang exits and returns %d",num);
}