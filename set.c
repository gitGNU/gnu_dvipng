#include "dvipng.h"

void CreateImage(void)
{
  int Background;

  if (page_imagep) 
    gdImageDestroy(page_imagep);
  if (x_width <= 0) x_width=1;
  if (y_width <= 0) y_width=1;
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

void WriteImage(char *pngname, int pagenum)
{
  char* pos;
  FILE* outfp=NULL;

  if ((pos=strchr(pngname,'%')) != NULL) {
    if (strchr(++pos,'%'))
      Fatal("too many %%'s in output file name");
    if (*pos == 'd' || strncmp(pos,"03d",3)==0) {
      /* %d -> pagenumber, so add 9 string positions 
	 since pagenumber max +-2^31 or +-2*10^9 */
      char* tempname = alloca(strlen(pngname)+9);
      sprintf(tempname,pngname,pagenum);
      pngname = tempname;
    } else {
      Fatal("unacceptible format spec. in output file name");
    }
  }
  if ((outfp = fopen(pngname,"wb")) == NULL)
      Fatal("Cannot open output file %s",pngname);
  gdImagePng(page_imagep,outfp);
  fclose(outfp);
  DEBUG_PRINT((DEBUG_DVI,"\n  WROTE:   \t%s\n",pngname));
  gdImageDestroy(page_imagep);
  page_imagep=NULL;
}



/*-->SetRule*/
/**********************************************************************/
/*****************************  SetRule  ******************************/
/**********************************************************************/
/*   this routine will draw a rule */
int32_t SetRule(int32_t a, int32_t b, int32_t h,int32_t v,int PassNo)
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
    min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)-yy+1);
    max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)+xx-1);
    max(y_max,PIXROUND(v, dvi->conv*shrinkfactor)+1);
    break;
  case PASS_DRAW:
    if ((yy>0) && (xx>0)) {
      /*
	Oh, bugger. Shrink rule properly. Currently produces too dark
	rules, but...  Positioned to imitate dvips' behaviour.
      */
      Color = gdImageColorResolve(page_imagep, Red,Green,Blue);
      gdImageFilledRectangle(page_imagep,
			     PIXROUND(h, dvi->conv*shrinkfactor)-1+x_offset,
			     PIXROUND(v, dvi->conv*shrinkfactor)-yy+1+y_offset,
			     PIXROUND(h, dvi->conv*shrinkfactor)+xx-2+x_offset,
			     PIXROUND(v, dvi->conv*shrinkfactor)+y_offset,
			     Color);
      DEBUG_PRINT((DEBUG_DVI,"\n  RULE \t%dx%d at (%d,%d) offset (%d,%d)",
		   xx, yy,
		   PIXROUND(h, dvi->conv*shrinkfactor),
		   PIXROUND(v, dvi->conv*shrinkfactor),
		   x_offset,y_offset));
    }
  }
  return(b);
}

