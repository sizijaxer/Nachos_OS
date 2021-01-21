// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.
#ifndef FILESYS_STUB

#include "copyright.h"
#include "debug.h"
#include "disk.h"
#include "pbitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		64	// MP4 
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format)
{ 
	DEBUG(dbgFile, "Initializing the file system.");
	if (format) {
		PersistentBitmap *freeMap = new PersistentBitmap(NumSectors);
		Directory *directory = new Directory(NumDirEntries);
		FileHeader *mapHdr = new FileHeader;
		FileHeader *dirHdr = new FileHeader;

		DEBUG(dbgFile, "Formatting the file system.");

		// First, allocate space for FileHeaders for the directory and bitmap
		// (make sure no one else grabs these!)
		freeMap->Mark(FreeMapSector);	    
		freeMap->Mark(DirectorySector);

		// Second, allocate space for the data blocks containing the contents
		// of the directory and bitmap files.  There better be enough space!

		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
		ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

		// Flush the bitmap and directory FileHeaders back to disk
		// We need to do this before we can "Open" the file, since open
		// reads the file header off of disk (and currently the disk has garbage
		// on it!).

		DEBUG(dbgFile, "Writing headers back to disk.");
		mapHdr->WriteBack(FreeMapSector);    
		dirHdr->WriteBack(DirectorySector);

		// OK to open the bitmap and directory files now
		// The file system operations assume these two files are left open
		// while Nachos is running.

		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
	 
		// Once we have the files "open", we can write the initial version
		// of each file back to disk.  The directory at this point is completely
		// empty; but the bitmap has been changed to reflect the fact that
		// sectors on the disk have been allocated for the file headers and
		// to hold the file data for the directory and bitmap.

		DEBUG(dbgFile, "Writing bitmap and directory back to disk.");
		freeMap->WriteBack(freeMapFile);	 // flush changes to disk
		directory->WriteBack(directoryFile);

		if (debug->IsEnabled('f')) {
			freeMap->Print();
			directory->Print();
		}
		delete freeMap; 
		delete directory; 
		delete mapHdr; 
		delete dirHdr;
	} else {
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
		freeMapFile = new OpenFile(FreeMapSector);
		directoryFile = new OpenFile(DirectorySector);
	}
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileSystem::~FileSystem
//----------------------------------------------------------------------
FileSystem::~FileSystem()
{
	delete freeMapFile;
	delete directoryFile;
}

//----------------------------------------------------------------------
// FileSystem::Create
//  MP4 MODIFIED
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------
bool FileSystem::Create(char *name, int initialSize)
{
	if(!check_len(name))return false;
    
	Directory *directory;
	OpenFile *directory_file;///MP4 mod 
	PersistentBitmap *freeMap;
	FileHeader *hdr;
	int sector;
	bool success;

	DEBUG(dbgFile, "Creating file " << name << " size " << initialSize);

	char* file_name = get_file_name(name);
	char* dir_name = get_dir_name(name);
	
	// Default put in root
	directory = new Directory(NumDirEntries);
	directory_file = directoryFile;

	// 不放在root, 切到目標dir
	if(dir_name!=NULL){
		Directory* root_dir = new Directory(NumDirEntries);
		root_dir->FetchFrom(directoryFile);

		sector = root_dir->Find(dir_name, true);
		if(sector == -1)return false; ///dir 不存在
		
		directory_file = new OpenFile(sector);	
	}

	directory->FetchFrom(directory_file);
	

	if (directory->Find(file_name, false) != -1){
		success = FALSE;			// file is already in directory
	}
	else {	///create!
		freeMap = new PersistentBitmap(freeMapFile,NumSectors);
		sector = freeMap->FindAndSet();	// find a sector to hold the file header
		if (sector == -1) {
			success = FALSE;		// no free block for file header 
		}		
		else if (!directory->Add(file_name, sector, FILE)){
			success = FALSE;	// no space in directory
		}
		else {
			hdr = new FileHeader;
			if (!hdr->Allocate(freeMap, initialSize)){
				success = FALSE;	// no space on disk for data
			}
			else {	
				success = TRUE;
				// everthing worked, flush all changes back to disk
				hdr->WriteBack(sector);
				directory->WriteBack(directory_file);
				freeMap->WriteBack(freeMapFile);
				DEBUG(dbgFile, "File Created Success");
			}
			delete hdr;
		}
		delete freeMap;
	}
	delete directory;
	return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile* FileSystem::Open(char *name){ 
	
	Directory *directory = new Directory(NumDirEntries);
	OpenFile *openFile = NULL;
	int sector;

	char* file_name = get_file_name(name);
	char* dir_name = get_dir_name(name);

	DEBUG(dbgFile, "Opening file" << name);
	
	directory->FetchFrom(directoryFile);
	if(dir_name != NULL){//如果不再root下，在更深的
		sector = directory->Find(dir_name, true);
		OpenFile *directory_file = new OpenFile(sector);
		directory->FetchFrom(directory_file);
		
	}

	sector = directory->Find(file_name, false); 
	if (sector >= 0){
		openFile = new OpenFile(sector);	// name was found in directory 
	}
	delete directory;
	return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
//	MP4 MODIFIED
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name, bool rr_flag)
{
	if(!check_len(name))return false;
    Directory *directory;
    PersistentBitmap *freeMap;
    FileHeader *fileHdr;
    int sector;

    char* file_name = get_file_name(name);
    char* dir_name = get_dir_name(name);
    
    directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    OpenFile* delete_file = directoryFile;

    if(rr_flag){
        freeMap = new PersistentBitmap(freeMapFile,NumSectors);
        sector = directory->Find(file_name,true);  
        if (sector == -1){
            delete directory;
            return FALSE; // file not found
        }
        Directory* tmp_dir = new Directory(NumDirEntries);
        OpenFile* tmp_dir_files = directoryFile;
        tmp_dir->FetchFrom(directoryFile);

        if(dir_name!=NULL){////要刪除的不在root那一層，在更深的
            delete_file = new OpenFile(directory->Find(dir_name,true));
            directory->FetchFrom(tmp_dir_files);
        }

        delete_file = new OpenFile(sector);
        directory->FetchFrom(delete_file);
        //directory->List(0,true);
        directory->remove_all_object(freeMap,delete_file);
        

        fileHdr = new FileHeader;
        fileHdr->FetchFrom(sector);

        fileHdr->Deallocate(freeMap); // remove data blocks
        freeMap->Clear(sector);       // remove header block
        
        //cout<<"delete"<<file_name<<"\n";
        //tmp_dir->List(0,true);

        /*tmp_dir->FetchFrom(fi);
        tmp_dir->List(0,true);*/
        tmp_dir->Remove(file_name,false);

        freeMap->WriteBack(freeMapFile);     // flush to disk
        tmp_dir->WriteBack(tmp_dir_files); // flush to disk
        //tmp_dir->List(0,true);


        ////記得刪除目標dir
        directory = new Directory(NumDirEntries);
        directory->FetchFrom(directoryFile);
        delete_file = directoryFile;
        if(dir_name!=NULL){////要刪除的不在root那一層，在更深的
            //cout<<"next want to delete not in root\n";
            delete_file = new OpenFile(directory->Find(dir_name,true));
            directory->FetchFrom(delete_file);
        }
        int fi = tmp_dir->Find(file_name,true);
        //cout<<fi<<"\n";
        sector = fi;
        
        freeMap->Clear(sector);       // remove header block
        directory->Remove(file_name,false);

        freeMap->WriteBack(freeMapFile);     // flush to disk
        directory->WriteBack(delete_file); // flush to disk


    }
    else{
        //cout<<"just can delete a file\n";
        if(dir_name!=NULL){////要刪除的不在root那一層，在更深的
            //cout<<"want to delete not in root\n";
            delete_file = new OpenFile(directory->Find(dir_name,true));
            directory->FetchFrom(delete_file);
        }

        sector = directory->Find(file_name,false);  
        if (sector == -1){
            delete directory;
            return FALSE; // file not found
        }
        fileHdr = new FileHeader;
        fileHdr->FetchFrom(sector);

        freeMap = new PersistentBitmap(freeMapFile, NumSectors);

        fileHdr->Deallocate(freeMap); // remove data blocks
        freeMap->Clear(sector);       // remove header block
        bool valid = directory->Remove(file_name,true);
        if(valid){ ///要求只能刪除檔案
            freeMap->WriteBack(freeMapFile);     // flush to disk
            directory->WriteBack(delete_file); // flush to disk
        }
    }

    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
}

///MP4 mod
void FileSystem::List(char *name, bool lr_flag)
{
	if(!check_len(name))return;
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    int path_len = strlen(name);
    if(path_len<=1){
        int depth = 0;
        directory->List(depth,lr_flag);
    }
    else if(path_len>1){///not  root
        char* target_dir = get_file_name(name); ///last is dir
        int dir_file_sector = directory->Find(target_dir,true);
        OpenFile* next_directory_files = new OpenFile(dir_file_sector);
        Directory* next_directory = new Directory(NumDirEntries);
        next_directory->FetchFrom(next_directory_files);
        int depth = 0;
        next_directory->List(0,lr_flag);
    }
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void
FileSystem::Print()
{
	FileHeader *bitHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	PersistentBitmap *freeMap = new PersistentBitmap(freeMapFile,NumSectors);
	Directory *directory = new Directory(NumDirEntries);

	printf("Bit map file header:\n");
	bitHdr->FetchFrom(FreeMapSector);
	bitHdr->Print();

	printf("Directory file header:\n");
	dirHdr->FetchFrom(DirectorySector);
	dirHdr->Print();

	freeMap->Print();

	directory->FetchFrom(directoryFile);
	directory->Print();

	delete bitHdr;
	delete dirHdr;
	delete freeMap;
	delete directory;
} 

///MP4 mod
void FileSystem::create_directory(char *name){

    if(!check_len(name))return;

    char* file_name = get_file_name(name);
    char* dir_name = get_dir_name(name);
    ///找free block，並初始化
    PersistentBitmap* freeMap = new PersistentBitmap(freeMapFile,NumSectors);
    OpenFile* freeMapFile = new OpenFile(FreeMapSector);

    FileHeader* hdr = new FileHeader;
    hdr->Allocate(freeMap, DirectoryFileSize);

    Directory* directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);

    ///要創建的新資料夾
    Directory* inside_directory = new Directory(NumDirEntries);

    if(dir_name!=NULL){
        int dir_file_sector = directory->Find(dir_name, true);
        if(dir_file_sector==-1)return;///沒找到
        Directory* current_directory = new Directory(NumDirEntries);
        OpenFile* current_directory_files = new OpenFile(dir_file_sector);
        current_directory->FetchFrom(current_directory_files);

        int sector = freeMap->FindAndSet();
        hdr->WriteBack(sector);

        OpenFile* inside_directory_file = new OpenFile(sector);
        inside_directory->WriteBack(inside_directory_file);

        current_directory->Add(file_name, sector, DIR);
        current_directory->WriteBack(current_directory_files);
    }
    else{//放在root
        int sector = freeMap->FindAndSet();
		if(sector==-1)return;///沒找到
        hdr->WriteBack(sector);
        OpenFile* inside_directory_file = new OpenFile(sector);
        inside_directory->WriteBack(inside_directory_file);
        directory->Add(file_name, sector, DIR);
        directory->WriteBack(directoryFile);
    }

    freeMap->WriteBack(freeMapFile);
	delete directory;
    delete inside_directory;
    delete freeMapFile;
	delete freeMap;
	delete hdr;

}
#endif // FILESYS_STUB