#include "dvipng.h"

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

  DEBUG_PRINTF2(DEBUG_DVI," '%.*s'",length,special);

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
    char* psfile = token+7;
    int llx=0,lly=0,urx=0,ury=0,rwi=0,rhi=0;
    int hresolution,vresolution;

    /* Remove quotation marks around filename */
    if (*psfile=='"') {
      char* tmp;
      psfile++;
      tmp=strchr(psfile,'"');
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
	  Warning("Unable to open file %s for writing", pngfile );
	  return;
	}
	psimage = gdImageCreateFromPng(pngf);
	fclose(pngf);
	DEBUG_PRINTF2(DEBUG_DVI,"\n  PS-PNG INCLUDE \t(%d,%d)", 
		      gdImageSX(psimage),gdImageSY(psimage));
	DEBUG_PRINTF2(DEBUG_DVI," resolution %dx%d", 
		      hresolution,vresolution);
	DEBUG_PRINTF2(DEBUG_DVI," at (%d,%d)", 
		      PIXROUND(h, dvi->conv*shrinkfactor),
		      PIXROUND(v, dvi->conv*shrinkfactor));
	DEBUG_PRINTF2(DEBUG_DVI," offset (%d,%d)", x_offset,y_offset);
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
      pngheight = vresolution*(ury - lly)/72;
      pngwidth  = hresolution*(urx - llx)/72;
      DEBUG_PRINTF2(DEBUG_DVI,"\n  PS-PNG INCLUDE \t(%d,%d)", 
		      pngwidth,pngheight);
      min(x_min,PIXROUND(h, dvi->conv*shrinkfactor));
      min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)-pngheight);
      max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)+pngwidth);
      max(y_max,PIXROUND(v, dvi->conv*shrinkfactor));
    }
    return;
  }
  Warning("at (%ld,%ld) unimplemented \\special{%.*s}. %s",
	  PIXROUND(h,dvi->conv*shrinkfactor),
	  PIXROUND(v,dvi->conv*shrinkfactor),length,special,token);
}


