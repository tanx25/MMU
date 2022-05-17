#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define OFFSET_MASK 255
#define PAGES 256
#define OFFSET_BITS 8
#define PAGE_SIZE 256
#define FRAMES 128
#define TLB_SIZE 16
#define BUFFER_SIZE 10

char * mmapfptr;
#define SIZE   256 
#define COUNT  128

#define MEMORY_SIZE FRAMES * PAGE_SIZE

signed char main_memory[MEMORY_SIZE];

int pageTable[PAGES];

struct TLBStructure{
    int page[TLB_SIZE];
    int frame[TLB_SIZE];
    int currentIndex;
};
struct FrameStructure{
    int frame[FRAMES];
    int currentIndex;
};

struct TLBStructure tlb;
struct FrameStructure frameTable;

void incrementFrameTable(){
    if (frameTable.currentIndex == FRAMES -1){
        frameTable.currentIndex = 0;
    }
    else {
        frameTable.currentIndex ++;
    }
}
void addToFrameTable(int pageNum){
    frameTable.frame[frameTable.currentIndex] = pageNum;
    incrementFrameTable(); 

}
void incrementTLB(){
    if (tlb.currentIndex == TLB_SIZE - 1){
        tlb.currentIndex = 0;
    }
    else{
        tlb.currentIndex ++;
    }
}
void TLB_Add(int pageNum, int frameNum){
    tlb.page[tlb.currentIndex] = pageNum; 
    tlb.frame[tlb.currentIndex] = frameNum;

    incrementTLB(); 
}
int TLB_Search(int pageNum){
    for (int i = 0; i < TLB_SIZE; i++){
        if (tlb.page[i] == pageNum){
            return tlb.frame[i];
        }
    
    }
    return -1;

}
void TLB_Update(int pageNum, int frameNum){
    for (int i = 0; i < TLB_SIZE; i++){
        if (tlb.page[i] == pageNum){
            tlb.frame[i] = frameNum;
        }
    }
}

void init(){
    for (int i = 0; i < PAGES; i++){
        pageTable[i] = -1;       
    }
    
    for (int i = 0; i < TLB_SIZE; i++){
        tlb.page[i] = -1;
        tlb.frame[i] = -1;
    }
    tlb.currentIndex = 0;
    for (int i = 0; i< FRAMES; i++){
        frameTable.frame[i] = -1;
    }



}

int main(int argc, const char *argv[]){
    init();
    int logicalAddress; 
    int total_addresses = 0;
    int tlb_hits = 0;
    int page_faults = 0;

    int readIndex = 0;
    int writeIndex = 0;
    
    FILE *fptr = fopen("addresses.txt", "r");
    char buff[BUFFER_SIZE];
    int mmapfile_fd = open("BACKING_STORE.bin", O_RDONLY);
    mmapfptr = mmap(0, 65536 , PROT_READ, MAP_PRIVATE, mmapfile_fd, 0);


    signed int free_frame = 128;
    int wpointer = 0;
    int rpointer = 0;
    
    while(fgets(buff, BUFFER_SIZE, fptr) != NULL) {
        total_addresses++;
        logicalAddress = atoi(buff);
        int pageNum = logicalAddress >> OFFSET_BITS;
        int pageOffset = logicalAddress & OFFSET_MASK;
        
        int frameNum = TLB_Search(pageNum); 
        
        if (TLB_Search(pageNum) != -1){
            frameNum = TLB_Search(pageNum);
            tlb_hits ++;
        } 

        
        else {
            frameNum = pageTable[pageNum] ; 
            if (frameNum == -1){
                page_faults++;
                 if (free_frame != 0){
                    free_frame--;
                    
                    memcpy(main_memory + wpointer * 256, mmapfptr + pageNum * 256, 256);
                    pageTable[pageNum] = wpointer;
                    frameNum = wpointer;
                    wpointer=(wpointer+1)%FRAMES;
                    addToFrameTable(pageNum);

                }else{
                    memcpy(main_memory + rpointer * PAGE_SIZE, mmapfptr + pageNum * PAGE_SIZE, PAGE_SIZE);
                     
                    for (int i=0;i<PAGES ;i++){
                        if (pageTable[i] == rpointer){
                            pageTable[i] = -1;
                            break;
                        }
                    }
                    pageTable[pageNum] = rpointer;
                    frameNum = rpointer;

                rpointer=(rpointer+1)% FRAMES;
                pageTable[pageNum] = frameTable.currentIndex; 
                addToFrameTable(pageNum);
                
                }
                
    }
            
            TLB_Add(pageNum, frameNum);
            

        }

        
        int physical_address = (frameNum << OFFSET_BITS) | pageOffset;
        signed char value = main_memory[physical_address];
       
        printf("Virtual address: %d Physical address = %d Value=%d \n", logicalAddress,physical_address,value);
        
    }
    printf("Total addresses = %d\n", total_addresses);
    printf("Page_faults = %d\n", page_faults);
    printf("TLB Hits = %d\n", tlb_hits);

}