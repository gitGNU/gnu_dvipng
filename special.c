#include "dvipng.h"
#include <sys/wait.h>

gdImagePtr
ps2png(const char *psfile, int hresolution, int vresolution, 
       int urx, int ury, int llx, int lly)
{
  int status, downpipe[2], uppipe[2];
  pid_t pid;
  char resolution[STRSIZE]; 
  /* For some reason, png256 gives inferior result */
  char *device="-sDEVICE=png16m";  
  gdImagePtr psimage=NULL;

  status=snprintf(resolution, STRSIZE, "-r%dx%d",hresolution,vresolution);
  /* png16m being the default, this code is not needed
   * #ifdef HAVE_GDIMAGECREATETRUECOLOR
   * if (truecolor) 
   * device="-sDEVICE=png16m";
   * #endif  
   */
  if (status>0 && status<STRSIZE 
      && pipe(downpipe)==0 && pipe(uppipe)==0) { /* Ready to fork */
    pid = fork ();
    if (pid == 0) { /* Child process.  Execute gs. */       
      close(downpipe[1]);
      dup2(downpipe[0], STDIN_FILENO);
      close(downpipe[0]);
      DEBUG_PRINT((DEBUG_GS,"\n  GS CALL:\t%s %s %s %s %s %s %s %s %s %s %s",
		   GS_PATH, device, resolution,
		   "-dBATCH", "-dNOPAUSE", "-dSAFER", "-q", 
		   "-sOutputFile=-", 
		   "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
		   "-"));
      close(uppipe[0]);
      dup2(uppipe[1], STDOUT_FILENO);
      close(uppipe[1]);
      execl (GS_PATH, GS_PATH, device, resolution,
	     "-dBATCH", "-dNOPAUSE", "-dSAFER", "-q", 
	     "-sOutputFile=-", 
	     "-dTextAlphaBits=4", "-dGraphicsAlphaBits=4",
	     "-",NULL);
      _exit (EXIT_FAILURE);
    }
    else if (pid > 0) { /* Parent process. */
      FILE *psstream, *pngstream;
      
      close(downpipe[0]);
      psstream=fdopen(downpipe[1],"w");
      if (psstream) {
	DEBUG_PRINT((DEBUG_GS,"\n  PS CODE:\t<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice",
		urx - llx, ury - lly,llx,lly));
	fprintf(psstream, "<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice\n",
		urx - llx, ury - lly,llx,lly);
	DEBUG_PRINT((DEBUG_GS,"\n  PS CODE:\t(%s) run", psfile));
	fprintf(psstream, "(%s) run\n", psfile);
	DEBUG_PRINT((DEBUG_GS,"\n  PS CODE:\tquit"));
	fprintf(psstream, "quit\n");
	fclose(psstream);
      }
      close(uppipe[1]);
      pngstream=fdopen(uppipe[0],"r");
      if (pngstream) {
	psimage = gdImageCreateFromPng(pngstream);
	fclose(pngstream);
      }
      if (psimage != NULL)
	DEBUG_PRINT((DEBUG_GS,"\n  GS OUTPUT:\t%dx%d image ",gdImageSX(psimage),gdImageSY(psimage)));
      else
	DEBUG_PRINT((DEBUG_GS,"\n  GS OUTPUT:\tNO IMAGE "));
    }
  }
  return psimage;
}

/*-->SetSpecial*/
/*********************************************************************/
/****************************  SetSpecial  ***************************/
/*********************************************************************/

void SetSpecial(char * special, int32_t length, int32_t h, int32_t v, 
		bool PassNo)
/* interpret a \special command, made up of keyword=value pairs */
/* Color specials only for now. Warn otherwise. */
{
  char *buffer, *token;

  DEBUG_PRINT((DEBUG_DVI," '%.*s'",length,special));

  buffer = malloc(sizeof(char)*(length+1));
  if (buffer==NULL) 
    Fatal("Cannot allocate space for special string");

  strncpy(buffer,special,length);
  buffer[length]='\0';

  token = strtok(buffer," ");

  /********************** Color specials ***********************/
  if (strcmp(token,"background")==0) {
    background(token+11);
    return;
  }
  if (strcmp(token,"color")==0) {
    token = strtok(NULL," ");
    if (strcmp(token,"push")==0)
      pushcolor(token+5);
    else 
      if (strcmp(token,"pop")==0)
	popcolor();
      else 
	resetcolorstack(token);
    return;
  }

  /******************* Postscript inclusion ********************/
  if (strncmp(token,"PSfile=",7)==0) { /* PSfile */
    char* psname = token+7;
    int llx=0,lly=0,urx=0,ury=0,rwi=0,rhi=0;
    int hresolution,vresolution;

    /* Remove quotation marks around filename */
    if (*psname=='"') {
      char* tmp;
      psname++;
      tmp=strrchr(psname,'"');
      if (tmp!=NULL) {
	*tmp='\0';
      }
    }

    while((token = strtok(NULL," ")) != NULL) {
      if (strncmp(token,"llx=",4)==0) llx = atoi(token+4);
      if (strncmp(token,"lly=",4)==0) lly = atoi(token+4);
      if (strncmp(token,"urx=",4)==0) urx = atoi(token+4);
      if (strncmp(token,"ury=",4)==0) ury = atoi(token+4);
      if (strncmp(token,"rwi=",4)==0) rwi = atoi(token+4);
      if (strncmp(token,"rhi=",4)==0) rhi = atoi(token+4);
    }
    
    /* Calculate resolution, and use our base resolution as a fallback. */
    /* The factor 10 is magic, the dvips graphicx driver needs this.    */
    hresolution = resolution/shrinkfactor*rwi/(urx - llx)/10;
    vresolution = resolution/shrinkfactor*rhi/(ury - lly)/10;
    if (vresolution==0) vresolution = hresolution;
    if (hresolution==0) hresolution = vresolution;
    if (hresolution==0) hresolution = vresolution = resolution/shrinkfactor;
    
    if (PassNo==PASS_DRAW) { /* PASS_DRAW */
      char* psfile = kpse_find_file(psname,kpse_pict_format,0);
      char* pngname = NULL;
      gdImagePtr psimage=NULL;

      if (cacheimages) {
	char* pngfile;

	pngname = malloc(sizeof(char)*(strlen(psname+5)));
	if (pngname==NULL) 
	  Fatal("Cannot allocate space for cached image filename");
	strcpy(pngname,psname);
	pngfile = strrchr(pngname,'.');
	if (pngfile==NULL)
	  strcat(pngname,".png");
	else {
	  pngfile[1] = 'p';
	  pngfile[2] = 'n';
	  pngfile[3] = 'g';
	  pngfile[4] = '\0';
	}
	pngfile = kpse_find_file(pngname,kpse_pict_format,0);
	if (pngfile!=NULL) {
	  FILE* pngfilep = fopen(pngfile,"rb");

	  if (pngfilep!=NULL)
	    psimage = gdImageCreateFromPng(pngfilep);
	}
      }
      Message(BE_NONQUIET,"<%s",psname);
      if (psimage==NULL) {
	if (psfile == NULL) 
	  Warning("PS file %s not found, image will be left blank", psname );
	else {
	  psimage = ps2png(psfile, hresolution, vresolution, 
			   urx, ury, llx, lly);
	  if ( psimage == NULL ) 
	    Warning("Unable to convert %s to PNG, image will be left blank", 
		    psfile );
	  else if (!truecolor)
	    gdImageTrueColorToPalette(psimage,0,256);
	}
      }
      if (pngname !=NULL && psimage != NULL) {
	FILE* pngfilep = fopen(pngname,"wb");
	gdImagePng(psimage,pngfilep);
      } 
      if (psimage!=NULL) {
	DEBUG_PRINT((DEBUG_DVI,
		     "\n  PS-PNG INCLUDE \t%s (%d,%d) res %dx%d at (%d,%d) offset (%d,%d)",
		     psfile,
		     gdImageSX(psimage),gdImageSY(psimage),
		     hresolution,vresolution,
		     PIXROUND(h, dvi->conv*shrinkfactor),
		     PIXROUND(v, dvi->conv*shrinkfactor),
		     x_offset,y_offset));
	gdImageCopy(page_imagep, psimage,
		    PIXROUND(h,dvi->conv*shrinkfactor)+x_offset,
		    PIXROUND(v,dvi->conv*shrinkfactor)-gdImageSY(psimage)
		    +y_offset,
		    0,0,
		    gdImageSX(psimage),gdImageSY(psimage));
	gdImageDestroy(psimage);
      }
      Message(BE_NONQUIET,">",psfile);
    } else { /* Not PASS_DRAW */
      int pngheight,pngwidth;
      
      /* Convert from postscript 72 dpi resolution to our given resolution */
      pngheight = (vresolution*(ury - lly)+71)/72; /* +71: do 'ceil' */
      pngwidth  = (hresolution*(urx - llx)+71)/72;
      DEBUG_PRINT((DEBUG_DVI,"\n  PS-PNG INCLUDE \t(%d,%d)", 
		   pngwidth,pngheight));
      min(x_min,PIXROUND(h, dvi->conv*shrinkfactor));
      min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)-pngheight);
      max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)+pngwidth);
      max(y_max,PIXROUND(v, dvi->conv*shrinkfactor));
    }
    return;
  }
#if 0
      char cmd[255],tmp[255];
      char *scale_file = tmpnam(tmp);
      char *pngfile = tmpnam(NULL);
      FILE* scalef;

      if ( (scalef = fopen(scale_file,"wb")) == NULL ) {
	Warning("Unable to open file %s for writing", scale_file );
	return;
      }
      fprintf(scalef, "<</PageSize[%d %d]/PageOffset[%d %d[1 1 dtransform exch]{0 ge{neg}if exch}forall]>>setpagedevice\n",  
	      urx - llx, ury - lly,llx,lly);
      fclose( scalef );
      sprintf(cmd,"gs -sDEVICE=png16m -r%dx%d -dBATCH -dSAFER -q -dNOPAUSE -sOutputFile=%s -dTextAlphaBits=4 -dGraphicsAlphaBits=4 %s %s",
	      hresolution,
	      vresolution,
	      pngfile,
	      scale_file,
	      psfile);
      
      if (system(cmd)) {
	Warning("execution of '%s' returned an error, image will be left blank.", cmd);
      } else {   
	gdImagePtr psimage;
	FILE* pngf;
	
	if ( (pngf = fopen(pngfile,"rb")) == NULL ) {
	  Warning("Unable to open file %s for reading", pngfile );
	  return;
	}
	psimage = gdImageCreateFromPng(pngf);
	fclose(pngf);
	DEBUG_PRINT((DEBUG_DVI,
	     "\n  PS-PNG INCLUDE \t(%d,%d) dpi %dx%d at (%d,%d) offset (%d,%d)",   
		     gdImageSX(psimage),gdImageSY(psimage),
		     hresolution,vresolution,
		     PIXROUND(h, dvi->conv*shrinkfactor),
		     PIXROUND(v, dvi->conv*shrinkfactor),
		     x_offset,y_offset));
	gdImageCopy(page_imagep, psimage,
		    PIXROUND(h,dvi->conv*shrinkfactor)+x_offset,
		    PIXROUND(v,dvi->conv*shrinkfactor)-gdImageSY(psimage)
		       +y_offset,
		    0,0,
		    gdImageSX(psimage),gdImageSY(psimage));
	gdImageDestroy(psimage);
	Message(BE_NONQUIET,"<%s>",psfile);
      }
      unlink(scale_file);
      unlink(pngfile);
    } else {
      int pngheight,pngwidth;

      /* Convert from postscript 72 dpi resolution to our given resolution */
      pngheight = (vresolution*(ury - lly)+71)/72; /* +71: do 'ceil' */
      pngwidth  = (hresolution*(urx - llx)+71)/72;
      DEBUG_PRINT((DEBUG_DVI,"\n  PS-PNG INCLUDE \t(%d,%d)", 
		   pngwidth,pngheight));
      min(x_min,PIXROUND(h, dvi->conv*shrinkfactor));
      min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)-pngheight);
      max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)+pngwidth);
      max(y_max,PIXROUND(v, dvi->conv*shrinkfactor));
    }
    return;
#endif
  Warning("at (%ld,%ld) unimplemented \\special{%.*s}. %s",
	  PIXROUND(h,dvi->conv*shrinkfactor),
	  PIXROUND(v,dvi->conv*shrinkfactor),length,special,token);
}


