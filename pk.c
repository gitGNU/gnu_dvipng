#include "dvipng.h"
#include <sys/mman.h>
#include <sys/stat.h>

#define PK_POST 245
#define PK_PRE 247
#define PK_ID 89

unsigned char   dyn_f;
int             repeatcount;
int             poshalf;

void    LoadAChar(int32_t, register struct pk_char *);

int32_t SetPK(int32_t c, int32_t h,int32_t v)
{
  register struct pk_char *ptr = currentfont->chr[c];
                                      /* temporary pk_char pointer */
  int red,green,blue;
  int Color,i;

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
		PIXROUND(h, dvi->conv*shrinkfactor)
		-PIXROUND(ptr->xOffset,shrinkfactor)
		+x_offset,
		PIXROUND(v, dvi->conv*shrinkfactor)
		-PIXROUND(ptr->yOffset,shrinkfactor)
		+y_offset,
		i,Color);
  }
  return(ptr->tfmw);
}


unsigned char getnyb(unsigned char** pos)
{
  if (poshalf == 0) {
    poshalf=1;
    return(**pos / 16);
  } else {
    poshalf=0;
    return(*(*pos)++ & 15);
  }
}

uint32_t pk_packed_num(unsigned char** pos)
{
  register int    i;
  uint32_t        j;

  i = (int)getnyb(pos);
  if (i == 0) {
    do {
      j = (uint32_t)getnyb(pos);
      i++;
    } while (j == 0);
    while (i > 0) {
      j = j * 16 + (uint32_t)getnyb(pos);
      i--;
    };
    return (j - 15 + (13 - dyn_f) * 16 + dyn_f);
  } else if (i <= (int)dyn_f) {
    return ((uint32_t)i);
  } else if (i < 14) {
    return ((i-(uint32_t)dyn_f - 1) * 16 + (uint32_t)getnyb(pos) 
	    + dyn_f + 1);
  } else {
    if (i == 14) {
      repeatcount = (int)pk_packed_num(pos);
    } else {
      repeatcount = 1;
    }
    return (pk_packed_num(pos));    /* tail end recursion !! */
  }
}

unsigned char* skip_specials(unsigned char* pos)
{
  uint32_t    i;

  while (*pos >= 240 && *pos != PK_POST) {
    i=0;
    switch (*pos++) {
    case 243:
      i = *pos++;
    case 242: 
      i = 256 * i + *pos++;
    case 241:
      i = 256 * i + *pos++;
    case 240:
      i = 256 * i + *pos++;
      DEBUG_PRINT((DEBUG_PK,"\n  PK SPECIAL\t'%.*s' ",i,pos));
      pos += i;
      break;
    case 244: 
#ifdef DEBUG
      { 
	uint32_t c;
	c=UNumRead(pos,4);
	DEBUG_PRINT((DEBUG_PK,"\n  PK SPECIAL\t%d",c));
      }
#endif
      pos += 4;
      break;
    case 245: 
      break;
    case 246:
      DEBUG_PRINT((DEBUG_PK,"\n  PK\tNOP "));
      break;
    case 247: case 248: case 249: case 250:
    case 251: case 252: case 253: case 254:
    case 255: 
      Fatal("Unexpected flagbyte %d!\n", (int)*pos);
    }
  }
  return(pos);
}

/*-->LoadAChar*/
/**********************************************************************/
/***************************** LoadAChar ******************************/
/**********************************************************************/
void LoadAChar(int32_t c, register struct pk_char * ptr)
{
  unsigned short   shrunk_width,shrunk_height;
  unsigned short   width,height;
  short   xoffset,yoffset;
  unsigned short   i_offset,j_offset;
  int   i,j,k,n;
  int   count=0;
  bool  paint_switch;
  unsigned char*   pos;

  DEBUG_PRINT((DEBUG_PK,"\n  LOAD PK CHAR\t%d",c));
  pos=ptr->mmap;
  if ((ptr->flag_byte & 7) == 7) n=4;
  else if ((ptr->flag_byte & 4) == 4) n=2;
  else n=1;
  dyn_f = ptr->flag_byte / 16;
  paint_switch = ((ptr->flag_byte & 8) != 0);
  /*
   *  Read character preamble
   */
  if (n != 4) {
    ptr->tfmw = UNumRead(pos, 3);
    /* +n:   vertical escapement not used */
    pos+=3+n;
  } else {
    ptr->tfmw = UNumRead(pos, 4);
    /* +4:  horizontal escapement not used */
    /* +n:   vertical escapement not used */
    pos+=8+n;
  }
  DEBUG_PRINT((DEBUG_PK," %d",ptr->tfmw));
  ptr->tfmw = (int32_t)
    ((int64_t) ptr->tfmw * currentfont->s / 0x100000 );
  DEBUG_PRINT((DEBUG_PK," (%d)",ptr->tfmw));
  
  width   = UNumRead(pos, n);
  height  = UNumRead(pos+=n, n);
  DEBUG_PRINT((DEBUG_PK," %dx%d",width,height));

  if (width > 0x7fff || height > 0x7fff)
    Fatal("Character %d too large in file %s", c, currentfont->name);

  /* 
   * Hotspot issues: Shrinking to the topleft corner rather than the
     hotspot will displace glyphs a fraction of a pixel. We deal with
     this in as follows: The glyph is shrunk to its hotspot by
     offsetting the bitmap somewhat to put the hotspot in the middle
     of a "shrink square". Shrinking to the topleft corner will then
     act as shrinking to the hotspot. This may enlarge the bitmap
     somewhat, of course.  (Also remember that the below calculation
     of i/j_offset is in integer arithmetics.)
     
     There will still be a displacement from rounding the dvi
     position, but vertically it will be equal for all glyphs on a
     line, so we displace a whole line vertically by fractions of a
     pixel. This is acceptible, IMHO.  Horizontally, this may produce
     a crude result, since two adjacent glyphs may be moved
     differently by rounding their dvi positions, and they get moved
     sideways by the shrinkage. Will do for now, I suppose.
   */
  xoffset = SNumRead(pos+=n, n);
  i_offset = ( shrinkfactor - 
	       (xoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  width += i_offset;
  ptr->xOffset = xoffset+i_offset;

  yoffset = SNumRead(pos+=n, n);
  j_offset = ( shrinkfactor - 
	       (yoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  height += j_offset;
  ptr->yOffset = yoffset+j_offset;
  DEBUG_PRINT((DEBUG_PK," (%dx%d)",width,height));
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
  pos+=n;
  if ((ptr->glyph.data 
       = (char*) calloc(shrunk_width*shrunk_height*shrinkfactor*shrinkfactor,
			sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);
  DEBUG_PRINT((DEBUG_GLYPH, "DRAW GLYPH %d\n", (int)c));
  /*
    Raster char
  */
  if (dyn_f == 14) {	/* get raster by bits */
    int bitweight = 0;
    for (j = j_offset; j < (int) height; j++) {	/* get all rows */
      for (i = i_offset; i < (int) width; i++) {    /* get one row */
	bitweight /= 2;
	if (bitweight == 0) {
	  count = *pos++;
	  bitweight = 128;
	}
	if (count & bitweight) {
	  ptr->glyph.data[i+j*width]++;
#ifdef DEBUG
	  DEBUG_PRINT((DEBUG_GLYPH, "*"));
	} else {
	  DEBUG_PRINT((DEBUG_GLYPH, " "));
#endif
	}
      }
      DEBUG_PRINT((DEBUG_GLYPH, "|\n"));
    }
  } else {		/* get packed raster */
    poshalf=0;
    repeatcount = 0;
    for(i=i_offset, j=j_offset; j<height; ) {
      count = pk_packed_num(&pos);
      while (count > 0) {
	if (i+count < width) {
	  if (paint_switch) 
	    for(k=0;k<count;k++) {
	      ptr->glyph.data[k+i+j*width]++;
	      DEBUG_PRINT((DEBUG_GLYPH,"*"));
	    }
#ifdef DEBUG
	  else for(k=0;k<count;k++) 
	    DEBUG_PRINT((DEBUG_GLYPH," "));
#endif
	  i += count;
	  count = 0;
	} else {
	  if (paint_switch) 
	    for(k=i;k<width;k++) {
	      ptr->glyph.data[k+j*width]++;
	      DEBUG_PRINT((DEBUG_GLYPH,"*"));
	    }
#ifdef DEBUG
	  else for(k=i;k<width;k++) 
	    DEBUG_PRINT((DEBUG_GLYPH," "));
#endif
	  DEBUG_PRINT((DEBUG_GLYPH,"|\n"));
	  j++;
	  count -= width-i;
	  /* Repeat row(s) */
	  for (;repeatcount>0; repeatcount--,j++) {
	    for (i = i_offset; i<width; i++) {
	      ptr->glyph.data[i+j*width]=
		ptr->glyph.data[i+(j-1)*width];
#ifdef DEBUG
	      if (ptr->glyph.data[i+j*width]>0) {
		DEBUG_PRINT((DEBUG_GLYPH,"*"));
	      } else {
		DEBUG_PRINT((DEBUG_GLYPH," "));
	      }
#endif
	    }
	    DEBUG_PRINT((DEBUG_GLYPH,"|\n"));
	  }
	  i=i_offset;
	}
      }
      paint_switch = 1 - paint_switch;
    }
    if (i>i_offset)
      Fatal("Wrong number of bits stored:  char. <%c>(%d), font %s, Dyn: %d", 
	    (char)c, (int)c, currentfont->name,dyn_f);
    if (j>height)
      Warning("Bad pk file (%s), too many bits", currentfont->name);
  }
  /*
    Shrink raster while doing antialiasing. (See above. The
    single-glyph output seems better than what xdvi at 300 dpi,
    shrinkfactor 3 produces.)
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
#ifdef DEBUG
  for (j = 0; j < shrunk_height; j++) {	
    for (i = 0; i < shrunk_width; i++) {    
      DEBUG_PRINT((DEBUG_GLYPH,"%d",ptr->glyph.data[i+j*shrunk_width]));
    }
    DEBUG_PRINT((DEBUG_GLYPH,"|\n"));
  }	 
#endif
  /*
    Separate the different greyscales with the darkest last.
    See SetChar in set.c
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
}

void InitPK(struct font_entry * tfontp)
{
  struct stat stat;
  unsigned char* position;
  struct pk_char *tcharptr; /* temporary pk_char pointer  */
  uint32_t    hppp, vppp, packet_length;
  uint32_t    c;

  DEBUG_PRINT(((DEBUG_DVI|DEBUG_PK),"\n  OPEN FONT:\t'%s'", tfontp->name));
  Message(BE_VERBOSE,"<%s>", tfontp->name);
  if ((tfontp->filedes = open(tfontp->name,O_RDONLY)) == -1) 
    Warning("font file %s could not be opened", tfontp->name);
  fstat(tfontp->filedes,&stat);
  tfontp->mmap = position = 
    mmap(NULL,stat.st_size, PROT_READ, MAP_SHARED,tfontp->filedes,0);
  if (tfontp->mmap == (unsigned char *)-1) 
    Fatal("cannot mmap PK file <%s> !\n",currentfont->name);
  if (*position++ != PK_PRE) 
    Fatal("unknown font format in file <%s> !\n",currentfont->name);
  if (*position++ != PK_ID) 
      Fatal( "wrong version of pk file!  (%d should be 89)\n",
	     (int)*(position-1));
  DEBUG_PRINT((DEBUG_PK,"\n  PK_PRE:\t'%.*s'",(int)*position, position+1));
  position += *position + 1;

  tfontp->designsize = UNumRead(position, 4);
  DEBUG_PRINT((DEBUG_PK," %d", tfontp->designsize));
  tfontp->type = FONT_TYPE_PK;
  
  c = UNumRead(position+4, 4);
  DEBUG_PRINT((DEBUG_PK," %d", c));
  CheckChecksum (tfontp->c, c, tfontp->name);

  hppp = UNumRead(position+8, 4);
  vppp = UNumRead(position+12, 4);
  DEBUG_PRINT((DEBUG_PK," %d %d", hppp,vppp));
  if (hppp != vppp)
    Warning("aspect ratio is %d:%d (should be 1:1)!", 
	    hppp, vppp);
  tfontp->magnification = (uint32_t)(hppp * 72.27 * 5 / 65536l + 0.5);
  position+=16;
  /* Read char definitions */
  position = skip_specials(position);
  while (*position != PK_POST) {
    DEBUG_PRINT((DEBUG_PK,"\n  @%ld PK CHAR:\t%d",
		  (long)(position - tfontp->mmap), *position));
    if ((tcharptr = malloc(sizeof(struct pk_char))) == NULL)
      Fatal("can't malloc space for pk_char");
    tcharptr->flag_byte = *position;
    tcharptr->glyph.data = NULL;
    tcharptr->tfmw = 0;
    if ((*position & 7) == 7) {
      packet_length = UNumRead(position+1,4);
      c = UNumRead(position+5, 4);
      position += 9;
    } else if (*position & 4) {
      packet_length = (*position & 3) * 65536l +
	UNumRead(position+1, 2);
      c = UNumRead(position+3, 1);
      position += 4;
    } else {
      packet_length = (*position & 3) * 256 +
	UNumRead(position+1, 1);
      c = UNumRead(position+2, 1);
      position += 3;
    }
  DEBUG_PRINT((DEBUG_PK," %d %d",packet_length,c));
  if (c > (LASTFNTCHAR))
    Fatal("Bad character (%d) in PK-File\n",(int)c);
  tcharptr->length = packet_length;
  tcharptr->mmap = position;
  tfontp->chr[c]=tcharptr;
  position += packet_length;
  position = skip_specials(position);
  }
}

