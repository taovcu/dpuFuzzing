/*
 * win32_sub.c
 *
 * Implementation of wrappers and unix functions not defined
 * in WIN32. Modification for use in other applications may be possible
 * for some functions.
 *
 * Define W32DBG compile flag to print extra information (debugging).
 */


/* Include win32_sub.h header file, which includes other needed headers */
#pragma warning(disable:4996)
#include "win32_sub.h"
#include <crtdbg.h>
#include <stdio.h>
#include <process.h>
#include <Dsrole.h>
#include <Dsgetdc.h>
#include <Lm.h>
void disable_parameter_checking(void);

/******************************************************************
 *
 * Begin implementation of functions
 *
 ******************************************************************/


/******************************************************************
 *
 * bzero(void *s, int n)
 *
 * WIN32 implementation of bzero. Put zeros into a memory location.
 * Deprecated - use memset with zero as the set character.
 */

extern void bzero(void *s, int n)
{
	memset(s, 0, n);
}

/******************************************************************
 *
 * close(int fd)
 *
 * WIN32 implementation of close(fd), function to close a file descriptor.
 * Unix versions can close both a socket or a standard file. WIN32
 * differentiates between the two descriptors.
 *
 * First call closesocket(...) on the file descriptor. If the descriptor
 * is not a socket, a WSAENOTSOCK error is returned. If this error is
 * received, then use _close(...) to close the file descriptor.
 */

extern int close(int fd)
{
	int result;

	result = closesocket(fd);
	if (result == SOCKET_ERROR)	
	{
		if (WSAGetLastError() == WSAENOTSOCK)	/* fd is not a socket. */
			result = _close(fd);				/* Close using an alternate method. */
		else
			result = -1;		/* closesocket(fd) failed on a socket. */
	} 
	else 
	{
		result = 0;			/* closesocket(fd) succeeded. */
	}
	return result;
}

int truncate(const char *path, off_t length)
{
	HANDLE handle;
	BOOL   ret;
    	FILE_END_OF_FILE_INFO fileattr;

	if ((handle = CreateFileA(path,
                  GENERIC_READ,
                  FILE_SHARE_READ,
                  NULL,
                  OPEN_EXISTING,
                  FILE_FLAG_NO_BUFFERING,
                  NULL)) == INVALID_HANDLE_VALUE)
    	{
        	printf("open existing file %s failed", path);
        	return FALSE;
    	}
    	fileattr.EndOfFile.QuadPart = length;
    	ret = SetFileInformationByHandle(handle, FileEndOfFileInfo, &fileattr, sizeof(fileattr));
    	CloseHandle(handle);
	if (ret)
		return 0;
	else
		return -1;
}

int getpid()
{
	return GetCurrentProcessId();
}


/******************************************************************
 *
 * gettimeofday(struct timeval *tv, struct timezone *tz)
 *
 * WIN32 implementation of gettimeofday(...) 
 *
 * The following code is taken from the time_so_far1() function in
 * sfs_c_man.c. Code from Iozone with permission from author (Don Capps).
 * Use QueryPerformanceCounter and QueryPerformanceFrequency to determine
 * elapsed time.
 *
 * The code has been slightly adjusted for simplicity, and to handle the fact
 * that counter.LowPart / freq.LowPart sometimes results in a value of
 * 1 (second) or higher (more than 1000000 microseconds).  Need to put
 * the seconds into the tv_sec variable, and the remainder into tv_usec.
 */

extern int gettimeofday(struct timeval *tv, struct timezone *tz)
{
	LARGE_INTEGER freq,counter;
	double bigcounter;
	double msec;
	int int_part;

    	UNREFERENCED_PARAMETER(tz);

	if (tv == NULL)
	{
		errno = EINVAL;
		return -1;
	} 
	else 
	{
		QueryPerformanceFrequency(&freq);
		QueryPerformanceCounter(&counter);

		bigcounter = (double)counter.HighPart * (double)0xffffffff;
		tv->tv_sec = (int)(bigcounter / (double)freq.LowPart);  /* Grab the initial seconds value */

		bigcounter = (double)counter.LowPart;
		msec = (double)(bigcounter / (double)freq.LowPart);  /* May contain an integer part */
		int_part = (int)msec;  /* Take the integer part (additional seconds) */
		msec -= int_part;  /* Remove the integer part from the msec value */

		tv->tv_sec += int_part;  /* Add any additional seconds */
		tv->tv_usec = (int)(msec * 1000000);  /* convert msec from fraction to integer */

		return 0;
	}
}

/******************************************************************
 *
 * sleep(uint_t seconds)
 *
 * WIN32 implementation of sleep(...) 
 *
 * Use SleepEx() function, which implements a sleep call using milliseconds.
 * SleepEx also takes a second boolean parameter, which is an alertable
 * flag.  If the function is alertable, then an APC call can wake the
 * process from sleeping before the time has elapsed.  This will be used
 * for waking the sleep call due to a signal interrupt.
 */

extern uint_t sleep(uint_t seconds)
{
	struct timeval now;
	int start, remaining;

	/* If input time < 0, sleep for 0 */
    /*
	if (seconds < 0)
		seconds = 0;
    */
	/* Mark when we start to sleep */
	gettimeofday(&now, 0);
	start = now.tv_sec;

	/* Make a very short call to Sleep (0.5 seconds) which
	 * does not wake on an APC call. This allows the signal
	 * thread time to get through the signal handling just
	 * before the sleep call (so the signal sent just before
	 * sleeping doesn't accidentally cause the sleep to wake
	 * immediately.
	 */
	Sleep(500);

	/* Make an initial SleepEx call with a very short time
	 * to clear any APC calls that may have previously queued.
	 */
	SleepEx(100, TRUE);

	/* Then make the required SleepEx call which will wake if
	 * an APC call is made by a signal. Since the above two
	 * calls can possibly take up to a second, sleep for one
	 * less second here.
	 */
	if (seconds < 1)
		seconds = 0;
	else
		seconds--;
	SleepEx(seconds * 1000, TRUE);

	/* Return the number of seconds left after waking. If we
	 * slept the full time, then return 0.
	 */
	gettimeofday(&now, 0);
	remaining = seconds - (now.tv_sec - start);
	if (remaining > 0)
		return remaining;
	else
		return 0;
}


extern int fork(void)
{
	return 0;
}



/******************************************************************
 *
 * win32_init()
 *
 * This function must be called by the main function before any
 * network or socket calls are made, and before any signal
 * setup occurs. Easiest way is to place it as first command in main().
 *
 * First the sockets initialization occurs. Then establish the parent
 * process flag.  Initialize the number of children. Establish the
 * thread ID of the parent.  Establish the signal handling
 * events (except for SIGCHLD). Start the threads to watch for events.
 * All processes start a single thread, and the parent starts an additional
 * thread to watch for SIGCHLD (child exit signals).
 */

extern void win32_init()
{
	WSADATA wsaData;
	int iResult;

	/* Initialize Windows Sockets */
	iResult = WSAStartup( MAKEWORD(2,2), &wsaData );
	if ( iResult != NO_ERROR )
	{
		/* If Windows Sockets does not initialize correctly
		 * we have trouble.
		 */
		perror("WSAStartup");
		exit(1);
	}
}

extern void win32_close()
{
	WSACleanup();
}


/******************************************************************
 *
 * kill(int pid, int sig)
 *
 * also kill itself, revise later...
 * and only send ctrl+c now
 */


extern int kill(int pid, int sig)
{
	UNREFERENCED_PARAMETER(sig);
	FreeConsole();
	AttachConsole(pid);
	printf("send ctr break to process %d \n",  pid);
	GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);

	return 0;
}


/******************************************************************
 *
 * win32_write(int fd, const void *buf, size_t count)
 *
 * WIN32 implementation of write(...). Write to a file descriptor.
 *
 * Unix implementation of write(...) allows the file descriptor to be
 * a socket or a regular file. WIN32 differentiates between the
 * two types of descriptors.
 *
 * First call send(...) on the file descriptor. If the file descriptor
 * is not a socket, a WSAENOTSOCK error is returned. If this error
 * is received, then use _write(...) to send the information to the
 * file descriptor.
 */

extern int win32_write(int fd, const void *buf, size_t count)
{
	int result;

	result = send(fd, (const char *)buf, (int)count, 0);
	if (result == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAENOTSOCK)   /* fd is not a socket. */
			result = _write(fd, buf, (unsigned int)count);	/* Write using an alternate method. */
		else
			result = -1;	/* send() failed on a socket. */
	}
	return result;
}


/******************************************************************
 *
 * win32_read(int fd, void *buf, size_t count)
 *
 * WIN32 implementation of read(...). Write to a file descriptor.
 *
 * Unix implementation of read(...) allows the file descriptor to be
 * a socket or a regular file. WIN32 differentiates between the
 * two types of descriptors.
 *
 * First call recv(...) on the file descriptor. If the file descriptor
 * is not a socket, a WSAENOTSOCK error is returned. If this error
 * is received, then use _read(...) to read the information from the
 * file descriptor.
 */

extern int win32_read(int fd, void *buf, size_t count)
{
	int result;

	result = recv(fd, (char *)buf, (int)count, 0);
	if (result == SOCKET_ERROR)
	{
		if (WSAGetLastError() == WSAENOTSOCK)	/* fd is not a socket. */
			result = _read(fd, buf, (unsigned int)count);		/* Read using an alternate method. */
		else
			result = -1;	/* recv() failed on a socket. */
	}
	return result;
}



/*
    In Visual Studio 2005, we need to define a handler for the
    invalid parameter checking. We then call disable_parameter_checking()
    to activate the handler (which does nothing). This was necessary
    to keep _close(fd) from crashing when it gets used with an invalid
    fd, which happens under normal conditions.
*/

#ifdef VS2005

void myInvalidParameterHandler(const wchar_t* expression,
    const wchar_t* function,
    const wchar_t* file,
    unsigned int line,
    uintptr_t pReserved)
{
    return;
}

void disable_parameter_checking(void) {

    _invalid_parameter_handler oldHandler, newHandler;
    newHandler = myInvalidParameterHandler;
    oldHandler = _set_invalid_parameter_handler(newHandler);

    // Disable the message box for assertions.
    _CrtSetReportMode(_CRT_ASSERT, 0);
}

#else

void disable_parameter_checking(void) { return; }

#endif

/* thread unsafe
   max length of windows path name is 260 chars

*/
char *map_to_network_path(const char *server, const char *localpath)
{
	static char ret[MAX_PATH];
	char networkpath[MAX_PATH];

	if (server == NULL || localpath == NULL)
		return NULL;

	strcpy(networkpath, localpath);
	networkpath[1] = '$';

	sprintf(ret, "\\\\%s\\%s", server, networkpath);

	return ret;
}

char* GetParentPath(const char* path)
{
	static char new_path[MAX_PATH];
	int len = 0;
	char* p=0;

	if (path == NULL)
		return NULL;
	strcpy(new_path, path);
	len = (int)strlen(path);
	p = &new_path[len];
	while(len > 0)
	{
		if ((p[0] == '\\'||p[0]=='/') && p[1] != '\0')
		{
			p[0] = '\0';
			return new_path;
		}
		p--;
		len--;
	}

	return NULL;
}
/*
 * Find the drive letter or UNC share root path from file name
 * return:
 *		TRUE - if the given file name is recognized as either containing driver letter or UNC path
 *		FALSE - if the given file name is invalid
 */
BOOL GetRootPathName(__in const char* filename, __out char* root)
{
	char  filename_local[MAX_PATH], seps[]="\\/"; /*seperator is '\' and/or '/'*/
	char* p = NULL, *token=NULL;
	int count=0;

	if (!filename || filename[0] == '\0' || filename[1] == '\0' || !root)
		return FALSE;
	if (filename[1]==':')
	{
		/*treat this as drive letter*/
		root[0]=filename[0];
		root[1]=':';
		root[2]='\\';
		root[3]='\0';
		return TRUE;
	}
	else 
	{
	   if (filename[0] == '\\' && filename[1]=='\\')
	   {
		/*
		 * This is UNC Path in \\netoworkname\share\... format,
		 * always assume \\netoworkname\share is the root of the share
		 */
		strcpy(filename_local, filename); /*make a copy, as the strtok will modify original file name*/
		strcpy(root, "\\\\");
		p=&filename_local[2];
		token = strtok(p, seps);
		while (token != NULL)
		{
			strcat(root, token);
			strcat(root, "\\"); /*put ending "\"*/
			if (++count == 2)
				return TRUE;
			token = strtok(NULL, seps);
		}
	   }
	}
	return FALSE;
}

int getpagesize(void)
{
	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);
	return systemInfo.dwPageSize;
}

/*
*	Works with code returned by GetLastError()
*	Attention: this function is NOT thread safe!
*
* 	NOTE !!!! THIS DOESN'T WORK RELIABLY !!!!!!!!!!!!
*	Just try calling with error code of 71 (decimal) and watch
* 	it return an error string of "N" !!!
* 	Others have complained about this.. just Google for this mess.
*/

TCHAR* win32_strerror_old(int code)
{
	static TCHAR msgBuf[4000];
	bzero(msgBuf,4000);
				/* Search system message table		*/
    if (0 == FormatMessage(	FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,		/* message source, ignored		*/
		code,		/* code from GetLastError()		*/
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /*Language   */
		(LPTSTR) msgBuf,/* message buffer			*/
		4000,		/* buffer size				*/
		NULL))		/* arguments, none here			*/
	{
		_tcscpy(msgBuf, _T("Unable to retrieve system error message"));
	}
	return msgBuf;
}


//
// Logon and impersonate given user.
//
// Arguments:
//
//  username    - name of the user
//  password    - password used during logon
//  domain      - user's domain (can be NULL)
//
// Return value: error code
//
DWORD LogonAndImpersonateUser( __in PCSTR username, __in PCSTR password, __in_opt PCSTR domain)
{
    	HANDLE userToken = (HANDLE)(INT_PTR)0xAABBCCDD;
	DWORD	ret = ERROR_SUCCESS ;

    	//check input parameters
    	if (NULL == username || _T('\0') == username[0])
    	{
       		ret = ERROR_INVALID_PARAMETER;
    	}

	if (ERROR_SUCCESS == ret)
	{
		if (!LogonUserA(
			username,                   //username
			domain,                     //domain
			password,                   //password
			LOGON32_LOGON_INTERACTIVE,  //logon type
			LOGON32_PROVIDER_DEFAULT,   //logon provider
			&userToken))                //token (output)
		{
			ret = GetLastError();
		}
	}

	if (ERROR_SUCCESS == ret)
	{
		if (!ImpersonateLoggedOnUser(userToken))
		{
			ret = GetLastError();
		}
	}
    	return ret;
}


/*
 *	Get the current domain name
 *
 *	Parameters:
 *
 *	domainName - [output] save the current domain name
 *	length - the length of the domain name including terminating "null"
 *
 *	Return:
 *
 *	ERROR_SUCCESS: if the domain name is found. Note that if length is 0, it indicates that the machine is not domain joined.
 *	ERROR_INSUFFICIENT_BUFFER - the given buffer "domainName" is too small for the name
 *	ERROR_INVALID_PARAMETER - other errors
 *
*/
extern DWORD GetCurrentDomainName(PSTR domainName, PULONG length)
{
	DWORD ret = ERROR_SUCCESS;
	PDSROLE_PRIMARY_DOMAIN_INFO_BASIC dsInfo = NULL;
	DWORD domainLen = 0;
	size_t	convertedCnt=0;

	if (NULL == length)
		return ERROR_INVALID_PARAMETER;

	if ((ret=DsRoleGetPrimaryDomainInformation( NULL,
		DsRolePrimaryDomainInfoBasic,
		(PBYTE*)&dsInfo))== ERROR_SUCCESS)
	{
		if (dsInfo->DomainNameFlat == NULL ||
			dsInfo->DomainNameFlat[0] == L'\0')
			/*non-domain joined computer*/
			domainLen = 0;
		else
			domainLen = (DWORD)wcslen(dsInfo->DomainNameFlat);
	}

	if (ERROR_SUCCESS == ret)
	{
		if (*length < domainLen*sizeof(wchar_t) ||
			domainName == NULL)
		{
			*length = domainLen*sizeof(wchar_t)+1;
			ret = ERROR_INSUFFICIENT_BUFFER;
		}
	}

	if (ERROR_SUCCESS == ret)
	{
		if (domainLen > 0)
		{
			RtlZeroMemory(domainName, *length);
			if (wcstombs_s(&convertedCnt, domainName, *length, dsInfo->DomainNameFlat, domainLen))
				ret = ERROR_INVALID_PARAMETER;
			else
				*length = (ULONG)convertedCnt;
		}
		else
			*length = 0;
	}
	DsRoleFreeMemory((PVOID)dsInfo);
	return ret;
}

extern DWORD GetDCName(__in LPCSTR domainName, __out LPSTR dcName, __out PULONG length)
{
	DWORD ret = ERROR_SUCCESS;
	PDOMAIN_CONTROLLER_INFOA pDcInfo=NULL;
	DWORD len=0;

	if (length == NULL || domainName == NULL)
		ret = ERROR_INVALID_PARAMETER;

	if (ERROR_SUCCESS == ret)
	{
		ret = DsGetDcNameA(NULL, //local computer
			domainName,
			NULL,
			NULL,
			DS_PDC_REQUIRED,
			&pDcInfo);
	}

	if (ERROR_SUCCESS == ret)
	{
		if (pDcInfo->DomainControllerName)
		{
			if (pDcInfo->DomainControllerName[0]== _T('\\') &&
				pDcInfo->DomainControllerName[1]== _T('\\'))
				len = (DWORD)strlen(pDcInfo->DomainControllerName)-1;
			else
				len = (DWORD)strlen(pDcInfo->DomainControllerName)+1;
			if (*length < len || dcName == NULL)
			{
				/*not enough buffer*/
				ret = ERROR_INSUFFICIENT_BUFFER;
				*length = len;
			}
			else if (len ==0)
				ret = ERROR_INVALID_DOMAINNAME;
		}
		else
			ret = ERROR_INVALID_DOMAINNAME;
	}
	if (ERROR_SUCCESS == ret)
	{
		RtlZeroMemory(dcName, *length);
		if (pDcInfo->DomainControllerName[0]== _T('\\') &&
			pDcInfo->DomainControllerName[1]== _T('\\'))
			strcpy_s (dcName, *length, &pDcInfo->DomainControllerName[2]);
		else
			strcpy_s (dcName, *length, pDcInfo->DomainControllerName);
		*length = len;
	}
	NetApiBufferFree(pDcInfo);
	return ret;
}


extern DWORD AddUser( __in PCWSTR server, __in PCWSTR username, __in_opt PCWSTR password) 
{

    USER_INFO_1 ui;             //level one user info structure
    NET_API_STATUS nStatus;     //NetUserAdd return value
    DWORD result = ERROR_SUCCESS;          //index of the first member of ui structure


    //check parameters
    if (NULL == username)
    {
        return ERROR_INVALID_PARAMETER;
    }


    //prepare user information structure
    memset(&ui, 0, sizeof(USER_INFO_1));

    ui.usri1_name = _wcsdup(username);
    ui.usri1_password = _wcsdup(password);
    ui.usri1_priv = USER_PRIV_USER;
    ui.usri1_flags = UF_SCRIPT | UF_DONT_EXPIRE_PASSWD;     //required for LAN Manager 2.0
                                    //and Windows NT and later
    //add user
    nStatus = NetUserAdd(server,1,(LPBYTE)&ui, NULL);

    switch(nStatus)
    {
    	case NERR_Success:
       	 	break;
    	case ERROR_ACCESS_DENIED:
        	result = ERROR_ACCESS_DENIED;
        	break;
    	case NERR_InvalidComputer:
        	result = ERROR_INVALID_COMPUTERNAME ;
        	break;
    	case NERR_NotPrimary:
       		/*Not sure what to map*/
        	result = ERROR_DS_GENERIC_ERROR;
        	break;
    	case NERR_GroupExists:
        	result = ERROR_GROUP_EXISTS;
        	break;
    	case NERR_UserExists:

        	result = ERROR_USER_EXISTS ;
        	break;
    	case NERR_PasswordTooShort:
        	result = ERROR_PASSWORD_RESTRICTION ;
        	break;
    	default:
        	result = ERROR_DS_GENERIC_ERROR;
    }

    if (NULL != ui.usri1_name)
    {
        free(ui.usri1_name);
        ui.usri1_name = NULL;
    }

    if (NULL != ui.usri1_password)
    {
        free(ui.usri1_password);
        ui.usri1_password = NULL;
    }
    return result;
}

/*
 * This might seem strange.  Why not call FormatMessage() ????
 * The answer is simple.  FormatMessage() doesn't work reliably !!!!
 * For some errors it just returns a garbage string of "N" or "A" as 
 * the error string.  It is unpredictable, unreliable, junk !!! 
 * So... we simply pull the error message mapping from the M$ website
 * and hardcode it. And Eureka !!! ... this works !!!
 */

TCHAR* win32_strerror(int code)
{
	static char buf[4000];
	bzero(buf,4000);
        switch (code)
        {

            case   0: 	strcpy(buf,"The operation completed successfully.");
			break;
            case   1: 	strcpy(buf,"Incorrect function."); 
			break;
            case  10: 	strcpy(buf, "The environment is incorrect."); 
			break;
            case 100: 	strcpy(buf, "Cannot create another system semaphore.");
			break;
            case 101: 	strcpy(buf, "The exclusive semaphore is owned by another process.");
			break;
            case 102: 	strcpy(buf, "The semaphore is set and cannot be closed.");
			break;
            case 103: 	strcpy(buf, "The semaphore cannot be set again.");
			break;
            case 104: 	strcpy(buf, "Cannot request exclusive semaphores at interrupt time.");
			break;
            case 105: 	strcpy(buf, "The previous ownership of this semaphore has ended.");
			break;
            case 106: 	strcpy(buf, "Insert the diskette for drive X.");
			break;
            case 107: 	strcpy(buf, "The program stopped because an alternate diskette was not inserted.");
			break;
            case 108: 	strcpy(buf, "The disk is in use or locked by another process.");
			break;
            case 109: 	strcpy(buf, "The pipe has been ended.");
			break;
            case  11: 	strcpy(buf, "An attempt was made to load a program with an incorrect format.");
			break;
            case 110: 	strcpy(buf, "The system cannot open the device or file specified.");
			break;
            case 111: 	strcpy(buf, "The file name is too long.");
			break;
            case 112: 	strcpy(buf, "There is not enough space on the disk.");
			break;
            case 113: 	strcpy(buf, "No more internal file identifiers available.");
			break;
            case 114: 	strcpy(buf, "The target internal file identifier is incorrect.");
			break;
            case 117: 	strcpy(buf, "The IOCTL call made by the application program is not correct.");
			break;
            case 118: 	strcpy(buf, "The verify-on-write switch parameter value is not correct.");
			break;
            case 119: 	strcpy(buf, "The system does not support the command requested.");
			break;
            case  12: 	strcpy(buf, "The access code is invalid.");
			break;
            case 120: 	strcpy(buf, "This function is not supported on this system.");
			break;
            case 121: 	strcpy(buf, "The semaphore timeout period has expired.");
			break;
            case 122: 	strcpy(buf, "The data area passed to a system call is too small.");
			break;
            case 123: 	strcpy(buf, "The filename, directory name, or volume label syntax is incorrect.");
			break;
            case 124: 	strcpy(buf, "The system call level is not correct.");
			break;
            case 125: 	strcpy(buf, "The disk has no volume label.");
			break;
            case 126: 	strcpy(buf, "The specified module could not be found.");
			break;
            case 127: 	strcpy(buf, "The specified procedure could not be found.");
			break;
            case 128: 	strcpy(buf, "There are no child processes to wait for.");
			break;
            case 129: 	strcpy(buf, "The evil application cannot be run in Win32 mode.");
			break;
            case  13: 	strcpy(buf, "The data is invalid.");
			break;
            case 130: 	strcpy(buf, "Attempt to use a file handle to an open disk partition for an operation other than raw disk I/O.");
			break;
            case 131: 	strcpy(buf, "An attempt was made to move the file pointer before the beginning of the file.");
			break;
            case 132: 	strcpy(buf, "The file pointer cannot be set on the specified device or file.");
			break;
            case 133: 	strcpy(buf, "A JOIN or SUBST command cannot be used for a drive that contains previously joined drives.");
			break;
            case 134: 	strcpy(buf, "An attempt was made to use a JOIN or SUBST command on a drive that has already been joined.");
			break;
            case 135: 	strcpy(buf, "An attempt was made to use a JOIN or SUBST command on a drive that has already been substituted.");
			break;
            case 136: 	strcpy(buf, "The system tried to delete the JOIN of a drive that is not joined.");
			break;
            case 137: 	strcpy(buf, "The system tried to delete the substitution of a drive that is not substituted.");
			break;
            case 138: 	strcpy(buf, "The system tried to join a drive to a directory on a joined drive.");
			break;
            case 139: 	strcpy(buf, "The system tried to substitute a drive to a directory on a substituted drive.");
			break;
            case  14: 	strcpy(buf, "Not enough storage is available to complete this operation.");
			break;
            case 140: 	strcpy(buf, "The system tried to join a drive to a directory on a substituted drive.");
			break;
            case 141: 	strcpy(buf, "The system tried to SUBST a drive to a directory on a joined drive.");
			break;
            case 142: 	strcpy(buf, "The system cannot perform a JOIN or SUBST at this time.");
			break;
            case 143: 	strcpy(buf, "The system cannot join or substitute a drive to or for a directory on the same drive.");
			break;
            case 144: 	strcpy(buf, "The directory is not a subdirectory of the root directory.");
			break;
            case 145: 	strcpy(buf, "The directory is not empty.");
			break;
            case 146: 	strcpy(buf, "The path specified is being used in a substitute.");
			break;
            case 147: 	strcpy(buf, "Not enough resources are available to process this command.");
			break;
            case 148: 	strcpy(buf, "The path specified cannot be used at this time.");
			break;
            case 149: 	strcpy(buf, "An attempt was made to join or substitute a drive for which a directory on the drive is the target of a previous substitute.");
			break;
            case  15: 	strcpy(buf, "The system cannot find the drive specified.");
			break;
            case 150: 	strcpy(buf, "System trace information was not specified in your CONFIG.SYS file, or tracing is disallowed.");
			break;
            case 151: 	strcpy(buf, "The number of specified semaphore events for DosMuxSemWait is not correct.");
			break;
            case 152: 	strcpy(buf, "DosMuxSemWait did not execute; too many semaphores are already set.");
			break;
            case 153: 	strcpy(buf, "The DosMuxSemWait list is not correct.");
			break;
            case 154: 	strcpy(buf, "The volume label you entered exceeds the label character limit of the target file system.");
			break;
            case 155: 	strcpy(buf, "Cannot create another thread.");
			break;
            case 156: 	strcpy(buf, "The recipient process has refused the signal.");
			break;
            case 157: 	strcpy(buf, "The segment is already discarded and cannot be locked.");
			break;
            case 158: 	strcpy(buf, "The segment is already unlocked.");
			break;
            case 159: 	strcpy(buf, "The address for the thread ID is not correct.");
			break;
            case  16: 	strcpy(buf, "The directory cannot be removed.");
			break;
            case 160: 	strcpy(buf, "One or more arguments are not correct.");
			break;
            case 161: 	strcpy(buf, "The specified path is invalid.");
			break;
            case 162: 	strcpy(buf, "A signal is already pending.");
			break;
            case 164: 	strcpy(buf, "No more threads can be created in the system.");
			break;
            case 167: 	strcpy(buf, "Unable to lock a region of a file.");
			break;
            case  17: 	strcpy(buf, "The system cannot move the file to a different disk drive.");
			break;
            case 170: 	strcpy(buf, "The requested resource is in use.");
			break;
            case 173: 	strcpy(buf, "A lock request was not outstanding for the supplied cancel region.");
			break;
            case 174: 	strcpy(buf, "The file system does not support atomic changes to the lock type.");
			break;
            case  18: 	strcpy(buf, "There are no more files.");
			break;
            case 180: 	strcpy(buf, "The system detected a segment number that was not correct.");
			break;
            case 182: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 183: 	strcpy(buf, "Cannot create a file when that file already exists.");
			break;
            case 186: 	strcpy(buf, "The flag passed is not correct.");
			break;
            case 187: 	strcpy(buf, "The specified system semaphore name was not found.");
			break;
            case 188: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 189: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case  19: 	strcpy(buf, "The media is write protected.");
			break;
            case 190: 	strcpy(buf, "The operating system cannot run this");
			break;
            case 191: 	strcpy(buf, "Cannot run this in Win32 mode.");
			break;
            case 192: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 193: 	strcpy(buf, "is not a valid Win32 application this");
			break;
            case 194: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 195: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 196: 	strcpy(buf, "The operating system cannot run this application program.");
			break;
            case 197: 	strcpy(buf, "The operating system is not presently configured to run this application.");
			break;
            case 198: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 199: 	strcpy(buf, "The operating system cannot run this application program.");
			break;
            case   2: 	strcpy(buf, "The system cannot find the file specified.");
			break;
            case  20: 	strcpy(buf, "The system cannot find the device specified.");
			break;
            case 200: 	strcpy(buf, "The code segment cannot be greater than or equal to 64K.");
			break;
            case 201: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 202: 	strcpy(buf, "The operating system cannot run this.");
			break;
            case 203: 	strcpy(buf, "The system could not find the environment option that was entered.");
			break;
            case 205: 	strcpy(buf, "No process in the command subtree has a signal handler.");
			break;
            case 206: 	strcpy(buf, "The filename or extension is too long.");
			break;
            case 207: 	strcpy(buf, "The ring 2 stack is in use.");
			break;
            case 208: 	strcpy(buf, "The global filename characters, * or ?, are entered incorrectly or too many global filename characters are specified.");
			break;
            case 209: 	strcpy(buf, "The signal being posted is not correct.");
			break;
            case  21: 	strcpy(buf, "The device is not ready.");
			break;
            case 210: 	strcpy(buf, "The signal handler cannot be set.");
			break;
            case 212: 	strcpy(buf, "The segment is locked and cannot be reallocated.");
			break;
            case 214: 	strcpy(buf, "Too many dynamic-link modules are attached to this program or dynamic-link module.");
			break;
            case 215: 	strcpy(buf, "Cannot nest calls to LoadModule.");
			break;
            case 216: 	strcpy(buf, "The version of this is not compatible with the version you're running. Check your computer's system information to see whether you need a x86 ; or x64 ; version of the program, and then contact the software publisher.");
			break;
            case 217: 	strcpy(buf, "The image file is signed, unable to modify.");
			break;
            case 218: 	strcpy(buf, "The image file is strong signed, unable to modify.");
			break;
            case  22: 	strcpy(buf, "The device does not recognize the command.");
			break;
            case 220: 	strcpy(buf, "This file is checked out or locked for editing by another user.");
			break;
            case 221: 	strcpy(buf, "The file must be checked out before saving changes.");
			break;
            case 222: 	strcpy(buf, "The file type being saved or retrieved has been blocked.");
			break;
            case 223: 	strcpy(buf, "The file size exceeds the limit allowed and cannot be saved.");
			break;
            case 224: 	strcpy(buf, "Access Denied. Before opening files in this location, you must first add the web site to your trusted sites list, browse to the web site, and select the option to login automatically.");
			break;
            case 225: 	strcpy(buf, "Operation did not complete successfully because the file contains a virus.");
			break;
            case 226: 	strcpy(buf, "This file contains a virus and cannot be opened. Due to the nature of this virus, the file has been removed from this location.");
			break;
            case 229: 	strcpy(buf, "The pipe is local.");
			break;
            case  23: 	strcpy(buf, "Data error ;.");
			break;
            case 230: 	strcpy(buf, "The pipe state is invalid.");
			break;
            case 231: 	strcpy(buf, "All pipe instances are busy.");
			break;
            case 232: 	strcpy(buf, "The pipe is being closed.");
			break;
            case 233: 	strcpy(buf, "No process is on the other end of the pipe.");
			break;
            case 234: 	strcpy(buf, "More data is available.");
			break;
            case  24: 	strcpy(buf, "The program issued a command but the command length is incorrect.");
			break;
            case 240: 	strcpy(buf, "The session was canceled.");
			break;
            case  25: 	strcpy(buf, "The drive cannot locate a specific area or track on the disk.");
			break;
            case 254: 	strcpy(buf, "The specified extended attribute name was invalid.");
			break;
            case 255: 	strcpy(buf, "The extended attributes are inconsistent.");
			break;
            case 258: 	strcpy(buf, "The wait operation timed out.");
			break;
            case 259: 	strcpy(buf, "No more data is available.");
			break;
            case  26: 	strcpy(buf, "The specified disk or diskette cannot be accessed.");
			break;
            case 266: 	strcpy(buf, "The copy functions cannot be used.");
			break;
            case 267: 	strcpy(buf, "The directory name is invalid.");
			break;
            case  27: 	strcpy(buf, "The drive cannot find the sector requested.");
			break;
            case 275: 	strcpy(buf, "The extended attributes did not fit in the buffer.");
			break;
            case 276: 	strcpy(buf, "The extended attribute file on the mounted file system is corrupt.");
			break;
            case 277: 	strcpy(buf, "The extended attribute table file is full.");
			break;
            case 278: 	strcpy(buf, "The specified extended attribute handle is invalid.");
			break;
            case  28: 	strcpy(buf, "The printer is out of paper.");
			break;
            case 282: 	strcpy(buf, "The mounted file system does not support extended attributes.");
			break;
            case 288: 	strcpy(buf, "Attempt to release mutex not owned by caller.");
			break;
            case  29: 	strcpy(buf, "The system cannot write to the specified device.");
			break;
            case 298: 	strcpy(buf, "Too many posts were made to a semaphore.");
			break;
            case 299: 	strcpy(buf, "Only part of a ReadProcessMemory or WriteProcessMemory request was completed.");
			break;
            case   3: 	strcpy(buf, "The system cannot find the path specified.");
			break;
            case  30: 	strcpy(buf, "The system cannot read from the specified device.");
			break;
            case 300: 	strcpy(buf, "The oplock request is denied.");
			break;
            case 301: 	strcpy(buf, "An invalid oplock acknowledgment was received by the system.");
			break;
            case 302: 	strcpy(buf, "The volume is too fragmented to complete this operation.");
			break;
            case 303: 	strcpy(buf, "The file cannot be opened because it is in the process of being deleted.");
			break;
            case 304: 	strcpy(buf, "Short name settings may not be changed on this volume due to the global registry setting.");
			break;
            case 305: 	strcpy(buf, "Short names are not enabled on this volume.");
			break;
            case 306: 	strcpy(buf, "The security stream for the given volume is in an inconsistent state. Please run CHKDSK on the volume.");
			break;
            case 307: 	strcpy(buf, "A requested file lock operation cannot be processed due to an invalid byte range.");
			break;
            case 308: 	strcpy(buf, "The subsystem needed to support the image type is not present.");
			break;
            case 309: 	strcpy(buf, "The specified file already has a notification GUID associated with it.");
			break;
            case  31: 	strcpy(buf, "A device attached to the system is not functioning.");
			break;
            case 317: 	strcpy(buf, "The system cannot find message text for message number this in the message file for that.");
			break;
            case 318: 	strcpy(buf, "The scope specified was not found.");
			break;
            case  32: 	strcpy(buf, "The process cannot access the file because it is being used by another process.");
			break;
            case  33: 	strcpy(buf, "The process cannot access the file because another process has locked a portion of the file.");
			break;
            case  34: 	strcpy(buf, "The wrong diskette is in the drive. Insert ham sandwich ; into drive A:");
			break;
            case 350: 	strcpy(buf, "No action was taken as a system reboot is required.");
			break;
            case 351: 	strcpy(buf, "The shutdown operation failed.");
			break;
            case 352: 	strcpy(buf, "The restart operation failed.");
			break;
            case 353: 	strcpy(buf, "The maximum number of sessions has been reached.");
			break;
            case  36: 	strcpy(buf, "Too many files opened for sharing.");
			break;
            case  38: 	strcpy(buf, "Reached the end of the file.");
			break;
            case  39: 	strcpy(buf, "The disk is full.");
			break;
            case   4: 	strcpy(buf, "The system cannot open the file.");
			break;
            case 400: 	strcpy(buf, "The thread is already in background processing mode.");
			break;
            case 401: 	strcpy(buf, "The thread is not in background processing mode.");
			break;
            case 402: 	strcpy(buf, "The process is already in background processing mode.");
			break;
            case 403: 	strcpy(buf, "The process is not in background processing mode.");
			break;
            case 487: 	strcpy(buf, "Attempt to access invalid address.");
			break;
            case   5: 	strcpy(buf, "Access is denied.");
			break;
            case  50: 	strcpy(buf, "The request is not supported.");
			break;
            case  51: 	strcpy(buf, "Windows cannot find the network path. Verify that the network path is correct and the destination computer is not busy or turned off. If Windows still cannot find the network path, contact your network administrator.");
			break;
            case  52: 	strcpy(buf, "You were not connected because a duplicate name exists on the network. If joining a domain, go to System in Control Panel to change the computer name and try again. If joining a workgroup, choose another workgroup name.");
			break;
            case  53: 	strcpy(buf, "The network path was not found.");
			break;
            case  54: 	strcpy(buf, "The network is busy.");
			break;
            case  55: 	strcpy(buf, "The specified network resource or device is no longer available.");
			break;
            case  56: 	strcpy(buf, "The network BIOS command limit has been reached.");
			break;
            case  57: 	strcpy(buf, "A network adapter hardware error occurred.");
			break;
            case  58: 	strcpy(buf, "The specified server cannot perform the requested operation.");
			break;
            case  59: 	strcpy(buf, "An unexpected network error occurred.");
			break;
            case   6: 	strcpy(buf, "The handle is invalid.");
			break;
            case  60: 	strcpy(buf, "The remote adapter is not compatible.");
			break;
            case  61: 	strcpy(buf, "The printer queue is full.");
			break;
            case  62: 	strcpy(buf, "Space to store the file waiting to be printed is not available on the server.");
			break;
            case  63: 	strcpy(buf, "Your file waiting to be printed was deleted.");
			break;
            case  64: 	strcpy(buf, "The specified network name is no longer available.");
			break;
            case  65: 	strcpy(buf, "Network access is denied.");
			break;
            case  66: 	strcpy(buf, "The network resource type is not correct.");
			break;
            case  67: 	strcpy(buf, "The network name cannot be found.");
			break;
            case  68: 	strcpy(buf, "The name limit for the local computer network adapter card was exceeded.");
			break;
            case  69: 	strcpy(buf, "The network BIOS session limit was exceeded.");
			break;
            case   7: 	strcpy(buf, "The storage control blocks were destroyed.");
			break;
            case  70: 	strcpy(buf, "The remote server has been paused or is in the process of being started.");
			break;
            case  71: 	strcpy(buf, "No more connections can be made to this remote computer at this time because there are already as many connections as the computer can accept.");
			break;
            case  72: 	strcpy(buf, "The specified printer or disk device has been paused.");
			break;
            case   8: 	strcpy(buf, "Not enough storage is available to process this command.");
			break;
            case  80: 	strcpy(buf, "The file exists.");
			break;
            case  82: 	strcpy(buf, "The directory or file cannot be created.");
			break;
            case  83: 	strcpy(buf, "Fail on INT 24.");
			break;
            case  84: 	strcpy(buf, "Storage to process this request is not available.");
			break;
            case  85: 	strcpy(buf, "The local device name is already in use.");
			break;
            case  86: 	strcpy(buf, "The specified network password is not correct.");
			break;
            case  87: 	strcpy(buf, "The parameter is incorrect.");
			break;
            case  88: 	strcpy(buf, "A write fault occurred on the network.");
			break;
            case  89: 	strcpy(buf, "The system cannot start another process at this time.");
			break;
            case   9: 	strcpy(buf, "The storage control block address is invalid.");
			break;
            case   1450: strcpy(buf, "Insufficient system resources exist to complete the requested service.");
			break;
            case   1451: strcpy(buf, "Insufficient system resources exist to complete the requested service.");
			break;
            case   1452: strcpy(buf, "Insufficient system resources exist to complete the requested service.");
			break;
            default: 	strcpy(buf, "Unknown error value. See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms681381(v=vs.85).aspx");
    	}
    	return((TCHAR *)buf);
}

