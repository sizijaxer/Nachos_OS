/**************************************************************
 *
 * userprog/ksyscall.h
 *
 * Kernel interface for systemcalls 
 *
 * by Marcus Voelp  (c) Universitaet Karlsruhe
 *
 **************************************************************/

#ifndef __USERPROG_KSYSCALL_H__
#define __USERPROG_KSYSCALL_H__

#include "kernel.h"

#include "synchconsole.h"

void SysHalt()
{
	kernel->interrupt->Halt();
}

int SysAdd(int op1, int op2)
{
	return op1 + op2;
}
int SysOpen(char* filename){
	OpenFile* open_file;
	open_file = kernel->fileSystem->Open(filename);
	int index = -1;
	for(int i=0;i<20;i++){
		if(kernel->fileSystem->fileDescriptorTable[i] == NULL){
			index = i;
			break;
		}
	}
	//cout<<"ava index: "<<index<<"\n";
	if(index==-1)return -1; ///full
	else kernel->fileSystem->fileDescriptorTable[index] = open_file;
	return index+1; //since need larger than 0 
}
int SysCreate(char *filename, int filesize)
{
	// return value
	// 1: success
	// 0: failed
	return kernel->fileSystem->Create(filename,filesize);
}
int SysWrite(char *buf, int len, int id){
	return kernel->fileSystem->Write(buf,len,id);
}

int SysRead(char *buf, int len, int id){
	return kernel->fileSystem->Read(buf,len,id);
}

int SysClose(int id){
	return kernel->fileSystem->Close(id);
}
#ifdef FILESYS_STUB
#endif

#endif /* ! __USERPROG_KSYSCALL_H__ */
