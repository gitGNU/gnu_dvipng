#include "dvipng.h"

void DoBop P1H(void)
{
  if (page_imagep) 
    gdImageDestroy(page_imagep);
  page_imagep=gdImageCreate(x_width,y_width);
  gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue); /* Set bg color */
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
void FormFeed P1C(int, pagenum)
{
  FILE* outfp=NULL;

  (void)sprintf(pngname,"%s%d.png",rootname,pagenum);
  if ((outfp = fopen(pngname,"wb")) == NULL)
      Fatal("Cannot open output file %s",pngname);
  gdImagePng(page_imagep,outfp);
  fclose(outfp);
  gdImageDestroy(page_imagep);
  page_imagep=NULL;
}


/*-->SetChar*/
/**********************************************************************/
/*****************************  SetChar  ******************************/
/**********************************************************************/
int32_t SetChar P2C(int32_t, c, int, PassNo)
{
  register struct char_entry *ptr;  /* temporary char_entry pointer */

  if ((ptr = vfstack[vfstackptr]->ch[c]) != NULL) {
    if (vfstack[vfstackptr]->is_vf) {
      return(SetVF(c, PassNo));
    } else {
      return(SetPK(c, PassNo));
    }
  } else {
    return(0);
  }
}

int32_t SetPK P2C(int32_t, c, int, PassNo)
{
  register struct char_entry *ptr;  /* temporary char_entry pointer */
  int red,green,blue;
  int Color,i;

  if ((ptr = vfstack[vfstackptr]->ch[c]) != NULL) {
    switch(PassNo) {
    case PASS_BBOX:
      if (!(ptr->isloaded)) {
	LoadAChar(c, ptr);
      }
      min(x_min,PIXROUND(h, conv*shrinkfactor)
	  -PIXROUND(ptr->xOffset,shrinkfactor));
      min(y_min,PIXROUND(v, conv*shrinkfactor)
	  -PIXROUND(ptr->yOffset,shrinkfactor));
      max(x_max,PIXROUND(h, conv*shrinkfactor)
	  -PIXROUND(ptr->xOffset,shrinkfactor)+ptr->glyph.w);
      max(y_max,PIXROUND(v, conv*shrinkfactor)
	  -PIXROUND(ptr->yOffset,shrinkfactor)+ptr->glyph.h);
      break;
    case PASS_DRAW:
      if (!(ptr->isloaded)) {
	LoadAChar(c, ptr);
      }
      /*
       * Draw character. Remember now, we have stored the different
       * greyscales in glyph.data with darkest last.  Draw the character
       * greyscale by greyscale, lightest first.
       */
      for( i=1; i<=ptr->glyph.nchars ; i++) {
	red = bRed-(bRed-Red)*i/shrinkfactor/shrinkfactor;
	green = bGreen-(bGreen-Green)*i/shrinkfactor/shrinkfactor;
	blue = bBlue-(bBlue-Blue)*i/shrinkfactor/shrinkfactor;
	Color = gdImageColorResolve(page_imagep,red,green,blue);
	gdImageChar(page_imagep, &(ptr->glyph),
		    PIXROUND(h, conv*shrinkfactor)
		    -PIXROUND(ptr->xOffset,shrinkfactor)
		    +x_offset,
		    PIXROUND(v, conv*shrinkfactor)
		    -PIXROUND(ptr->yOffset,shrinkfactor)
		    +y_offset,
		    i,Color);
      }
    }
  }
#ifdef DEBUG
  if (Debug)
    printf("(at (%d,%d)-(%d,%d) offset (%d,%d)) ",
	   PIXROUND(h, conv*shrinkfactor),
	   PIXROUND(v, conv*shrinkfactor),
	   PIXROUND(ptr->xOffset,shrinkfactor),
	   PIXROUND(ptr->yOffset,shrinkfactor),
	   x_offset,y_offset);
#endif
  return(ptr->isloaded?ptr->tfmw:0);
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
    xx = (int32_t)PIXROUND(b, conv*shrinkfactor);     /* width */
    yy = (int32_t)PIXROUND(a, conv*shrinkfactor);     /* height */
  }
  switch(PassNo) {
  case PASS_BBOX:
    min(x_min,PIXROUND(h, conv*shrinkfactor)-1);
    min(y_min,PIXROUND(v, conv*shrinkfactor)-yy+1-1);
    max(x_max,PIXROUND(h, conv*shrinkfactor)+xx-1-1);
    max(y_max,PIXROUND(v, conv*shrinkfactor)-1);
    break;
  case PASS_DRAW:
    if ((yy>0) && (xx>0)) {
      /*
	Oh, bugger. Shrink rule properly. Currently produces too dark
	rules, but...  Why do I need the -1's? Beats me.
      */
      Color = gdImageColorResolve(page_imagep, Red,Green,Blue);
      gdImageFilledRectangle(page_imagep,
			     PIXROUND(h, conv*shrinkfactor)+x_offset-1,
			     PIXROUND(v, conv*shrinkfactor)-yy+1+y_offset-1,
			     PIXROUND(h, conv*shrinkfactor)+xx-1+x_offset-1,
			     PIXROUND(v, conv*shrinkfactor)+y_offset-1,
			     Color);
#ifdef DEBUG
      if (Debug)
	printf("Rule (%d,%d) at (%d,%d) offset (%d,%d)\n",
	       xx, yy,
	       PIXROUND(h, conv*shrinkfactor),
	       PIXROUND(v, conv*shrinkfactor),
	       x_offset,y_offset);
#endif
    }
  }
  return(b);
}

