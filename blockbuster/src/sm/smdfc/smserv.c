/*
** $RCSfile: smserv.c,v $
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
** $Id: smserv.c,v 1.3 2007/06/13 19:00:32 wealthychef Exp $
**
*/
/*
**
**  Abstract:  Server for the dfc memory file system.
**
**  Author: rjf
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include "smservP.h"

#include <pthread.h>
#include <sched.h>

int	max_rank;
int	rank;
int	blocksize = 8192;
short	port = BASE_PORT;
char	*key = "";
off64_t max_size = 1024*1024*10;

/* runtime global state */
off64_t file_size = 0;
int	isdying = 0;
unsigned char *buffer = NULL;
int listen_socket = -1;
pthread_mutex_t list_mutex;
dfccmd *cmdlist = NULL;
volatile int nthreads = 0;

/* local prototypes */
off64_t get_local_size(void);
off64_t get_local_req_size(basecmd *cmd, int rank);
off64_t get_local_req_offset(basecmd *cmd, int rank);
void *handle_proc(void *);
void *accept_proc(void *);
void handle_cmdlist(dfccmd *l);
void handle_command(basecmd *cmd,unsigned char *buff,int r);
void ensure_length(int l,unsigned char **v,int *len);
int get_node_buffer_offset(int block,int r);
void pack_buffer(basecmd *cmd,unsigned char *src,unsigned char *dst);
void unpack_buffer(basecmd *cmd,unsigned char *src,unsigned char *dst);

void cmdline(char *s)
{
	if (!rank) {
	fprintf(stderr,"(%s) usage: %s [preloadfile]\n",__DATE__,s);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"\t-blocksize num  Striping block size (default:8192)\n");
	fprintf(stderr,"\t-portbase num  First port number to try (default:%d)\n",BASE_PORT);
	fprintf(stderr,"\t-key string  Cookie key to use (default:\"\")\n");
	fprintf(stderr,"\t-maxsize num  Maximum size of cache (default:preloadfilesize or 10MB)\n");
	fprintf(stderr,"\t-ssi MPI on an single system image (default:distributed)\n");
	}

	MPI_Finalize();
	exit(1);
}

void ensure_length(int l,unsigned char **v,int *len)
{
	if (l < *len) return;
	if (*v) free(*v);
	*v = (unsigned char *)malloc(l);
	*len = l;
	return;
}

/* Entry point for the dfc file service */
int main(int argc,char **argv)
{
	pthread_t acc_thr;
	int 	fd = -1;
	int 	i = 1;
	int 	done;
	int	ssi = 0;

	int		cmd_status;
	MPI_Request	data_req;
	MPI_Request	cmd_req;
	basecmd		cmd_buffer;
	unsigned char   *lcl_buffer = NULL;
	int		lcl_buffer_len = 0;

	MPI_Init(&argc,&argv);

	MPI_Comm_size(MPI_COMM_WORLD, &max_rank);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	while((i < argc) && (argv[i][0] == '-')) {
		if ((strcmp(argv[i],"-blocksize") == 0) && (i+1 < argc)) {
			blocksize = atoi(argv[++i]);
		} else if ((strcmp(argv[i],"-portbase") == 0) && (i+1 < argc)) {
			port = atoi(argv[++i]);
		} else if (strcmp(argv[i],"-ssi") == 0) {
			ssi = 1;
		} else if ((strcmp(argv[i],"-key") == 0) && (i+1 < argc)) {
			key = argv[++i];
		} else if ((strcmp(argv[i],"-maxsize") == 0) && (i+1 < argc)) {
			double d;
			sscanf(argv[++i],"%lf",&d);
			max_size = d;
		} else {
			if (!rank) {
				fprintf(stderr,"Unknown option: %s\n",argv[i]);
			}
			cmdline(argv[0]);
		}
		i++;
	}
	if (i == argc-1) {
		off64_t	flen;
		fd = OPEN(argv[i],O_RDONLY);
		if (fd == -1) {
			fprintf(stderr,"Unable to open file %s\n",argv[i]);
			MPI_Finalize();
			exit(1);
		}
		flen = LSEEK64(fd,0,SEEK_END);
		if (flen > max_size) max_size = flen;
	} else if (i != argc) cmdline(argv[0]);

	/* find a local (group) port number to use */
	done = 0;
	listen_socket = -1;
	if (ssi) port += rank;
	port--;
	i = 0;
	while(done != max_rank) {
		int iOk = 0;
		struct sockaddr_in sin;
		port++; 

		/* too many tries? */
		i++;
		if (i > 10000) {
			fprintf(stderr,"Unable to set up listen socket\n");
			MPI_Finalize();
			exit(0);
		}

		/* try the next port */
		if (listen_socket != -1) close(listen_socket);
		listen_socket = socket(AF_INET,SOCK_STREAM,0);
		if (listen_socket != -1) {
			sin.sin_family = AF_INET;
			sin.sin_addr.s_addr = htonl(INADDR_ANY);
			sin.sin_port = htons(port);
			if (bind(listen_socket,(struct sockaddr *)&sin,
				sizeof(sin)) != -1) iOk = 1;
		}

		/* do we all agree? */
		MPI_Allreduce(&iOk,&done,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
	}
	if (listen(listen_socket,5) == -1) {
		fprintf(stderr,"Unable to set up listen socket\n");
		MPI_Finalize();
		exit(0);
	}

	/* get buffer memory */
	buffer = (unsigned char *)malloc(get_local_size());
	if (!buffer) {
		/* TODO: tell others to "KILL" */
		fprintf(stderr,"Unable to allocate local buffer\n");
		MPI_Finalize();
		exit(-1);
	}

	/* report back to the user the "URL" */
	if (rank == 0) printf("dfc:localhost:%d:%s\n",port,key);

	/* read prefetch file */
	if (fd != -1) {
		unsigned char *p = buffer;
		off64_t loc = rank*blocksize;
		file_size = LSEEK64(fd,0,SEEK_END);
		while(loc < file_size) {
			int j = blocksize;
			LSEEK64(fd,loc,SEEK_SET);
			if (loc+j > file_size) j = file_size - loc;
			if (j > 0) READ(fd,p,j);
			p += blocksize;
			loc += max_rank*blocksize;
		}
		CLOSE(fd);
	}

	/* init the list control mutex */
	pthread_mutex_init(&list_mutex,NULL);

	/* launch the "accept" thread */
	pthread_create(&acc_thr,NULL,accept_proc,NULL);
	pthread_detach(acc_thr);

	/* watch for messages & handle the command list */
	MPI_Irecv(&cmd_buffer,sizeof(cmd_buffer),MPI_BYTE,MPI_ANY_SOURCE,
		CMD_TAG,MPI_COMM_WORLD,&cmd_req);
	cmd_status = 1; /* posted cmd_recv, check for reply */

	/* the "forever" loop */
	while((!isdying)||cmdlist) {

		/* check on a pending command recv */
		if (cmd_status == 1) {
			MPI_Status status;
			MPI_Test(&cmd_req,&done,&status);
			if (done) {
				if (cmd_buffer.command == DFC_CMD_KILL) {
					isdying = 1;
					cmd_status = 3;
				} else if (cmd_buffer.command==DFC_CMD_READ) {
					int size=get_local_req_size(&cmd_buffer,
						rank);
					ensure_length(size,&lcl_buffer,
						&lcl_buffer_len);
					/* make the reply */
					handle_command(&cmd_buffer,lcl_buffer,
						rank);
					/* send the reply */
					MPI_Isend(lcl_buffer,size,MPI_BYTE,
					    cmd_buffer.srcrank,
					    cmd_buffer.sock_tag,
					    MPI_COMM_WORLD,
					    &(data_req));
					cmd_status = 2;
				} else if (cmd_buffer.command==DFC_CMD_WRITE) {
					int size=get_local_req_size(&cmd_buffer,
						rank);
					ensure_length(size,&lcl_buffer,
						&lcl_buffer_len);
					/* post a wait for a data reply */
					MPI_Irecv(lcl_buffer,size,MPI_BYTE,
					    cmd_buffer.srcrank,
					    cmd_buffer.sock_tag,
					    MPI_COMM_WORLD,
					    &(data_req));
					cmd_status = 2;
				} else {
					/* other commands? OPEN/CLOSE/STAT? */
					cmd_status = 3;
				}
			}
		}

		/* reply sent, waiting for ack */
		if (cmd_status == 2) {
			MPI_Status status;
			MPI_Test(&data_req,&done,&status);
			if (done) {
				if (cmd_buffer.command == DFC_CMD_WRITE) {
					/* handle the reply */
					handle_command(&(cmd_buffer),
						lcl_buffer,rank);
				}
				cmd_status = 3;
			}
		}

		/* command done, post recv for next one */
		if (cmd_status == 3) {
			/* post a new receive */
			MPI_Irecv(&cmd_buffer,sizeof(cmd_buffer),
				MPI_BYTE,MPI_ANY_SOURCE,CMD_TAG,
				MPI_COMM_WORLD,&cmd_req);
			cmd_status = 1;
		}

		/* handle any new commands or push others forward */
		pthread_mutex_lock(&list_mutex);
		if (cmdlist) {
			dfccmd *l = cmdlist;
			while(l) {
				handle_cmdlist(l);
				l = l->next;
			}
		}
		pthread_mutex_unlock(&list_mutex);

		/* let someone else play */
		sched_yield();
	}

	/* wait for all threads to die (must be volatile) */
	while(nthreads);

	/* cleanup */
	pthread_mutex_destroy(&list_mutex);
	free(buffer);

	/* byebye and buy bonds... */
	MPI_Finalize();

	exit(0);
}

void handle_cmdlist(dfccmd *l) 
{
	int i,done,size;

	/* bootstrap the I/O */
	if (l->status == DFC_STATUS_PENDING) {
		l->status = DFC_STATUS_WORKING;
		for(i=0;i<max_rank;i++) {
			l->work[i].status = 0;
	    		if (i != rank) {
				if (l->cmd.command == DFC_CMD_KILL) {
					l->work[i].status = 1;
					MPI_Isend(&(l->cmd),sizeof(basecmd),
						MPI_BYTE,i,CMD_TAG,
						MPI_COMM_WORLD,
						&(l->work[i].cmd_req));
				} else {
					size = get_local_req_size(&(l->cmd),i);
					if (size < 0) continue;
					l->work[i].status = 1;
					MPI_Isend(&(l->cmd), sizeof(basecmd),
						MPI_BYTE,i,CMD_TAG,
						MPI_COMM_WORLD,
						&(l->work[i].cmd_req));
					if (l->cmd.command == DFC_CMD_READ) {
						l->work[i].status |= 2;
						MPI_Irecv(l->buffer+
					    	    get_local_req_offset(
							&(l->cmd),i),
						    size,MPI_BYTE,i,
						    l->cmd.sock_tag,
					    	    MPI_COMM_WORLD,
					    	    &(l->work[i].data_req));
					} else if (l->cmd.command == 
						DFC_CMD_WRITE) {
						l->work[i].status |= 2;
						MPI_Isend(l->buffer+
						    get_local_req_offset(
							&(l->cmd),i),
						    size,MPI_BYTE,i,
						    l->cmd.sock_tag,
					    	    MPI_COMM_WORLD,
					    	    &(l->work[i].data_req));
					}
				}
	    		}
		}
		l->work[rank].status = 0;
		handle_command(&(l->cmd),l->buffer,rank);

	/* progress the I/O */
	} else if (l->status == DFC_STATUS_WORKING) {
		int count = 0;
		for(i=0;i<max_rank;i++) {
	    		if ((i != rank) && (l->work[i].status & 1)) {
				/* check for command reply */
				MPI_Status status;
				MPI_Test(&(l->work[i].cmd_req),&done,&status);
				if (done) l->work[i].status &= (~1);
			}
	    		if ((i != rank) && (l->work[i].status & 2)) {
				/* check for data reply */
				MPI_Status status;
				MPI_Test(&(l->work[i].data_req),&done,&status);
				if (done) l->work[i].status &= (~2);
			}
			if (l->work[i].status) count++;
		}
		if (!count) {
			/* when done, flag and signal */
			l->status = DFC_STATUS_COMPLETE;
			pthread_cond_signal(&l->wait_cond);
		}
	}
	return;
}

/* how much local memory do we need? (rounded to blocks) */
off64_t get_local_size(void)
{
	off64_t	size = max_size/(blocksize*max_rank);
	size += 1;
	size *= blocksize;
	return(size);
}

off64_t get_local_req_size(basecmd *cmd, int r)
{
	off64_t	size = 0;
	int 	block0,block1;
	int	off0,off1;
	int	i;

	block0 = cmd->location / blocksize;
	off0 = cmd->location % blocksize;
	block1 = (cmd->location+cmd->length) / blocksize;
	off1 = (cmd->location+cmd->length) % blocksize;

	/* special case, all on one node (one block) */
	if (block0 == block1) {
		if ((block0 % max_rank) == r) size = cmd->length;
		off0 = -1;
		off1 = -1;
	}
	/* handle first block */
	if (off0 > 0) {
		if ((block0 % max_rank) == r) size += (blocksize-off0);
	}
	/* handle last block */
	if (off1 > 0) {
		if ((block1 % max_rank) == r) size += off1;
	}

	/* count the blocks */
	for(i=block0+1;i<block1;i++) {
		if ((i % max_rank) == r) size += blocksize;
	}
	return(size);
}
off64_t get_local_req_offset(basecmd *cmd, int r)
{
	off64_t	pos = 0;
	int 	i;

	/* crappy implementation for now */
	for(i=0;i<r;i++) pos += get_local_req_size(cmd,i);

	return(pos);
}

/* where in the local node buffer can I find the block (if at all),
	given the provided rank */
int get_node_buffer_offset(int block,int r)
{
	int off;

	int rnk = block % max_rank;
	if (rnk != r) return(-1);

	off = (block - rnk)/max_rank;
	off *= blocksize;

	return(off);
}

/* handle a single command */
void  handle_command(basecmd *cmd,unsigned char *buff,int r)
{
	if (cmd->command == DFC_CMD_READ) {
		unsigned char 	*p = buff;
		unsigned int	len = cmd->length;
		unsigned int	off = cmd->location % blocksize;
		unsigned int	block = cmd->location / blocksize;

		while(len) {
			int place = get_node_buffer_offset(block,r);
			int size = blocksize-off;
			if (size > len) size = len;
			if (place >= 0) {
				memcpy(p,buffer+place+off,size);
				p += size;
			} 
			len -= size;
			off = 0;
			block += 1;
		}
	} else if (cmd->command == DFC_CMD_WRITE) {
		unsigned char 	*p = buff;
		unsigned int	len = cmd->length;
		unsigned int	off = cmd->location % blocksize;
		unsigned int	block = cmd->location / blocksize;

		while(len) {
			int place = get_node_buffer_offset(block,r);
			int size = blocksize-off;
			if (size > len) size = len;
			if (place >= 0) {
				memcpy(buffer+place+off,p,size);
				p += size;
			}
			len -= size;
			off = 0;
			block += 1;
		}
	} else if (cmd->command == DFC_CMD_KILL) {
		isdying = 1;
	}
}

/* thread to watch the listen port and accept new connections (Open()) */
void *accept_proc(void *dummy)
{
	struct timeval time = {0};
	fd_set	set;
	int	err;

	nthreads++;
	while(!isdying) {
		time.tv_sec = 1;
		FD_ZERO(&set);
		FD_SET(listen_socket,&set);
		err = select(listen_socket+1,&set,NULL,NULL,&time);
		if (err == 0) {
			/* timeout */
		} else if (err < 0) {
			/* real error */
		} else if (FD_ISSET(listen_socket,&set)) {
			struct sockaddr_in	peer_addr;
			int 		s;
			int 		addr_len;
			pthread_t	tmp;

			addr_len = sizeof(struct sockaddr_in);
			s = accept(listen_socket,
				(struct sockaddr *)&peer_addr,&addr_len);

			/* fork off a handler thread */
			pthread_create(&tmp,NULL,handle_proc,(void *)s);
			pthread_detach(tmp);
		}
	}
	close(listen_socket);
	nthreads--;

	return(NULL);
}

void *handle_proc(void *s_i)
{
	int	localclose = 0;
	struct timeval time = {0};
	fd_set	set;
	int	err,status;
	int	first_cmd = 1;
	int	sock = (int)s_i;
	dfccmd	thecmd;
	int	lcl_buffer_len = 0;
	unsigned char	*lcl_buffer = NULL;
	short	endian = 1;

	/* ignore write to socket w/o readers signal */
	/* the error will be caught via normal means */
	signal(SIGPIPE, SIG_IGN);

	thecmd.work = (cmd_rply *)malloc(sizeof(cmd_rply)*max_rank);
	if (!thecmd.work) {
		fprintf(stderr,"Unable to allocate work buffer queue\n");
		return(NULL);
	}
	thecmd.cmd.sock_tag = sock;

	err = sock_write(sock,sizeof(short),0,(char *)&endian);
	if (err != sizeof(short)) localclose = 1;
	err = sock_write(sock,sizeof(max_size),0,(char *)&max_size);
	if (err != sizeof(max_size)) localclose = 1;

	nthreads++;
	pthread_cond_init(&thecmd.wait_cond,NULL);

	while(!isdying && !localclose) {
		unsigned int cmdhdr[3];

		time.tv_sec = 1;
		FD_ZERO(&set);
		FD_SET(sock,&set);
		err = select(sock+1,&set,NULL,NULL,&time);
		if (err == 0) {
			/* timeout */
		} else if (err < 0) {
			/* real error */
			localclose = 1;
		} else if (FD_ISSET(sock,&set)) {
			err = sock_read(sock,3*sizeof(int),0,(char *)cmdhdr);
			if (err != 3*sizeof(int)) {
				localclose = 1;
				continue;
			}
			thecmd.cmd.command = cmdhdr[0];
			thecmd.cmd.length = cmdhdr[1];
			thecmd.cmd.srcrank = rank;
			thecmd.buffer = NULL;
			thecmd.status = DFC_STATUS_PENDING;
			thecmd.next = NULL;
			switch(thecmd.cmd.command) {
				case DFC_CMD_WRITE:
					ensure_length(thecmd.cmd.length*2,
						&lcl_buffer,&lcl_buffer_len);
					thecmd.buffer = lcl_buffer;
					status=sock_read(sock,
						sizeof(thecmd.cmd.location),0,
						(char *)&(thecmd.cmd.location));
					status=sock_read(sock,
						thecmd.cmd.length,0,
						(char *)thecmd.buffer+
						thecmd.cmd.length);
					pack_buffer(&(thecmd.cmd),
					    thecmd.buffer+thecmd.cmd.length,
					    thecmd.buffer);
					break;
				case DFC_CMD_OPEN:
					ensure_length(thecmd.cmd.length,							&lcl_buffer,&lcl_buffer_len);
					thecmd.buffer = lcl_buffer;
					status=sock_read(sock,thecmd.cmd.length,
						0,(char *)thecmd.buffer);
					break;
				case DFC_CMD_READ:
					ensure_length(thecmd.cmd.length*2,
						&lcl_buffer,&lcl_buffer_len);
					thecmd.buffer = lcl_buffer;
					status=sock_read(sock,
						sizeof(thecmd.cmd.location),0,
						(char *)&(thecmd.cmd.location));
					break;
				case DFC_CMD_CLOSE:
					localclose = 1;
					break;
				case DFC_CMD_STAT:
				case DFC_CMD_KILL:
					break;
			}

			/* make security check (vs key) */
			if (first_cmd) {
				if (thecmd.cmd.command != DFC_CMD_OPEN) {
					localclose = 1;
					continue;
				} else {
					if (strcmp(key,thecmd.buffer) != 0) {
						localclose = 1;
					}
					first_cmd = 0;
				}
			}

			/* post the command and wait for replies... */
			if ((thecmd.cmd.command != DFC_CMD_OPEN) &&
			    (thecmd.cmd.command != DFC_CMD_CLOSE) &&
			    (thecmd.cmd.command != DFC_CMD_STAT)) {

				/* Only send READ,WRITE,KILL */
				/* place command into list */
				pthread_mutex_lock(&list_mutex);
				if (!cmdlist) {
					cmdlist = &thecmd;
				} else {
					dfccmd *l = cmdlist;
					while(l->next) l = l->next;
					l->next = &thecmd;
				}

				/* wait for it to complete */
				while(thecmd.status != DFC_STATUS_COMPLETE) {
					pthread_cond_wait(&thecmd.wait_cond,
						&list_mutex);
				}

				/* remove from list */
				if (cmdlist == (&thecmd)) {
					cmdlist = thecmd.next;
				} else {
					dfccmd *l = cmdlist;
					while(l->next != (&thecmd)) l = l->next;
					l->next = thecmd.next;
				}
				pthread_mutex_unlock(&list_mutex);

			}

			/* send the reply */
			switch(thecmd.cmd.command) {
				case DFC_CMD_WRITE:
					if (thecmd.cmd.length+
					    thecmd.cmd.location > file_size) {
						file_size = thecmd.cmd.length+
							thecmd.cmd.location;
					}
				case DFC_CMD_OPEN:
					break;
				case DFC_CMD_READ:
					unpack_buffer(&(thecmd.cmd),
					    thecmd.buffer,
					    thecmd.buffer+thecmd.cmd.length);
					/* send them */
					status=sock_write(sock,
						thecmd.cmd.length,0,
						(char *)thecmd.buffer+
						thecmd.cmd.length);
					break;
				case DFC_CMD_CLOSE:
				case DFC_CMD_KILL:
					localclose = 1;
					break;
				case DFC_CMD_STAT: {
					status=sock_write(sock,
						sizeof(file_size),0,
						(char *)&file_size);
					}
					break;
			}
		}
	}

	/* cleanup */
	close(sock);
	pthread_cond_destroy(&thecmd.wait_cond);
	nthreads--;
	free(thecmd.work);
	if(lcl_buffer_len) free(lcl_buffer);

	return(NULL);
}

/* pack a linear command array into a rank ordered buffer */
void pack_buffer(basecmd *cmd,unsigned char *src,unsigned char *dst)
{
	unsigned char **ptrs,*p;
	int	i;
	off64_t	loc = 0;
	unsigned int	len = cmd->length;
	unsigned int	block = cmd->location / blocksize;
	unsigned int	off = cmd->location % blocksize;
	int 	rnk = block % max_rank;

	ptrs = (unsigned char **)malloc(max_rank*sizeof(unsigned char *));
	for(i=0;i<max_rank;i++) {
		ptrs[i] = dst+loc;
		loc += get_local_req_size(cmd,i);
	}
	p = src;
	while(len) {
		int size = blocksize - off;
		if (size > len) size = len;

		memcpy(ptrs[rnk],p,size);
		ptrs[rnk] += size;
		p += size;
		len -= size;

		rnk = (rnk + 1) % max_rank;
		off = 0;
	}
	free(ptrs);

	return;
}

/* unpack a rank ordered buffer to a linear command array */
void unpack_buffer(basecmd *cmd,unsigned char *src,unsigned char *dst)
{
	unsigned char **ptrs,*p;
	int	i;
	off64_t	loc = 0;
	unsigned int	len = cmd->length;
	unsigned int	block = cmd->location / blocksize;
	unsigned int	off = cmd->location % blocksize;
	int 	rnk = block % max_rank;

	ptrs = (unsigned char **)malloc(max_rank*sizeof(unsigned char *));
	for(i=0;i<max_rank;i++) {
		ptrs[i] = src+loc;
		loc += get_local_req_size(cmd,i);
	}
	p = dst;
	while(len) {
		int size = blocksize - off;
		if (size > len) size = len;

		memcpy(p,ptrs[rnk],size);
		ptrs[rnk] += size;
		p += size;
		len -= size;

		rnk = (rnk + 1) % max_rank;
		off = 0;
	}
	free(ptrs);

	return;
}
