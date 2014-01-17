/*
** $RCSfile: rename.C,v $
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
** 	http://www.llnl.gov/sccd/lc/img/
**
**      or contact: asciviz@llnl.gov
**
** For copyright and disclaimer information see:
**      $(ASCIVIS_ROOT)/copyright_notice_1.txt
**
** 	or man llnl_copyright
**
** $Id: rename.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:  Program for walking a numbered list of files sequentially
**	       and performing some operation, including renumbering.
**
**  Author:
**
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sm/sm.h>

int	verbose = 0;

void ex_err(char *s)
{
	smdbprintf(0,"Fatal error:%s\n",s);
	exit(1);
}

void cmd_err(char *s)
{
	smdbprintf(0,"(%s) Usage: %s [options] intemp outtemp\n",__DATE__,s);
	smdbprintf(0,"Options: -f(istart) first input file number default:1\n");
	smdbprintf(0,"         -l(iend) last input file number default:124\n");
	smdbprintf(0,"         -i(istep) input file number increment default:1\n");
	smdbprintf(0,"         -F(ostart) first output file number default:(istart)\n");
	smdbprintf(0,"         -I(ostep) output file number increment default:(istep)\n");
	smdbprintf(0,"         -v verbose mode\n");
	smdbprintf(0,"         -t test mode (print the commands only)\n");
	smdbprintf(0,"         -c(cmd) unix command to issue default:mv\n");
	exit(1);
}

#define UNKNOWN	-987654

int main(int argc,char **argv)
{
	long int	istart = 1;
	long int	iend = 124;
	long int	istep = 1;
	long int	testmode = 0;
	long int	ostart = UNKNOWN;
	long int	ostep = UNKNOWN;
	char		cmd[1024];

	char		intemp[1024],outtemp[1024];
	char		tstr[1024],syscmd[2024];
	long int	i;

/* default command */
	strcpy(cmd,"mv");

	i = 1;
	while ((i < argc) && (argv[i][0] == '-')) {
		if (argv[i][1] == '\0') break;
		switch (argv[i][1]) {
			case 'f':
				istart = atoi(&(argv[i][2])); 
				break;
			case 'l':
				iend = atoi(&(argv[i][2]));
				break;
			case 'i':
				istep = atoi(&(argv[i][2]));
				break;
			case 'F':
				ostart = atoi(&(argv[i][2]));
				break;
			case 'I':
				ostep = atoi(&(argv[i][2]));
				break;
			case 'c':
				strcpy(cmd,&(argv[i][2]));
				break;
			case 'v':
				verbose = 1;
				break;
			case 't':
				testmode = 1;
				break;
			default:	
				cmd_err(argv[0]);
				break;
		}
		i++;
	}
	if ((argc-i) != 2) cmd_err(argv[0]);
	strcpy(intemp,argv[i]);
	strcpy(outtemp,argv[i+1]);

/* if unspecified, set it */
	if (ostart == UNKNOWN) ostart = istart;
	if (ostep == UNKNOWN) ostep = istep;

/* for each file number */
	for(i=istart;i<=iend;i=i+istep) {
/* build command "{cmd} file1 file2" */
		strcpy(syscmd,cmd);
		strcat(syscmd," ");

		sprintf(tstr,intemp,i);
		strcat(syscmd,tstr);
		strcat(syscmd," ");

		sprintf(tstr,outtemp,ostart);
		strcat(syscmd,tstr);

		smdbprintf(verbose?1:0,"%s\n",syscmd);
		if (testmode) {
			fprintf(stdout,"%s\n",syscmd);
		} else {
			system(syscmd);
		}

		ostart += ostep;
	}

/* done */
	exit(0);
}
