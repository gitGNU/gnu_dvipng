#include "dvipng.h"

/*
#define DRAWGLYPH
*/

/*-->PkRaster*/
/*********************************************************************/
/**************************** PkRaster *******************************/
/*********************************************************************/

unsigned char   bitweight, inputbyte;
unsigned char   dyn_f;
unsigned char   *pkloc;
int             repeatcount;

unsigned char
#if NeedFunctionPrototypes
getnyb(FILE* fp)
#else
getnyb(fp)
FILE* fp;
#endif
{
  register unsigned char  temp;
  if ( bitweight == 0 ) {
    inputbyte = NoSignExtend(fp,1);
    bitweight = 16;
  }
  temp = inputbyte / bitweight;
  inputbyte -= temp * bitweight;
  bitweight /= 16;
  return ( temp );
}


long4 
#if NeedFunctionPrototypes
pk_packed_num(FILE* fp)
#else
pk_packed_num(fp)
     FILE* fp;
#endif
{ /*@<Packed number procedure@>= */
  register int    i;
  long4    j;

  i = (int)getnyb(fp);
  if (i == 0) {
    do {
      j = (long4)getnyb(fp);
      i++;
    } while (j == 0);
    while (i > 0) {
      j = j * 16 + (long4)getnyb(fp);
      i--;
    };
    return (j - 15 + (13 - dyn_f) * 16 + dyn_f);
  } else if (i <= (int)dyn_f) {
    return ((long4)i);
  } else if (i < 14) {
    return ((i-(long4)dyn_f - 1) * 16 + (long4)getnyb(fp) + dyn_f + 1);
  } else {
    if (i == 14) {
      repeatcount = (int)pk_packed_num(fp);
    } else {
      repeatcount = 1;
    }
    /*     fprintf(ERR_STREAM,"repeatcount = [%d]\n",repeatcount);    */
    return (pk_packed_num(fp));    /* tail end recursion !! */
  }
}


/*-->LoadAChar*/
/**********************************************************************/
/***************************** LoadAChar ******************************/
/**********************************************************************/
void
#if NeedFunctionPrototypes
LoadAChar(long4 c, register struct char_entry *ptr)
#else
LoadAChar(c, ptr)
long4    c;
register struct char_entry *ptr;
#endif
{
  long  bytes;
  unsigned short   shrunk_width,shrunk_height;
  unsigned short   width,height;
  short   xoffset,yoffset;
  unsigned short   i_offset,j_offset;
  int   i,j,k,l,n;
  int   count=0;
  bool  paint_switch;

  if (ptr->fileOffset == NONEXISTANT) {
    ptr->isloaded = _FALSE;
    return;
  }

  OpenFontFile();

#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM, "LoadAChar: <%c>(%ld) from file at pos %ld\n",
            (char)c,(long)c,(long)ptr->fileOffset);
#endif

  FSEEK(pxlfp, ptr->fileOffset, SEEK_SET);

  if ((ptr->flag_byte & 7) == 7) n=4;
  else if ((ptr->flag_byte & 4) == 4) n=2;
  else n=1;

  dyn_f = ptr->flag_byte / 16;
  paint_switch = ((ptr->flag_byte & 8) != 0);

  /*
   *  Read character preamble
   */

  if (n != 4)     ptr->tfmw = NoSignExtend(pxlfp, 3);
  else {
    ptr->tfmw = NoSignExtend(pxlfp, 4);
    (void) NoSignExtend(pxlfp, 4); /* horizontal escapement not used */
  }
  (void) NoSignExtend(pxlfp, n); /* vertical escapement not used */

  width   = (unsigned short) NoSignExtend(pxlfp, n);
  height  = (unsigned short) NoSignExtend(pxlfp, n);

  if (width > 0x7fff || height > 0x7fff)
    Fatal("Character %d too large in file %s", c, fontptr->name);

  /* 
     Hotspot issues: Shrinking to the topleft corner rather than the
     hotspot will displace glyphs a fraction of a pixel. We deal with
     this in two ways:
     
     -Vertically, the glyph is shrunk to its hotspot by offsetting the
     bitmap somewhat to put the hotspot in the middle of a "shrink
     square". Shrinking to the topleft corner will then act as
     shrinking to the hotspot. This may enlarge the bitmap somewhat,
     of course.  (Also remember that the below calculation of j_offset
     is in integer arithmetics.) There will still be a displacement
     but it will be equal for all glyphs on a line, so we displace a
     whole line vertically by fractions of a pixel. This is
     acceptible, IMHO.

     -Horizontally, this may produce a crude result, since two
      adjacent glyphs may have different offsets, and get moved
      sideways by the shrinkage. Will do for now, I suppose.
   */
  
  xoffset = (short) SignExtend(pxlfp, n);
  i_offset = ( shrinkfactor - 
	       (xoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  width += i_offset;
  ptr->xOffset = xoffset+i_offset;

  yoffset = (short) SignExtend(pxlfp, n);
  j_offset = ( shrinkfactor - 
	       (yoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  height += j_offset;
  ptr->yOffset = yoffset+j_offset;

  ptr->tfmw = (long4)
    ( ptr->tfmw * (double)fontptr->s / (double)0x100000 );
  
  /* 
     Extra marginal so that we do not crop the image when shrinking.
  */

  shrunk_width = (width + shrinkfactor - 1) / shrinkfactor;
  shrunk_height = (height + shrinkfactor - 1) / shrinkfactor;

  /* 
     The glyph structure is a gdFont structure. Each glyph gets a font
     consisting of greyscales for the glyph. There is then enough
     space to hold the unshrunk glyph as well.
  */

  ptr->glyph.w = shrunk_width;
  ptr->glyph.h = shrunk_height;
  ptr->glyph.offset = 1;
  ptr->glyph.nchars = shrinkfactor*shrinkfactor;


  if ((ptr->glyph.data 
       = (char*) calloc(shrunk_width*shrunk_height*shrinkfactor*shrinkfactor,
			sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);

#ifdef DRAWGLYPH
  printf("Drawing character <%c>(%d), font %s\n",(char)c,(int)c,fontptr->name);
  printf("Size:%dx%d\n",width,height);
#endif

  /*
    Raster char
  */

  if (dyn_f == 14) {	/* get raster by bits */
    bitweight = 0;
    for (j = j_offset; j < (int) height; j++) {	/* get all rows */
      for (i = i_offset; i < (int) width; i++) {    /* get one row */
	bitweight /= 2;
	if (bitweight == 0) {
	  count = NoSignExtend(pxlfp,1);
	  bitweight = 128;
	}
	if (count & bitweight) {
	  ptr->glyph.data[i+j*width]++; 
#ifdef DRAWGLYPH
	  printf("*");
	} else {
	  printf(" ");
#endif
	}
      }
#ifdef DRAWGLYPH
      printf("\n");
#endif
    }
  } else {		/* get packed raster */
    bitweight = 0;
    repeatcount = 0;
    for(i=i_offset, j=j_offset; j<height; ) {
      count = pk_packed_num(pxlfp);
      while (count > 0) {
	if (i+count < width) {
	  if (paint_switch) {
	    for(k=0;k<count;k++)
	      ptr->glyph.data[k+i+j*width]++;
#ifdef DRAWGLYPH
	    for(k=0;k<count;k++) printf("*");
	  } else {
	    for(k=0;k<count;k++) printf(" ");
#endif
	  }
	  i += count;
	  count = 0;
	} else {
	  if (paint_switch) {
	    for(k=i;k<width;k++) 
	      ptr->glyph.data[k+j*width]++;
#ifdef DRAWGLYPH
	    for(k=i;k<width;k++) printf("*");
	    printf("\n");
	  } else {
	    for(k=i;k<width;k++) printf(" ");
	    printf("\n");
#endif
	  }
	  j++;
	  count -= width-i;
	  /* Repeat row(s) */
	  for (;repeatcount>0; repeatcount--,j++) {
	    for (i = i_offset; i<width; i++) {
	      ptr->glyph.data[i+j*width]=
		ptr->glyph.data[i+(j-1)*width];
#ifdef DRAWGLYPH
	      if (ptr->glyph.data[i+j*width]>0) 
		printf("*");
	      else
		printf(" ");
#endif
	    }
#ifdef DRAWGLYPH
	    printf("\n");
#endif
	  }
	  i=i_offset;
	}
      }
      paint_switch = 1 - paint_switch;
    }
    if (i>i_offset)
      Fatal("Wrong number of bits stored:  char. <%c>(%d), font %s, Dyn: %d", 
	    (char)c, (int)c, fontptr->name,dyn_f);
    if (j>height)
      Warning("Bad pk file (%s), too many bits", fontptr->name);
  }

  /*
    Shrink raster while doing antialiasing. (Somewhat crude. Glyphs
    get moved around by this. The single-glyph output seems better
    than what xdvi produces.)
  */

  for (j = 0; j < (int) height; j++) {	
    for (i = 0; i < (int) width; i++) {    
      if (((i % shrinkfactor) == 0) && ((j % shrinkfactor) == 0))
	ptr->glyph.data[i/shrinkfactor+j/shrinkfactor*shrunk_width] =
	  ptr->glyph.data[i+j*width];
      else 
	ptr->glyph.data[i/shrinkfactor+j/shrinkfactor*shrunk_width] +=
	  ptr->glyph.data[i+j*width];
    }
  }	

#ifdef DRAWGLYPH
  for (j = 0; j < shrunk_height; j++) {	
    for (i = 0; i < shrunk_width; i++) {    
      printf("%d",ptr->glyph.data[i+j*shrunk_width]);
    }
    printf("\n");
  }	 
#endif

  /*
    Separate the different greyscales with the darkest
    last. glyph.data will point to the lightest greyscale, but the
    drawing routine will deal with this.
  */

  for (j = 0; j < shrunk_height; j++) {	
    for (i = 0; i < shrunk_width; i++) {    
      for (k = shrinkfactor*shrinkfactor; k>0; k--) {
	ptr->glyph.data[i+j*shrunk_width + 
			(k-1)*shrunk_width*shrunk_height] = 
	  (ptr->glyph.data[i+j*shrunk_width] == k) ? 1 : 0;
      }
    }
  }	

#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM,
	    "Allocating Char <%c>, FileOffset=%lX, Bytes=%ld (%d) <%d>\n",
	    (char)c, ptr->fileOffset, bytes,
	    (int)bytes, (unsigned int)bytes);
#endif
  ptr->isloaded = _TRUE;
}

