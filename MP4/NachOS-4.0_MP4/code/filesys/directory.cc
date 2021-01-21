// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#define NumDirEntries 64 ///MP4 一個dir 最多只能放64個檔案或dir
//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];

    // MP4 mod tag
    memset(table, 0, sizeof(DirectoryEntry) * size); // dummy operation to keep valgrind happy

    tableSize = size;
    for (int i = 0; i < tableSize; i++)
        table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse && !strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name, bool need_find_sub_dir)
{
    int i = FindIndex(name);

    if (i != -1)return table[i].sector;
    else{
        //cout<<"part3\n";
        if(need_find_sub_dir){
            int result = -1;
			for(int j=0; j<tableSize; j++){
				if(table[j].inUse && (table[j].type==DIR)){
					Directory* next_directory = new Directory(NumDirEntries);
					OpenFile* next_directory_files = new OpenFile(table[j].sector);
					next_directory->FetchFrom(next_directory_files);
					result = next_directory->Find(name, true);
					delete next_directory;
					delete next_directory_files;
				}
				if(result != -1){
					return result;
				}
			}
        }
    }
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, int fileType)
{
    if (FindIndex(name) != -1)
        return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse)
        {
            table[i].inUse = TRUE;
            strncpy(table[i].name, name, FileNameMaxLen);
            table[i].sector = newSector;
            table[i].type = fileType;///MP4 mod
            return TRUE;
        }
    return FALSE; // no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name, bool is_remove_file)
{
    int i = FindIndex(name);
    if(is_remove_file && table[i].type==DIR)return FALSE;
    
    if (i == -1)
        return FALSE; // name not in directory
    table[i].inUse = FALSE;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------
///MP4Mod
void Directory::List(int depth, bool lr_flag)
{
    for (int i = 0; i < tableSize; i++){
        if (table[i].inUse){
            
            if(table[i].type==FILE){
                for(int j=0;j<depth;j++)cout<<"   ";
                cout<<"[F] "<<table[i].name<<"\n";
            }
            else if(table[i].type==DIR){
                for(int j=0;j<depth;j++)cout<<"   ";
                cout<<"[D] "<<table[i].name<<"\n";
                if(lr_flag){
                    Directory* next_directory = new Directory(NumDirEntries);
                    OpenFile* next_directory_files = new OpenFile(table[i].sector);
                    next_directory->FetchFrom(next_directory_files);
                    int next_depth = depth+1;
                    next_directory->List(next_depth, true);
                }
            }
        }
    }
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
            hdr->FetchFrom(table[i].sector);
            hdr->Print();
        }
    printf("\n");
    delete hdr;
}

bool Directory::remove_all_object(PersistentBitmap* freeMap, OpenFile* delete_file){
	for(int i=0;i<tableSize;i++){
        if(table[i].inUse){
            if(table[i].type==DIR){
                ////delete inside dir first
                Directory* next_directory = new Directory(NumDirEntries);
                OpenFile* next_directory_files = new OpenFile(table[i].sector);
                next_directory->FetchFrom(next_directory_files);
                next_directory->remove_all_object(freeMap, next_directory_files);
                delete next_directory;
                delete next_directory_files;
            }

            FileHeader *hdr = new FileHeader;
            hdr->FetchFrom(table[i].sector);
            table[i].inUse = false;
            hdr->Deallocate(freeMap);
            freeMap->Clear(table[i].sector);
            delete hdr;
        }
    }
    this->WriteBack(delete_file);
}

