#include "dvipng.h"

void DoBop P1H(void)
{
  int Background;

  if (page_imagep) 
    gdImageDestroy(page_imagep);
#ifdef DEBUG
  if (Debug)
    printf("(image %dx%d) ",x_width,y_width);
#endif
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

/*-->FormFeed*/
/**********************************************************************/
/*****************************  FormFeed ******************************/
/**********************************************************************/
void FormFeed P2C(struct dvi_data*, dvi, int, pagenum)
{
  char  pngname[STRSIZE];       
  FILE* outfp=NULL;

  (void)sprintf(pngname,"%s%d.png",dvi->outname,pagenum);
  if ((outfp = fopen(pngname,"wb")) == NULL)
      Fatal("Cannot open output file %s",pngname);
  gdImagePng(page_imagep,outfp);
  fclose(outfp);
#ifdef DEBUG
  if (Debug)
    printf("(wrote %s)\n",pngname);
#endif
  gdImageDestroy(page_imagep);
  page_imagep=NULL;
}


/*-->SetChar*/
/**********************************************************************/
/*****************************  SetChar  ******************************/
/**********************************************************************/
int32_t SetChar P2C(int32_t, c, int, PassNo)
{
  switch(currentfont->type) {
  case FONT_TYPE_PK:
    return(SetPK(c, PassNo));
    break;
  case FONT_TYPE_VF:
    return(SetVF(c, PassNo));
    break;
  default:
  }
  return(0);
}


/*-->SetRule*/
/**********************************************************************/
/*****************************  SetRule  ******************************/
/**********************************************************************/
/*   this routine will draw a rule */
int32_t SetRule P3C(int32_t, a, int32_t, b, int, PassNo)
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
#ifdef DEBUG
      if (Debug)
	printf("Rule (%d,%d) at (%d,%d) offset (%d,%d)\n",
	       xx, yy,
	       PIXROUND(h, dvi->conv*shrinkfactor),
	       PIXROUND(v, dvi->conv*shrinkfactor),
	       x_offset,y_offset);
#endif
    }
  }
  return(b);
}

