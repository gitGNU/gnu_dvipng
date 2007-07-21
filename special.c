/* special.c */

/************************************************************************

  Part of the dvipng distribution

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301 USA.

  Copyright (C) 2002-2006 Jan-Åke Larsson

************************************************************************/

#include "dvipng.h"
#include <fcntl.h>
#if HAVE_ALLOCA_H
# include <alloca.h>
#endif

#define SKIPSPACES(s) while(s && *s==' ' && *s!='\0') s++

/* PostScript can come as a string (headers and raw specials) or 
   a memory-mapped file (headers and included EPS figures). */

struct pscode {
  struct pscode*  next;
  char*           code;     /* PS string, null if a file */
  char*           header;   /* header filename, null if not a header */
  char*           filename; /* full path, null if a string */
  struct filemmap fmmap;    /* file mmap */
};

struct pscode* psheaderp=NULL; /* static, DVI-specific header list */

void PSCodeInit(struct pscode *entry, char *code, char* header)
{
  entry->next=NULL;
  entry->code=code;
  entry->header=header;
  entry->filename=NULL;
  entry->fmmap.mmap=NULL;
}

void ClearPSHeaders(void)
{
  struct pscode *temp=psheaderp;

  while(temp!=NULL) {
    psheaderp=psheaderp->next;
    if (temp->fmmap.mmap!=NULL)
      UnMmapFile(&(temp->fmmap));
    free(temp);
    temp=psheaderp;
  }
}

void writepscode(struct pscode* pscodep, FILE* psstream)
{
  while (pscodep!=NULL) {
    if (pscodep->code!=NULL)
      fputs(pscodep->code,psstream);
    if (pscodep->header!=NULL) {
      if (pscodep->filename==NULL) {
	pscodep->filename=
	  kpse_find_file(pscodep->header,kpse_tex_ps_header_format,false);
	if (pscodep->filename==NULL) {
	  Warning("Cannot find PostScript file %s, ignored", pscodep->filename);
	  flags |= PAGE_GAVE_WARN;
	}
      }	
      if (pscodep->filename!=NULL && pscodep->fmmap.mmap==NULL 
	  && MmapFile(pscodep->filename,&(pscodep->fmmap))) {
	Warning("PostScript file %s unusable, ignored", pscodep->filename);
	flags |= PAGE_GAVE_WARN;
      }
    }
    if (pscodep->fmmap.mmap!=NULL) {
      unsigned char* position;
      position=(unsigned char*)pscodep->fmmap.mmap;
      while(position 
	    < (unsigned char*)pscodep->fmmap.mmap + pscodep->fmmap.size) {
	putc(*position,psstream);
	position++;
      }
    }
    pscodep=pscodep->next;
  }
}


gdImagePtr
ps2png(struct pscode* pscodep, char *device, int hresolution, int vresolution, 
       int llx, int lly, int urx, int ury, int bgred, int bggreen, int bgblue)
{
#ifndef MIKTEX
  int downpipe[2], uppipe[2];
  pid_t pid;
#else
  HANDLE hPngStream;
  HANDLE hPsStream;
  HANDLE hStdErr;
  PROCESS_INFORMATION pi;
  _TCHAR szCommandLine[2048];
  _TCHAR szGsPath[_MAX_PATH];
#define GS_PATH szGsPath
  int fd;
#endif
  FILE *psstream=NULL, *pngstream=NULL;
  char resolution[STRSIZE]; 
  /*   char devicesize[STRSIZE];  */
  gdImagePtr psimage=NULL;
  static bool showpage=false;

  sprintf(resolution, "-r%dx%d",hresolution,vresolution);
  /* Future extension for \rotatebox
  status=sprintf(devicesize, "-g%dx%d",
		 //(int)((sin(atan(1.0))+1)*
		 (urx - llx)*hresolution/72,//), 
		 //(int)((sin(atan(1.0))+1)*
		 (ury - lly)*vresolution/72);//);
  */
  DEBUG_PRINT(DEBUG_GS,
	      ("\n  GS CALL:\t%s %s %s %s %s %s %s %s %s %s %s",/* %s", */
	       GS_PATH, device, resolution, /*devicesize,*/
	       "-dBATCH", "-dNOPAUSE", "-q", "-sOutputFile=-", 
	       "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
	       (flags & NO_GSSAFER) ? "-": "-dSAFER", 
	       (flags & NO_GSSAFER) ? "": "- "));
#ifndef MIKTEX
  if (pipe(downpipe) || pipe(uppipe)) return(NULL);
  /* Ready to fork */
  pid = fork ();
  if (pid == 0) { /* Child process.  Execute gs. */       
    close(downpipe[1]);
    dup2(downpipe[0], STDIN_FILENO);
    close(downpipe[0]);
    close(uppipe[0]);
    dup2(uppipe[1], STDOUT_FILENO);
    close(uppipe[1]);
    execlp(GS_PATH, GS_PATH, device, resolution, /*devicesize,*/
	   "-dBATCH", "-dNOPAUSE", "-q", "-sOutputFile=-", 
	   "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
	   (flags & NO_GSSAFER) ? "-": "-dSAFER", 
	   (flags & NO_GSSAFER) ? NULL: "-",
	   NULL);
    _exit (EXIT_FAILURE);
  }
  /* Parent process. */
  
  close(downpipe[0]);
  psstream=fdopen(downpipe[1],"wb");
  if (psstream == NULL) 
    close(downpipe[1]);
  close(uppipe[1]);
  pngstream=fdopen(uppipe[0],"rb");
  if (pngstream == NULL) 
    close(uppipe[0]);
#else /* MIKTEX */
  if (! miktex_find_miktex_executable("mgs.exe", szGsPath)) {
      Warning("Ghostscript could not be found");
      return(NULL);
  }
  sprintf(szCommandLine,"\"%s\" %s %s %s %s %s %s %s %s %s %s",/* %s",*/
	  szGsPath, device, resolution, /*devicesize,*/
	  "-dBATCH", "-dNOPAUSE", "-q", "-sOutputFile=-", 
	  "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
	  (flags & NO_GSSAFER) ? "-": "-dSAFER", 
	  (flags & NO_GSSAFER) ? "": "-");
  if (! miktex_start_process_3(szCommandLine, &pi, INVALID_HANDLE_VALUE,
			       &hPsStream, &hPngStream, &hStdErr, 0)) {
      Warning("Ghostscript could not be started");
      return(NULL);
  }
  CloseHandle (pi.hThread);
  fd = _open_osfhandle((intptr_t)hPsStream, _O_WRONLY);
  if (fd >= 0) { 
    psstream = _tfdopen(fd, "wb");
    if (psstream == NULL) 
      _close (fd);
  }
  fd = _open_osfhandle((intptr_t)hPngStream, _O_RDONLY);
  if (fd >= 0) {
    pngstream = _tfdopen(fd, "rb");
    if (pngstream == NULL) 
      _close (fd);
  }
#endif 
  if (psstream) {
    DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\t<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice",
			  urx - llx, ury - lly,llx,lly));
    fprintf(psstream, "<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice\n",
	    urx - llx, ury - lly,llx,lly);
    if ( bgred < 255 || bggreen < 255 || bgblue < 255 ) {
      DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\tgsave %f %f %f setrgbcolor clippath fill grestore",
			    bgred/255.0, bggreen/255.0, bgblue/255.0));
      fprintf(psstream, "gsave %f %f %f setrgbcolor clippath fill grestore",
	      bgred/255.0, bggreen/255.0, bgblue/255.0);
    }
    DEBUG_PRINT(DEBUG_GS,("\n  PS DATA:\t..."));
    writepscode(psheaderp,psstream);
    writepscode(pscodep,psstream);
    if (showpage) {
      DEBUG_PRINT(DEBUG_GS,("\n  PS CODE:\tshowpage"));
      fprintf(psstream, " showpage ");
    }
    fclose(psstream);
  }
  if (pngstream) {
    psimage = gdImageCreateFromPng(pngstream);
    fclose(pngstream);
  }
#ifdef MIKTEX
  CloseHandle(pi.hProcess);
#endif
  if (psimage == NULL) {
    DEBUG_PRINT(DEBUG_GS,("\n  GS OUTPUT:\tNO IMAGE "));
    if (!showpage) {
      showpage=true;
      DEBUG_PRINT(DEBUG_GS,("(will try adding \"showpage\") "));
      psimage=ps2png(pscodep,
		     device, hresolution, vresolution, llx, lly, urx, ury,
		     bgred,bggreen,bgblue);
      showpage=false;
    }
#ifdef DEBUG
  } else {
    DEBUG_PRINT(DEBUG_GS,("\n  GS OUTPUT:\t%dx%d image ",
			  gdImageSX(psimage),gdImageSY(psimage)));
#endif
  }
  return psimage;
}

gdImagePtr
rescale(gdImagePtr psimage, int pngwidth, int pngheight)
{
  gdImagePtr scaledimage=psimage;
  /* Rescale unless correct size */
  if (psimage!=NULL
      && gdImageSX(psimage)!=pngwidth 
      && gdImageSY(psimage)!=pngheight) {
    DEBUG_PRINT(DEBUG_DVI,
		("\n  RESCALE INCLUDED BITMAP \t(%d,%d) -> (%d,%d)",
		 gdImageSX(psimage),gdImageSY(psimage),
		 pngwidth,pngheight));
#ifdef HAVE_GDIMAGECREATETRUECOLOR
    scaledimage=gdImageCreateTrueColor(pngwidth,pngheight);
    gdImageCopyResampled(scaledimage,psimage,0,0,0,0,
			 pngwidth,pngheight,
			 gdImageSX(psimage),gdImageSY(psimage));
#else
    scaledimage=gdImageCreate(pngwidth,pngheight);
    gdImageCopyResized(scaledimage,psimage,0,0,0,0,
		       pngwidth,pngheight,
		       gdImageSX(psimage),gdImageSY(psimage));
#endif
    gdImageDestroy(psimage);
  }
  return(scaledimage);
}


/*-->SetSpecial*/
/*********************************************************************/
/****************************  SetSpecial  ***************************/
/*********************************************************************/

void SetSpecial(char * special, int32_t length, int32_t hh, int32_t vv)
/* interpret a \special command, made up of keyword=value pairs */
{
  char *buffer;

  DEBUG_PRINT(DEBUG_DVI,(" '%.*s'",length,special));

  buffer = alloca(sizeof(char)*(length+1));
  if (buffer==NULL) 
    Fatal("cannot allocate space for special string");

  strncpy(buffer,special,length);
  buffer[length]='\0';

  SKIPSPACES(buffer);
  /********************** Color specials ***********************/
  if (strncmp(buffer,"background ",11)==0) {
    background(buffer+11);
    return;
  }
  if (strncmp(buffer,"color ",6)==0) {
    buffer+=6;
    SKIPSPACES(buffer);
    if (strncmp(buffer,"push ",5)==0) {
      pushcolor(buffer+5);
    } else {
      if (strcmp(buffer,"pop")==0)
	popcolor();
      else 
	resetcolorstack(buffer);
    }
    return;
  }

  /******************* Image inclusion ********************/

  /* Needed tests for regression: PNG, GIF, JPEG and EPS inclusion,
   * for different gd versions */

  if (strncmp(buffer,"PSfile=",7)==0) { /* PSfile */
    char* psname = buffer+7;
    int llx=0,lly=0,urx=0,ury=0,rwi=0,rhi=0;
    bool clip=false;
    int hresolution,vresolution;
    int pngheight,pngwidth;

    /* Remove quotation marks around filename */
    if (*psname=='"') {
      char* tmp;
      psname++;
      tmp=strrchr(psname,'"');
      if (tmp!=NULL) {
	*tmp='\0';
	buffer=tmp+1;
      } else
	buffer=NULL;
    }
    
    /* Retrieve parameters */
    SKIPSPACES(buffer);
    while(buffer && *buffer) {
      if (strncmp(buffer,"llx=",4)==0) llx = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"lly=",4)==0) lly = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"urx=",4)==0) urx = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"ury=",4)==0) ury = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"rwi=",4)==0) rwi = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"rhi=",4)==0) rhi = strtol(buffer+4,&buffer,10);
      else if (strncmp(buffer,"clip",4)==0) {clip = true; buffer=buffer+4;}
      while (*buffer && *buffer!=' ') buffer++;
      SKIPSPACES(buffer);
    }
    
    /* Calculate resolution, and use our base resolution as a fallback. */
    /* The factor 10 is magic, the dvips graphicx driver needs this.    */
    hresolution = ((dpi*rwi+urx-llx-1)/(urx - llx)+9)/10;
    vresolution = ((dpi*rhi+ury-lly-1)/(ury - lly)+9)/10;
    if (vresolution==0) vresolution = hresolution;
    if (hresolution==0) hresolution = vresolution;
    if (hresolution==0) hresolution = vresolution = dpi;
      
    /* Convert from postscript 72 dpi resolution to our given resolution */
    pngwidth  = (dpi*rwi+719)/720; /* +719: round up */
    pngheight = (dpi*rhi+719)/720;
    if (pngwidth==0)  
      pngwidth  = ((dpi*rhi*(urx-llx)+ury-lly-1)/(ury-lly)+719)/720;
    if (pngheight==0) 
      pngheight = ((dpi*rwi*(ury-lly)+urx-llx-1)/(urx-llx)+719)/720;
    if (pngheight==0) {
      pngwidth  = (dpi*(urx-llx)+71)/72;
      pngheight = (dpi*(ury-lly)+71)/72;
    }    
    if (page_imagep != NULL) { /* Draw into image */
      struct pscode image;
      gdImagePtr psimage=NULL;
#ifndef HAVE_GDIMAGECREATEFROMPNGPTR
      FILE* psstream;
#endif

      PSCodeInit(&image,NULL,NULL);
      TEMPSTR(image.filename,kpse_find_file(psname,kpse_pict_format,0));
      if (MmapFile(image.filename,&(image.fmmap)) || image.fmmap.size==0) {
	Warning("Image file %s unusable, image will be left blank",
		image.filename);
	flags |= PAGE_GAVE_WARN;
	return;
      } 
      Message(BE_NONQUIET," <%s",psname);
      switch ((unsigned char)*image.fmmap.mmap) {
      case 0x89: /* PNG magic: "\211PNG\r\n\032\n" */
	DEBUG_PRINT(DEBUG_DVI,("\n  INCLUDE PNG \t%s",image.filename));
#ifdef HAVE_GDIMAGECREATEFROMPNGPTR
	psimage=gdImageCreateFromPngPtr(image.fmmap.size,image.fmmap.mmap);
#else
	psstream=fopen(image.filename,"rb");
	psimage=gdImageCreateFromPng(psstream);
	fclose(psstream);
#endif
	psimage=rescale(psimage,pngwidth,pngheight);
	break;
      case 'G': /* GIF magic: "GIF87" or "GIF89" */
	DEBUG_PRINT(DEBUG_DVI,("\n  INCLUDE GIF \t%s",image.filename));
#ifdef HAVE_GDIMAGEGIF
	psimage=rescale(gdImageCreateFromGifPtr(image.fmmap.size,
						image.fmmap.mmap),
			pngwidth,pngheight);
#else
	DEBUG_PRINT(DEBUG_DVI,(" (NO GIF DECODER)"));
#endif
	break;
      case 0xff: /* JPEG magic: 0xffd8 */
	DEBUG_PRINT(DEBUG_DVI,("\n  INCLUDE JPEG \t%s",image.filename));
#ifdef HAVE_GDIMAGECREATETRUECOLOR
#ifdef HAVE_GDIMAGECREATEFROMPNGPTR
	psimage=gdImageCreateFromJpegPtr(image.fmmap.size,image.fmmap.mmap);
#else
	psstream=fopen(image.filename,"rb");
	psimage=gdImageCreateFromJpeg(psstream);
	fclose(psstream);
#endif
	psimage=rescale(psimage,pngwidth,pngheight);
#else
	DEBUG_PRINT(DEBUG_DVI,(" (NO JPEG DECODER)"));
#endif
	break;
      default:  /* Default, PostScript magic: "%!PS-Adobe" */
	if (flags & NO_GHOSTSCRIPT) {
	  Warning("GhostScript calls disallowed by --noghostscript" );
	  flags |= PAGE_GAVE_WARN;
	} else {
	  /* Use alpha blending, and render transparent postscript
	     images. The alpha blending works correctly only from
	     libgd 2.0.12 upwards */
#ifdef HAVE_GDIMAGEPNGEX
	  if (page_imagep->trueColor) {
	    int tllx=llx,tlly=lly,turx=urx,tury=ury;

	    DEBUG_PRINT((DEBUG_DVI | DEBUG_GS),
			("\n  GS RENDER \t%s -> pngalpha ",image.filename));
	    if (!clip) {
	      /* Render across the whole image */ 
	      tllx=llx-(hh+1)*72/hresolution;
	      tlly=lly-(gdImageSY(page_imagep)-vv-1)*72/vresolution;
	      turx=llx+(gdImageSX(page_imagep)-hh)*72/hresolution;
	      tury=lly+(vv+1)*72/vresolution;
	      DEBUG_PRINT((DEBUG_DVI | DEBUG_GS),
			  ("\n  EXPAND BBOX \t%d %d %d %d -> %d %d %d %d",
			   llx,lly,urx,ury,tllx,tlly,turx,tury));
#ifdef DEBUG
	    } else {
	      DEBUG_PRINT((DEBUG_DVI | DEBUG_GS),(", CLIPPED TO BBOX"));
#endif
	    }
	    psimage = ps2png(&image, "-sDEVICE=pngalpha", 
			     hresolution, vresolution, 
			     tllx, tlly, turx, tury,
			     255,255,255);
	    if (psimage==NULL)
	      Warning("No GhostScript pngalpha output, opaque image inclusion");
	  } else
	    Warning("Palette output, opaque image inclusion");
#endif
	  if (psimage==NULL) {
	    /* png256 gives inferior result */
	    DEBUG_PRINT((DEBUG_DVI | DEBUG_GS),
			("\n  GS RENDER \t%s -> png16m", image.filename));
	    psimage = ps2png(&image, "-sDEVICE=png16m",
			     hresolution, vresolution, 
			     llx, lly, urx, ury,
			     cstack[0].red,cstack[0].green,cstack[0].blue);
	    clip=true;
	    flags |= PAGE_GAVE_WARN;
	  }
	  if (!clip) {
	    /* Rendering across the whole image */
	    hh=0;
	    vv=gdImageSY(psimage)-1;
	  }
	}
      }
      UnMmapFile(&(image.fmmap));
      if (psimage!=NULL) {
	DEBUG_PRINT(DEBUG_DVI,
		    ("\n  GRAPHIC(X|S) INCLUDE \t%s (%d,%d) res %dx%d at (%d,%d)",
		     psname,gdImageSX(psimage),gdImageSY(psimage),
		     hresolution,vresolution,hh,vv));
#ifdef HAVE_GDIMAGECREATETRUECOLOR
	if (psimage->trueColor && !page_imagep->trueColor)
	  gdImageTrueColorToPalette(psimage,0,256);
#endif
#ifdef HAVE_GDIMAGEPNGEX
	gdImageAlphaBlending(page_imagep,1);
#else
	Warning("Using libgd < 2.0.12, opaque image inclusion");
	flags |= PAGE_GAVE_WARN;
#endif
	gdImageCopy(page_imagep, psimage, 
		    hh, vv-gdImageSY(psimage)+1,
		    0,0,
		    gdImageSX(psimage),gdImageSY(psimage));
#ifdef HAVE_GDIMAGEPNGEX
	gdImageAlphaBlending(page_imagep,0);
#endif
	gdImageDestroy(psimage);
      } else {
        Warning("Unable to load %s, image will be left blank",image.filename);
        flags |= PAGE_GAVE_WARN;
      } 
      Message(BE_NONQUIET,">");
    } else { /* Don't draw */
      flags |= PAGE_TRUECOLOR;
      DEBUG_PRINT(DEBUG_DVI,
		  ("\n  GRAPHIC(X|S) INCLUDE \t%s (%d,%d) res %dx%d at (%d,%d)",
		   psname,pngheight,pngwidth,
		   hresolution,vresolution,hh,vv));
      min(x_min,hh);
      min(y_min,vv-pngheight+1);
      max(x_max,hh+pngwidth);
      max(y_max,vv+1);
    }
    return;
  }

  /******************* Raw PostScript ********************/

  if (strncmp(buffer,"!/preview@version(",18)==0) { 
    buffer+=18;
    length-=18;
    while (length>0 && buffer[length]!=')') 
      length--;
    if (page_imagep==NULL) 
      Message(BE_NONQUIET," (preview-latex version %.*s)",length,buffer);
    return;
  }

  /* preview-latex' tightpage option */
  if (strncmp(buffer,"!/preview@tightpage",19)==0) { 
    buffer+=19;
    SKIPSPACES(buffer);
    if (strncmp(buffer,"true",4)==0) {
      if (page_imagep==NULL) 
	Message(BE_NONQUIET," (preview-latex tightpage option detected, will use its bounding box)");
      flags |= PREVIEW_LATEX_TIGHTPAGE;
      return;
    }
  }
  if (strncmp(buffer,"!userdict",9)==0 
      && strstr(buffer+10,"7{currentfile token not{stop}if 65781.76 div")!=NULL) {
    if (page_imagep==NULL && ~flags & PREVIEW_LATEX_TIGHTPAGE) 
      Message(BE_NONQUIET," (preview-latex <= 0.9.1 tightpage option detected, will use its bounding box)");
    flags |= PREVIEW_LATEX_TIGHTPAGE;
    return;
  }

  /* preview-latex' dvips bop-hook redefinition */
  if (strncmp(buffer,"!userdict",9)==0 
      && strstr(buffer+10,"preview-bop-")!=NULL) {
    if (page_imagep==NULL) 
      Message(BE_VERBOSE," (preview-latex beginning-of-page-hook detected)");
    return;
  }

  if (strncmp(buffer,"ps::",4)==0) {
    /* Hokay, decode bounding box */
    dviunits adj_llx,adj_lly,adj_urx,adj_ury,ht,dp,wd;
    adj_llx = strtol(buffer+4,&buffer,10);
    adj_lly = strtol(buffer,&buffer,10);
    adj_urx = strtol(buffer,&buffer,10);
    adj_ury = strtol(buffer,&buffer,10);
    ht = strtol(buffer,&buffer,10);
    dp = strtol(buffer,&buffer,10);
    wd = strtol(buffer,&buffer,10);
    if (wd>0) {
      x_offset_tightpage = 
	(-adj_llx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
      x_width_tightpage  = x_offset_tightpage
	+(wd+adj_urx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    } else {
      x_offset_tightpage = 
	(-wd+adj_urx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
      x_width_tightpage  = x_offset_tightpage
	+(-adj_llx+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    }
    /* y-offset = height - 1 */
    y_offset_tightpage = 
      (((ht>0)?ht:0)+adj_ury+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor-1;
    y_width_tightpage  = y_offset_tightpage+1
      +(((dp>0)?dp:0)-adj_lly+dvi->conv*shrinkfactor-1)/dvi->conv/shrinkfactor;
    return;
  }

  if (buffer[0]=='"' || strcmp(buffer,"ps:")) { /* Raw PostScript */
    if (page_imagep != NULL) { /* Draw into image */
      struct pscode *pscodep,*tmp;
      gdImagePtr psimage=NULL;
      unsigned char*  command;
      
      Message(BE_NONQUIET," <raw PostScript");
      /* Init pscode with the raw PostScript snippet */
      if ((pscodep=alloca(sizeof(struct pscode)))==NULL)
	Fatal("cannot allocate space for raw PostScript struct");
      if (buffer[0]=='"')
	PSCodeInit(pscodep,buffer+1,NULL);
      else
	PSCodeInit(pscodep,buffer+3,NULL);
      /* Some packages split their raw PostScript code into several
	 specials. Check for those, and concatenate them so that
	 they're given to one and the same invocation of gs */
      tmp=pscodep;
      while(DVIIsNextPSSpecial(dvi)) {
	command=DVIGetCommand(dvi);
	DEBUG_PRINT(DEBUG_DVI,("DRAW CMD:\t%s", dvi_commands[*command]));
	length=UNumRead(command+1, dvi_commandlength[*command]-1);
	DEBUG_PRINT(DEBUG_DVI,
		    (" %d", UNumRead(command+1, dvi_commandlength[*command]-1)));
	if ((tmp->next=alloca(sizeof(struct pscode)+length+1))==NULL)
	  Fatal("cannot allocate space for raw PostScript struct");
	tmp=tmp->next;
	PSCodeInit(tmp,(char*)tmp+sizeof(struct pscode),NULL);
	strncpy(tmp->code,(char*)command + dvi_commandlength[*command],length);
	tmp->code[length]='\0';
	DEBUG_PRINT(DEBUG_DVI,(" '%s'",tmp->code));
	if (tmp->code[0]=='"')
	  tmp->code++;
	else
	  tmp->code+=3;
      }
      /* Now, render image */
      if (flags & NO_GHOSTSCRIPT)
	Warning("GhostScript calls disallowed by --noghostscript" );
      else {
	/* Use alpha blending, and render transparent postscript
	   images. The alpha blending works correctly only from libgd
	   2.0.12 upwards */
#ifdef HAVE_GDIMAGEPNGEX
	if (page_imagep->trueColor) {
	  //	  DEBUG_PRINT((DEBUG_DVI | DEBUG_GS),
	  //      ("\n  GS RENDER \t%s -> pngalpha ",image.filename));
	  /* Render across the whole image */ 
	  psimage = ps2png(pscodep, "-sDEVICE=pngalpha", 
			   dpi,dpi, 
			   -(hh+1)*72/dpi,
			   -(gdImageSY(page_imagep)-vv-1)*72/dpi,
			   (gdImageSX(page_imagep)-hh)*72/dpi,
			   (vv+1)*72/dpi,
			   255,255,255);
	  if (psimage!=NULL) {
	    gdImageAlphaBlending(page_imagep,1);
	    gdImageCopy(page_imagep, psimage, 
			0,0,0,0,
			gdImageSX(psimage),gdImageSY(psimage));
	    gdImageAlphaBlending(page_imagep,0);
	    gdImageDestroy(psimage);
	  }
	  else
	    Warning("No GhostScript pngalpha output, cannot render raw PostScript");
	} else
	  Warning("Palette output, cannot include raw PostScript");
#else
	Warning("Using libgd < 2.0.12, unable to include raw PostScript");
	flags |= PAGE_GAVE_WARN;
#endif
      }
      Message(BE_NONQUIET,">");
    } else { /* Don't draw */
      flags |= PAGE_TRUECOLOR;
    }
    return;
  }

  if (strncmp(buffer,"papersize=",10)==0) { /* papersize spec, ignored */
    return;
  }

  if (strncmp(buffer,"header=",7)==0) { /* PS header file */
    struct pscode* tmp=psheaderp;

    while (tmp!=NULL && (tmp->header==NULL || strcmp(tmp->header,buffer+7))!=0)
      tmp=tmp->next;
    if ( tmp == NULL ) {
      DEBUG_PRINT(DEBUG_GS,("\n  PS HEADER:\t'%s'", tmp->header));
      while (tmp->next!=NULL)
	tmp=tmp->next;
      if ((tmp->next=malloc(sizeof(struct pscode)+strlen(buffer+7)))==NULL) {
	Warning("cannot malloc space for psheader name, ignored");
	flags |= PAGE_GAVE_WARN;
	return;
      }
      tmp=tmp->next;
      PSCodeInit(tmp,NULL,(char*)tmp+sizeof(struct pscode));
      strcpy(tmp->header,buffer+7);
    }
    return;
  }

  if (buffer[0]=='!') { /* raw PS header */
    struct pscode* tmp=psheaderp;

    while (tmp!=NULL && (tmp->code==NULL || strcmp(tmp->code,buffer+1))!=0)
      tmp=tmp->next;
    if ( tmp == NULL ) {
      DEBUG_PRINT(DEBUG_GS,("\n  PS RAW HEADER:\t'%s'", buffer+1));
      while (tmp->next!=NULL)
	tmp=tmp->next;
      if ((tmp->next=malloc(sizeof(struct pscode)+strlen(buffer+1)))==NULL) {
	Warning("cannot malloc space for raw psheader, ignored");
	flags |= PAGE_GAVE_WARN;
	return;
      }
      tmp=tmp->next;
      PSCodeInit(tmp,(char*)tmp+sizeof(struct pscode),NULL);
      strcpy(tmp->code,buffer+1);
    }
    return;
  }

  if (strncmp(buffer,"src:",4)==0) { /* source special */
    if ( page_imagep != NULL )
      Message(BE_NONQUIET," at (%ld,%ld) source \\special{%.*s}",
	      hh, vv, length,special);
    return;
  }
  if ( page_imagep != NULL ) {
    Warning("at (%ld,%ld) unimplemented \\special{%.*s}",
	    hh, vv, length,special);
    flags |= PAGE_GAVE_WARN;
  }
}

