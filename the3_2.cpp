#include <iostream>
#include "ext21.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


/*
ptr = mmap(NULL, length, PROT_READ|PROT_WRITE, 0, fd, offset);

    PROT_READ --> Pages can be read
    PROT_WRITE --> Pages can be written to

    The argument set to "0" in the example call sets various flags, 0 means no flags are set. 

    will return a <length> bytes pointer to <offset> byte of the file opened as <fd>.

*/


#define BLOCK_SIZE 1024


void print_bytes_bitwise(int fd, int num_bytes){
    unsigned char byte; 

    while(num_bytes){
        read(fd, &byte, 1); 
        for(int i = 0; i < 8; i++){
            std::cout <<  (byte % 2); 
            byte = byte/2; 
        }
        std::cout << std::endl;
        num_bytes--;
    }

}



void read_db1(){
    int fd = open("image.img", O_RDWR);
    struct ext2_super_block sp;
    struct ext2_group_desc bd; 
    int the_next_four_bytes;

    lseek(fd, 2 * BLOCK_SIZE, SEEK_CUR);
    read(fd, &bd, sizeof(bd));


    std::cout << "The block bitmap address is --> "<< bd.bg_block_bitmap << std::endl;

    lseek(fd, 1*BLOCK_SIZE, SEEK_CUR); 

    print_bytes_bitwise(fd, 10);

}


int main(){


    read_db1();

    return 0; 
}