/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe, 
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: helperFunctions.h
*
* Description:Interface of helper functions
**************************************************************/



#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

#include "fsLow.h"
#include "fsFreeSpace.h"
#include "mfs.h"


#ifndef _HELPERFUNCTIONS_H
#define _HELPERFUNCTIONS_H

#define BLOCK_SIZE 512
#define MAX_NAME_LENGTH 64

typedef struct
{
    DirectoryEntry * parent;
    int index;
    char * lastElement;
} PPinfo;

DirLocation initDir(uint64_t initDE, DirectoryEntry * parent);
DirectoryEntry *loadDir(DirectoryEntry * dir);
DirectoryEntry loadFile(DirectoryEntry * dirEnt, int index);
int parsePath(char * pathName, PPinfo * ppi);
int findEntryInDir(DirectoryEntry * dirEnt, char *entryName);
int isDirectory(DirectoryEntry * dirEnt);
int isFile(DirectoryEntry * dirEnt, int index);
int setRootDirectory(DirLocation rdl);
int findFreeEntry(DirectoryEntry * dirEnt);
int isDirEmpty(DirectoryEntry * dirEnt);
int maxEntryChecker(DirectoryEntry * dirEnt);
int findDirIndex(DirectoryEntry * parent, DirectoryEntry * child);
int isUsedEntry(DirectoryEntry * dirEnt);
void markDEUnused(DirectoryEntry * dirEnt);
void markFileUnused(DirectoryEntry * parent, int index);
void freeDirectoryFromDE(DirectoryEntry * dirEnt);
void freeFileFromDE(DirectoryEntry * dirEnt, int index);
void setFileType(unsigned char * fileType, DirectoryEntry * dirEnt);
void fileRelocate(DirectoryEntry * dirEnt, PPinfo * ppinfo);
void getFileName(char * pathname);
int compareFileLocation(char *src, char *dest);
#endif