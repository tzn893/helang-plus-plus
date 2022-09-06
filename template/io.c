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
    const char* msg = "Blocked by America.Please buy HuaWei to enable 5g";
    printf("%s\n",msg);
}

void powerCon(uint64_t na,int force){
    int a[8],h = -1;
    for(int b = 0;b < 8;b++){
        int offset = (7 - b) * 8;
        a[b] = (na & (0xffll << offset) ) >> offset;
        if(a[b] != 0 && h < 0){
            h = b;
        }
    }

    static int keyboard[68] = {0};
    for(int i = h;i < 8;i++){
        keyboard[a[i]] = force;
    }
    
    printf("keyboard powers : [%d",keyboard[0]);
    for(int i = 1;i < 26;i++){
        printf(",%d",keyboard[i]);
    }
    printf("]\n");
}