#include "syscall.h"

int main(void)
{
	char test[] = "abcdefghijklmnopqrstuvwxyz";
	int success= Create("file1.test");
	OpenFileId fid;

	int i;
	if (success != 1) MSG("Failed on creating file");
	fid = Open("file1.test");

	/* 10/15 測試 一次 open 超過 20 次*/
	/*for(i=0;i<21;i++){
		fid = Open("file1.test");
		if (fid < 0) MSG("Failed on opening file, exceed 20 files openned");
	}*/
	
	
	if (fid < 0) MSG("Failed on opening file");
	
	for (i = 0; i < 26; ++i) {
		int count = Write(test + i, 1, fid);
		if (count != 1) MSG("Failed on writing file");
	}
       
	success = Close(fid);
	if ( success != 1) MSG("Failed on closing file");
	MSG("Success on creating file1.test");
	Halt();
}

