#include "dvipng.h"

#if NeedFunctionPrototypes
void DoBop(void)
#else
void DoBop()
#endif
{
  if (page_imagep) gdImageDestroy(page_imagep);
  /*
  page_imagep=gdImageCreate(h_pgsiz_dots,v_pgsiz_dots);
  */
  page_imagep=gdImageCreate(x_max-x_min,y_max-y_min);
  x_goffset=-x_min;
  y_goffset=-y_min;
  x_min=0; /* Always include topright corner */
  y_min=0;
  x_max=0;
  y_max=0;
  gdImageColorAllocate(page_imagep,bRed,bGreen,bBlue); /* Set bg color */
}

/*-->FormFeed*/
/**********************************************************************/
/*****************************  FormFeed ******************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void FormFeed(int pagenum)
#else
void FormFeed(pagenum)
     int pagenum;
#endif
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
void
#if NeedFunctionPrototypes
SetChar(long4 c, short command, int PassNo)
#else
SetChar(c, command, PassNo)
long4   c;
short   command;
int     PassNo;
#endif
{
  register struct char_entry *ptr;  /* temporary char_entry pointer */
  int red,green,blue;
  int Color,i;

  ptr = &(fontptr->ch[c]);
  if (!(ptr->isloaded))
    LoadAChar(c, ptr);
  if (PassNo == 0) {
    min(x_min,PIXROUND(h, hconv*shrinkfactor)
	-PIXROUND(ptr->xOffset,shrinkfactor));
    min(y_min,PIXROUND(v, hconv*shrinkfactor)
	-PIXROUND(ptr->yOffset,shrinkfactor));
    max(x_max,PIXROUND(h, hconv*shrinkfactor)
	-PIXROUND(ptr->xOffset,shrinkfactor)+ptr->glyph.w);
    max(y_max,PIXROUND(v, hconv*shrinkfactor)
	-PIXROUND(ptr->yOffset,shrinkfactor)+ptr->glyph.h);
  } else if (fontptr->font_file_id != NO_FILE) {
    /*
      Draw character. Remember now, we have stored the different
      greyscales in glyph.data with darkest last.  Draw the
      character greyscale by greyscale, lightest first. 
    */
    for( i=1; i<=ptr->glyph.nchars ; i++) {
      red = bRed-(bRed-Red)*i/shrinkfactor/shrinkfactor;
      green = bGreen-(bGreen-Green)*i/shrinkfactor/shrinkfactor;
      blue = bBlue-(bBlue-Blue)*i/shrinkfactor/shrinkfactor;
      Color = gdImageColorResolve(page_imagep,red,green,blue);
      gdImageChar(page_imagep, &(ptr->glyph),
		  PIXROUND(h, hconv*shrinkfactor)
		  -PIXROUND(ptr->xOffset,shrinkfactor)
		  +x_goffset,
		  PIXROUND(v, vconv*shrinkfactor)
		  -PIXROUND(ptr->yOffset,shrinkfactor)
		  +y_goffset,
		  i,Color);
    }

#ifdef DEBUG
    if (Debug)
      printf("<%c> at (%d,%d)-(%d,%d)\n",(char)c,
	     PIXROUND(h, hconv*shrinkfactor),
	     PIXROUND(v, vconv*shrinkfactor),
	     ptr->xOffset,ptr->yOffset);
#endif
  }

  if (command <= SET4)
    h += ptr->tfmw;
}


/*-->SetRule*/
/**********************************************************************/
/*****************************  SetRule  ******************************/
/**********************************************************************/
/*   this routine will draw a rule */
#if NeedFunctionPrototypes
void SetRule(long4 a, long4 b, int PassNo, int Set)
#else
void SetRule(a, b, PassNo, Set)
long4    a, b;
int     PassNo, Set;
#endif
{
  long4    xx=0, yy=0;
  int Color;

  if ( a > 0 && b > 0 ) {
    /*SetPosn(h, v);             /* lower left corner */
    xx = (long4)PIXROUND(b, hconv*shrinkfactor);     /* width */
    yy = (long4)PIXROUND(a, vconv*shrinkfactor);     /* height */
  }
  if (PassNo == 0) {
    min(x_min,PIXROUND(h, hconv*shrinkfactor)-1);
    min(y_min,PIXROUND(v, hconv*shrinkfactor)-yy+1-1);
    max(x_max,PIXROUND(h, hconv*shrinkfactor)+xx-1-1);
    max(y_max,PIXROUND(v, hconv*shrinkfactor)-1);
    return;
  } else {
#ifdef DEBUG
    if (Debug)
      fprintf(ERR_STREAM,"Rule xx=%ld, yy=%ld\n", (long)xx, (long)yy);
#endif
    if ((yy>0) && (xx>0)) {
      /*
	Oh, bugger. Shrink rule properly. Currently crude, but...
	Why do I need the -1's? Beats me.
      */

      Color = gdImageColorResolve(page_imagep, Red,Green,Blue);
      gdImageFilledRectangle(page_imagep,
			     PIXROUND(h, hconv*shrinkfactor)+x_goffset-1,
			     PIXROUND(v, vconv*shrinkfactor)-yy+1+y_goffset-1,
			     PIXROUND(h, hconv*shrinkfactor)+xx-1+x_goffset-1,
			     PIXROUND(v, vconv*shrinkfactor)+y_goffset-1,
			     Color);
    }
  }
  if (Set)
    h += b;
}

