/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe, 
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: helperFunctions.c
*
* Description:This C file handles various helper functions
* such as: Parsing path, loading Directory, Initializing
*          Directory Initialization, etc...
**************************************************************/



#include "helperFunctions.h"
DirectoryEntry * currDir;
DirectoryEntry * rootDir;
DirectoryEntry * cwd;

/*
    Used the code given by Professor Bierman in
    one our classes. 

    This function initializes directories. 
    It takes in the number directory entries and
    parent directory. 

    Returns the start block of the Directory
*/

DirLocation initDir(uint64_t initDE, DirectoryEntry * parent)
{
    DirLocation rdl;

    //calculate the bytes neeeded
    //sizeof(dirEntry) = 112
    uint64_t bytesNeeded = initDE * sizeof(DirectoryEntry); 

    //calculate blocks needed
    //blocksNeeded = (m+(n-1))/n
    uint64_t blocksNeeded = (bytesNeeded + (BLOCK_SIZE-1))/BLOCK_SIZE;

    //check if we can fit more DirEntries
    bytesNeeded = blocksNeeded * BLOCK_SIZE;
    uint64_t actualDE = bytesNeeded / sizeof(DirectoryEntry);

    //allocate memory for DE
    DirectoryEntry * dir = malloc(bytesNeeded);
    if(dir == NULL) 
    {
        printf("Error allocating directory entry for iniializing directories\n");
        exit(1);
    }

    /*
        Allocate blocks from the free space system.
        Since free space is contigious, this function will 
        return a -1 if no contigious space are found. 

        Returns the starting block of the allocated blocks.
    */
    uint64_t startBlock = allocBlocks(blocksNeeded);
    
    /*
        Initialize the directory entries.
        Set each directory entries' name to NULL
        and set isUsed to 1 (Free).
    */
    for (uint64_t i = 0; i < actualDE; i++)
    {
        //Mark entry as unused
        dir[i].name[0] = '\0';
        dir[i].isUsed = FREE;
    }
    
    /*
        Set the first directory entry to the '.' directory.
    */
    strcpy(dir[0].name, ".");
    dir[0].size = actualDE * sizeof(DirectoryEntry);
    dir[0].location.dirLocation = startBlock;
    dir[0].location.offSet = blocksNeeded;
    dir[0].isDir = IS_DIR;
    dir[0].isUsed = USED;
    time_t t = time(NULL);
    dir[0].dateCreated = t;
    dir[0].dateModified = t;
    dir[0].lastAccessed = t;

    /*
        Pointer to a Directory entry. If parent is NULL, this means
        that the caller is initialized the root directory. If not null, 
        this means that the caller is initializing a directory with a parent.
    */
    DirectoryEntry * p;
    if(parent != NULL)
    {
        p = parent;
    }
    else
    {
        rootDir = dir;
        cwd = dir;
        p = &dir[0];
    }

    strcpy(dir[1].name, "..");
    dir[1].size = p->size;
    dir[1].location.dirLocation = p->location.dirLocation;
    dir[1].location.offSet = p->location.offSet;
    dir[1].isDir = p->isDir;
    dir[1].isUsed = p->isUsed;
    dir[1].dateCreated = p-> dateCreated;
    dir[1].dateModified = p-> dateModified;
    dir[1].lastAccessed = p-> lastAccessed;
    
    rdl.offSet = blocksNeeded;
    rdl.dirLocation = startBlock;
    currDir = dir;
    
    LBAwrite(dir, blocksNeeded, startBlock);

    free(dir);
    return rdl;
}

/*
    Parses the pathname given by the caller. 
    Stores the parent, index, and last element.
    The return value is the path validation.
    Return 0 if path is valid, else -1 or -2
    if path is invalid
*/
int parsePath (char * path, PPinfo * ppi)
{
    DirectoryEntry * startPath;
    DirectoryEntry * par;
    char * savePtr;
    char * token1;
    char * token2;

    if (path == NULL)
    {
        return -1;
    }

    if (ppi == NULL)
    {
        return -1;
    }

    if (path[0] == '/')
    {
        startPath = rootDir;
    }
    else
    {
        startPath = cwd;
    }
    par = startPath;

    token1 = strtok_r(path, "/", &savePtr);

    if (token1 == NULL)
    {
        if(strcmp(path, "/") == 0)
        {
            ppi -> parent = par;
            ppi -> index = -1;
            ppi -> lastElement = NULL;
            return 0;
        }
        return -1;
    }
    int index;
    while (token1 != NULL)
    {
        index = findEntryInDir(par, token1);
        token2 = strtok_r(NULL, "/", &savePtr);

        if (token2 == NULL)
        {
            ppi -> parent = par;
            ppi -> lastElement = token1;
            ppi -> index = index;
            return 0;
        }

        if(index == -1)
        {
            return -2;
        }

        if(!isDirectory(&par[index]))
        {
            return -2;
        }

        DirectoryEntry * temp = loadDir(&(par[index]));
        if(temp == NULL)
        {
            return -1;
        }

        if(par != startPath)
        {
            free(par);
        }
        
        par = temp;
        token1 = token2;
    }
}

/*
    Load the directory using LBARead() and 
    return it to the caller.
*/
DirectoryEntry * loadDir(DirectoryEntry * dirEnt) 
{
    if (dirEnt == NULL) 
    {
        return NULL; // Check for a valid pointer
    }

    DirLocation startBlock = dirEnt->location;
    DirectoryEntry * loadedDir = malloc(dirEnt->location.offSet*BLOCK_SIZE);

    if (loadedDir == NULL) 
    {
        printf("Error loading directory entry\n");
        exit(1);
    }
    LBAread(loadedDir, startBlock.offSet, startBlock.dirLocation);
    return loadedDir;
}

DirectoryEntry loadFile(DirectoryEntry * dirEnt, int index) 
{
    DirLocation startBlock = dirEnt[index].location;
    DirectoryEntry loadedDir;

    LBAread(&loadedDir, startBlock.offSet, startBlock.dirLocation);
    return loadedDir;
}

/*
    Finc the Directory Entry in the specified directory
*/
int findEntryInDir(DirectoryEntry * dirEnt, char * entryName)
{   
    for (int i = 0; i < dirEnt->size/sizeof(DirectoryEntry); i++) 
    {
        if (strcmp(dirEnt[i].name, entryName) == 0) 
        {
            return i;
        }
    }
    return -1; 
}

/*
    Check if a Directory Entry is a directory
*/
int isDirectory(DirectoryEntry * dirEnt) 
{
    return dirEnt->isDir == IS_DIR;
}

/*
    Check if a Directory Entry is a file
*/
int isFile(DirectoryEntry * dirEnt, int index) 
{
    return dirEnt[index].isDir == NOT_DIR;
}

/*
    Get the information of Root Directory
    and store it in memory. The information
    will be assigned to a global variable.
*/
int setRootDirectory(DirLocation rdl)
{
    rootDir = malloc(rdl.offSet*BLOCK_SIZE);
    LBAread(rootDir, rdl.offSet, rdl.dirLocation);
    cwd = rootDir;

    if (rootDir->location.dirLocation == 6)
    {
        return 0;
    }
    else
    {
        return -1;
    }
} 

/*
    Find an unused directory entry.
*/
int findFreeEntry(DirectoryEntry * dirEnt)
{
    for (int i = 0; i < dirEnt->size/sizeof(DirectoryEntry); i++) 
    {
        if (dirEnt[i].isUsed == FREE) 
        {
            return i;
        }
    }
    return -1; 
}

/*
    Check if a directory is empty. If empty,
    return 1, else return 0.
*/
int isDirEmpty(DirectoryEntry * dirEnt)
{
    for (uint64_t i = 2; i < dirEnt->size/sizeof(DirectoryEntry); i++)
    {
        if(dirEnt[i].isUsed == USED)
        {
            return 0;
        }
    }  
    return 1;
}

/*
    Release the blocks allocated in Free Space
    by Directory.
*/
void freeDirectoryFromDE(DirectoryEntry * dirEnt)
{
    releaseBlocks(dirEnt->location.dirLocation, dirEnt->location.offSet);
}

/*
    Release the blocks allocated in Free Space
    by File.
*/
void freeFileFromDE(DirectoryEntry * dirEnt, int index)
{
    releaseBlocks(dirEnt[index].location.dirLocation, dirEnt[index].location.offSet);
}

/*
    Set the Directory as unused.
*/
void markDEUnused(DirectoryEntry * dirEnt)
{
    for (u_int64_t i = 0; i < sizeof(dirEnt->name); i++)
    {
        dirEnt->name[i] = '\0';
    }
    
    dirEnt->isUsed = FREE;
}

/*
    Set the file as unused.
*/
void markFileUnused(DirectoryEntry * parent, int index)
{   
    for (u_int64_t i = 0; i < sizeof(parent->name); i++)
    {
        parent[index].name[i] = '\0';
    }
    parent[index].isUsed = FREE;
}

/*
    A function that check how much directory entries
    are in the directory. 
*/
int maxEntryChecker(DirectoryEntry * dirEnt)
{
    int numDE = 0;
    for (uint64_t i = 0; i < dirEnt->size/sizeof(DirectoryEntry); i++)
    {
        if(dirEnt[i].isUsed == USED)
        {
            numDE++;
        }
    }
    
    return numDE;
}

/*
    Find the entryIndex in the parent directory.
*/
int findDirIndex(DirectoryEntry * parent, DirectoryEntry * child)
{   
    if (parent == NULL) 
    {
        return -1; // Check for valid pointers
    }

    if (child == NULL)
    {
        return -1;
    }

    for (int i = 0; i < parent->size/sizeof(DirectoryEntry); i++) 
    {
        if(parent[i].location.dirLocation == child->location.dirLocation)
        {
            return i;
        }
    }
    return -1;
}

/*
    Check if an entry is used or not
*/
int isUsedEntry(DirectoryEntry * dirEnt)
{
    return dirEnt->isUsed == 0;
}

/*
    Sets the file type of an entry.
*/
void setFileType(unsigned char * fileType, DirectoryEntry * dirEnt)
{
    if (dirEnt->isDir == IS_DIR)
    {
        fileType = "Directory";
    }
    else
    {
        fileType = "File";
    }
}

/*
    A function that relocates the blocks of a file.
*/
void fileRelocate(DirectoryEntry * dirEnt, PPinfo * ppinfo)
{
    char * buffer = malloc(5000);
    if (buffer == NULL) 
    {
        printf("Malloc Failed");
        exit(-1);
    }
    LBAread(buffer, dirEnt->location.offSet, dirEnt->location.dirLocation);

    markDEUnused(dirEnt);
    LBAwrite(dirEnt, 1, dirEnt->location.dirLocation);

    releaseBlocks(dirEnt->location.dirLocation,dirEnt->location.offSet-1);

    int startBlock = allocBlocks(dirEnt->location.offSet);

    strcpy(dirEnt->name, ppinfo->lastElement);
    dirEnt->location.dirLocation = startBlock;
    dirEnt->isUsed = USED;
    LBAwrite(buffer, dirEnt->location.offSet, startBlock);
    free(buffer);
}

/*
   Get the entry name from path.
*/
void getFileName(char * path) 
{
    char pathname[MAX_NAME_LENGTH];
    strcpy(pathname, path);
    PPinfo * ppinfo = malloc(sizeof(PPinfo));
    if (ppinfo == NULL)
    {
        printf("Malloc Failed");
        exit(-1);
    }
    parsePath(pathname, ppinfo);
    strcpy(path, ppinfo->lastElement);
    free(ppinfo);
    ppinfo = NULL;
}

/*
    Compare the file location of two paths.
*/
int compareFileLocation(char *src, char *dest) {
    char srcName[MAX_NAME_LENGTH];
    char destName[MAX_NAME_LENGTH];
    strcpy(srcName, src);
    strcpy(destName, dest);
    PPinfo * srcInfo = malloc(sizeof(PPinfo));
    PPinfo * destInfo = malloc(sizeof(PPinfo));
    parsePath(srcName, srcInfo);
    parsePath(destName, destInfo);

    if(srcInfo->parent[srcInfo->index].location.dirLocation == destInfo->parent[destInfo->index].location.dirLocation) {
        free(srcInfo);
        free(destInfo);
        return 1;
    }
    return 0;
    
}