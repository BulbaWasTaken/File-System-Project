/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe,
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: fsFreeSpace.c
*
* Description: This file is where the free space management
* is implemented. This file contains the functions that will
* be used to manage the free space of the file system.
**************************************************************/

#include "fsFreeSpace.h"

/*
  We are using a bitmap to keep track of free space.
  Each bit represents a block (1 = free, 0 = allocated).
*/
uint64_t * bitmap;

/*
  Allocate memory for a bitmap and initialize it to all 1's. 
  Set the first 6 bits to used. Bit/Block 0 is where VCB is located.
  Bit/Block 1-5 is where the free space will be stored for the default size.
  Bit Map initialization is dynamic, which means free space management
  is going to be stored from block 1 to any blocks needed.

  This returns the start block of the free space in Disk. 
*/
uint64_t initBitMap(uint64_t blockCount, uint64_t bytesPerBlock)
{
  bitmap = malloc(bytesPerBlock * (((blockCount/8)/512)+1));
  if(bitmap == NULL) {
    printf("Error allocating memory for bitmap\n");
    exit(1);
  }

  // Initialize bitmap to all 1's: 0xFF is 11111111
  memset(bitmap, 0xFF, bytesPerBlock * (((blockCount/8)/512)+1));

  // Set the first n bits to 0. Default case: n = 5. 
  for (int bitNumber = 0; bitNumber < (((blockCount/8)/512)+1)+1; bitNumber++)
  {
    setBitUsed(bitmap, bitNumber);
  }
  
  //Write the bit map to disk starting at block 1.
  LBAwrite(bitmap, (((blockCount/8)/512)+1), 1);
  return 1;
}

//Set bit to Allocated(0)
void setBitUsed(uint64_t * bitmap, int bitNumber)
{
  bitmap[bitNumber / 64] &= ~(1ULL << (bitNumber % 64));
}

//Set bit to Free(1)
void setBitFree(uint64_t * bitmap, int bitNumber)
{
  bitmap[bitNumber / 64] |= (1ULL << (bitNumber % 64));
}

/*
  Check if the bit in bitmap is used. If used, or 0, it will return
  true.
*/

bool isBitUsed(uint64_t * bitmap, int bitNumber)
{
    int arrayIndex = bitNumber / 64;
    int bitOffset = bitNumber % 64;
    return (bitmap[arrayIndex] & (1ULL << bitOffset)) == 0;
}


/*
  This function iterate through the free space system, and 
  look for a block that is free. Since we are doing contigious, 
  this function will check if the following blocks are free. It 
  will check until the requested blocks are met. If it finds a
  used block/bit, it will try to find a free block again and repeat
  until the requested blocks are met. 

  This returns the starting block it finds.
*/
uint64_t allocBlocks(uint64_t blocksNeeded) 
{
    uint64_t blocksAllocated = 0;
    uint64_t startingBlock = -1;
    bitmap = malloc(BLOCK_SIZE * 5);
    LBAread(bitmap, 5, 1);

    //Iterate throught the bitmap
    for (uint64_t i = 0; i < BLOCK_SIZE*5; i++)
    {
      if(isBitUsed(bitmap, i) == 0)
      {
        startingBlock = i;
        for (uint64_t j = i; j < i + blocksNeeded; j++)
        {
          //If a bit is found used before reaching the requested blocks
          //get our the loop and find another free bit.
          if(isBitUsed(bitmap, j) == 1)
          {
            blocksAllocated = 0;
            startingBlock = -1;
            break;
          }
          else
          {
            blocksAllocated++;
          }
        }
        if(blocksAllocated == blocksNeeded)
        {
          for (uint64_t k = i; k < i + blocksNeeded; k++)
          {
            setBitUsed(bitmap, k);
          }
          break;
        }
      }
    }

    LBAwrite(bitmap, 5, 1);
    free(bitmap);
    return startingBlock;
}

/*
  Release the blocks and return it to the free space.
*/
void releaseBlocks(uint64_t start, uint64_t count)
{
  bitmap = malloc(BLOCK_SIZE * 5);
  LBAread(bitmap, 5, 1);
  
  //Iterate throught the bitmap
  for (uint64_t i = start; i < start+count; i++)
  {
    setBitFree(bitmap, i);
  }
  LBAwrite(bitmap, 5, 1);
  free(bitmap);
}