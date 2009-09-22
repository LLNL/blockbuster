/*
** $RCSfile: sm2img.C,v $
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

** $Id: sm2img.C,v 1.1 2007/06/13 18:59:34 wealthychef Exp $
**
*/
/*
**
**  Abstract:
**
**  Author:
**
*/


// Utility to combine image files into movie
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "sm/smRLE.h"
#include "sm/smGZ.h"
#include "sm/smLZO.h"
#include "sm/smRaw.h"

#include "zlib.h"
#include "pngsimple.h"

//define int32 int32hack
extern "C" {
#include <tiff.h>
#include <tiffio.h>
}
//undef int32

#include "libimage/sgilib.h"
#include "libpnmrw/libpnmrw.h"
#include "simple_jpeg.h"

// Prototypes 
void cmdline(char *app,int binfo);

void cmdline(char *app,int binfo)
{
    if (binfo) {
	fprintf(stderr,"(%s) usage: %s smfile\n",
		__DATE__,app);
    } else {

	fprintf(stderr,"(%s) usage: %s [options] smfile [outputtemplate]\n",
		__DATE__,app);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"\t-v Verbose mode.\n");
	fprintf(stderr,"\t-ignore Ignore invalid output templates. Default:check.\n");
	fprintf(stderr,"\t-first num Select first frame number to extract. Default:0.\n");
	fprintf(stderr,"\t-last num Select last frame number to extract. Default:last frame.\n");
	fprintf(stderr,"\t-step num Select frame number step factor. Default:1.\n");
	fprintf(stderr,"\t-quality num Select JPEG output quality (0-100). Default: 75\n");
	fprintf(stderr,"\t-form [\"tiff\"|\"sgi\"|\"pnm\"|\"png\"|\"jpg\"|\"YUV\"] Output file format (default:sgi)\n");
	fprintf(stderr,"\t-region offsetX offsetY sizeX sizeY -- Output will be the given subregion of the input.  Default: offsets 0 0, original size .  \n");
	fprintf(stderr,"\t-mipmap Extract frame from mipmap level. Default: 0\n");
	fprintf(stderr,"\tNote: without an output template, movie stats will be displayed.\n");
    }
    exit(1);
}

int main(int argc,char **argv)
{
	char		*sTemplate = NULL;
	char		*sInput = NULL;
	int	iVerb = 0;
	int	iType = 1;
	int	iFirst = 0;
	int	iLast = -1;
	int	iStep = 1;
	int	iIgnore = 0;
	int	originalImageSize[2];
	int		bIsInfo = 0;
	int		iQuality = 75;
	int             blocksize[3] = {0,0,3};
	int             blockoffset[2] = {0,0};
	int             mipmap=0;
	int	i,j,x,y;
	char		tstr[1024],tstr2[1024];
	smBase 		*sm = NULL;
	unsigned char	*img;

	TIFF		*tif = NULL;
	sgi_t		*libi = NULL;
	FILE		*fp = NULL;

        if (strstr(argv[0],"sminfo")) {

		bIsInfo = 1;
		i = 1;
	   	while ((i<argc) && (argv[i][0] == '-')) {
			if (strcmp(argv[i],"-v")==0) {
				iVerb = 1;
			} else {
				cmdline(argv[0],bIsInfo);
			}
			i++;
		}
		if ((argc - i) != 1) cmdline(argv[0],bIsInfo);

	} else {

	/* parse the command line ... */
	i = 1;
	while ((i<argc) && (argv[i][0] == '-')) {
		if (strcmp(argv[i],"-v")==0) {
			iVerb = 1;
		} else if (strcmp(argv[i],"-ignore")==0) {
			iIgnore = 1;
		} else if (strcmp(argv[i],"-quality")==0) {
			if ((argc-i) > 1) {
				iQuality = atoi(argv[i+1]);
			} else {
				cmdline(argv[0],bIsInfo);
			}
			i++;
		} else if (strcmp(argv[i],"-first")==0) {
			if ((argc-i) > 1) {
				iFirst = atoi(argv[i+1]);
			} else {
				cmdline(argv[0],bIsInfo);
			}
			i++;
		} else if (strcmp(argv[i],"-last")==0) {
			if ((argc-i) > 1) {
				iLast = atoi(argv[i+1]);
			} else {
				cmdline(argv[0],bIsInfo);
			}
			i++;
		} else if (strcmp(argv[i],"-step")==0) {
			if ((argc-i) > 1) {
				iStep = atoi(argv[i+1]);
			} else {
				cmdline(argv[0],bIsInfo);
			}
			i++;
		} else if (strcmp(argv[i],"-region")==0) {
		  if ((argc-i) > 4) {
		    blockoffset[0] = atoi(argv[++i]);
		    blockoffset[1] = atoi(argv[++i]);
		    blocksize[0] = atoi(argv[++i]);
		    blocksize[1] = atoi(argv[++i]);
		    if (blocksize[0] <= 0 || blocksize[1] <= 0){
		      fprintf(stderr, "Error: image size must be greater than zero.\n");
		      exit(1);
		    }
		    if (blockoffset[0] < 0 || blockoffset[1] < 0){
		      fprintf(stderr, "Error: image offsets must be nonnegative.\n");
		      exit(1);
		    }
		  } else {
		    cmdline(argv[0],bIsInfo);
		  }
		} else if (strcmp(argv[i],"-mipmap")==0) {
		  if ((argc-i) > 1) {
		    mipmap = atoi(argv[++i]);
		   
		  } else {
		    cmdline(argv[0],bIsInfo);
		  }
		} else if (strcmp(argv[i],"-form")==0) {
			if ((argc-i) > 1) {
				if (strcmp(argv[i+1],"tiff") == 0) {
					iType = 0;
				} else if (strcmp(argv[i+1],"sgi") == 0) {
					iType = 1;
				} else if (strcmp(argv[i+1],"pnm") == 0) {
					iType = 2;
				} else if (strcmp(argv[i+1],"YUV") == 0) {
					iType = 3;
				} else if (strcmp(argv[i+1],"png") == 0) {
					iType = 4;
				} else if (strcmp(argv[i+1],"jpg") == 0) {
					iType = 5;
				} else {
					fprintf(stderr,"Invalid format: %s\n",
						argv[i+1]);
					exit(1);
				}
				i++;
			} else {
				cmdline(argv[0],bIsInfo);
			}
		} else {
			fprintf(stderr,"Unknown option: %s\n\n",argv[i]);
			cmdline(argv[0],bIsInfo);
		}
		i++;
	}

        }

	smBase::init();

	// Movie info case... (both sminfo and sm2img file)
	if ((argc - i) == 1) {
		sInput = argv[i];

		sm = smBase::openFile(sInput);
		if (!sm) {
			fprintf(stderr,"Unable to open the file: %s\n",sInput);
			exit(1);
		}

		printf("File: %s\n",sInput);
		printf("Streaming movie version: %d\n",sm->getVersion());
		if (sm->getType() == 1) {   // smRLE::typeID
			printf("Format: RLE compressed\n");
		} else if (sm->getType() == 2) {   // smGZ::typeID
			printf("Format: gzip compressed\n");
		} else if (sm->getType() == 4) {   // smJPG::typeID
			printf("Format: JPG compressed\n");
		} else if (sm->getType() == 3) {   // smLZO::typeID
			printf("Format: LZO compressed\n");
		} else if (sm->getType() == 0) {   // smRaw::typeID
			printf("Format: RAW uncompressed\n");
		} else {
			printf("Format: unknown\n");
		}
		printf("Size: %d %d\n",sm->getWidth(),sm->getHeight());
		printf("Frames: %d\n",sm->getNumFrames());
		printf("FPS: %0.2f\n",sm->getFPS());
		double len = 0;
		double len_u = 0;
		int j;
		for(j=0;j<sm->getNumResolutions();j++) {
		    for(i=0;i<sm->getNumFrames();i++) {
			len += (double)(sm->getCompFrameSize(i,j));
			len_u += (double)(sm->getWidth(j)*sm->getHeight(j)*3);
		    }
		}
		printf("Compression ratio: %0.4f%%\n",(len/len_u)*100.0);
		printf("Number of resolutions: %d\n",sm->getNumResolutions());
		for(i=0;i<sm->getNumResolutions();i++) {
			printf("    Level: %d : size %d x %d : tile %d x %d\n",
				i,sm->getWidth(i),sm->getHeight(i),
				sm->getTileWidth(i),sm->getTileHeight(i));

		}
		printf("Flags: ");
		if (sm->getFlags() & SM_FLAGS_STEREO) printf("Stereo ");
		printf("\n");

		if (iVerb) {
			printf("Frame\tOffset\tLength\n");
			for(i=0;i<sm->getNumFrames()*sm->getNumResolutions();i++) {
				sm->printFrameDetails(stdout,i);
			}
		}

		delete sm;

		exit(0);
	}

	if ((argc - i) != 2) cmdline(argv[0],bIsInfo);

	// get the arguments
	sInput = argv[i];
	sTemplate = argv[i+1];

	if (!iIgnore) {
		// Bad template name?
		sprintf(tstr,sTemplate,0);
		sprintf(tstr2,sTemplate,1);
		if (strcmp(tstr,tstr2) == 0) {
			fprintf(stderr,"Output specification is not a sprintf template (see -ignore)\n");
			exit(1);
		}
	}

	sm = smBase::openFile(sInput);
	if (!sm) {
		fprintf(stderr,"Unable to open the file: %s\n",sInput);
		exit(1);
	}

	if(sm->getVersion() == 1) {
	  mipmap = 0;
	}
	if(mipmap >= sm->getNumResolutions()) {
	  fprintf(stderr,"Error: Mipmap Level %d Not Available : Choose Levels 0->%d\n",mipmap,sm->getNumResolutions()-1);
	  exit(1);
	}
	/* originalImageSize[2] = 3; unused */
	originalImageSize[0] = sm->getWidth(mipmap);
	originalImageSize[1] = sm->getHeight(mipmap);

	/* check user inputs for consistency with image size*/
	if (blocksize[0]+blockoffset[0] > originalImageSize[0]) {
	  fprintf(stderr, "Error: X size (%d) + X offset (%d) cannot be greater than image width (%d)\n", 
		  blocksize[0], blockoffset[0], originalImageSize[0]);
	  exit(1);
	}
	if (blocksize[1]+blockoffset[1] > originalImageSize[1]) {
	  fprintf(stderr, "Error: Y size (%d) + Y offset (%d) cannot be greater than image height (%d)\n", 
		  blocksize[1], blockoffset[1], originalImageSize[1]);
	  exit(1);
	}

	if (!blocksize[0] && !blocksize[1]){ /* no size given -- use image size minus offsets*/
	  blocksize[0] = originalImageSize[0] - blockoffset[0];
	  blocksize[1] = originalImageSize[1] - blockoffset[1];
	}


	if (iLast < 0) iLast = sm->getNumFrames() - 1;
	if (iLast < iFirst) cmdline(argv[0],bIsInfo);
	if (iFirst < 0) cmdline(argv[0],bIsInfo);
	if (iLast >= sm->getNumFrames())  cmdline(argv[0],bIsInfo);

	if (iStep < 1) {
		fprintf(stderr,"Error, frame stepping must be >= 1\n");
		exit(1);
        }

	img = (unsigned char *)malloc(3*blocksize[0]*blocksize[1]);
	if (!img) {
		fprintf(stderr,"Unable to allocate buffer memory\n");
		exit(1);
	}

	// here we go...
	for(i=iFirst;i<=iLast;i+=iStep) {	
		if (iVerb) fprintf(stderr,"Working on %d of %d\n",i-iFirst+1,
			iLast - iFirst+1);
		if(sm->getVersion() > 1) {
		  int destRowStride = 0;
		  sm->getFrameBlock(i, img, 0, destRowStride,&blocksize[0],&blockoffset[0],NULL,mipmap);
		}
		else {
		  sm->getFrame(i, img, 0);
		}

		sprintf(tstr,sTemplate,i);
		switch(iType) {
			case 0: {  // TIFF
				  unsigned char *p;
				  tif = TIFFOpen(tstr,"w");
				  if (tif) {
// Header stuff 
#ifdef Tru64
				    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (uint32)blocksize[0]);
				    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (uint32)blocksize[1]);
#else
				    TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, (unsigned int)blocksize[0]);
				    TIFFSetField(tif, TIFFTAG_IMAGELENGTH, (unsigned int)blocksize[1]);
#endif
TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, 8);
TIFFSetField(tif, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
TIFFSetField(tif, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
TIFFSetField(tif, TIFFTAG_DOCUMENTNAME, tstr);
TIFFSetField(tif, TIFFTAG_IMAGEDESCRIPTION, "sm2img TIFF image");
TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, 3);
TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, 1);
TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);

				    for(y=0;y<blocksize[1];y++) {
					p = img + (blocksize[0]*3)*(blocksize[1]-y-1);
					TIFFWriteScanline(tif,p,y,0);
				    }
				    TIFFFlushData(tif);
				    TIFFClose(tif);
				  }
				}
				break;
			case 1: {  // SGI
				  unsigned short   buf[8192];
				  libi = sgiOpen(tstr,SGI_WRITE,SGI_COMP_RLE,1,
					blocksize[0],blocksize[1],3);
				  if (libi) {
				    for(y=0;y<blocksize[1];y++) {
				      for(j=0;j<3;j++) {
				        for(x=0;x<blocksize[0];x++) {
				  	  buf[x]=img[(x+(y*blocksize[0]))*3+j];
				        }
				        sgiPutRow(libi,buf,y,j);
 				      }
				    }
				    sgiClose(libi);
				  }
				}
				break;
			case 2: {  // PNM
				  fp = pm_openw(tstr);
				  if (fp) {
                                     xel* xrow;
                                     xel* xp;
				     xrow = pnm_allocrow( blocksize[0] );
				     pnm_writepnminit( fp, blocksize[0], blocksize[1], 
				       255, PPM_FORMAT, 0 );
                                     for(y=blocksize[1]-1;y>=0;y--) {
                                        xp = xrow;
                                        for(x=0;x<blocksize[0];x++) {
                                          int r1,g1,b1;
                                          r1 = img[(x+(y*blocksize[0]))*3+0];
                                          g1 = img[(x+(y*blocksize[0]))*3+1];
                                          b1 = img[(x+(y*blocksize[0]))*3+2];
				          PPM_ASSIGN( *xp, r1, g1, b1 );
                                          xp++;
                                        }
                                        pnm_writepnmrow( fp, xrow, blocksize[0], 
                                          255, PPM_FORMAT, 0 );
                                     }
                                     pnm_freerow(xrow);
				     pm_closew(fp);
				  }
				}
				break;
			case 3: {  // YUV
				  int dx = blocksize[0] & 0xfffffe;
				  int dy = blocksize[1] & 0xfffffe;
				  unsigned char *buf = (unsigned char *)malloc(
					(unsigned int)(1.6*dx*dy));
				  unsigned char *Ybuf = buf;
				  unsigned char *Ubuf = Ybuf + (dx*dy);
				  unsigned char *Vbuf = Ubuf + (dx*dy)/4;

				  /* convert RGB to YUV  */
				  unsigned char *p = img;
				  for(y=0;y<blocksize[1];y++) {
			             for(x=0;x<blocksize[0];x++) {
				        float rd=p[0];
				        float gd=p[1];
				        float bd=p[2];
					float yd= 0.2990*rd+0.5870*gd+0.1140*bd;
				        float ud=-0.1687*rd-0.3313*gd+0.5000*bd;
				        float vd= 0.5000*rd-0.4187*gd-0.0813*bd;
					int   Y=(int)floor(yd+0.5);
					int   U=(int)floor(ud+128.5);
					int   V=(int)floor(vd+128.5);
					if (Y<0)   Y=0;
					if (Y>255) Y=255;
					if (U<0)   U=0;
					if (U>255) U=255;
					if (V<0)   V=0;
					if (V>255) V=255;
					*p++ = Y;
					*p++ = U;
					*p++ = V;
				     }
				  }

				  /* pull apart into Y,U,V buffers */
				  /* down-sample U/V */
				  for(y=0;y<dy;y++) {
				     p = img + (blocksize[1]-y-1)*3*blocksize[0];
				     for(x=0;x<dx;x++) {
				        *Ybuf++ = *p++;
					if ((x&1) || (y&1)) {
					   p += 2;
					} else {
					   *Ubuf++ = *p++;
					   *Vbuf++ = *p++;
					}
			             }
				  }

				  /* write the 3 files */
				  char	ttstr[4096];
				  p = buf;
				  sprintf(ttstr,"%s.Y",tstr);
				  fp = fopen(ttstr,"wb");
				  if (fp) {
			             fwrite(p,dx*dy,1,fp);
				     fclose(fp);
			          }
				  p += dx*dy;
				  sprintf(ttstr,"%s.U",tstr);
				  fp = fopen(ttstr,"wb");
				  if (fp) {
			             fwrite(p,dx*dy/4,1,fp);
				     fclose(fp);
			          }
				  p += dx*dy/4;
				  sprintf(ttstr,"%s.V",tstr);
				  fp = fopen(ttstr,"wb");
				  if (fp) {
			             fwrite(p,dx*dy/4,1,fp);
				     fclose(fp);
			          }

			          free(buf);
				}
				break;
			case 4: {  // PNG
				  write_png_file(tstr,img,blocksize);
				}
				break;
			case 5: {  // JPEG
				  write_jpeg_image(tstr,img,blocksize,iQuality);
				}
				break;
		}
	}

	free(img);

	delete sm;

	exit(0);
}
