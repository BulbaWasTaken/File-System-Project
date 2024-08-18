/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe, 
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: fsFreeSpace.h
*
* Description: The header for free space management
**************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "fsLow.h"
#ifndef _FSFREESPACE_H
#define _FSFREESPACE_H


#define BLOCK_SIZE 512
#define FREE 1
#define USED 0
#define IS_DIR 1
#define NOT_DIR 0


uint64_t initBitMap(uint64_t blockCount, uint64_t bytesPerBlock);
void setBitUsed(uint64_t * bitmap, int bitNumber);
void setBitFree(uint64_t * bitmap, int bitNumber);
bool isBitUsed(uint64_t * bitmap, int bitNumber);
uint64_t allocBlocks(uint64_t blocksNeeded);
void releaseBlocks(uint64_t start, uint64_t count);

#endif
