#include "dvipng.h"
/*-->GetFontDef*/
/**********************************************************************/
/**************************** GetFontDef  *****************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void GetFontDef(void)
#else
void GetFontDef()
#endif
/***********************************************************************
   Read the font  definitions as they  are in the  postamble of the  DVI
   file.
***********************************************************************/
{
  unsigned char   byte;
  while (((byte = (unsigned char) NoSignExtend(dvifp, 1)) >= FNT_DEF1) &&
         (byte <= FNT_DEF4)) {

    switch (byte) {
    case FNT_DEF1:
      ReadFontDef( NoSignExtend(dvifp, 1));
      break;
    case FNT_DEF2:
      ReadFontDef( NoSignExtend(dvifp, 2));
      break;
    case FNT_DEF3:
      ReadFontDef( NoSignExtend(dvifp, 3));
      break;
    case FNT_DEF4:
      ReadFontDef( NoSignExtend(dvifp, 4));
      break;
    default:
      Fatal("Bad byte value in font defs");
      break;
    }

  }
  if (byte != POST_POST)
    Fatal("POST_POST missing after fontdefs");
}




/*-->OpenFontFile*/
/**********************************************************************/
/************************** OpenFontFile  *****************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void OpenFontFile(void)
#else
void OpenFontFile()
#endif
/***********************************************************************
    The original version of this dvi driver reopened the font file  each
    time the font changed, resulting in an enormous number of relatively
    expensive file  openings.   This version  keeps  a cache  of  up  to
    MAXOPEN open files,  so that when  a font change  is made, the  file
    pointer, pxlfp, can  usually be  updated from the  cache.  When  the
    file is not found in  the cache, it must  be opened.  In this  case,
    the next empty slot  in the cache  is assigned, or  if the cache  is
    full, the least used font file is closed and its slot reassigned for
    the new file.  Identification of the least used file is based on the
    counts of the number  of times each file  has been "opened" by  this
    routine.  On return, the file pointer is always repositioned to  the
    beginning of the file.
***********************************************************************/

#if MAXOPEN > 1

{
  int     i, least_used, current;
  struct pixel_list tmp;
  FILEPTR fid;
  struct font_entry *fp;

#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM,"open font file %s\n", fontptr->name);
#endif
  /*
    fprintf(ERR_STREAM,"? %lx == %lx\n", pfontptr,fontptr);
    */
  if ((pfontptr == fontptr) && (pxlfp != NO_FILE))
    return;         /* we need not have been called */

  if (fontptr->font_file_id == NO_FILE)
    return;         /* we need not have been called */

  tmp = pixel_files[1];
  current = 1;
  while (current <= nopen && tmp.pixel_file_id != fontptr->font_file_id) {
    ++current;
    tmp = pixel_files[current];
  }
  /* try to find file in open list */ 

  if (current <= nopen)       /* file already open */ {
    if ( pixel_files[current].pixel_file_id != NO_FILE ) {
      pxlfp = pixel_files[current].pixel_file_id;
      /* reposition to start of file */
      FSEEK(pxlfp, 0l, SEEK_SET);
    }
  } else {
    /*
    fprintf(ERR_STREAM,"Really open font file %s\n", fontptr->name);
    /* file not in open list          */
    if (nopen < MAXOPEN)    /* just add it to list    */
      current = ++nopen;
    else  {
      /* list full -- find least used file,     */
      /* close it, and reuse slot for new file  */
      least_used = 1;
      for (i = 2; i <= MAXOPEN; ++i)
        if (pixel_files[least_used].use_count > pixel_files[i].use_count)
          least_used = i;
      if ((fid = pixel_files[least_used].pixel_file_id) != NO_FILE) {
        /* mark file as being closed in the entry */
        fp = hfontptr;
        while (fp != NULL && fp->font_file_id != fid) 
          fp = fp->next;
        if (fp == NULL)
          Fatal("Open file %x not found in font entry list.\n", fid);
        else {
          fp->font_file_id = FPNULL;
        }
        BCLOSE( fid );
      }
#ifdef DEBUG
      if (Debug)
        fprintf(ERR_STREAM,"\n__reuse slot %d\n", least_used);
#endif
      current = least_used;
    }
    if ((pxlfp = BINOPEN(fontptr->name)) == FPNULL) {
      Warning("PXL-file %s could not be opened", fontptr->name);
      pxlfp = NO_FILE;
    } else {
#ifdef DEBUG
      if (Debug)
        fprintf(ERR_STREAM,"Opening File  <%s> /%p/, Size(font_entry)=%d\n",
                fontptr->name, pxlfp, sizeof(struct font_entry ));
#endif

    }
    pixel_files[current].pixel_file_id = pxlfp;
    pixel_files[current].use_count = 0;
  }
  pfontptr = fontptr;         /* make previous = current font */
  fontptr->font_file_id = pxlfp;      /* set file identifier */
  pixel_files[current].use_count++;   /* update reference count */
}

#else /* ! MAXOPEN > 1 */

{
  FILEPTR f;
  struct font_entry *fp;

  if (pfontptr == fontptr && pxlfp != NO_FILE)
    return;         /* we need not have been called */
  if (fontptr->font_file_id == NO_FILE)
    return;         /* we need not have been called */

  f = pfontptr->font_file_id;
  if (f != FPNULL) {
    if (pxlfp != FPNULL) {
      fp = hfontptr;
      while ((fp != NULL) && (fp->font_file_id != f))
        fp = fp->next;

      if (fp == NULL) 
        Fatal("Open file %x not found in font entry list.\n",f);
      else 
        fp->font_file_id = FPNULL;
    }
    BCLOSE(f);
  }
  if ((pxlfp = BINOPEN(fontptr->name)) == FPNULL) {
    Warning("PXL-file %s could not be opened", fontptr->name);
    pxlfp = NO_FILE;
  }
  pfontptr = fontptr;
  fontptr->font_file_id = pxlfp;
}

#endif

/*-->ReadFontDef*/
/**********************************************************************/
/****************************  ReadFontDef  ***************************/
/**********************************************************************/

#if NeedFunctionPrototypes
unsigned char skip_specials(void)
#else
unsigned char skip_specials()
#endif
{
  long4    i, j;
  register unsigned char  flag_byte;
  do {
    flag_byte = (unsigned char) NoSignExtend(pxlfp, 1);

    if (flag_byte  >= 240)
      switch (flag_byte) {
      case 240:
      case 241:
      case 242:
      case 243 : {
        i = 0;
        for (j = 240; j <= (long4)flag_byte; j++) {
          i = 256 * i + NoSignExtend(pxlfp, 1);
        }
        for (j = 1; j <= i; j++) {
          (void) NoSignExtend(pxlfp, 1);
        }
        break;
      }
      case 244 : {
        i = NoSignExtend(pxlfp, 4);
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

/* Report a warning if both checksums are nonzero, they don't match, and
   the user hasn't turned it off.  */

static void
check_checksum (c1, c2, name)
    unsigned c1, c2;
    const char *name;
{
  if (c1 && c2 && c1 != c2
#ifdef KPATHSEA
      && !kpse_tex_hush ("checksum")
#endif
      ) {
     Warning ("Checksum mismatch in %s", name) ;
   }
}



#if NeedFunctionPrototypes
void ReadFontDef(long4 k)
#else
void ReadFontDef(k)
long4    k;
#endif
{
  long4    t;
  unsigned short i;
  struct font_entry *tfontptr; /* temporary font_entry pointer   */
  struct char_entry *tcharptr; /* temporary char_entry pointer  */
  static int      plusid = 0;
  bool font_found = _FALSE;
  int depth, max_depth;

#ifdef DEBUG
  if (Debug)
    fprintf(ERR_STREAM,"Mallocating %d Bytes)...\n", sizeof(struct font_entry ));
#endif

  if ((tfontptr = NEW(struct font_entry )) == NULL)
    Fatal("can't malloc space for font_entry");

  allocated_storage += sizeof(struct font_entry );

  tfontptr->next = hfontptr;
  tfontptr->font_file_id = FPNULL;
  fontptr = hfontptr = tfontptr;
  tfontptr->ncdl = 0;
  tfontptr->k = k;
  tfontptr->c = NoSignExtend(dvifp, 4); /* checksum */
  tfontptr->s = NoSignExtend(dvifp, 4); /* space size */
  tfontptr->d = NoSignExtend(dvifp, 4); /* design size */
  tfontptr->a = (int)NoSignExtend(dvifp, 1); /* length for font name */
  tfontptr->l = (int)NoSignExtend(dvifp, 1); /* device length */

  tfontptr->max_width = tfontptr->max_height = tfontptr->max_yoff =
    max_depth = 0;

  GetBytes(dvifp, tfontptr->n, tfontptr->a + tfontptr->l);
  tfontptr->n[tfontptr->a+tfontptr->l] = '\0';

  tfontptr->font_mag = 
    (long4)((ActualFactor((long4)(1000.0*tfontptr->s/(double)tfontptr->d+0.5))
             * ActualFactor(mag) * resolution * 5.0) + 0.5);
  /*
printf("[%ld]=%lf * %lf * %lf + 0.5 = %ld\n",
    ((long)(1000.0*tfontptr->s/(double)tfontptr->d+0.5)),
    ActualFactor((long4)(1000.0*tfontptr->s/(double)tfontptr->d+0.5)),
    ActualFactor(mag),
    (double)resolution * 5,
    (long)tfontptr->font_mag );
*/

#ifdef KPATHSEA
    {
      kpse_glyph_file_type font_ret;
      char *name;
      unsigned dpi
        = kpse_magstep_fix ((unsigned) (tfontptr->font_mag / 5.0 + .5),
                            resolution, NULL);
      tfontptr->font_mag = dpi * 5; /* save correct dpi */
      
      name = kpse_find_pk (tfontptr->n, dpi, &font_ret);
      if (name)
        {
          font_found = _TRUE;
          strcpy (tfontptr->name, name);
          free (name);
          
          if (!FILESTRCASEEQ (tfontptr->n, font_ret.name)) {
              fprintf (stderr,
                       "dvilj: Font %s not found, using %s at %d instead.\n",
                       tfontptr->n, font_ret.name, font_ret.dpi);
              tfontptr->c = 0; /* no checksum warning */
            }
          else if (!kpse_bitmap_tolerance ((double)font_ret.dpi, (double) dpi))
            fprintf (stderr,
                     "dvilj: Font %s at %d not found, using %d instead.\n",
                     tfontptr->name, dpi, font_ret.dpi);
          if (G_verbose)
            fprintf(stderr,"%d: using font <%s>\n", plusid,tfontptr->name);
        }
      else
        {
          tfontptr->font_file_id = NO_FILE;
          fprintf (stderr,
            "dvilj: Font %s at %u not found, characters will be left blank.\n",
            tfontptr->n, dpi);
        }
    }
#else /* not KPATHSEA */
    if (!(findfile(PXLpath,
                   tfontptr->n,
                   tfontptr->font_mag,
                   tfontptr->name,
                   _FALSE,
                   0))) {
      Warning(tfontptr->name); /* contains error messsage */
      tfontptr->font_file_id = NO_FILE;
#ifdef __riscos
      MakeMetafontFile(PXLpath, tfontptr->n, tfontptr->font_mag);
#endif
    }
    else {
      font_found = _TRUE;
      if (G_verbose)
        fprintf(ERR_STREAM,"%d: using font <%s>\n", plusid, tfontptr->name);
    }
#endif /* not KPATHSEA */

  tfontptr->plusid = plusid;
  plusid++;

  /* sprintf(tfontptr->psname,"%s.%ld.%d",
       tfontptr->n, (long)tfontptr->font_mag, (long)tfontptr->plusid);*/

  if (tfontptr != pfontptr) {
    if (font_found)
      OpenFontFile();
    else
      pxlfp = NO_FILE;
  }
  if ( pxlfp == NO_FILE ) {        /* allow missing pxl files */
    tfontptr->magnification = 0;
    tfontptr->designsize = 0;
    for (i = FIRSTFNTCHAR; i <= LASTFNTCHAR; i++) {
      tcharptr = &(tfontptr->ch[i]);
      tcharptr->xOffset = 0;
      tcharptr->yOffset = 0;
      tcharptr->isloaded = _FALSE;
      tcharptr->fileOffset = NONEXISTANT;
      tcharptr->tfmw = 0;
    }
    return;
  }
  t = NoSignExtend(pxlfp, 1);
  if (t == PK_PRE) {
    unsigned char   temp_byte;
    temp_byte = (unsigned char) NoSignExtend(pxlfp, 1);
    if (temp_byte != PK_ID) 
      Fatal( "Wrong Version of pk file!  (%d should be 89)\n",
	     (int)temp_byte);
    else
      tfontptr->id = pk89;
  } else
    Fatal("unknown font format in file <%s> !\n",fontptr->name);

  { /* PK 89 format */
    unsigned char   temp_byte;
    register unsigned char  flag_byte;
    long4    hppp, vppp, packet_length;
    int     car, ii;

    /* read comment */
    for ( ii = temp_byte = (unsigned char)NoSignExtend(pxlfp, 1);
	  ii>0; ii--) {
      flag_byte = (unsigned char) NoSignExtend(pxlfp, 1);
#ifdef DEBUG
      if (Debug) fprintf(ERR_STREAM, "%c", flag_byte );
#endif
    }
#ifdef DEBUG
    if (Debug) fprintf(ERR_STREAM, "\n");
#endif
    tfontptr->designsize = NoSignExtend(pxlfp, 4);

    t = NoSignExtend(pxlfp, 4);
    check_checksum (tfontptr->c, t, tfontptr->name);

    hppp = NoSignExtend(pxlfp, 4);
    vppp = NoSignExtend(pxlfp, 4);
    if (hppp != vppp)
      Warning("aspect ratio is %ld:%ld (should be 1:1)!", 
              (long)hppp, (long)vppp);
    tfontptr->magnification = (long4)(hppp * 72.27 * 5 / 65536l + 0.5);

    flag_byte = skip_specials();

    while (flag_byte != PK_POST) {
      if ((flag_byte & 7) == 7) {
        packet_length = (unsigned long4)NoSignExtend(pxlfp,4);
        car = (int)NoSignExtend(pxlfp, 4);
      } else if (flag_byte & 4) {
        packet_length = ((long4)flag_byte & 3) * 65536l +
          (unsigned short) NoSignExtend(pxlfp, 2);
        car = (int)NoSignExtend(pxlfp, 1);
      } else {
        packet_length = ((long4)flag_byte & 3) * 256 +
          NoSignExtend(pxlfp, 1);
        car = (int)NoSignExtend(pxlfp, 1);
      }
      if (car > (LASTFNTCHAR))
          Fatal("Bad character (%d) in PK-File\n",(int)car);
      tcharptr = &(tfontptr->ch[car]);
      tcharptr->fileOffset = FTELL(pxlfp);
      tcharptr->flag_byte = flag_byte;
      FSEEK(pxlfp, (long)packet_length, SEEK_CUR);
      flag_byte = skip_specials();

    } /* end of while */
    tfontptr->max_height = max_depth ? tfontptr->max_yoff+max_depth :
      tfontptr->max_yoff+1;

    /*
printf("fontid=%d: max_width=%u, max_height=%d, max_yoff=%u\n",
        tfontptr->plusid, tfontptr->max_width,
        tfontptr->max_height, tfontptr->max_yoff);
        */

  }
}





/*-->SetFntNum*/
/**********************************************************************/
/****************************  SetFntNum  *****************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void SetFntNum(long4 k, bool Emitting)
#else
void SetFntNum(k, Emitting)
long4    k;
bool Emitting;
#endif
/*  this routine is used to specify the font to be used in printing future
    characters */
{
#ifdef LJ
  static unsigned short plusid = 0;
#endif
  fontptr = hfontptr;
  while ((fontptr != NULL) && (fontptr->k != k))
    fontptr = fontptr->next;
  if (fontptr == NULL)
    Fatal("font %ld undefined", (long)k);
  /*  if (Emitting && fontptr->font_file_id != NO_FILE) {
    if (!fontptr->used_on_this_page) {
      fontptr->used_on_this_page = _TRUE;
    }
#ifdef DEBUG
    if (Debug)
      fprintf(ERR_STREAM, "Switching to font #%ld (%s).\n", k, fontptr->n);
#endif
    /* activate font *
  }*/
#ifdef LJ    /* reassignment of printer font id  0.48 */
  else if (fontptr->font_file_id != NO_FILE) {
    if (fontptr->ncdl == 0) {
#ifdef DEBUG
      if (Debug)
        fprintf(ERR_STREAM, "Changing plusid from %d to %d\n",
                fontptr->plusid, (int)plusid);
#endif
      fontptr->plusid = plusid;
      plusid ++;
    }
  }
#endif
}



/*-->SkipFontDef*/
/**********************************************************************/
/****************************  SkipFontDef  ***************************/
/**********************************************************************/
#if NeedFunctionPrototypes
void SkipFontDef(void)
#else
void SkipFontDef()
#endif
{
  int     a, l;
  char    n[STRSIZE];
  
  (void) NoSignExtend(dvifp, 4);
  (void) NoSignExtend(dvifp, 4);
  (void) NoSignExtend(dvifp, 4);
  a = (int)NoSignExtend(dvifp, 1);
  l = (int)NoSignExtend(dvifp, 1);
  GetBytes(dvifp, n, a + l);
}

    
