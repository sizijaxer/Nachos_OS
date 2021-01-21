// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "filehdr.h"
#include "debug.h"
#include "synchdisk.h"
#include "main.h"

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::FileHeader
//	There is no need to initialize a fileheader,
//	since all the information should be initialized by Allocate or FetchFrom.
//	The purpose of this function is to keep valgrind happy.
//----------------------------------------------------------------------
FileHeader::FileHeader()
{
	next_hdf = NULL;///MP4 bo
	next_hdf_sector = -1;///MP4 bo

	numBytes = -1;
	numSectors = -1;
	memset(dataSectors, -1, sizeof(dataSectors));
}

//----------------------------------------------------------------------
// MP4 mod tag
// FileHeader::~FileHeader
//	Currently, there is not need to do anything in destructor function.
//	However, if you decide to add some "in-core" data in header
//	Always remember to deallocate their space or you will leak memory
//----------------------------------------------------------------------
FileHeader::~FileHeader()
{
	// nothing to do now
	if(next_hdf!=NULL) delete next_hdf;
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
///MP4 mod
bool FileHeader::Allocate(PersistentBitmap *freeMap, int fileSize) ///MP4 mod: indexed allocation 
{	
	/*=====================indexed Alocation===========================*/
	/*cout<<"max sectors: "<<NumSectors<<"\n";
	numBytes = fileSize;
	numSectors = divRoundUp(fileSize, SectorSize); ///切分成一個一個 sectors
	cout<<"need: "<<numSectors<<" sectors\n";
	if (freeMap->NumClear() < numSectors){
		return FALSE; // not enough space
	}
	/*for (int i = 0; i < numSectors; i++)
	{
		dataSectors[i] = freeMap->FindAndSet();
		// since we checked that there was enough free space,
		// we expect this to succeed
		ASSERT(dataSectors[i] >= 0);
	}*/
	/*=====================linked-index===========================*/
	if(fileSize<MaxFileSize)numBytes = fileSize;
	else numBytes = MaxFileSize;
	int remain_file_size = fileSize - numBytes;
	numSectors = divRoundUp(numBytes,SectorSize);

	if(freeMap->NumClear()<numSectors)return false;

	for(int i=0;i<numSectors;i++){
		dataSectors[i] = freeMap->FindAndSet();
		ASSERT(dataSectors[i] >= 0);
	}
	if(remain_file_size>0){///need next hdf
		next_hdf_sector = freeMap->FindAndSet();
		if(next_hdf_sector==-1)return false;///not enough space
		next_hdf = new FileHeader;
		return next_hdf->Allocate(freeMap,remain_file_size);
	}
	return true;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
///MP4 mod
void FileHeader::Deallocate(PersistentBitmap *freeMap)
{
	for (int i = 0; i < numSectors; i++) {
      	ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
      	freeMap->Clear((int)dataSectors[i]);
  	}
  	if (next_hdf_sector != -1){
 		ASSERT(next_hdf != NULL);
 		next_hdf->Deallocate(freeMap);
  	}
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------
///MP4
void FileHeader::FetchFrom(int sector)
{
	kernel->synchDisk->ReadSector(sector, ((char *)this) + sizeof(FileHeader*));

	/*
		MP4 Hint:
		After you add some in-core informations, you will need to rebuild the header's structure
	*/

	if(next_hdf_sector!=-1){
		next_hdf = new FileHeader;
		next_hdf->FetchFrom(next_hdf_sector);
	}
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------
///MP4 mod
void FileHeader::WriteBack(int sector)
{
	kernel->synchDisk->WriteSector(sector, ((char *)this) + sizeof(FileHeader*));

	/*
		MP4 Hint:
		After you add some in-core informations, you may not want to write all fields into disk.
		Use this instead:
		char buf[SectorSize];
		memcpy(buf + offset, &dataToBeWritten, sizeof(dataToBeWritten));
		...
	*/
	if(next_hdf_sector!=-1){
		ASSERT(next_hdf != NULL);
		next_hdf->WriteBack(next_hdf_sector);
	}
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
///MP4 mod
int FileHeader::ByteToSector(int offset)
{
	int index = offset / SectorSize;
  	if (index < NumDirect)
  		 return (dataSectors[index]);
  	else
  	{
  		 ASSERT(next_hdf != NULL);
  		 return next_hdf->ByteToSector(offset - MaxFileSize);
  	}
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
	return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", cal_file_size());
    cout<<"file header size: "<<numBytes<<"\n";
    for (i = 0; i < numSectors; i++)
    	printf("%d ", dataSectors[i]);
    cout<<"\n\n";
    /*printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++) {
	    kernel->synchDisk->ReadSector(dataSectors[i], data);
      for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		    printf("%c", data[j]);
      else
		    printf("\\%x", (unsigned char)data[j]);
	    }
      printf("\n"); 
    }*/

	
    delete [] data;
}

int FileHeader::cal_file_size(){
	int result = 0;
    FileHeader* next_hdf = this;
    while (next_hdf != NULL)
    {
        result += next_hdf->FileLength();
        next_hdf = next_hdf->get_next_hdf();
    }
    return result;
}
