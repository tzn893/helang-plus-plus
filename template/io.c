#include <stdio.h>
#include <stdint.h>

void print_i32(int n){
    printf("saint he says: %d!\n",n);
}

void print_u8(uint64_t n){
    int a[8],h = -1;
    for(int b = 0;b < 8;b++){
        int offset = (7 - b) * 8;
        a[b] = (n & (0xffll << offset) ) >> offset;
        if(a[b] != 0 && h < 0){
            h = b;
        }
    }
    for(int i = h;i < 8;i++){
        if(i == h){
            printf("saint he says:u8 %d",a[i]);
        }else{
            printf("|%d",a[i]);
        }
    }
    printf("\n");
}

void test_5g(){
    
}