/*
** $RCSfile: smdfc.c,v $
** $Name:  $
**
** ASCI Visualization Project 
**
** Lawrence Livermore National Laboratory
** Information Management and Graphics Group
** P.O. Box 808, Mail Stop L-561
** Livermore, CA 94551-0808
**
** For information about this project see:
** 	http://www.llnl.gov/icc/sdd/img/viz.shtml
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: smdfc.c,v 1.4 2008/02/08 21:55:05 wealthychef Exp $
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "smdfcP.h"
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
dfcfd *fdlist = NULL;
pthread_mutex_t listmutex = PTHREAD_MUTEX_INITIALIZER;

/* DFC filenames:   dfc:hostname:port:key{:filename} */
int parsedfcname(const char *in,char *host, int *port, char *key)
{
	const char *p = in;

	/* prefix */
	if (strncmp(p,"dfc:",4) != 0) return(0);
	p += 4;

	/* hostname */
	while(*p && (*p != ':')) *host++ = *p++;
	*host++ = '\0';
	if (!strlen(host)) strcpy(host,"localhost");
	if (!*p) return(0);
	p++;
	if (!*p) return(0);

	/* port number */
	*port = atoi(p);
	p = strchr(p,':');
	if (!p) return(0);

	/* key */
	while(*p && (*p != ':')) *key++ = *p++;
	*key++ = '\0';

	/* no filename for now */

	return(1);
}
dfcfd *new_fd(char *host,int port)
{
        struct  sockaddr_in sin;
        struct  hostent *hp;
	int	addr_len;
	int	err;
	short	endian;
 
	dfcfd *p = (dfcfd *)calloc(sizeof(dfcfd),1);
	if (!p) return(NULL);
	
	/* open the connection */
	hp = gethostbyname(host);
	if (!hp) {
		free(p);
		return(NULL);
	}

	/* create the socket */
	p->socket = socket(AF_INET,SOCK_STREAM,0);
	if (p->socket == -1) {
		free(p);
		return(NULL);
	}

	/* connect the socket to the server thread */
	sin.sin_addr.s_addr = ((struct in_addr *)(hp->h_addr))->s_addr;
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	addr_len = sizeof(sin);
	if (connect(p->socket,(const struct sockaddr *)&sin,addr_len) == -1) {
		close(p->socket);
		free(p);
		return(NULL);
	}

	/* all swapping is done on the client (if needed) */
	/* the server sent us a 0x0001 */
	err = sock_read(p->socket,sizeof(endian),0,(char *)&endian);
	if (err != sizeof(endian)) {
		close(p->socket);
		free(p);
		return(NULL);
	}
	if (endian != 1) {
		p->swab = 1;
	} else {
		p->swab = 0;
	}
	/* to ensure we do not read/write past max buffer length */
	err = sock_read(p->socket,sizeof(p->max_size),
		p->swab*sizeof(p->max_size),(char *)&(p->max_size));
	if (err != sizeof(p->max_size)) {
		close(p->socket);
		free(p);
		return(NULL);
	}

	pthread_mutex_lock(&listmutex);
	p->next = fdlist;
	fdlist = p;
	pthread_mutex_unlock(&listmutex);

	return(p);
}
void free_fd(dfcfd *t)
{
	dfcfd *p,*l = NULL;
	pthread_mutex_lock(&listmutex);
	p = fdlist;
	while((p) && (p != t)) {l = p; p = p->next; }
	if (p) {
		if (l) {
			l->next = p->next;
		} else {
			fdlist = p->next;
		}

		/* close down connection */
		close(t->socket);

		free(t);
	} else {
		fprintf(stderr,"Internal error: freeing unknown fd\n");
	}
	pthread_mutex_unlock(&listmutex);
	return;
}
dfcfd *isdfcfd(int fd)
{
	dfcfd *p;
	pthread_mutex_lock(&listmutex);
	p = fdlist;
	while(p) {
		if (p->socket == fd) {
			pthread_mutex_unlock(&listmutex);
			return(p);
		}
		p = p->next;
	}
	pthread_mutex_unlock(&listmutex);
	return(NULL);
}
int smdfs_open(const char *path, int flags, int mode)
{
	char	host[256];
	char 	key[256];
	int	port,ret;
	dfcfd	*p;

#ifdef CMD_DEBUG
printf("smdfs_open_begin: %s\n",path);
#endif
	if (!parsedfcname(path,host,&port,key)) return(OPEN(path,flags,mode));
	p = new_fd(host,port);
	if (!p) return(-1);
	ret = send_command(p,DFC_CMD_OPEN,strlen(key)+1,key,flags);
	if (ret == -1) {
		free_fd(p);
		return(ret);
	}
#ifdef CMD_DEBUG
printf("smdfs_open_end: %d\n",p->socket);
#endif
	return(p->socket);
}
off64_t smdfs_lseek64(int fd, off64_t offset, int whence)
{
	dfcfd *f = isdfcfd(fd);
    if (!f) return(LSEEK64(fd,offset,whence));
      
#ifdef CMD_DEBUG
printf("smdfs_lseek: %d %lld %d\n",fd,offset,whence);
#endif
	if (whence == SEEK_SET) {
		f->offset = offset;
	} else if (whence == SEEK_CUR) {
		f->offset += offset;
	} else if (whence == SEEK_END) {
		/* must get the current file size for this */
		send_command(f,DFC_CMD_STAT,0,NULL,0);
		f->offset = f->size - offset;
	} else {
		return(-1);
	}
	return(f->offset);
}
int smdfs_read(int fd, void *buf, unsigned int count)
{
	dfcfd *f = isdfcfd(fd);
	if (!f) return(READ(fd,buf,count));
#ifdef CMD_DEBUG
printf("smdfs_read: %d %d\n",fd,count);
#endif
	return(send_command(f,DFC_CMD_READ,count,buf,0));
}
int smdfs_write(int fd, void *buf, unsigned int count)
{
	dfcfd *f = isdfcfd(fd);
	if (!f) return(WRITE(fd,buf,count));
#ifdef CMD_DEBUG
printf("smdfs_write: %d %d\n",fd,count);
#endif
	return(send_command(f,DFC_CMD_WRITE,count,buf,0));
}
int smdfs_close(int fd)
{
	int ret;
	dfcfd *f = isdfcfd(fd);
	if (!f) return(CLOSE(fd));
#ifdef CMD_DEBUG
printf("smdfs_close: %d\n",fd);
#endif
	ret = send_command(f,DFC_CMD_CLOSE,0,NULL,0);
	free_fd(f);
	return(ret);
}
int smdfs_kill(int fd)
{
	int ret;
	dfcfd *f = isdfcfd(fd);
	if (!f) return(-1);
#ifdef CMD_DEBUG
printf("smdfs_kill: %d\n",fd);
#endif
	ret = send_command(f,DFC_CMD_KILL,0,NULL,0);
	free_fd(f);
	return(ret);
}

/* Commands are sent as:
 * Command 4 bytes
 * Payload size 4 bytes (or return value size)
 * Flags 4 bytes
 */
int send_command(dfcfd *p,int cmd,unsigned int cnt,void *buf,int flg)
{
	int status;
	unsigned int buffer[3];

	if ((cmd == DFC_CMD_READ) || (cmd == DFC_CMD_WRITE)) {
		/* illegal past the end of buffer */
		if (p->offset+cnt > p->max_size) return(-1);
	}

	/* send the command + size + flags */
	buffer[0] = cmd;
	buffer[1] = cnt;
	buffer[2] = flg;
	status=sock_write(p->socket,3*sizeof(int),p->swab*sizeof(int),
		(char *)buffer);
	if (status != 3*sizeof(int)) return(-1);

	/* write payload */
	switch(cmd) {
		case DFC_CMD_WRITE: {
			off64_t	off = p->offset;
			/* send position */
			sock_write(p->socket,sizeof(p->offset),
				p->swab*sizeof(p->offset),(char *)&(off));
			/* send the raw data */
			status=sock_write(p->socket,cnt,0,(char *)buf);
			if (status != cnt) return(-1);
			p->offset += status;
			}
			break;
		case DFC_CMD_OPEN:
			/* send the key */
			status=sock_write(p->socket,cnt,0,(char *)buf);
			if (status != cnt) return(-1);
			status = 0;
			break;
		case DFC_CMD_READ: {
			off64_t	off = p->offset;
			/* send position */
			sock_write(p->socket,sizeof(p->offset),
				p->swab*sizeof(p->offset),(char *)&(off));
			}
			break;
		case DFC_CMD_CLOSE:
		case DFC_CMD_STAT:
		case DFC_CMD_KILL:
			status = 0;
			break;
	}
	/* read payload */
	switch(cmd) {
		case DFC_CMD_OPEN:
			p->offset = 0;
			break;
		case DFC_CMD_READ:
			/* read the raw data */
			status=sock_read(p->socket,cnt,0,(char *)buf);
			if (status != cnt) return(-1);
			p->offset += status;
			break;
		case DFC_CMD_STAT:
			/* read the file size */
			status=sock_read(p->socket,sizeof(p->size),
				sizeof(p->size)*p->swab,(char *)&(p->size));
			if (status != sizeof(p->size)) return(-1);
			status = 0;
			break;
		case DFC_CMD_WRITE:
			break;
		case DFC_CMD_CLOSE:
		case DFC_CMD_KILL:
			status = 0;
			break;
	}
	return(status);
}

int sock_write(int s,int num, int size, char *buffer)
{
	int n = 0;
#ifdef IO_DEBUG
printf("Writing: %d %d Data: ",num,size);
#endif
	if (size > 1) byteswap(buffer,num,size);
	while(num) {
		int status = write(s,buffer,num);
		if (status == -1) {
			if ((errno != EINTR)  && (errno != EAGAIN)) {
				return(-1);
			}
			status = 0;
		} else if (status == 0) {
			return(-1);
		}
#ifdef IO_DEBUG
{ int i; for(i=0;i<status;i++) printf(" %d ",buffer[i]); }
#endif
		buffer += status;
		num -= status;
		n += status;
	}
#ifdef IO_DEBUG
printf("Done\n");
#endif
	return(n);
}
int sock_read(int s,int num, int size, char *buffer)
{
	char *p = buffer;
	int n = 0;
#ifdef IO_DEBUG
printf("Reading: %d %d Data: ",num,size);
#endif
	while(num) {
		int status = read(s,buffer,num);
		if (status == -1) {
			if ((errno != EINTR)  && (errno != EAGAIN)) {
				return(-1);
			}
			status = 0;
		} else if (status == 0) {
			return(-1);
		}
#ifdef IO_DEBUG
{ int i; for(i=0;i<status;i++) printf(" %d ",buffer[i]); }
#endif
		buffer += status;
		num -= status;
		n += status;
	}
#ifdef IO_DEBUG
printf("Done\n");
#endif
	if (size > 1) byteswap(p,n,size);
	return(n);
}

void byteswap(void *buffer,long int len,long int swapsize)
{
    long int num;
    char *p = (char *)buffer;
    char  t;

    switch(swapsize) {
    case 2:
        num = len/swapsize;
        while(num--) {
            t = p[0]; p[0] = p[1]; p[1] = t;
            p += swapsize;
        }
        break;
    case 4:
        num = len/swapsize;
        while(num--) {
            t = p[0]; p[0] = p[3]; p[3] = t;
            t = p[1]; p[1] = p[2]; p[2] = t;
            p += swapsize;
        }
        break;
    case 8:
        num = len/swapsize;
        while(num--) {
            t = p[0]; p[0] = p[7]; p[7] = t;
            t = p[1]; p[1] = p[6]; p[6] = t;
            t = p[2]; p[2] = p[5]; p[5] = t;
            t = p[3]; p[3] = p[4]; p[4] = t;
            p += swapsize;
        }
        break;
    default:
        break;
    }
    return;
}

