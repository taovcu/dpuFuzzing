/*
 * -----------------------------------------------------------------------------
 *        Version 1.1       2016           Don Capps (Iozone.org)
 *	     Don Capps
 *	     7417 Crenshaw Drive
 *	     Plano, Texas 75025
 *	     A simple mechanism to create the needed file types for Emerald.
 *
 *	     Implemented simple method to produce the 4 types of files.
 *	        1. Compression_no_dedupe
 *              2. Dedupe_no_compression
 *              3. Compression_and_dedupe
 *              4. Irreducible
 * 	     Added help screen
 *	     Moved all command line parsing to standard getopt() processing.
 *           Moved all random number generation to standard Park & Miller LC RNG.
 *           Fix code to be 32 and 64 bit compatible.
 *	     Cleaned up code to have prototypes for all functions.
 *	     Removed all dead code.
 *	     Implemented version control under RCS.
 *	     Added support for raw block devices.
 *	     Added support for O_DIRECT
 *	     Added support for Visual studio builds.
 *
 * -----------------------------------------------------------------------------
 *  Copyright 2009-2016 SNIA. All rights reserved.
 *
 *  This program is licensed under the BSD license.
 *  No warranties made or implied.
 *
 * -----------------------------------------------------------------------------
 */

/* 
 * The methods:
 *
 * Compressible non-dedupable:
 *   Fill buffer with repeating random number, changing to a new random number
 *   every GRANULE_SIZE bytes.  Where GRANULE_SIZE is the minimum number of bytes
 *   needed to trigger deduplication.
 * 
 * Dedupable non-compressible:
 *   Fill buffer with integers, where every integer is a random value. Then
 *   repeat this buffer over and over. Default to a sufficiently large buffer
 *   so as to defeat a compression algorithm from putting this in its dictionary.
 *
 * Compressible and Dedupable:
 *   Fill buffer with repeating random number, then repeat this buffer over 
 *   and over.
 *
 * Irreducible:
 *   Fill every buffer with random values. Buffer is refilled with random
 *   values before every write to fill the file.
 */

/*
 * Needed to pull in the definition of O_DIRECT on Linux.
 */
#if !defined(WIN32)
#define _GNU_SOURCE
#endif

#if defined(WIN32)
#pragma warning(disable:4996)
#pragma warning(disable:4267)
#pragma warning(disable:4133)
#pragma warning(disable:4244)
#pragma warning(disable:4102)
#pragma warning(disable:4018)

#define _CRT_RAND_S

#include <win32_sub.h>
#include <win32_getopt.h>
#include <time.h>
#endif

/* Standard headers */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#if !defined(WIN32)
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#endif

/* 
 * The following is used by the RCS source control system. It will 
 * automatically update the version number.
 */
#define VERSION "1.17"
#define BUILD "$Revision: 1.7 $"


/* Prototypes */
void usage(void);
static void   _park_miller_srand();
static int    _park_miller_rand(void);
static double _park_miller_ran(void);
void AlternativeFiles(unsigned int, unsigned int, unsigned int);
void fillBlock(unsigned int, void *, unsigned int);
void fillBlock2(unsigned int, void *, unsigned int);
int createExtDedupeFiles(unsigned int, unsigned int, unsigned int,unsigned long *);
int createExtComprAndDedupFiles(unsigned int, unsigned int, unsigned int,unsigned long *);
int createExtIrreducibleFiles(unsigned int, unsigned int, unsigned int,unsigned long *);
/* Prototypes */

/* Define minimum number of bytes that may trigger dedupe */
#define GRANULE_SIZE 256

/* Define the density of compression within the granule size */
#define USE_DENSITY

#define DENSITY 5

#if defined(WIN32)
#define _MKDIR(path,mask)	_mkdir(path)
#else
#define _MKDIR mkdir	
#endif

#if defined(_Solaris_)
#define O_DIRECT 0
#endif

#if defined(_macos_)
#define O_DIRECT 0
#endif

#if defined(_hpux_)
#define O_DIRECT 0
#endif

typedef unsigned char uchar;
#define CALLOC(typ, n)  (typ*)(calloc((n), sizeof(typ)))

/* variables */
static int park_miller_seedi = 2231;
int do_irreducible, do_compress, do_dedupe, do_both;
unsigned int rannum;
int use_dev,use_dir,use_o_direct;
long page_size = 4096;
char fileName[256];      /* holds the file names. 	*/
char myname[256];        /* holds the myname. 	*/

/* Used for getopt */
int cret;

#if defined(_LARGEFILE_)
#define I_OPEN(x,y,z)   open64(x,(int)(y),(int)(z))
#else
#define I_OPEN(x,y,z)   open(x,(int)(y),(int)(z))
#endif

/*
 * Main entry point.
 */
int main(int argc, char* argv[])
{
	int	salt = 0;
	int	r;
	time_t	t1, t2;
	unsigned int blocksize=0, filesize=0, numberfiles=0;
	char dirname[256];

	strcpy(myname,argv[0]);
	if (argc < 2) 
	{
		fprintf(stderr, "\nEmerald COM data generator \tVersion %s  RCS  %s\n",VERSION,BUILD);
		usage();
		exit(1);
	}

	t1 = time(NULL);

	/*
	 * Various command line options 
	 */
	while((cret = getopt(argc,argv,"OCDBImvd:s:b:f:n:r:")) != EOF)
	{
		switch(cret){
		case 'b':	/* Use this blocksize */
			blocksize=(unsigned int) strtol(optarg,NULL,10);
			break;
		case 'v':	/* Print Version number				*/
			printf("Version %s \t RCS %s\n",VERSION,BUILD);
			exit(0);
			break;
		case 'O':	/* Use O_DIRECT 				*/
			use_o_direct=1;
			break;
		case 'C':	/* Enable Compression only 			*/
			do_compress=1;
			break;
		case 'D':	/* Enable Dedupe only 				*/
			do_dedupe=1;
			break;
		case 'B':	/* Enable Compression and dedupe 		*/
			do_both=1;
			break;
		case 'I':	/* Enable Irreducible only 			*/
			do_irreducible=1;
			break;
		case 'f':	/* Filesize in GiB 				*/
			filesize = (unsigned int) strtol(optarg,NULL,10);
			break;
		case 'n':	/* Number of files 				*/
			numberfiles = (unsigned int)strtol(optarg,NULL,10);
			break;
		case 's':	/* Salt value 					*/
			salt = (unsigned int)strtol(optarg,NULL,10);
			break;
		case 'd':	/* Chdir to this directory 			*/
			/*
			 * Change to <dir>, creating it if necessary
			 */
			use_dir=1;
			strcpy(dirname,optarg);
			r = chdir(dirname);

			if (r != 0) 
			{
				if (errno == ENOENT) 
				{
					fprintf(stderr, "Creating directory '%s'...\n", dirname);
					r = _MKDIR(dirname, 0777);
					if (r != 0) 
					{
						fprintf(stderr, "Failed to create '%s', error %d\n",
							dirname, errno);
						exit(-5);
					}
					r = chdir(dirname);
					if (r != 0) 
					{
						fprintf(stderr, "Failed to change directory to '%s' after creating\n", dirname);
						exit(-5);
					}
				}
				else 
				{
					fprintf(stderr, "Failed to change directory to '%s', error %d\n", dirname, errno);
					exit(-5);
				}
			}
			break;
		case 'r':
			strcpy(fileName,optarg);
			use_dev=1;
			break;
		default:
			usage();
			exit(1);
			break;
		}
	}
	
	if(use_dev && !((do_compress || do_dedupe || do_both || do_irreducible) && ((do_compress + do_dedupe + do_both + do_irreducible) == 1))  )
	{
		fprintf(stderr,"When using a raw device one must select one pattern type.\n");
		if(do_compress)
			fprintf(stderr,"Found Compression selection.\n");
		if(do_dedupe)
			fprintf(stderr,"Found Dedupe selection.\n");
		if(do_both)
			fprintf(stderr,"Found both Compression and Dedupe selection.\n");
		if(do_irreducible)
			fprintf(stderr,"Found Irreducible selection.\n");
		fprintf(stderr,"Device name:  %s.\n",fileName);
		exit(-5);
	}
	if(use_dir && use_dev)
	{
		fprintf(stderr,"You can not use -r and -d at the same time. Please make up your mind :-)\n");
		exit(-4);
	}
	if (salt == 0)
		salt = 79;

	fprintf(stderr, "\nEmerald COM data generator \tVersion %s  RCS  %s\n",VERSION,BUILD);
	/* 
	 * Start with repeatable salt value.
	 */
	_park_miller_srand(salt + 10000);

	/*
	 * Write the files.
	 */
	AlternativeFiles(blocksize, filesize, numberfiles);

	/*
	 * Record the time used.
	 */
	t2 = time(NULL);
	t2 -= t1;
	fprintf(stdout, "Elapsed time: ");
	if (t2 / 3600 > 0) {
		fprintf(stdout, "%d hours, ", (int)(t2 / 3600));
		t2 %= 3600;
	}
	if (t2 / 60 > 0) {
		fprintf(stdout, "%d minutes, ", (int)(t2 / 60));
		t2 %= 60;
	}
	fprintf(stdout, "%d seconds. \n", (int)t2);
	return 0;
}

/* 
 * Used to create buffers that are compressible, but not dedupable.
 * The input parameters to this function:
 *
 *  bls  	..	Block size in kbyte units.
 *  memarea 	..	Location where to put the data 
 *  gransz	..	size in bytes of the minimum granule to enable dedupe
 */
void fillBlock(unsigned int bls, void *memarea, unsigned int gransz)
{
	unsigned int totBytes;   /* Total size of the block in bytes.		*/
	unsigned limitloop;      /* Total number of loops to fill the block.	*/
	unsigned int i;          /* traditional loop control			*/
	unsigned int *placeData; /* pointer to fill data in the reserved mem area. */
	unsigned int dat_in;     /* used to fill the buffer. 			*/

	placeData = memarea;
	/* Get random number and start filling the data. */
	rannum = (unsigned int) _park_miller_rand();
	dat_in = (unsigned int)rannum;
	totBytes = bls * 1024;
	limitloop = totBytes / sizeof(totBytes); 
	for (i = 0; i < limitloop; i++)
	{

#if defined(USE_DENSITY)
		if((i % DENSITY) == 0)
			dat_in++;
#else
		dat_in++;
#endif

		/* switch to new random number at granule boundary */
		if( (( i * sizeof(dat_in)) % gransz) == 0)	
		{
		  dat_in = (unsigned int) _park_miller_rand();
		}
		/*placeData[i] = dat_in;*/

		/*
		 * We need to convert to Network neutral format, or there will be 
 	 	 * different compression for Big and Little Endians.
		 */
		placeData[i]=htonl(dat_in); /* Convert to one neutral format */
	}
	return; /* exit after filling the block. */
}

/* 
 * Used to create buffer that is non-compressible, but may be used for dedupe.
 * The input parameters to this function:
 *
 * bls  	..	Block size in kbyte units.
 * memarea 	..	Location where to put the data 
 * gransz	..	size in bytes of the minimum granule to enable dedupe
 */
void fillBlock2(unsigned int bls, void *memarea, unsigned int gransz)
{
	unsigned int totBytes;   /* Total size of the block in bytes.	*/
	unsigned limitloop;      /* Total number of loops to fill the block.*/
	unsigned int dat_in;     /* used to fill the buffer. 		*/
	unsigned int i;          /* traditional loop control.		*/
	unsigned int *placeData; /* pointer to fill data 		*/

	placeData = memarea;
	/* Get the random number and start filling the data. */
	rannum = (unsigned int) _park_miller_rand();
	dat_in = (unsigned int)rannum;
	totBytes = bls * 1024;
	limitloop = totBytes / sizeof(totBytes);
	for (i = 0; i < limitloop; i++)
	{
		dat_in = (unsigned int) _park_miller_rand();
		placeData[i] = dat_in;
	}
	return; /* Exit after filling the buffer. */
}

/*
 * Create file that is compressible but not dedupable.
 */
int createExtComprFiles(unsigned int numFiles, unsigned int flsz, unsigned int blcksz,unsigned long block[])
{
	/* 
	 * numfiles is the number of files to create.
	 * flsz is the file size, for the moment this is a fixed size.
	 * blcksz is the block size to fill the file.
	 * block[] points to the memory area that contains the data to save.
	 */
	unsigned long i, j;   /* i is the file counter,j is the Block counter */
	double fls, bls;      /* file and block size in bytes held */
	unsigned long blcksTWrt; /* Number of blocks to write */
	int ret,fd;
	int flags=0;
	int pmode;

#if defined(WIN32)
	if(use_dev)
	   	flags = _O_WRONLY|_O_BINARY;
	else
		flags = _O_CREAT|_O_WRONLY|_O_BINARY;
	pmode = _S_IWRITE;
	if(use_o_direct) /* Don't have this in windows */
	   flags |= 0;

#else
	pmode = 0666;
	flags = O_CREAT|O_RDWR;
	if(use_o_direct)
	   flags |= O_DIRECT;
#endif
	/* Calculate the number of blocks to write. 						*/
	fls = (double)flsz*1024*1024*1024; /* file Size in GiBs; we need the number of bytes. . */
	bls = blcksz * 1024;               /* block size in KiB; we need the number of bytes.   */
	blcksTWrt = fls/bls;               /* here is the total of blocks to write. 		*/
				
	/*
 	 * This loop generates the files of the data set.
	 */
	if(use_dev)
		numFiles=1;
	for (i = 0; i < numFiles; i++)
	{
		if(!use_dev)
		{
			sprintf(fileName, "Compress_no_dedupe_%d.dat", (int)i);
			remove(fileName);
		}
		fd=I_OPEN(fileName,flags,pmode);
         	if(fd <0)
         	{
			fprintf(stderr,"Error opening file/device: %s\n",strerror(errno));
			exit(-2);
		}
#if defined(_Solaris_)
		if(use_o_direct)
			directio(fd,DIRECTIO_ON);
#endif

#if defined(_macos_)
		if(use_o_direct)
			fcntl(fd,F_NOCACHE,1);
#endif
#if defined(_hpux_)
		if(use_o_direct)
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
#endif
		if(use_dev)
			fprintf(stderr, "Filling device: %s with compressible data\n", fileName); 
		else
			fprintf(stderr, "Filling file: %s with compressible data\n", fileName); 

		/* Dump the blocks into the file. */
		for (j = 0; j < blcksTWrt; j++)
		{
			/* RE-DO THE PATTERN FOR EVERY BLOCK */
			fillBlock(blcksz, block, GRANULE_SIZE); 
#if defined(WIN32)
			ret=_write(fd, block, (blcksz * 1024));
#else
			ret=write(fd, block, (blcksz * 1024));
#endif
			if(ret < 0)
			{
				printf("%s\n",strerror(errno));
				exit(-3);
			}
		};
#if !defined(WIN32)
		fsync(fd);
#endif
		close(fd);
		if(use_dev)
			fprintf(stderr, "Filled device %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
		else
			fprintf(stderr, "Generated file %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
	};
	if(!use_dev)
		fprintf(stderr, "Created %d files for this data set\n\n", (int)i);
	return 0;
}

/*
 * Create files that are Dedupable but not compressible.
 *
 * numfiles is the number of files to create.
 * flsz is the file size, for the moment this is a fixed size.
 * blcksz is the block size to fill the file.		
 * block[] points to the memory area that contains the data to save.
 */
int createExtDedupeFiles(unsigned int numFiles, unsigned int flsz, unsigned int blcksz,unsigned long block[])
{

	unsigned long i, j;      /* i is the file counter, j is the Block counter 		*/
	double fls, bls;         /* file and block size in bytes held here respectively. 	*/
	unsigned long blcksTWrt; /* Number of blocks to write 					*/
	int ret,fd;
	int flags = 0;
	int pmode;

#if defined(WIN32)
	if(use_dev)
	   	flags = _O_WRONLY|_O_BINARY;
	else
		flags = _O_CREAT|_O_WRONLY|_O_BINARY;
	pmode = _S_IWRITE;
	if(use_o_direct) /* Don't have this in windows */
	   flags |= 0;
#else
	pmode = 0666;
	flags = O_CREAT|O_RDWR;
	if(use_o_direct)
	   flags |= O_DIRECT;
#endif
	/* Calculate the number of blocks to write. 						*/
	fls = (double)flsz*1024*1024*1024; /* file Size in GiBs; we need the number of bytes. 	*/
	bls = blcksz * 1024;               /* block size in KiB; we need the number of bytes. 	*/
	blcksTWrt = fls/bls;               /* here is the total of blocks to write. 		*/

	/* 
	 * This loop generates the files of the data set.
	 */
	if(use_dev)
		numFiles=1;

	for (i = 0; i < numFiles; i++)
	{

		if(!use_dev)
		{
			sprintf(fileName, "Dedupe_no_compress_%d.dat", (int)i);
			unlink(fileName);
		}
		fd=I_OPEN(fileName,flags,pmode);
         	if(fd <0)
         	{
			fprintf(stderr,"Error opening file/device: %s\n",strerror(errno));
			exit(-2);
		}
#if defined(_Solaris_)
		if(use_o_direct)
			directio(fd,DIRECTIO_ON);
#endif
#if defined(_macos_)
		if(use_o_direct)
			fcntl(fd,F_NOCACHE,1);
#endif
#if defined(_hpux_)
		if(use_o_direct)
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
#endif
		if(use_dev)
			fprintf(stderr, "Filling device: %s with Dedupe data\n", fileName); 
		else
			fprintf(stderr, "Filling file: %s with Dedupe data\n", fileName); 

		fillBlock2(blcksz, block, GRANULE_SIZE);  /* Create non-compressible pattern */
		/* dump the blocks into the file. */
		for (j = 0; j < blcksTWrt; j++)
		{
#if defined(WIN32)
			ret=_write(fd, block, (blcksz * 1024));
#else
			ret=write(fd, block, (blcksz * 1024));
#endif
			if(ret < 0)
			{
				printf("%s\n",strerror(errno));
				exit(-3);
			}
		};
#if !defined(WIN32)
		fsync(fd);
#endif
		close(fd);
		if(use_dev)
			fprintf(stderr, "Filled device %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
		else
			fprintf(stderr, "Generated file %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
	};
	if(!use_dev)
		fprintf(stderr, "Created %d files for this data set\n\n", (int)i);
	return 0;
}

/*
 * Create files that are both compressible and dedupable.
 * 
 * numfiles is the number of files to create.
 * flsz is the file size, for the moment this is a fixed size.
 * blcksz is the block size to fill the file.
 * block[] points to the memory area that contains the data to save.
 */
int createExtComprAndDedupFiles(unsigned int numFiles, unsigned int flsz, unsigned int blcksz,unsigned long block[])
{
	unsigned long i, j;      /* i is the file counter, j is the Block counter 			*/
	double fls, bls;         /* file and block size in bytes held here respectively. 		*/
	unsigned long blcksTWrt; /* Number of blocks to write 						*/
	int ret,fd;
	int flags = 0;
	int pmode;
#if defined(WIN32)
	if(use_dev)
	   	flags = _O_WRONLY|_O_BINARY;
	else
		flags = _O_CREAT|_O_WRONLY|_O_BINARY;
	pmode = _S_IWRITE;
	if(use_o_direct) /* Don't have this in windows */
	   flags |= 0;
#else
	pmode = 0666;
	flags = O_CREAT|O_RDWR;
	if(use_o_direct)
	   flags |= O_DIRECT;
#endif

	/* Calculate the number of blocks to write. */
	fls = (double)flsz*1024*1024*1024; /* file Size in GiBs; we need the number of bytes. 		*/
	bls = blcksz * 1024;               /* block size in KiB; we need the number of bytes. 		*/
	blcksTWrt = fls/bls;               /* here is the total of blocks to write. 			*/

	/*
	 * This loop generates the files of the data set.
	 */
	if(use_dev)
		numFiles=1;
	for (i = 0; i < numFiles; i++)
	{
		if(!use_dev)
		{
			sprintf(fileName, "Compress_and_dedupe_%d.dat", (int)i);
			unlink(fileName);
		}
		fd=I_OPEN(fileName,flags,pmode);
         	if(fd <0)
         	{
			fprintf(stderr,"Error opening file/device: %s\n",strerror(errno));
			exit(-2);
		}
#if defined(_Solaris_)
		if(use_o_direct)
			directio(fd,DIRECTIO_ON);
#endif
#if defined(_macos_)
		if(use_o_direct)
			fcntl(fd,F_NOCACHE,1);
#endif
#if defined(_hpux_)
		if(use_o_direct)
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
#endif
		if(use_dev)
			fprintf(stderr, "Filling device: %s with compress and dedupe data\n", fileName); 
		else
			fprintf(stderr, "Filling file: %s with compress and dedupe data\n", fileName); 

		fillBlock(blcksz, block, GRANULE_SIZE); /* RE-DO THE PATTERN FOR EVERY BLOCK */
		/* Dump the blocks into the file. */
		for (j = 0; j < blcksTWrt; j++)
		{
#if defined(WIN32)
			ret=_write(fd, block, (blcksz * 1024));
#else
			ret=write(fd, block, (blcksz * 1024));
#endif
			if(ret < 0)
			{
				printf("%s\n",strerror(errno));
				exit(-3);
			}
		};
#if !defined(WIN32)
		fsync(fd);
#endif
		close(fd);
		if(use_dev)
			fprintf(stderr, "Filled device %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
		else
			fprintf(stderr, "Generated file %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
	};
	if(!use_dev)
		fprintf(stderr, "Created %d files for this data set\n\n", (int)i);
	return 0;
}

/*
 * Create all of the 4 types of files needed for the Emerald COM tests.
 */
void AlternativeFiles(unsigned int blocksize, unsigned int filesize, unsigned int numberfiles)
{
	/* 
	 * This function can generate huge files and even bigger data sets.
	 * The defaults are:
	 *   block size: ........ 32 KiB
	 *   file size: ......... 1 GiB 
	 *   Number of Files: ... 4
	 */
	void *block;

	if(!blocksize)
		blocksize = 32;
	if(!filesize)
		filesize = 1;
	if(!numberfiles)
		numberfiles = 4;
	
	/* Reserve the memory space for one single block of data. */
	block = calloc(1, ((blocksize * 1024) + page_size));
	if (block == NULL)
	{
		fprintf(stderr, "Error: out of memory\n");
		exit(-1);
	}
	block =(char *)(((long)block+(long)page_size) & (long)~(page_size-1));

	/* If no further selection, then do all 4 types */
	if(!do_compress && !do_dedupe && !do_both && !do_irreducible)
	{
		createExtComprFiles(numberfiles, filesize, blocksize, block);
		createExtDedupeFiles(numberfiles, filesize, blocksize, block);
		createExtComprAndDedupFiles(numberfiles, filesize, blocksize, block);
		createExtIrreducibleFiles(numberfiles, filesize, blocksize, block);
	}	

	/* Permit individual selection */
	if(do_compress)
		createExtComprFiles(numberfiles, filesize, blocksize, block);
	if(do_dedupe)
		createExtDedupeFiles(numberfiles, filesize, blocksize, block);
	if(do_both)
		createExtComprAndDedupFiles(numberfiles, filesize, blocksize, block);
	if(do_irreducible)
		createExtIrreducibleFiles(numberfiles, filesize, blocksize, block);
	return;
}

/*
 * Create files that are irreducible.
 *
 * numfiles is the number of files to create.
 * flsz is the file size, for the moment this is a fixed size.
 * blcksz is the block size to fill the file.
 * block[] points to the memory area that contains the data to save.
 */
int createExtIrreducibleFiles(unsigned int numFiles, unsigned int flsz, unsigned int blcksz,unsigned long block[])
{
	unsigned long i, j;      /* i is the file counter, j is the Block counter		*/
	double fls, bls;         /* file and block size in bytes held here respectively.	*/
	unsigned long blcksTWrt; /* Number of blocks to write 					*/
	int fd,ret;
	int flags = 0;
	int pmode;

#ifdef WIN32
	if(use_dev)
	   	flags = _O_WRONLY|_O_BINARY;
	else
		flags = _O_CREAT|_O_WRONLY|_O_BINARY;
	pmode = _S_IWRITE;
	if(use_o_direct) /* Don't have this in Windows */
	   flags |= 0;
#else
	pmode = 0666;
	flags = O_CREAT|O_RDWR;
	if(use_o_direct)
	   flags |= O_DIRECT;
#endif

	/* Calculate the number of blocks to write. */
	fls = (double)flsz*1024*1024*1024; /* file Size in GiBs; we need the number of bytes. 	*/
	bls = blcksz * 1024;               /* block size in KiB; we need the number of bytes. 	*/
	blcksTWrt = fls/bls;               /* here is the total of blocks to write. 		*/

	/*
	 * This loop generates the files of the data set.
	 */
	if(use_dev)
		numFiles=1;
	for (i = 0; i < numFiles; i++)
	{
		if(!use_dev)
		{
			sprintf(fileName, "Irreducible_%d.dat", (int)i);
			unlink(fileName);
		}
		fd=I_OPEN(fileName,flags,pmode);
         	if(fd <0)
         	{
			fprintf(stderr,"Error opening file/device: %s\n",strerror(errno));
			exit(-2);
		}
#if defined(_Solaris_)
		if(use_o_direct)
			directio(fd,DIRECTIO_ON);
#endif
#if defined(_macos_)
		if(use_o_direct)
			fcntl(fd,F_NOCACHE,1);
#endif
#if defined(_hpux_)
		if(use_o_direct)
			ioctl(fd,VX_SETCACHE,VX_DIRECT);
#endif
		if(use_dev)
			fprintf(stderr, "Filling device: %s with irreducible data\n", fileName); 
		else
			fprintf(stderr, "Filling file: %s with irreducible data\n", fileName); 
		/* Dump the blocks into the file. */
		for (j = 0; j < blcksTWrt; j++)
		{
			fillBlock2(blcksz, block, GRANULE_SIZE); /* RE DO THE PATTERN FOR EVERY BLOCK */
#if defined(WIN32)
			ret=_write(fd, block, (blcksz * 1024));
#else
			ret=write(fd, block, (blcksz * 1024));
#endif
			if(ret < 0)
			{
				printf("%s\n",strerror(errno));
				exit(-3);
			}
		};
#if !defined(WIN32)
		fsync(fd);
#endif
		close(fd);
		if(use_dev)
			fprintf(stderr, "Filled device %s with %d Blocks of size %dKiB\n", fileName,(int)j, (int)blcksz);
		else
			fprintf(stderr, "Generated file %s with %d Blocks of size %dKiB\n", fileName, (int)j, (int)blcksz);
	};
	if(!use_dev)
		fprintf(stderr, "Created %d files for this data set\n\n", (int)i);
	return 0;
}


/* 
 * The Park and Miller LC random number generator entry points.
 */

/*
 * Seed the random number generator.
 */
static void
_park_miller_srand(int seed)
{
    park_miller_seedi = seed;
}

/*
 * Returns a random number.
 */
static int
_park_miller_rand(void)
{
    (void) _park_miller_ran();
    return(park_miller_seedi);
}

/***************************************************************/
/* See "Random Number Generators: Good Ones Are Hard To Find", */
/*     Park & Miller, CACM 31#10 October 1988 pages 1192-1201. */
/***************************************************************/
/* THIS IMPLEMENTATION REQUIRES AT LEAST 32 BIT INTEGERS !     */
/***************************************************************/

/*
 * Compute the next random number.
 */
static double
_park_miller_ran(void)

#if defined(WIN32)
#define int32_t int
#endif

#define _PARK_A_MULTIPLIER  16807L
#define _PARK_M_MODULUS     2147483647L /* (2**31)-1 */
#define _PARK_Q_QUOTIENT    127773L     /* 2147483647 / 16807 */
#define _PARK_R_REMAINDER   2836L       /* 2147483647 % 16807 */
{
    	int32_t	lo;
    	int32_t	hi;
    	int32_t	test;

    	hi = park_miller_seedi / _PARK_Q_QUOTIENT;
    	lo = park_miller_seedi % _PARK_Q_QUOTIENT;
    	test = _PARK_A_MULTIPLIER * lo - _PARK_R_REMAINDER * hi;
    	if (test > 0) 
	{
		park_miller_seedi = test;
    	} 
	else 
	{
		park_miller_seedi = test + _PARK_M_MODULUS;
    	}
    	return((float) park_miller_seedi / _PARK_M_MODULUS);
}

void
usage(void)
{
	fprintf(stderr, "Usage: %s\n",myname);
	fprintf(stderr,"\t-d <dir> \n");
	fprintf(stderr,"\t[-s] <salt>] \n");
	fprintf(stderr,"\t[-n  number of files]  Defaults to 4.\n");
	fprintf(stderr,"\t[-b  blocksize] (in KiB) Enables pattern generation.\n");
	fprintf(stderr,"\t[-C] Selective pattern generation for Compression no dedupe files.\n");
	fprintf(stderr,"\t[-D] Selective pattern generation for Dedupe no compression files.\n");
	fprintf(stderr,"\t[-B] Selective pattern generation for Dedupe and compression files.\n");
	fprintf(stderr,"\t[-I] Selective pattern generation for irreducible files.\n");
	fprintf(stderr,"\t[-f  filesize] (in GiB)  Enables pattern generation.\n");
	fprintf(stderr,"\t[-r  devicename] Use this raw block device.\n");
	fprintf(stderr,"\t[-O] Use O_DIRECT. ( If it works on your box )\n");
	fprintf(stderr,"\t[-v] Print version number. \n\n");
	fprintf(stderr, "\tWarning: %s writes a minimum of 4GB of files to the directory \n\tspecified in <dir>\n", myname);
}
