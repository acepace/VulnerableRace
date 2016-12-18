#include "stdafx.h"

#define INFO_BUFFER_SIZE 32767

int main()
{

	TCHAR  infoBuf[INFO_BUFFER_SIZE] = { 0 };
	DWORD  bufCharCount = INFO_BUFFER_SIZE;

	_tprintf(TEXT("Attack the program!\n"));
	_tprintf(TEXT("If you see this, The program successfully ran!\n"));
	
	//Print running details, copy this as a sample
	if (!GetUserName(infoBuf, &bufCharCount))
	{
		printf("Failed GetUserName\n");
	}
	_tprintf(TEXT("Current username is %s"), infoBuf);

	_tprintf(TEXT("Will now wait for 1 minute times"));

	Sleep(1000 * 60);

    return 0;
}

