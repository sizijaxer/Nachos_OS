// filesys.h
//	Data structures to represent the Nachos file system.
//
//	A file system is a set of files stored on disk, organized
//	into directories.  Operations on the file system have to
//	do with "naming" -- creating, opening, and deleting files,
//	given a textual file name.  Operations on an individual
//	"open" file (read, write, close) are to be found in the OpenFile
//	class (openfile.h).
//
//	We define two separate implementations of the file system.
//	The "STUB" version just re-defines the Nachos file system
//	operations as operations on the native UNIX file system on the machine
//	running the Nachos simulation.
//
//	The other version is a "real" file system, built on top of
//	a disk simulator.  The disk is simulated using the native UNIX
//	file system (in a file named "DISK").
//
//	In the "real" implementation, there are two key data structures used
//	in the file system.  There is a single "root" directory, listing
//	all of the files in the file system; unlike UNIX, the baseline
//	system does not provide a hierarchical directory structure.
//	In addition, there is a bitmap for allocating
//	disk sectors.  Both the root directory and the bitmap are themselves
//	stored as files in the Nachos file system -- this causes an interesting
//	bootstrap problem when the simulated disk is initialized.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef FS_H
#define FS_H

#include "copyright.h"
#include "sysdep.h"
#include "openfile.h"

typedef int OpenFileId;

#ifdef FILESYS_STUB // Temporarily implement file system calls as
// calls to UNIX, until the real file system
// implementation is available
class FileSystem
{
public:
	FileSystem()
	{
		for (int i = 0; i < 20; i++)
			fileDescriptorTable[i] = NULL;
	}

	bool Create(char *name)
	{
		int fileDescriptor = OpenForWrite(name);

		if (fileDescriptor == -1)return FALSE;
		Close(fileDescriptor);
		return TRUE;
	}

	OpenFile *Open(char *name)
	{
		int fileDescriptor = OpenForReadWrite(name, FALSE);

		if (fileDescriptor == -1)
			return NULL;
		return new OpenFile(fileDescriptor);
	}

	bool Remove(char *name) { return Unlink(name) == 0; }

	OpenFile *fileDescriptorTable[20];
};

#else // FILESYS ///MP4 只會看到下方 class
class FileSystem
{
public:
	FileSystem(bool format); // Initialize the file system.
							 // Must be called *after* "synchDisk"
							 // has been initialized.
							 // If "format", there is nothing on
							 // the disk, so initialize the directory
							 // and the bitmap of free blocks.
	// MP4 mod tag
	~FileSystem();

	bool Create(char *name, int initialSize);
	// Create a file (UNIX creat)

	OpenFile *Open(char *name); // Open a file (UNIX open)

	bool Remove(char *name, bool rr_flag); // Delete a file (UNIX unlink)

	void List(char *name, bool lr_flag); // List all the files in the file system

	void Print(); // List all the files and their contents



	OpenFile* fileDescriptorTable[20];

	int Close(int id){
		int index = id-1;
		if(index<0 || index>19)return 0; 
		if(fileDescriptorTable[index]==NULL)return 0;
		fileDescriptorTable[index] = 0;
		return 1;
	}

	int Read(char *buf, int len, int id){
		int index = id-1;
		if(index<0 || index>19)return -1; 
		if(fileDescriptorTable[index]==NULL)return -1;

		return fileDescriptorTable[index]->Read(buf, len);
	}

	int Write(char *buf, int len, int id){
		int index = id-1;
		
		if(index<0)return -1;
		if(index>19)return -1; 
		if(fileDescriptorTable[index]==NULL)return -1;

		return fileDescriptorTable[index]->Write(buf, len);
	}

	char* get_file_name(char* name){
		char* file_name = strrchr(name, '/');
		file_name++;
		return file_name;
	}
	char* get_dir_name(char* name){ ///MP4 
		int name_len = strlen(name)+1;
		char* tmp = new char[name_len];
		memset(tmp,0,name_len);
		memcpy(tmp, name, name_len);
		char* file_name = get_file_name(name);
		char* dir_name = strtok(name, "/");
		char *ans = new char[255];
		memset(ans,0,255);
		while(dir_name != file_name){
			memcpy(ans, dir_name, strlen(dir_name)+1);
			dir_name = strtok(NULL, "/");
		}
		memcpy(name, tmp, name_len);
		delete tmp;
		if(strlen(ans)==0)return NULL;
		return ans;
	}
	bool check_len(char* name){///MP4
		char* file_name = get_file_name(name);
		int file_len = strlen(file_name);
		int path_len = strlen(name);
		if(file_len>9)return false;///檔案名稱太長
		if(path_len>255)return false;///路徑名稱太長
		return true;
	}

	void create_directory(char *name);

private:
	OpenFile *freeMapFile;	 // Bit map of free disk blocks,
							 // represented as a file
	OpenFile *directoryFile; // "Root" directory -- list of
							 // file names, represented as a file
};

#endif // FILESYS

#endif // FS_H
