/**************************************************************
* Class:  CSC-415-01 Fall 2023
* Names: Ivan Ramos, Edward Chen, Karl Layco, Alec Nagal
* Student IDs: 920520754, 920853606, 922536937, 918485625
* GitHub Name: acraniaaa, BDJslime-UnaverageJoe, 
*               BulbaWasTaken, itsmeAlec
* Group Name: Meta
* Project: Basic File System
*
* File: mfs.c
*
* Description: This C file handles various file system functions, 
* such as: mkdir, rmdir, etc...
**************************************************************/

#include "mfs.h"
#include "helperFunctions.h"

#define MAX_PATH_NAME 256

int fs_mkdir(const char * pathname, mode_t mode)
{
  char path[MAX_PATH_NAME];
  strncpy(path, pathname, sizeof(path));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) 
  {
    printf("Malloc Failed");
    exit(-1);
  }
  int parse = parsePath(path, ppinfo);
  /*
    Parse the path name. If path is valid, check if directory already exists.
    If not, check if parent directory is full. If not full, make the directory.
  */
  if(parse == 0)
  {
    if(ppinfo->index == -1)
    {
      if(maxEntryChecker(ppinfo->parent) < ppinfo->parent->size/sizeof(DirectoryEntry))
      {
        DirLocation dirLoc = initDir(50, ppinfo->parent);
        int frDE = findFreeEntry(ppinfo->parent);
        strcpy(ppinfo->parent[frDE].name, ppinfo->lastElement);
        ppinfo->parent[frDE].size = currDir[0].size;
        ppinfo->parent[frDE].location.dirLocation = dirLoc.dirLocation;
        ppinfo->parent[frDE].location.offSet = dirLoc.offSet;
        ppinfo->parent[frDE].isDir = IS_DIR;
        ppinfo->parent[frDE].isUsed = USED;
        ppinfo->parent[frDE].dateCreated = currDir[0].dateCreated;
        ppinfo->parent[frDE].dateModified = currDir[0].dateModified;
        ppinfo->parent[frDE].lastAccessed = currDir[0].lastAccessed;
        LBAwrite(ppinfo->parent, ppinfo->parent->location.offSet,
               ppinfo->parent->location.dirLocation);
        printf("Directory Successfully Created\n");
      }
      else
      {
        printf("md Failed: Directory is full\n");
        return -1;
      }
      
    }
    else
    {
      printf("md Failed: Directory already exists\n");
    }
  }
  else
  {
    printf("md Failed: Invalid Path\n");
  }

  free(ppinfo);
  ppinfo = NULL;
  return parse;
}

int fs_rmdir(const char *pathname)
{
  char path[MAX_PATH_NAME];
  strncpy(path, pathname, sizeof(path));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) 
  {
    printf("Malloc Failed");
    exit(-1);
  }
  int parse = parsePath(path, ppinfo);

  /*
    Check if directory exists.
  */
  if(ppinfo->index == -1)
  {
    printf("rm Failed: Invalid Path\n");
    return -1;
  }
  if(ppinfo->index < 2)
  {
    printf("rm Failed: Invalid Path\n");
    return -1;
  }
  
  /*
    Check if last element of the path is a directory
  */
  if(!isDirectory(&ppinfo->parent[ppinfo->index]))
  {
    printf("rm Failed: Last Element is not a directory\n");
    return -1;
  }

  /*
    Check if directory is empty
  */
  if (!isDirEmpty(loadDir(&ppinfo->parent[ppinfo->index])))
  {
    printf("rm Failed: Directory is not empty\n");
    return -1;
  }

  /*
    Release the blocks back to free space.
  */
  freeDirectoryFromDE(&ppinfo->parent[ppinfo->index]);

  /*
    Mark directory as unused.
  */
  markDEUnused(&ppinfo->parent[ppinfo->index]);
  /*
    Write the updated information of the parent directory to disk
  */
  LBAwrite(ppinfo->parent, ppinfo->parent->location.offSet,
               ppinfo->parent->location.dirLocation);

  free(ppinfo);
  ppinfo = NULL;

  return 0;
}

fdDir * fs_opendir(const char *pathname)
{
  char path[MAX_PATH_NAME];
  strncpy(path, pathname, sizeof(path));
  
  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) 
  {
    printf("Malloc Failed");
    exit(-1);
  }

  fdDir * dir = malloc(sizeof(fdDir));
  if (dir == NULL)
  {
    printf("Malloc Failed");
    exit(-1);
  }

  int parse = parsePath(path, ppinfo);
  if(parse == 0)
  {
    if(ppinfo->index == -1)
    {
      /*
        A case when the parent is root directory and index is -1.
      */
      dir->d_reclen = sizeof(struct fs_diriteminfo);
      dir->dirEntryPosition = 0;
      dir->dirEnt = loadDir(&ppinfo->parent[findDirIndex(ppinfo->parent, rootDir)]);
      dir->di= malloc(sizeof(struct fs_diriteminfo));
    }
    else
    {
      dir->d_reclen = sizeof(struct fs_diriteminfo);
      dir->dirEntryPosition = 0;
      dir->dirEnt = loadDir(&ppinfo->parent[ppinfo->index]);
      dir->di= malloc(sizeof(struct fs_diriteminfo));
    }
  }
  else
  {
    printf("fs_opendir Failed: Invalid Path\n");
  }
  
  free(ppinfo);
  return dir;
}

struct fs_diriteminfo* fs_readdir(fdDir *dirp) 
{
  int entries = dirp->dirEnt->size / sizeof(DirectoryEntry);
  for (int i = dirp->dirEntryPosition; i < entries; i++)
  {
    if(isUsedEntry(&(dirp->dirEnt[i])) == 1)
    {
      strcpy(dirp->di->d_name, dirp->dirEnt[i].name);
      setFileType(&dirp->di->fileType, &(dirp->dirEnt[i]));
      dirp->dirEntryPosition = i+1;
      return dirp->di;
    }
  }
  return NULL;
}

int fs_closedir(fdDir *dirp)
{
  time_t t = time(NULL);
  dirp->dirEnt->lastAccessed = t;
  free(dirp->dirEnt);
  free(dirp);
  return 0;
}

char * fs_getcwd(char *pathname, size_t size) 
{
  DirectoryEntry * temp1 = cwd;
  DirectoryEntry * temp2;

  char * tempBuffer = malloc(size);
  if (tempBuffer == NULL) {
    printf("Memory Allocation Error.\n");
    exit(-1);
  }
  strcpy(tempBuffer, ""); // Makes sure tempBuffer is empty

  char * tempPath = malloc(size);

  if (tempPath == NULL) {
    printf("Memory Allocation Error.\n");
    exit(-1);
  }
  strcpy(tempPath, ""); // Makes sure tempPath is empty

  /*
    Check if current working directory is root directory.
    If so, return '/', else traverse through the parents.
  */
  if(temp1->location.dirLocation == rootDir->location.dirLocation)
  {
    strcpy(tempPath, "/");
  }
  else
  {
    /*
      While root directory has not been reached, load the parent
      directory and append the name of the current entry. 
      Set the current entry to parent then loop again. 
    */
    while(temp1->location.dirLocation != rootDir->location.dirLocation)
    {
      temp2=loadDir(&temp1[1]);
      strcpy(tempBuffer, "/");
      strcat(tempBuffer, temp2[findDirIndex(temp2, temp1)].name);
      strcat(tempBuffer, tempPath);
      strcpy(tempPath, tempBuffer);
      temp1 = temp2;
    }
  }
  /*
    This makes sure pathname is never over the size.
  */
  strncpy(pathname, tempPath, size);
  free(tempBuffer);
  free(tempPath);
  return pathname;
}

int fs_setcwd(char *pathname) 
{
  // given a pathname, set the cwd to that directory
  char path[MAX_PATH_NAME];
  strncpy(path, pathname, sizeof(path));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) {
    printf("Memory Allocation Error.\n");
    exit(-1);
  }

  int parse = parsePath(path, ppinfo);
  if(parse == 0)
  {
    if(ppinfo->index == -1)
    {
      printf("cd Failed: Invalid Path\n");
      free(ppinfo);
      ppinfo = NULL;
      return -1;
    }

    if(!isDirectory(&ppinfo->parent[ppinfo->index]))
    {
      printf("Error: Last Element is not a directory\n");
      free(ppinfo);
      ppinfo = NULL;
      return -1;
    }

    cwd = loadDir(&ppinfo->parent[ppinfo->index]);

    free(ppinfo);
    ppinfo = NULL;
    
    return 0;
  }
  else
  {
    printf("Error: Invalid Path\n");
    free(ppinfo);
    ppinfo = NULL;
    return -1;
  }
}

int fs_isFile(char * filename)
{
  /*
    Check if the given filename is a file.
  */
  char path[MAX_PATH_NAME];
  strncpy(path, filename, sizeof(path));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) 
  {
    printf("Memory Allocation Error.\n");
    exit(-1);
  }

  int parse = parsePath(path, ppinfo);
  //if file does not exist
  if (ppinfo->index == -1)
	{
		return -2;
	}
  int returnVal = isFile(ppinfo->parent, ppinfo->index);
  
  free(ppinfo);
  ppinfo = NULL;
  return returnVal;
}

int fs_isDir(char * pathname)
{
  /*
    Check if the given pathname is a directory.
  */
  char path[MAX_PATH_NAME];
  strncpy(path, pathname, sizeof(path));
  PPinfo * ppinfo = malloc(sizeof(PPinfo));  
  if (ppinfo == NULL) 
  {
    printf("Memory Allocation Error.\n");
    return -1;
  }
  int returnVal;
  int parse = parsePath(path, ppinfo);
  
  //if directory does not exist
  if ((ppinfo->index == -1) && (strcmp(path, "/")==0))
	{
		returnVal = isDirectory(ppinfo->parent);
	}
  else
  {
    returnVal = isDirectory(&ppinfo->parent[ppinfo->index]);
  }
  
  free(ppinfo);
  ppinfo = NULL;
  return returnVal;
}

int fs_delete(char* filename)
{
  char path[MAX_PATH_NAME];
  strncpy(path, filename, sizeof(path));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
  if (ppinfo == NULL) {
    printf("Memory Allocation Error.\n");
    return -1;
  }
  
  int parse = parsePath(path, ppinfo);

  if(ppinfo->index == -1)
  {
    printf("Error: File does not exist");
    return -1;
  }

  if(!isFile(ppinfo->parent, ppinfo->index))
  {
    printf("Error: Last Element is not a file\n");
    return -1;
  }
  /*
    Load the directory, release the blocks back to the free space, mark
    directory entry as unused, then update the parent directory and the
    file entry then write on disk. 
  */
  DirectoryEntry entry = ppinfo->parent[ppinfo->index];
  freeFileFromDE(ppinfo->parent, ppinfo->index);
  markFileUnused(ppinfo->parent, ppinfo->index);
  strcpy(entry.name, ppinfo->parent[ppinfo->index].name);
  entry.isUsed = ppinfo->parent[ppinfo->index].isUsed;

  //Update the file information block
  LBAwrite(&entry, 1,
              entry.location.dirLocation);

  //Update the parent directory
  LBAwrite(ppinfo->parent, ppinfo->parent->location.offSet, 
              ppinfo->parent->location.dirLocation);
  free(ppinfo);
  ppinfo = NULL;
  return parse;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
  char pathname[MAX_PATH_NAME];
  strncpy(pathname, path, sizeof(pathname));

  PPinfo * ppinfo = malloc(sizeof(PPinfo));
 if (ppinfo == NULL) {
    printf("Memory Allocation Error.\n");
    return -1;
  }
  
  int parse = parsePath(pathname, ppinfo);

  if(ppinfo->index == -1)
  {
    printf("Error: Directory entry does not exist\n");
    return -1;
  }

  if (parse == 0)
  {
    /*
      Copy the information of the directory entry to the buffer.
    */
    strcpy(buf->st_name, ppinfo->parent[ppinfo->index].name);
    buf->st_size = ppinfo->parent[ppinfo->index].size;
    buf->st_blksize = BLOCK_SIZE;
    buf->st_blocks = ppinfo->parent[ppinfo->index].location.offSet;
    buf->st_accesstime = ppinfo->parent[ppinfo->index].lastAccessed;
    buf->st_modtime = ppinfo->parent[ppinfo->index].dateModified;
    buf->st_createtime = ppinfo->parent[ppinfo->index].dateCreated;
  }
  else
  {
    printf("Error: Invalid Path.\n");
  }
  free(ppinfo);
  ppinfo = NULL;
  return 0;
}

