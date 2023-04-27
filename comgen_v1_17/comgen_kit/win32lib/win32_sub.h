/* win32_sub.h
 *
 * WIN32 subroutine header file.  Implements unix functionality
 * not available or handled differently in WIN32.
 */
#ifndef __WIN32_SUB_H__
#define __WIN32_SUB_H__
/* Include required headers */
#include <stdio.h>
#include <io.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <sys/timeb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <tchar.h>
/* Include implementation specific definitions */
#include "win32_types.h"
/* Function Mapping */
#if defined(WIN32)
#include <direct.h>

#define access  _access
/* file existence */
#define F_OK    00
/* file execution, no such mode in _access call, mapped to file existence */
#define X_OK    00

#define getcwd          _getcwd

#define chdir           _chdir
#define mkdir(dirname, mode)           _mkdir(dirname)
#define unlink          _unlink
#define truncate        _truncate
#define rmdir			_rmdir
#define stat			_stat
#define open			_open
#define lseek			_lseek
#define chmod			_chmod
#define fstat			_fstat64
#define popen			_popen
#define pclose			_pclose
#define read			win32_read
#define write			win32_write
#define fsync(fd)		FlushFileBuffers(fd)
#define strncasecmp(s1, s2, n) _strnicmp((const char*)(s1), (const char*)(s2), (size_t)(n))

#define O_APPEND		_O_APPEND
#define O_RDWR			_O_RDWR
#define O_CREAT			_O_CREAT
#define O_RDONLY		_O_RDONLY
#define O_WRONLY		_O_WRONLY
#define off_t			int
#define off64_t			long long
#endif

/* Not used */
struct timezone {
	int 			tz_minuteswest;
	int 			tz_dsttime;
};

/* Functions and wrappers not included in WIN32 */

extern void bzero(void *, int);

extern int close(int);

extern int gettimeofday(struct timeval *, struct timezone *);

extern uint_t sleep(uint_t);

extern int fork(void);

extern void win32_init();

extern void win32_close();

extern int kill(int, int);

extern int truncate(const char *path, off_t length);

extern int getpid();
extern char *map_to_network_path(const char *server, const char *localpath);
extern int win32_read(int fd, void *buf, size_t count);
extern int win32_write(int fd, const void *buf, size_t count);
extern int getpagesize(void);
extern BOOL GetRootPathName(__in const char* filename, __out char* root);
extern char* GetParentPath(const char* path);
extern TCHAR* win32_strerror(int code);
extern DWORD LogonAndImpersonateUser(
									__in PCSTR username,
									__in PCSTR password,
									__in_opt PCSTR domain
									);
extern DWORD GetCurrentDomainName(__out PSTR domainName, __inout PULONG length);
extern DWORD GetDCName(__in LPCSTR domainName, 
							__out LPSTR dcName, 
							__out PULONG length);
extern DWORD AddUser(
						__in PCWSTR server,
					    __in PCWSTR username, 
					    __in_opt PCWSTR password
					    );
#endif
