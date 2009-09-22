#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#else
#define off64_t __int64
#endif
                                                                                
#ifndef off64_t
#define off64_t int64_t
#endif

#include "smdfc.h"

int main(int argc, char **argv)
{
	int	fd;
	char	*file;
	int	action,location,size;
	unsigned char	*p,val;
	int	status;

	/* filename action loc size */
	if (argc != 6) {
		fprintf(stderr,"Usage: %s file action loc size val\n",argv[0]);
		exit(1);
	}
	file = argv[1];
	action = atoi(argv[2]);
	location = atoi(argv[3]);
	size = atoi(argv[4]);
	val = atoi(argv[5]);

	fd = smdfs_open(file,O_RDONLY,0);
	if (fd == -1) {
		fprintf(stderr,"Open(%s) failed %d\n",file,fd);
		exit(1);
	}

	p = (unsigned char *)malloc(size);
	memset(p,val,size);

	switch(action) {
		case 0:
			status = smdfs_lseek64(fd,0,SEEK_END);
			printf("Stat=%d\n",status);
			break;
		case 1:
			status = smdfs_lseek64(fd,location,SEEK_SET);
			printf("Stat=%d\n",status);
			status = smdfs_read(fd,p,size);
			printf("Read=%d\n",status);
			break;
		case 2:
			status = smdfs_lseek64(fd,location,SEEK_SET);
			printf("Stat=%d\n",status);
			status = smdfs_write(fd,p,size);
			printf("Write=%d\n",status);
			break;
		case 3:
			status = smdfs_kill(fd);
			printf("Kill=%d\n",status);
			break;
	}

	free(p);

	smdfs_close(fd);

	exit(0);
}
