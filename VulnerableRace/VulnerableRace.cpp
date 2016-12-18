#include "stdafx.h"
#include "resource.h"


#define TEMP_FILE_TEMPLATE "att"
#define TEMP_DIR_NAME "C:\\Documents and Settings\\Public"

#define SVCNAME TEXT("vulnerableService")

SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;




//Housekeeping functions

//
// Purpose: 
//   Sets the current service status and reports it to the SCM.
//
// Parameters:
//   dwCurrentState - The current state (see SERVICE_STATUS)
//   dwWin32ExitCode - The system error code
//   dwWaitHint - Estimated time for pending operation, 
//     in milliseconds
// 
// Return value:
//   None
//
VOID ReportSvcStatus(DWORD dwCurrentState,
	DWORD dwWin32ExitCode,
	DWORD dwWaitHint)
{
	static DWORD dwCheckPoint = 1;

	// Fill in the SERVICE_STATUS structure.

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;

	if (dwCurrentState == SERVICE_START_PENDING)
		gSvcStatus.dwControlsAccepted = 0;
	else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

	if ((dwCurrentState == SERVICE_RUNNING) ||
		(dwCurrentState == SERVICE_STOPPED))
		gSvcStatus.dwCheckPoint = 0;
	else gSvcStatus.dwCheckPoint = dwCheckPoint++;

	// Report the status of the service to the SCM.
	SetServiceStatus(gSvcStatusHandle, &gSvcStatus);
}

VOID WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	// Handle the requested control code. 

	switch (dwCtrl)
	{
	case SERVICE_CONTROL_STOP:
		ReportSvcStatus(SERVICE_STOP_PENDING, NO_ERROR, 0);

		// Signal the service to stop.

		SetEvent(ghSvcStopEvent);
		ReportSvcStatus(gSvcStatus.dwCurrentState, NO_ERROR, 0);

		return;

	case SERVICE_CONTROL_INTERROGATE:
		break;

	default:
		break;
	}

}

//
// Purpose: 
//   Logs messages to the event log
//
// Parameters:
//   szFunction - name of function that failed
// 
// Return value:
//   None
//
// Remarks:
//   The service must have an entry in the Application event log.
//
VOID SvcReportEvent(LPTSTR szFunction)
{
	HANDLE hEventSource;
	LPCTSTR lpszStrings[2];
	TCHAR Buffer[80];

	hEventSource = RegisterEventSource(NULL, SVCNAME);

	if (NULL != hEventSource)
	{
		StringCchPrintf(Buffer, 80, TEXT("%s failed with %d"), szFunction, GetLastError());

		lpszStrings[0] = SVCNAME;
		lpszStrings[1] = Buffer;

		ReportEvent(hEventSource,        // event log handle
			EVENTLOG_ERROR_TYPE, // event type
			0,                   // event category
			4150,           // event identifier
			NULL,                // no security identifier
			2,                   // size of lpszStrings array
			0,                   // no binary data
			lpszStrings,         // array of strings
			NULL);               // no binary data

		DeregisterEventSource(hEventSource);
	}
}



bool startupService() {
	// Register the handler function for the service

	gSvcStatusHandle = RegisterServiceCtrlHandler(
		SVCNAME,
		SvcCtrlHandler);

	if (!gSvcStatusHandle)
	{
		SvcReportEvent(TEXT("RegisterServiceCtrlHandler"));
		return false;
}	

	// These SERVICE_STATUS members remain as set here

	gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	gSvcStatus.dwServiceSpecificExitCode = 0;

	// Report initial status to the SCM

	ReportSvcStatus(SERVICE_START_PENDING, NO_ERROR, 3000);
	return true;
}

int SvcInit() {
	DWORD dwRetVal = 0;
	UINT uRetVal = 0;
	BOOL fSuccess = FALSE;

	//Temp Files
	TCHAR lpTempPathBuffer[MAX_PATH] = { 0 };
	TCHAR szTempFileName[MAX_PATH] = { 0 };
	HANDLE hTempFile = INVALID_HANDLE_VALUE;


	//Resource variables
	HRSRC resourceExecutable = NULL;
	DWORD cbResourceSize = 0;
	HGLOBAL hResourceData = NULL;
	void* pResourceData = NULL;
	DWORD dwBytesWritten = 0;

	//Process execution
	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = { 0 };


	//Load resource
	resourceExecutable = FindResource(NULL, MAKEINTRESOURCE(101), RT_RCDATA);
	cbResourceSize = SizeofResource(NULL, resourceExecutable);
	hResourceData = LoadResource(NULL, resourceExecutable);
	pResourceData = LockResource(hResourceData);


	//  Generates a temporary file name. 
	uRetVal = GetTempFileName(TEXT(TEMP_DIR_NAME), // directory for tmp files
		TEXT(TEMP_FILE_TEMPLATE),     // temp file name prefix 
		0,                // create unique name 
		szTempFileName);  // buffer for name 
	if (uRetVal == 0)
	{
		_tprintf(TEXT("\n** ERROR ** %s: %s\n"), TEXT("TempFile"), TEXT("GetTempFileName failed"));
		SvcReportEvent(TEXT("GetTempFileName failed"));
		return 2;
	}
	//  Creates the new file to write to for the upper-case version.
	hTempFile = CreateFile((LPTSTR)szTempFileName, // file name 
		GENERIC_WRITE,        // open for write 
		0,                    // do not share 
		NULL,                 // default security 
		CREATE_ALWAYS,        // overwrite existing
		FILE_ATTRIBUTE_NORMAL,// normal file 
		NULL);                // no template 
	if (hTempFile == INVALID_HANDLE_VALUE)
	{
		_tprintf(TEXT("\n** ERROR ** %s: %s\n"), TEXT("TempFile"), TEXT("CreateFile failed"));
		SvcReportEvent(TEXT("CreateFile failed"));
		return 3;
	}

	//Writes the resource
	fSuccess = WriteFile(hTempFile, pResourceData, cbResourceSize, &dwBytesWritten, NULL);
	if ((!fSuccess) || (dwBytesWritten != cbResourceSize)) {
		_tprintf(TEXT("\n** ERROR ** %s: %s\n"), TEXT("TempFile"), TEXT("WriteFile failed"));
		SvcReportEvent(TEXT("WriteFile failed"));
		return 4;
	}

	FlushFileBuffers(hTempFile);
	CloseHandle(hTempFile);

	Sleep(1500); //Wait for flush to disk

				 //Now we'll execute the resource file
	fSuccess = CreateProcess(NULL, szTempFileName,
		NULL, NULL, NULL,
		0, NULL, NULL, &si, &pi);
	if (!fSuccess) {
		_tprintf(TEXT("\n** ERROR ** %s: %s\n"), TEXT("Process Creation"), TEXT("CreateProcess failed"));
		_tprintf(TEXT("\nFail reason %d\n"), GetLastError());
		return 5;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);
	_tprintf(TEXT("\n** ERROR ** %s: %s\n"), TEXT("Process Wait"), TEXT("Finished waiting"));

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	DeleteFile(szTempFileName);

	return 0;
}

void main(void)
{

	SvcInit();
	return;
}

