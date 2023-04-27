/*
 * win32_types.h
 * 
 * Type definitions not present in WIN32.
 */


/*
 * rlimit structure, for implementing getrlimit().
 * Include definition of resource limits, for calling 
 * getrlimit() function.
 */

typedef unsigned long   rlim_t;

#ifndef RLIMIT_CPU

struct rlimit {
        rlim_t  rlim_cur;               /* current limit */
        rlim_t  rlim_max;               /* maximum value for rlim_cur */
};

#define RLIMIT_CPU      0               /* cpu time in milliseconds */
#define RLIMIT_FSIZE    1               /* maximum file size */
#define RLIMIT_DATA     2               /* data size */
#define RLIMIT_STACK    3               /* stack size */
#define RLIMIT_CORE     4               /* core file size */
#define RLIMIT_NOFILE   5               /* file descriptors */
#define RLIMIT_VMEM     6               /* maximum mapped memory */
#define RLIMIT_AS       RLIMIT_VMEM

#endif



/*
 * POSIX Extensions
 */
typedef unsigned char   uchar_t;
typedef unsigned short  ushort_t;
typedef unsigned int    uint_t;
typedef unsigned long   ulong_t;

typedef char            *caddr_t;       /* ?<core address> type */
typedef long            daddr_t;        /* <disk address> type */
typedef short           cnt_t;          /* ?<count> type */

typedef char			*__caddr_t;


/*
 * POSIX and XOPEN Declarations
 */

#ifndef _UID_T
#define _UID_T
#if defined(_LP64) || defined(_I32LPx)
typedef int     uid_t;                  /* UID type             */
#else
typedef long    uid_t;                  /* (historical version) */
#endif
#endif  /* _UID_T */

typedef uid_t   gid_t;                  /* GID type             */




#if defined(_LP64) || defined(_I32LPx)
typedef uint_t nlink_t;                 /* file link type       */
typedef int     pid_t;                  /* process id type      */
#else
typedef ulong_t nlink_t;                /* (historical version) */
typedef long    pid_t;                  /* (historical version) */
#endif




/* Define types found in asm/types.h - for CIFS */
typedef unsigned char 		__u8;
typedef unsigned short 		__u16;
typedef unsigned int 		__u32;
typedef unsigned __int64 	__u64;

typedef signed char 		__s8;
typedef signed short 		__s16;
typedef signed long 		__s32;
typedef signed __int64	 	__s64;

typedef unsigned int __mode_t;
typedef __mode_t mode_t;
typedef unsigned short int sa_family_t;
typedef unsigned short int uint16_t;


#if !defined(WIN32_UTSNAME)
#define WIN32_UTSNAME

/* Structure describing the system and machine. */  
struct utsname
  {
    /* Name of the implementation of the operating system. */  
    char sysname[64];

    /* Name of this node on the network. */  
    char nodename[64];

    /* Current release level of this implementation. */  
    char release[64];

    /* Current version level of this release. */  
    char version[64];

    /* Name of the hardware type the system is running on. */  
    char machine[64];
  };

#endif /* WIN32_UTSNAME */
