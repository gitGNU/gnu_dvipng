#include "dvipng.h"

void CreateImage(void)
{
  int Background;

  if (page_imagep) 
    gdImageDestroy(page_imagep);
  page_imagep=gdImageCreate(x_width,y_width);
  /* Set bg color */
  Background = gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue);
  if (borderwidth<0) {
    gdImageColorTransparent(page_imagep,Background); 
  }
  if (borderwidth>0) {
    int Transparent;

    /* Set ANOTHER bg color, transparent this time */
    Transparent = gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue); 
    gdImageColorTransparent(page_imagep,Transparent); 
    gdImageFilledRectangle(page_imagep,0,0,
			   x_width-1,borderwidth-1,Transparent);
    gdImageFilledRectangle(page_imagep,0,0,
			   borderwidth-1,y_width-1,Transparent);
    gdImageFilledRectangle(page_imagep,x_width-borderwidth,0,
			   x_width-1,y_width-1,Transparent);
    gdImageFilledRectangle(page_imagep,0,y_width-borderwidth,
			   x_width-1,y_width-1,Transparent);
  }
}

void WriteImage(char *outname, int pagenum)
{
  char  pngname[STRSIZE];       
  FILE* outfp=NULL;

  (void)sprintf(pngname,"%s%d.png",outname,pagenum);
  if ((outfp = fopen(pngname,"wb")) == NULL)
      Fatal("Cannot open output file %s",pngname);
  gdImagePng(page_imagep,outfp);
  fclose(outfp);
  DEBUG_PRINTF(DEBUG_DVI,"\n  WROTE:   \t%s\n",pngname);
  gdImageDestroy(page_imagep);
  page_imagep=NULL;
}

/*-->SetChar*/
/**********************************************************************/
/*****************************  SetChar  ******************************/
/**********************************************************************/
int32_t SetChar(int32_t c, int PassNo)
{
  switch(currentfont->type) {
  case FONT_TYPE_PK:
    return(SetPK(c, PassNo));
    break;
  case FONT_TYPE_VF:
    return(SetVF(c, PassNo));
  }
  return(0);
}


/*-->SetRule*/
/**********************************************************************/
/*****************************  SetRule  ******************************/
/**********************************************************************/
/*   this routine will draw a rule */
int32_t SetRule(int32_t a, int32_t b, int PassNo)
{
  int32_t    xx=0, yy=0;
  int Color;

  if ( a > 0 && b > 0 ) {
    xx = (int32_t)PIXROUND(b, dvi->conv*shrinkfactor);     /* width */
    yy = (int32_t)PIXROUND(a, dvi->conv*shrinkfactor);     /* height */
  }
  switch(PassNo) {
  case PASS_BBOX:
    min(x_min,PIXROUND(h, dvi->conv*shrinkfactor)-1);
    min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)-yy+1-1);
    max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)+xx-1-1);
    max(y_max,PIXROUND(v, dvi->conv*shrinkfactor)-1);
    break;
  case PASS_DRAW:
    if ((yy>0) && (xx>0)) {
      /*
	Oh, bugger. Shrink rule properly. Currently produces too dark
	rules, but...  Why do I need the -1's? Beats me.
      */
      Color = gdImageColorResolve(page_imagep, Red,Green,Blue);
      gdImageFilledRectangle(page_imagep,
			     PIXROUND(h, dvi->conv*shrinkfactor)+x_offset-1,
			     PIXROUND(v, dvi->conv*shrinkfactor)-yy+1+y_offset-1,
			     PIXROUND(h, dvi->conv*shrinkfactor)+xx-1+x_offset-1,
			     PIXROUND(v, dvi->conv*shrinkfactor)+y_offset-1,
			     Color);
      DEBUG_PRINTF2(DEBUG_DVI,"\n  RULE \t(%d,%d)", xx, yy);
      DEBUG_PRINTF2(DEBUG_DVI," at (%d,%d)", 
		    PIXROUND(h, dvi->conv*shrinkfactor),
		    PIXROUND(v, dvi->conv*shrinkfactor));
      DEBUG_PRINTF2(DEBUG_DVI," offset (%d,%d)", x_offset,y_offset);
    }
  }
  return(b);
}

