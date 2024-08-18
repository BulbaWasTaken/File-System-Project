/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe,
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "fsLow.h"
#include "mfs.h"
#include "fsFreeSpace.h"
#include "helperFunctions.h"

//Global variable for the VCB
VolumeControlBlock vcb;



// Magic number to identify VCB. 
// Stands for "METAGRP"
#define VCB_SIGNATURE 0x005052474154454D


/*
    Initialize the File system
*/
int initFileSystem (uint64_t numberOfBlocks, uint64_t blockSize){
    printf ("Initializing File System with %ld blocks with a block size of %ld\n", 
                        numberOfBlocks, blockSize);

    /*
        A pointer to VolumeControlBlock. This is where
        we initialize all the values.
        Allocate memory for reading the VCB from block 0
    */
    VolumeControlBlock* tempVCB = (VolumeControlBlock*)malloc(blockSize);
    if (!tempVCB) {
        printf("Memory allocation failed for VCB\n");
        return -1;  // Error
    }

	// Read block 0 where the VCB should be stored
    LBAread(tempVCB, 1, 0);

	// Check if the VCB is already initialized using the signature/magic number
    if (tempVCB->signature == VCB_SIGNATURE) 
    {
        printf("\nFile system is already initialized.\n");
        /*
            If initialized, Load the VCB into our global VCB variable
        */
        memcpy(&vcb, tempVCB, sizeof(VolumeControlBlock));
    } 
    else 
    {
        printf("\nFile system is not initialized. Formatting...\n");

		//Set up the tempVCB
        tempVCB->signature = VCB_SIGNATURE;
        tempVCB->totalBlocks = numberOfBlocks;
        tempVCB->blockSize = blockSize;
        tempVCB->freeSpaceBitmap = initBitMap(numberOfBlocks, blockSize);
        tempVCB->rootLocation = initDir(50, NULL);
        strcpy(tempVCB-> vcbName, "Initial VCB");

        // Write the tempVCB to block 0
        LBAwrite(tempVCB, 1, 0);
        memcpy(&vcb, tempVCB, sizeof(VolumeControlBlock));
    }
    int setRoot = setRootDirectory(vcb.rootLocation);
    if(setRoot == -1)
    {
        printf("Error: Set Root Failed");
        exit(-1);
    }
	free(tempVCB);
	return 0;
}


	
void exitFileSystem ()
	{
	printf ("System exiting\n");
	}
