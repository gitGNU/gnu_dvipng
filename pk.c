#include "dvipng.h"

/*
#define DRAWGLYPH
*/
#define PK_POST 245
#define PK_PRE 247
#define PK_ID 89

unsigned char   bitweight, dyn_f;
int             repeatcount;

unsigned char getnyb P1C(FILE*, fp)
{
  register unsigned char  temp;
  static unsigned char    inputbyte;
  if ( bitweight == 0 ) {
    inputbyte = fgetc(fp);
    bitweight = 16;
  }
  temp = inputbyte / bitweight;
  inputbyte -= temp * bitweight;
  bitweight /= 16;
  return(temp);
}


uint32_t pk_packed_num P1C(FILE*, fp)
{ /*@<Packed number procedure@>= */
  register int    i;
  uint32_t        j;

  i = (int)getnyb(fp);
  if (i == 0) {
    do {
      j = (uint32_t)getnyb(fp);
      i++;
    } while (j == 0);
    while (i > 0) {
      j = j * 16 + (uint32_t)getnyb(fp);
      i--;
    };
    return (j - 15 + (13 - dyn_f) * 16 + dyn_f);
  } else if (i <= (int)dyn_f) {
    return ((uint32_t)i);
  } else if (i < 14) {
    return ((i-(uint32_t)dyn_f - 1) * 16 + (uint32_t)getnyb(fp) + dyn_f + 1);
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

unsigned char skip_specials P1H(void)
{
  uint32_t    i, j;
  register unsigned char  flag_byte;
  do {
    flag_byte = fgetc(vfstack[vfstackptr]->filep);

    if (flag_byte  >= 240)
      switch (flag_byte) {
      case 240:
      case 241:
      case 242:
      case 243 : {
        i = 0;
        for (j = 240; j <= (uint32_t)flag_byte; j++) {
          i = 256 * i + fgetc(vfstack[vfstackptr]->filep);
        }
        for (j = 1; j <= i; j++) {
          (void) fgetc(vfstack[vfstackptr]->filep);
        }
        break;
      }
      case 244 : {
        fseek(vfstack[vfstackptr]->filep, 4, SEEK_CUR);
        break;
      }
      case 245 :
        break;
      case 246 :
        break;
      case 247:
      case 248:
      case 249:
      case 250:
      case 251:
      case 252:
      case 253:
      case 254:
      case 255: {
        Fatal("Unexpected flagbyte %d!\n", (int)flag_byte);
      }
      }
  } while (!((flag_byte < 240) || (flag_byte == PK_POST)));
  return(flag_byte);
}

/*-->LoadAChar*/
/**********************************************************************/
/***************************** LoadAChar ******************************/
/**********************************************************************/
void LoadAChar P2C(int32_t, c, register struct char_entry *, ptr)
{
  unsigned short   shrunk_width,shrunk_height;
  unsigned short   width,height;
  short   xoffset,yoffset;
  unsigned short   i_offset,j_offset;
  int   i,j,k,n;
  int   count=0;
  bool  paint_switch;
  unsigned char    buffer[28];

  if (ptr->fileOffset == NONEXISTANT) {
    ptr->isloaded = _FALSE;
    return;
  }

  OpenFont(vfstack[vfstackptr]);

#ifdef DEBUG
  if (Debug)
    printf("(LOAD %d pos %d) ",c ,ptr->fileOffset);
#endif

  FSEEK(vfstack[vfstackptr]->filep, ptr->fileOffset, SEEK_SET);

  if ((ptr->flag_byte & 7) == 7) n=4;
  else if ((ptr->flag_byte & 4) == 4) n=2;
  else n=1;

  dyn_f = ptr->flag_byte / 16;
  paint_switch = ((ptr->flag_byte & 8) != 0);

  /*
   *  Read character preamble
   */

  i= (n!=4) ? 3 : 8;
  fread(buffer, 1, i+5*n, vfstack[vfstackptr]->filep);

  if (n != 4) {
    ptr->tfmw = UNumRead(buffer, 3);
  } else {
    ptr->tfmw = UNumRead(buffer, 4);
    /* +4:  horizontal escapement not used */
  }
  /* +n:   vertical escapement not used */

  width   = UNumRead(buffer+i+n, n);
  height  = UNumRead(buffer+i+2*n, n);

  if (width > 0x7fff || height > 0x7fff)
    Fatal("Character %d too large in file %s", c, vfstack[vfstackptr]->name);

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
  
  xoffset = SNumRead(buffer+i+3*n, n);
  i_offset = ( shrinkfactor - 
	       (xoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  width += i_offset;
  ptr->xOffset = xoffset+i_offset;

  yoffset = SNumRead(buffer+i+4*n, n);
  j_offset = ( shrinkfactor - 
	       (yoffset-shrinkfactor/2) % shrinkfactor ) % shrinkfactor;
  height += j_offset;
  ptr->yOffset = yoffset+j_offset;

  ptr->tfmw = (int32_t)
    ( ptr->tfmw * (double)vfstack[vfstackptr]->s / (double)0x100000 );
  
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
  /*
  printf("PK: %s (%d) %dx%d\n",vfstack[vfstackptr]->name,
         c,ptr->glyph.w,ptr->glyph.h);
  */
  if ((ptr->glyph.data 
       = (char*) calloc(shrunk_width*shrunk_height*shrinkfactor*shrinkfactor,
			sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);

#ifdef DRAWGLYPH
  printf("Drawing character <%c>(%d), font %s\n",
	 (char)c,(int)c,vfstack[vfstackptr]->name);
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
	  count = fgetc(vfstack[vfstackptr]->filep);
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
      count = pk_packed_num(vfstack[vfstackptr]->filep);
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
	    (char)c, (int)c, vfstack[vfstackptr]->name,dyn_f);
    if (j>height)
      Warning("Bad pk file (%s), too many bits", vfstack[vfstackptr]->name);
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

#ifdef DRAWGLYPH
  for (j = 0; j < shrunk_height; j++) {	
    for (i = 0; i < shrunk_width; i++) {    
      printf("%d",ptr->glyph.data[i+j*shrunk_width]);
    }
    printf("\n");
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

  ptr->isloaded = _TRUE;
}

void InitPK  P1C(struct font_entry *,tfontp)
{
  int32_t    t;
  struct char_entry *tcharptr; /* temporary char_entry pointer  */
  char temp[16];

  OpenFont(tfontp);
  t = fgetc(tfontp->filep);
  if (t != PK_PRE) 
    Fatal("unknown font format in file <%s> !\n",vfstack[vfstackptr]->name);

  t = fgetc(tfontp->filep);
  if (t != PK_ID) 
    Fatal( "Wrong Version of pk file!  (%d should be 89)\n",t);

  { /* PK 89 format */
    register unsigned char  flag_byte;
    uint32_t    hppp, vppp, packet_length;
    int     car;

    /* skip comment */
    fseek(tfontp->filep,fgetc(tfontp->filep), SEEK_CUR);

    fread(temp,1,16,tfontp->filep);

    tfontp->designsize = UNumRead(temp, 4);
    tfontp->is_vf = _FALSE;
    t = UNumRead(temp+4, 4);
    CheckChecksum (tfontp->c, t, tfontp->name);

    hppp = UNumRead(temp+8, 4);
    vppp = UNumRead(temp+12, 4);
    if (hppp != vppp)
      Warning("aspect ratio is %d:%d (should be 1:1)!", 
              hppp, vppp);
    tfontp->magnification = (uint32_t)(hppp * 72.27 * 5 / 65536l + 0.5);

    flag_byte = skip_specials();

    while (flag_byte != PK_POST) {
      if ((flag_byte & 7) == 7) {
        fread(temp,1,8,tfontp->filep);
	packet_length = UNumRead(temp,4);
        car = UNumRead(temp+4, 4);
      } else if (flag_byte & 4) {
	fread(temp,1,3,tfontp->filep);
	packet_length = ((uint32_t)flag_byte & 3) * 65536l +
          (unsigned short) UNumRead(temp, 2);
        car = UNumRead(temp+2, 1);
      } else {
	fread(temp,1,2,tfontp->filep);
        packet_length = ((uint32_t)flag_byte & 3) * 256 +
          UNumRead(temp, 1);
        car = UNumRead(temp+1, 1);
      }
      if (car > (LASTFNTCHAR))
          Fatal("Bad character (%d) in PK-File\n",(int)car);
      if ((tcharptr = tfontp->ch[car] = NEW(struct char_entry )) == NULL)
	Fatal("can't malloc space for char_entry");
      tcharptr->fileOffset = FTELL(tfontp->filep);
      tcharptr->flag_byte = flag_byte;
      tcharptr->isloaded = _FALSE;
      fseek(tfontp->filep, (long)packet_length, SEEK_CUR);
      flag_byte = skip_specials();
    } /* end of while */
    /*tfontp->max_height = max_depth ? tfontp->max_yoff+max_depth :
      tfontp->max_yoff+1;*/
    /*
      printf("fontid=%d: max_width=%u, max_height=%d, max_yoff=%u\n",
        tfontp->plusid, tfontp->max_width,
        tfontp->max_height, tfontp->max_yoff);
        */
  }
}
