#include <iostream>
#include "ext21.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <math.h>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
using namespace std;



#define BASE_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define IMAGE "image.img"
#define EXT2_NAME_LEN 255

typedef unsigned char bmap;
#define __NBITS (8 * (int) sizeof (bmap))
#define __BMELT(d) ((d) / __NBITS)
#define __BMMASK(d) ((bmap) 1 << ((d) % __NBITS))
#define BM_SET(d, set) ((set[__BMELT (d)] |= __BMMASK (d)))
#define BM_CLR(d, set) ((set[__BMELT (d)] &= ~__BMMASK (d)))
#define BM_ISSET(d, set) ((set[__BMELT (d)] & __BMMASK (d)) != 0)
#define BASE_OFFSET 1024
#define BLOCK_SIZE 1024
unsigned int block_size;
#define BLOCK_OFFSET(block) ((block)*block_size)

struct ext2_super_block SUPERMAN;

int ALLBLOCKSCOUNT;
int BLOCKCOUNTPERGROUP;
int HOWMANYGROUPS;
int INODECOUNTPERGROUP;
struct stat STATOFFILE;
int readSuper(int fd, struct ext2_super_block* superBlock)
{
    lseek(fd, BASE_OFFSET, SEEK_SET);
    read(fd, superBlock, sizeof(*superBlock));

    return superBlock->s_magic != EXT2_SUPER_MAGIC;
}


void readFileStats(const char* filename) {


    stat(filename, &STATOFFILE);

}

std::bitset<32> convertToBitwise(int x){

    return std::bitset<32>(x);
}



void getSUPERMAN(int fd,const char* file){

    lseek(fd, BASE_OFFSET, SEEK_SET);
    read(fd, &SUPERMAN,BLOCK_SIZE);
    ALLBLOCKSCOUNT=SUPERMAN.s_blocks_count;
    BLOCKCOUNTPERGROUP=SUPERMAN.s_blocks_per_group;
    HOWMANYGROUPS=ceil((double)ALLBLOCKSCOUNT/(double)BLOCKCOUNTPERGROUP);
    INODECOUNTPERGROUP=SUPERMAN.s_inodes_per_group;

}

int * FindFreeDataBlockList(int fdImage,int neededDataBlockCount,struct ext2_group_desc* groupDesc){
    //this function gives neededDataBlockCount number of blocks' absolute block indexes as a list
    bmap *bitmap = (bmap*)malloc(block_size);

    int *tobePlaced= new int[neededDataBlockCount];


    int count=0;

    for (int i = 0; i < HOWMANYGROUPS; i++) {
        if(count==neededDataBlockCount){ break;}

        if((groupDesc[i].bg_free_blocks_count)<=0) {
            continue;
        }
            lseek(fdImage, (groupDesc[i].bg_block_bitmap)*block_size, SEEK_SET);
            read(fdImage, bitmap,block_size);
            for (int j = 0; j < BLOCKCOUNTPERGROUP; j++) {
             //   if(count==neededDataBlockCount){ break;}

                if(BM_ISSET(j, bitmap)){
                    continue;
                }
                else{
                    if(count<neededDataBlockCount){
                        tobePlaced[count]=(i*BLOCKCOUNTPERGROUP)+j+1;
                        //cout << "(i*BLOCKCOUNTPERGROUP)+j+1 =" << (i*BLOCKCOUNTPERGROUP)+j+1 << endl;
                        count++;
                    } else{
                        break;
                    }


                }



        }

    }



    free(bitmap);
    return tobePlaced;
}
int FindFreeInode(int fdImage,struct ext2_group_desc* groupDesc){
    //this function gives an absolute block index of free inode founded
    bmap *bitmap = (bmap*)malloc(block_size);

    int whereIsIt;

    int found=0;

    for (int i = 0; i < HOWMANYGROUPS; i++) {
        if(found) {
            break;
        }
        if((groupDesc[i].bg_free_inodes_count)<=0) {
            continue;
        }
        lseek(fdImage, (groupDesc[i].bg_inode_bitmap)*block_size, SEEK_SET);
        read(fdImage, bitmap,block_size);
        for (int j = 0; j < BLOCKCOUNTPERGROUP; j++) {
            if(found) {
                break;
            }


            if(BM_ISSET(j, bitmap)){
                continue;
            }
            else{

                    whereIsIt=(i*INODECOUNTPERGROUP)+j+1;
                    found=1;
                }
            }
        }
    free(bitmap);
    return whereIsIt;

}

void setBitmap(int fdImage,struct ext2_group_desc* groupDesc,int blockAbs,int action){

    bmap *bitmap = (bmap*)malloc(block_size);
    bmap *bitmaptmp = (bmap*)malloc(block_size);
    int groupID;
    int AbsInGroup;
    if(action==1){
        groupID=blockAbs/BLOCKCOUNTPERGROUP;
        AbsInGroup=(blockAbs % BLOCKCOUNTPERGROUP)-1;
        lseek(fdImage, (groupDesc[groupID].bg_block_bitmap)*block_size, SEEK_SET);
        read(fdImage, bitmap, block_size);
        BM_SET(AbsInGroup,bitmap);
        lseek(fdImage, (groupDesc[groupID].bg_block_bitmap)*block_size, SEEK_SET);
        write(fdImage, bitmap, block_size);

    } else{
        groupID=blockAbs/INODECOUNTPERGROUP;
        AbsInGroup=(blockAbs % INODECOUNTPERGROUP)-1;

        lseek(fdImage, (groupDesc[groupID].bg_inode_bitmap)*block_size, SEEK_SET);
        read(fdImage, bitmap, block_size);
        BM_SET(AbsInGroup,bitmap);
        lseek(fdImage, (groupDesc[groupID].bg_inode_bitmap)*block_size, SEEK_SET);
        write(fdImage, bitmap, block_size);
    }









    free(bitmap);
    free(bitmaptmp);

}

int giveDataBlockOfFreeBitmap(int orderOfFreeBitmap,int groupID){

   return groupID*BLOCKCOUNTPERGROUP + orderOfFreeBitmap;


}
void setGroups(int fdImage,int blockAbs,int action,int decreaseCount){



    struct ext2_group_desc* tmpGroups = new struct ext2_group_desc[HOWMANYGROUPS];
    int groupID;
    if(action==1){
        groupID=blockAbs/BLOCKCOUNTPERGROUP;
    } else{
        groupID=blockAbs/INODECOUNTPERGROUP;
    }


    if(block_size==1024){
        lseek(fdImage,BLOCK_OFFSET(2), SEEK_SET);
    } else{
        lseek(fdImage,BLOCK_OFFSET(1), SEEK_SET);
    }

    read(fdImage, tmpGroups, sizeof(struct ext2_group_desc)*HOWMANYGROUPS);


    if(action==1){
        tmpGroups[groupID].bg_free_blocks_count-=decreaseCount;
    }else if(action==2){
        tmpGroups[groupID].bg_free_inodes_count-=decreaseCount;
    }

    if(block_size==1024){
        lseek(fdImage,BLOCK_OFFSET(2), SEEK_SET);
    } else{
        lseek(fdImage,BLOCK_OFFSET(1), SEEK_SET);
    }

    write(fdImage, tmpGroups, block_size);

    free(tmpGroups);

}
void setSuperman(int fdImage,int INode,int DataBlock){

    SUPERMAN.s_free_inodes_count-=INode;
    SUPERMAN.s_free_blocks_count-=DataBlock;

    lseek(fdImage, 1024,SEEK_SET);
    write(fdImage, &SUPERMAN, 1024);

}
int setInode(int fdImage,int targetINODE,struct ext2_inode * sourceinode,struct ext2_group_desc * groupDesc){

    int groupID=0;
    int targetBlock;

    groupID=targetINODE/INODECOUNTPERGROUP;
    //cout<<"wkebflwjeh:"<<groupID<<endl;

    struct ext2_inode tmpinode;


    int nth_inode =((targetINODE % INODECOUNTPERGROUP)-1) * sizeof(struct ext2_inode);

   // targetBlock=groupDesc[groupID].bg_inode_table + ((targetINODE % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE;
   // targetBlock= targetBlock * BLOCK_SIZE + nth_inode - sizeof(struct ext2_inode);
    //cout<<"where to write :"<<groupDesc[groupID].bg_inode_table + (targetINODE % INODECOUNTPERGROUP)<<endl;
   // cout<<"targetINODE: "<<targetINODE<<endl;
   // cout<<"groupDesc[groupID].bg_inode_table :"<<groupDesc[groupID].bg_inode_table + ((targetINODE % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE<<endl;
    lseek(fdImage,BLOCK_OFFSET(groupDesc[groupID].bg_inode_table) + nth_inode, SEEK_SET);
    write(fdImage, sourceinode, block_size);



}
int getTargetInodeABSBlock(const char* targetInode,struct ext2_group_desc* groupDesc){
    int targetBlock;

    int targetIn=atoi(targetInode);
    int groupID=0;


    groupID=targetIn/INODECOUNTPERGROUP;
    //cout<<"groupID: "<< groupID<<endl;
  //  cout<<"targetin :"<<targetIn<<endl;
  //  cout<<"inodepergroup :"<<INODECOUNTPERGROUP<<endl;
  //  cout<<"(double)targetIn/(double)INODECOUNTPERGROUP "<<targetIn/INODECOUNTPERGROUP<<endl;
  //  cout<<"GROUP ID :"<<groupID<<endl;
  // cout<<"groupDesc["<<groupID<<"].bg_inode_table "<<groupDesc[groupID].bg_inode_table<<endl;
    int nth_inode =((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode)) % block_size;
   // cout<<"groupDesc[groupID].bg_inode_table  : "<<groupDesc[groupID].bg_inode_table<<endl;
   // cout<<"((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE: " <<((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE<<endl;
//cout<<"nth_inode :"<<nth_inode<<endl;
   targetBlock=groupDesc[groupID].bg_inode_table + ((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ block_size;
    return targetBlock * block_size + nth_inode - sizeof(struct ext2_inode);
}
int getTargetInodeABSBlock2(int targetInode,struct ext2_group_desc* groupDesc){
    int targetBlock;

    int targetIn=targetInode;
    int groupID=0;


    groupID=targetIn/INODECOUNTPERGROUP;
    //cout<<"groupID: "<< groupID<<endl;
    //  cout<<"targetin :"<<targetIn<<endl;
    //  cout<<"inodepergroup :"<<INODECOUNTPERGROUP<<endl;
    //  cout<<"(double)targetIn/(double)INODECOUNTPERGROUP "<<targetIn/INODECOUNTPERGROUP<<endl;
    //  cout<<"GROUP ID :"<<groupID<<endl;
    // cout<<"groupDesc["<<groupID<<"].bg_inode_table "<<groupDesc[groupID].bg_inode_table<<endl;
    int nth_inode =((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode)) % block_size;
    // cout<<"groupDesc[groupID].bg_inode_table  : "<<groupDesc[groupID].bg_inode_table<<endl;
    // cout<<"((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE: " <<((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ BLOCK_SIZE<<endl;
//cout<<"nth_inode :"<<nth_inode<<endl;
    targetBlock=groupDesc[groupID].bg_inode_table + ((targetIn % INODECOUNTPERGROUP) * sizeof(struct ext2_inode))/ block_size;
    return targetBlock * block_size + nth_inode - sizeof(struct ext2_inode);
}
void read_inode(int fd,int inode_no, struct ext2_group_desc * group, struct ext2_inode * inode){

    lseek(fd, BLOCK_OFFSET(group->bg_inode_table)+(inode_no-1)*sizeof(struct ext2_inode), SEEK_SET);
    read(fd, inode, sizeof(struct ext2_inode));
}

void setDirEntry(int connectedINODE,int fdImage, unsigned int targetInode,struct ext2_group_desc* groupDesc,const char *givenFile){

    struct ext2_dir_entry *entry;
    unsigned int size;
    unsigned char block[block_size];
    struct ext2_inode inode;

    lseek(fdImage, getTargetInodeABSBlock2(targetInode,groupDesc), SEEK_SET);
    read(fdImage, &inode, sizeof(struct ext2_inode));



    int block_index=0;
    while (block_index<12) {
        lseek(fdImage, BLOCK_OFFSET(inode.i_block[block_index]), SEEK_SET);
        read(fdImage, block, block_size);
        entry = (struct ext2_dir_entry *) block;
        struct ext2_dir_entry *prev_entry;

        size = 0;
        while (size < block_size) {
            char file_name[EXT2_NAME_LEN + 1];
            memcpy(file_name, entry->name, entry->name_len);
            file_name[entry->name_len] = 0;              /* append null char to the file name */
     //       printf("%10u %s\n", entry->inode, file_name);
            prev_entry=entry;
            entry = (struct ext2_dir_entry *) ((void *) entry + entry->rec_len);      /* move to the next entry */
            size += entry->rec_len;
        }



        int old=prev_entry->rec_len;
        int preventry_new_rec_len = prev_entry->name_len+8;

        if (preventry_new_rec_len % 4) { preventry_new_rec_len += (4 - (preventry_new_rec_len % 4));}
        prev_entry->rec_len=preventry_new_rec_len;

        int lenx = strlen(givenFile);


        if (lenx + 8 <= old - prev_entry->rec_len) {//is it necessary?

            struct ext2_dir_entry newx;
            memcpy(newx.name, givenFile, lenx);
            newx.file_type = 1;
            newx.name_len = lenx;
            newx.inode = connectedINODE;
//            cout<<"((struct ext2_inode)(newx.inode)).i_size "<<((struct ext2_inode)(newx.inode)).i_size<<endl;
            newx.rec_len = old - prev_entry->rec_len;
          /*cout<<"newx.file_type :" <<newx.file_type<<endl;
            cout<<"newx.name_len :" <<newx.name_len<<endl;
            cout<<"newx.inode :" <<newx.inode<<endl;
            cout<<"newx.rec_len :" <<newx.rec_len<<endl;
            */
            prev_entry->rec_len = preventry_new_rec_len;


            lseek(fdImage, BLOCK_OFFSET(inode.i_block[block_index])+ block_size-old, SEEK_SET);
            write(fdImage, prev_entry, prev_entry->rec_len);
            //cout<<old<<endl;
            lseek(fdImage, BLOCK_OFFSET(inode.i_block[block_index])+ block_size + preventry_new_rec_len - old, SEEK_SET);
            write(fdImage, &newx, newx.rec_len);

            break;
        }

        block_index++;
    }

}

int parser(int fdImage,const char * path,struct ext2_group_desc* groupDesc){

struct ext2_inode MYINODE;

    int inode=-1;

    std::vector<string> dirs;
    if(strlen(path)==1 && *path=='/'){
       // std::cout << "exiting" << endl;
        return 2;
    }
    else if(*path=='/'){


        std::vector<char> dirs_sub;
        path++;

        int asa=0;
        while (*path != '\0'){

            if(*path == '/'){
                path++;
            }
            while (*path && *path!='/'){
                dirs_sub.push_back(*path);
                path++;
            }
            std::string tmm=std::string(dirs_sub.begin(),dirs_sub.end());

            dirs.push_back(std::string(dirs_sub.begin(),dirs_sub.end()));
            dirs_sub.erase(dirs_sub.begin(),dirs_sub.end());

        }

    }
    else{
        char temp_path[strlen(path)+2];

        temp_path[0] = '/';

        for (int j = 1; j < strlen(path)+1 ; j++) {
            temp_path[j] = path[j-1];
        }

        temp_path[strlen(path)+1] = 0;

        char *xd = temp_path;

        std::vector<char> dirs_sub;
        xd++;

        int asa=0;
        while (*xd != '\0'){

            if(*xd == '/'){
                xd++;
            }
            while (*xd && *xd!='/'){
                dirs_sub.push_back(*xd);
                xd++;
            }
            std::string tmm=std::string(dirs_sub.begin(),dirs_sub.end());

            dirs.push_back(std::string(dirs_sub.begin(),dirs_sub.end()));
            dirs_sub.erase(dirs_sub.begin(),dirs_sub.end());

        }
    }


if(dirs.size()<=0){

}else{

int X=0;
    char *block =new char[block_size];
    inode=2;
   // cout<<"dirs.size() :" <<dirs.size()<<endl;
    for (int i = 0; i <dirs.size() ; i++) {

        lseek(fdImage, getTargetInodeABSBlock2(inode,groupDesc), SEEK_SET);
        read(fdImage, &MYINODE, sizeof(MYINODE));
        //((inode_id) - 1) / (INODES_PER_GROUP)

      //  cout<<"MYINODE.i_links_count: "<<MYINODE.i_links_count<<endl;
        X=0;
        int block_index=0;
        while (block_index<12) {
           // cout<<"IM HERE"<<endl;

            if(X){ break;}
            lseek(fdImage, BLOCK_OFFSET(MYINODE.i_block[block_index]), SEEK_SET);
            read(fdImage, block, block_size);
            struct ext2_dir_entry *entry;
            entry = (struct ext2_dir_entry *) block;



            int size = 0;
            while (size < block_size) {

                if(X){ break;}
               // cout<<"entry->name :"<<entry->name<<endl;
               // cout<<"dirs[i]     :"<<dirs[i]<<endl;
                if(entry->name==dirs[i]){
                    //cout<<"inodel     :"<<inode<<endl;
                    //cout<<"inoder     :"<<entry->inode<<endl;
                    //cout<<"HERE :"<<endl;
                    inode=entry->inode;
                   // cout<<"equality :"<<inode<<endl;

                    X=1;
                    break;

                }
                size += entry->rec_len;
                entry = (struct ext2_dir_entry *) ((void *) entry + entry->rec_len);      /* move to the next entry */



            }
            block_index++;
            }


        }

    }

//cout<<"result :" << inode<<endl;
    return inode;



}
int main(int argc,const char * argv[]){
    const char * imageFILE=argv[1];
    const char * source=argv[2];
    unsigned int inodeTarget;
   // const char * path=argv[3];



    readFileStats(source);
    //int neededDataBlockCount=STATOFFILE.st_size;
    int fdImage = open(imageFILE, O_RDWR);
    getSUPERMAN(fdImage,imageFILE);
    block_size=block_size = 1024 << SUPERMAN.s_log_block_size;//get block SIZE


//get Group Descriptors////////////////////////////////////////
    struct ext2_group_desc *groupDesc=new struct ext2_group_desc[HOWMANYGROUPS];

    if(block_size==1024){
        lseek(fdImage,BLOCK_OFFSET(2), SEEK_SET);
    } else{
        lseek(fdImage,BLOCK_OFFSET(1), SEEK_SET);
    }

    read(fdImage, groupDesc, sizeof(struct ext2_group_desc)*HOWMANYGROUPS);
////////////////////////////////////////////////////////////////

    if(argv[3][0] <= '9' && argv[3][0] >= '0'){ //Its an Inode number!
        inodeTarget = atoi(argv[3]);
    }

    else{   // Its a directory!
        inodeTarget =  (parser(fdImage,argv[3],groupDesc)); // call the new goddamn function
    }


    //cout<< "Integer: " <<inodeTarget<<endl;


//get Inode Metadata from source file//////////////////////////


    struct ext2_inode MYINODE;

    MYINODE.i_mode=STATOFFILE.st_mode;
    MYINODE.i_uid=STATOFFILE.st_uid;
    MYINODE.i_gid=STATOFFILE.st_gid;
    MYINODE.i_size=STATOFFILE.st_size;
    //cout << "STATOFFILE.st_size = " << STATOFFILE.st_size << endl;
    MYINODE.i_atime=STATOFFILE.st_atime;
    MYINODE.i_ctime=STATOFFILE.st_ctime; //modification is the same for access of stat
    MYINODE.i_mtime=STATOFFILE.st_mtime;
    MYINODE.i_links_count = STATOFFILE.st_nlink;
/*
    cout<<"MYINODE.i_gid            :"<<MYINODE.i_gid<<endl;
    cout<<"MYINODE.i_uid            :"<<MYINODE.i_uid<<endl;
    cout<<"MYINODE.i_size           :"<<MYINODE.i_size<<endl;
    cout<<"MYINODE.i_blocks         :"<<MYINODE.i_blocks<<endl;
    cout<<"MYINODE.i_links_count    :"<<MYINODE.i_links_count<<endl;
    cout<<"MYINODE.i_ctime          :"<<MYINODE.i_ctime<<endl;
    cout<<"MYINODE.i_dtime          :"<<MYINODE.i_dtime<<endl;
    cout<<"MYINODE.i_mtime          :"<<MYINODE.i_mtime<<endl;
    cout<<"MYINODE.i_atime          :"<<MYINODE.i_atime<<endl;
    cout<<"MYINODE.i_mode           :"<<MYINODE.i_mode<<endl;
    cout<<"MYINODE.i_file_acl       :"<<MYINODE.i_file_acl<<endl;
    cout<<"MYINODE.i_dir_acl        :"<<MYINODE.i_dir_acl<<endl;
    cout<<"MYINODE.i_generation     :"<<MYINODE.i_generation<<endl;
    cout<<"MYINODE.osd1             :"<<MYINODE.osd1<<endl;
    cout<<"MYINODE.extra            :"<<MYINODE.extra<<endl;
    cout<<"MYINODE.i_faddr          :"<<MYINODE.i_faddr<<endl;
    cout<<"MYINODE.i_flags          :"<<MYINODE.i_flags<<endl;
*/

    int fullBlocks_of_File=STATOFFILE.st_size / block_size;
    int RemainingBytes_of_Files = STATOFFILE.st_size % block_size;

    if (RemainingBytes_of_Files){
        MYINODE.i_blocks=((fullBlocks_of_File+1)*block_size)/512;
    } else{
        MYINODE.i_blocks=(fullBlocks_of_File*block_size)/512;
    }
////////////////////////////////////////////////////////////////

//GET FREE BLOCKS///////////////////////////////////////////////
    int requiredFreeDataBlockCount=0;
    int filesize=MYINODE.i_size;


    while(filesize>0){
        requiredFreeDataBlockCount++;
        filesize-=block_size;
    }


   // cout << "requiredFreeDataBlockCount = " << requiredFreeDataBlockCount << endl;
    int * newFreeBlocks=FindFreeDataBlockList(fdImage,requiredFreeDataBlockCount,groupDesc);
    int * ptrToSetGroup=newFreeBlocks;
    int * printJob=newFreeBlocks;
////////////////////////////////////////////////////////////////

//FILL DATA BLOCMYINODEK ADDRESSES OF DATA BLOCKS OF NEW INODE
int countofFrees=0;
    for (int i = 0; i < 15; i++) {
        if(*newFreeBlocks){


        MYINODE.i_block[i]=*newFreeBlocks;
      //      cout<<"MYINODE.i_block[i :]"<<MYINODE.i_block[i]<<endl;
        newFreeBlocks++;
        countofFrees++;
        } else{

            MYINODE.i_block[i]=0;
        }
    //    cout<<"MYINODE.i_block[i] :"<<MYINODE.i_block[i]<<endl;
    }

///////////////////////////////////////////////////////////////////

//WRITE DATA

    char * tempBlock=new char[block_size];
    int fdSource = open(source, O_RDONLY, 0);

   // char * tempBlocktmp=new char[block_size];


    for (int i = 0; i < countofFrees; i++) {
        if(MYINODE.i_block[i]){

           // cout<<"i * block_size : "<<i * block_size<<endl;
            lseek(fdSource, i * block_size, SEEK_SET);
            read(fdSource, tempBlock, block_size);
          //  cout<<"MYINODE.i_block[i] : "<<MYINODE.i_block[i]<<endl;
            lseek(fdImage, BLOCK_OFFSET(MYINODE.i_block[i]), SEEK_SET);
            write(fdImage, tempBlock, block_size);

           // lseek(fdImage, BLOCK_OFFSET(MYINODE.i_block[i]), SEEK_SET);
           // read(fdImage, tempBlocktmp, block_size);
           // cout<<"writed into block: " << MYINODE.i_block[i] << "data is : "<< tempBlocktmp<<endl;

            setBitmap(fdImage,groupDesc,MYINODE.i_block[i],1);

        } else{

            break;
        }
    }






    //set INODE

    int freeINODE=FindFreeInode(fdImage,groupDesc);
   // cout<<"free inode : "<<freeINODE<<endl;


    setInode(fdImage,freeINODE,&MYINODE,groupDesc);
    setBitmap(fdImage,groupDesc,freeINODE,2);

    //setDirEntry

   // cout<<"freeINODE :"<<freeINODE<<endl;
    setDirEntry(freeINODE,fdImage,inodeTarget,groupDesc,source);

    //set superblocks

    setSuperman(fdImage,1,requiredFreeDataBlockCount);//decrease freeinodecount by 1, freedatablockcount by requiredFreeDataBlockCount

    //set group 0 info
    for (int i = 0; i < 12; i++) {
        if(*ptrToSetGroup){

            setGroups(fdImage,*ptrToSetGroup,1,1); //decrease free block counts (action=1 decrease free datablock case indicator)
            ptrToSetGroup++;
        } else{

            break;
        }
    }


    //decrease free inode count of related group that we took new inode from
    setGroups(fdImage,freeINODE,2,1);      //decrease inode count by 1 (action=2 decrease free inode case indicator)


    free(groupDesc);
   // free(newFreeBlocks);
    free(tempBlock);
  //  free(tempBlocktmp);

    close(fdSource);

    cout<<freeINODE;

    cout<<" "<<*printJob;

    int tmp=0;
    tmp=*printJob;
    printJob++;

    for (int j = 1; j < requiredFreeDataBlockCount; j++) {
        if(*printJob && *printJob!=tmp){

            cout<<","<<*printJob;
            printJob++;
        } else{
            printJob++;
            tmp=*printJob;
        }
    }

cout<<endl;
    return 0;
}