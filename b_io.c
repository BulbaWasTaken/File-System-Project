/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe, 
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>			// for malloc
#include <string.h>			// for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "helperFunctions.h"
#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define MAXLENGTH 4096
typedef struct b_fcb
	{
	PPinfo * ppinfo; 		//holds the information from parsePath()
	DirectoryEntry fi; 	// holds the file info
	char * fcbBuf;			//holds the open file buffer
	int fcbBufIndex;		//holds the current position in the buffer
	int flag;				//holds the flags for the control block
	int bptr;				//holds the current block pointer to the file
	int filePos;			//holds the current pointer to the file
	int fileDesc;			//holds the file descriptor of the file
	} b_fcb;
	
b_fcb fcbArray[MAXFCBS];

int startup = 0;	//Indicates that this has not been initialized

//Method to initialize our file system
void b_init ()
	{
	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fcbBuf = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fcbBuf == NULL)
			{
			return i;		//Not thread safe (But do not worry about it for this assignment)
			}
		}
	return (-1);  //all in use
	}
	
// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open (char * filename, int flags)
	{
	b_io_fd returnFd;
		
	if (startup == 0) b_init();  //Initialize our system
	
	returnFd = b_getFCB();				// get our own file descriptor
										// check for error - all used FCB's
	if (returnFd < 0) return (-1);

	char path[MAXLENGTH];
	strcpy(path, filename);

	//Parse filename
	PPinfo * ppinfo = malloc(sizeof(PPinfo));
	int parse = parsePath(path, ppinfo); // check if path is valid
	if(parse != 0) 
	{
		printf("Error: Invalid path\n");
		return (-1);
	}

	/*
		If O_CREAT flag is called and the element does not exist, make a new file.
		Otherwise, do nothing and continue.
	*/
	if(flags & O_CREAT)
	{
		if(ppinfo->index == -1)
		{
			/*
				If directory is not full, make the file, otherwise, do exit.
			*/
			if(maxEntryChecker(ppinfo->parent) < ppinfo->parent->size/sizeof(DirectoryEntry))
			{
				uint64_t blocksNeeded = (sizeof(DirectoryEntry) + (BLOCK_SIZE-1))/BLOCK_SIZE;
				uint64_t startBlock = allocBlocks(blocksNeeded);
				int frDE = findFreeEntry(ppinfo->parent);

				strcpy(ppinfo->parent[frDE].name, ppinfo->lastElement);
				ppinfo->parent[frDE].size = 0;
				ppinfo->parent[frDE].location.dirLocation = startBlock;
				ppinfo->parent[frDE].location.offSet = blocksNeeded;
				ppinfo->parent[frDE].isDir = NOT_DIR;
				ppinfo->parent[frDE].isUsed = USED;
				time_t t = time(NULL);
				ppinfo->parent[frDE].dateCreated = t;
				ppinfo->parent[frDE].dateModified = t;
				ppinfo->parent[frDE].lastAccessed = t;
				fcbArray[returnFd].fi = ppinfo->parent[frDE];
			}
			else
			{
				printf("Error: Directory is full\n");
				return (-1);
			}
		}
	}
	
	/*
		If O_TRUNC flag is called and file exists, set the filesize to 0.
	*/
	if(flags & O_TRUNC) 
	{
		if(ppinfo->index != -1)
		{
			ppinfo->parent[ppinfo->index].size = 0;
		}
	}

	/*
		If O_RDWR flag is called, load up the directory entry from the parent.
	*/
	if(flags & O_RDWR)
	{
		fcbArray[returnFd].fi = ppinfo->parent[ppinfo->index];
	}

	/*
		If O_RDONLY flag is called, load up the directory entry from the parent.
	*/
	if(flags == 0)
	{
		fcbArray[returnFd].fi = ppinfo->parent[ppinfo->index];
	}

	/*
		If O_WRONLY flag is called and the element exist, load up the directory entry
		from the parent to the fcbArray.
	*/
	if(flags & O_WRONLY) 
	{	
		if(ppinfo->index != -1)
		{
			fcbArray[returnFd].fi = ppinfo->parent[ppinfo->index];
		}
	}
	
	fcbArray[returnFd].fcbBuf = (char *) malloc(B_CHUNK_SIZE);
	if(fcbArray[returnFd].fcbBuf == NULL)
	{
		printf("Memory Allocation Error.\n");
		exit(-1);
	}

	/*
		If O_APPEND flag is called, set the filepointer to filesize or end of file.
	*/
	if(flags & O_APPEND)
	{
		fcbArray[returnFd].filePos = ppinfo->parent[ppinfo->index].size;
		fcbArray[returnFd].bptr = ppinfo->parent[ppinfo->index].location.offSet;
		fcbArray[returnFd].fcbBufIndex = ppinfo->parent[ppinfo->index].size % B_CHUNK_SIZE;
		printf("StartBlock: %lu\n", fcbArray[returnFd].fi.location.dirLocation);
		printf("OFFSET: %lu\n", fcbArray[returnFd].fi.location.offSet);
		LBAread(fcbArray[returnFd].fcbBuf, 1, fcbArray[returnFd].fi.location.dirLocation
											+fcbArray[returnFd].fi.location.offSet-1);
		printf("BUFINDEX: %s\n", fcbArray[returnFd].fcbBuf);
	}
	else
	{
		fcbArray[returnFd].filePos = 0;
		fcbArray[returnFd].bptr = 0;
		fcbArray[returnFd].fcbBufIndex = 0;
	}
		
	/*
		Initialize fcbArray then return the index.
	*/
	fcbArray[returnFd].ppinfo = ppinfo;
	time_t t = time(NULL);
	fcbArray[returnFd].fi.lastAccessed = t;
	fcbArray[returnFd].ppinfo->parent[findEntryInDir(ppinfo->parent, 
							fcbArray[returnFd].fi.name)].lastAccessed = t;

	fcbArray[returnFd].flag = flags;
	return (returnFd);						// all set
	}

// Interface to seek function	
int b_seek (b_io_fd fd, off_t offset, int whence)
	{
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}
	
	if (fcbArray[fd].fcbBuf == NULL) // check if fd is in use
	{
		return (-1); 
	}

	int ptr;

	if (whence & SEEK_CUR){ptr = fcbArray[fd].filePos + offset;}
	else if(whence & SEEK_END){ptr = fcbArray[fd].fi.size + offset;}
	else if(whence & SEEK_SET){ptr = offset;}
	else {return (-1);} //invalid whence

	if ((ptr < 0) || (ptr >= fcbArray[fd].fi.size)) // Out of bounds check
		{
		return (-1); 					
		}

	if (ptr / B_CHUNK_SIZE != fcbArray[fd].bptr) // Check if new ptr is not in current block
	{
		LBAread(fcbArray[fd].fcbBuf, 1, 
					fcbArray[fd].fi.location.dirLocation + 1 + ptr / B_CHUNK_SIZE);
		fcbArray[fd].bptr = ptr / B_CHUNK_SIZE;
	}

	fcbArray[fd].fcbBufIndex = ptr % B_CHUNK_SIZE; 
	fcbArray[fd].filePos = ptr;
	
	return (ptr);
	}


// Interface to write function	
int b_write (b_io_fd fd, char * buffer, int count)
{
	int bytesWritten = 0;
	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
	{
		return (-1); 					//invalid file descriptor
	}
	// check if fd is in use
	if (fcbArray[fd].fcbBuf == NULL)
	{
		return (-1); 
	}
	
	/*
		If the flags are not O_WRONLY or O_RDWR, we end the function and return -1.
	*/
	if (!(fcbArray[fd].flag & O_WRONLY || fcbArray[fd].flag & O_RDWR))
	{ 
		return (-1);
	}
	else
	{
		int bytesToWrite = count; 
		int availableBytes = B_CHUNK_SIZE - fcbArray[fd].fcbBufIndex; //bytes left in buffer

		/*
			If the remaining bytes left is greater then the requested number of
			bytest to write, the remaining bytes left to write in buffer to 
			count/bytesToWrite
		*/
		if(availableBytes > bytesToWrite)
		{ 
			availableBytes = bytesToWrite;
		}
		
		/*
			If the requested count plus file buffer index is less then 512, 
			we copy the the buffer to the file buffer then return how much we
			wrote.

			If the requested count plus file buffer index is greater then 512,
			copy the number of bytes that can fit the file buffer then relocate
			to avoid writing over other data and write on disk. After, copy the
			remaining number of bytes to be written. 
		*/
		if(count + fcbArray[fd].fcbBufIndex <= B_CHUNK_SIZE)
		{	
			memcpy(fcbArray[fd].fcbBuf + fcbArray[fd].fcbBufIndex, buffer, count);
			fcbArray[fd].fcbBufIndex += availableBytes;
			bytesWritten += availableBytes;
			fcbArray[fd].fi.size += bytesWritten;
			return (bytesWritten); //Change this
		}
		else
		{	
			while (bytesToWrite + fcbArray[fd].fcbBufIndex > B_CHUNK_SIZE)
			{
				/*
					Relocate entry so it does not trample other data
				*/
				memcpy(fcbArray[fd].fcbBuf + fcbArray[fd].fcbBufIndex, 
							buffer, availableBytes);
				fcbArray[fd].fi.location.offSet += 1;
				fileRelocate(&fcbArray[fd].fi, fcbArray[fd].ppinfo);	

				LBAwrite(&fcbArray[fd].fi, 1, fcbArray[fd].fi.location.dirLocation);
				LBAwrite(fcbArray[fd].fcbBuf, 1, 
						fcbArray[fd].fi.location.dirLocation + 1 + fcbArray[fd].bptr);
				
				bytesWritten += availableBytes;
				fcbArray[fd].bptr += 1;
				fcbArray[fd].fcbBufIndex = 0;

				memcpy(fcbArray[fd].fcbBuf + fcbArray[fd].fcbBufIndex, 
							buffer+availableBytes, count);
				fcbArray[fd].fcbBufIndex += count-availableBytes;
				bytesWritten += count-availableBytes;
				fcbArray[fd].fi.size += bytesWritten;

				bytesToWrite = bytesWritten - availableBytes;
			}
		}
	}
}



// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill 
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read (b_io_fd fd, char * buffer, int count)
{

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	if (fcbArray[fd].fcbBuf == NULL) // check if fd is in use
	{
		return (-1); 
	}
	
	/*
		If the flags are not O_RDONLY or O_RDWR, we end the function and return -1.
	*/
	if (!(fcbArray[fd].flag == 0 || fcbArray[fd].flag & O_RDWR))
	{ //check for valid flags
		return (-1);
	}

	strcpy(buffer, "");

	if (fcbArray[fd].ppinfo->index == -1)
	{
		printf("Error: File does not exist.\n");
		return (-1);
	}
	else
	{
		int bytesRead = 0;
		int inputBufferPos = 0;

		while(1)
		{
			/*
				Check if the file index plus requested count is greater the file size.
				If so, set count to the remaining bytes left, which means end of file.
			*/
			if(fcbArray[fd].fi.size <= count + fcbArray[fd].filePos)
			{
				count = fcbArray[fd].fi.size - fcbArray[fd].filePos;
			}

			/*
				Populate the file buffer
			*/
			if(fcbArray[fd].fcbBufIndex == 0)
			{
				LBAread(fcbArray[fd].fcbBuf, 1, 
							fcbArray[fd].fi.location.dirLocation + 1 + fcbArray[fd].bptr);
			}
			/*
				If the requsted amount is less that the remaining bytes 
				left in the file buffer, copy the file buffer into the user buffer. 
			*/
			if (count <= B_CHUNK_SIZE - fcbArray[fd].fcbBufIndex)
			{
				memcpy(buffer + inputBufferPos, 
							fcbArray[fd].fcbBuf + fcbArray[fd].fcbBufIndex, count);

				//increase the file buffer index by requested amount.
				fcbArray[fd].fcbBufIndex += count;

				//increase the file position by requested amount.
				fcbArray[fd].filePos += count;

				//increase the amount of bytes read.
				bytesRead += count;
				return (bytesRead);
			}
			else
			{	
				//space left in the file buffer.
				int remainingSpace = B_CHUNK_SIZE-fcbArray[fd].fcbBufIndex;

				memcpy(buffer + inputBufferPos, 
						fcbArray[fd].fcbBuf + fcbArray[fd].fcbBufIndex, remainingSpace);

				//reset the file buffer index back to 0.
				fcbArray[fd].fcbBufIndex = 0;

				//increase the file position by remaining space.
				fcbArray[fd].filePos += remainingSpace;

				//reduce count by the amount we read.
				count -= remainingSpace;

				// the buffer position
				inputBufferPos += remainingSpace;

				//increase the amount of bytes read.
				bytesRead += remainingSpace;
				fcbArray[fd].bptr += 1;
			}
		}
	}
}
	
// Interface to Close the file	
int b_close (b_io_fd fd)
{
	if (startup == 0) return (0); // If not init, why bother with it
	
	/*
		Case when file buffer is not empty and flags are not read and
		append.
	*/
	if((fcbArray[fd].fcbBufIndex != 0) && (fcbArray[fd].flag != 0)
			&& !(fcbArray[fd].flag & O_APPEND))
	{
		/*
			Relocate entry so it does not trample other data then write on disk
		*/
		fcbArray[fd].fi.location.offSet += 1;
		fileRelocate(&fcbArray[fd].fi, fcbArray[fd].ppinfo);
		/*
			Update the information in the file information block.
		*/
		LBAwrite(&fcbArray[fd].fi, 1, fcbArray[fd].fi.location.dirLocation);
		/*
			Write the remaining data from file buffer to disk
		*/
		LBAwrite(fcbArray[fd].fcbBuf, 1, 
			fcbArray[fd].fi.location.dirLocation + 1 + fcbArray[fd].bptr);
		
		fcbArray[fd].fcbBufIndex = 0; //set the index back to 0

		/*
			Update dateModified of the file
		*/
		time_t t = time(NULL);
		fcbArray[fd].fi.dateModified = t;

		/*
			Load up the directory of the file so we can update the information later.
		*/
		fcbArray[fd].ppinfo->parent[findEntryInDir(fcbArray[fd].ppinfo->parent,
							fcbArray[fd].ppinfo->lastElement)] = fcbArray[fd].fi;
	}

	/*
		Case for O_APPEND flag.
	*/
	if ((fcbArray[fd].flag & O_APPEND) && (fcbArray[fd].fcbBufIndex != 0))
	{
		/*
			Relocate entry so it does not trample other data then write on disk
		*/
		fcbArray[fd].fi.location.offSet += 1;
		fileRelocate(&fcbArray[fd].fi, fcbArray[fd].ppinfo);

		/*
			Update the information in the file information block.
		*/
		LBAwrite(&fcbArray[fd].fi, 1, fcbArray[fd].fi.location.dirLocation);

		/*
			Write the remaining data from file buffer to disk
		*/
		LBAwrite(fcbArray[fd].fcbBuf, 1, 
				fcbArray[fd].fi.location.dirLocation +  fcbArray[fd].bptr -1);
		fcbArray[fd].fcbBufIndex = 0;

		time_t t = time(NULL);
		fcbArray[fd].fi.dateModified = t;
		/*
			Load up the directory of the file so we can update the information later.
		*/

		fcbArray[fd].ppinfo->parent[findEntryInDir(fcbArray[fd].ppinfo->parent,
							fcbArray[fd].ppinfo->lastElement)] = fcbArray[fd].fi;
	}

	//update file information block
	LBAwrite(&fcbArray[fd].fi, 1, fcbArray[fd].fi.location.dirLocation);
	//update file information in directory
	LBAwrite(fcbArray[fd].ppinfo->parent, 
					fcbArray[fd].ppinfo->parent->location.offSet, 
						fcbArray[fd].ppinfo->parent->location.dirLocation);
	
	free(fcbArray[fd].fcbBuf); // Free the Buffer
	fcbArray[fd].fcbBuf = NULL; // Clean/mark as free
	free(fcbArray[fd].ppinfo);
	fcbArray[fd].ppinfo = NULL;

	return (0);
}
