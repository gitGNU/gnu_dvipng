#include "dvipng.h"

/*-->OpenFontFile*/
/**********************************************************************/
/************************** OpenFontFile  *****************************/
/**********************************************************************/
void OpenFontFile P1H(void)
/***********************************************************************
     Taken from dvilj. Is this necessary in modern OS'es? What is
     MAXOPEN generally? We're aiming for deamon mode in the long run,
     so caching like this is perhaps not the way?
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
    */
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
unsigned char skip_specials P1H(void)
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
check_checksum P3C(unsigned, c1, unsigned, c2, const char*, name)
{
  if (c1 && c2 && c1 != c2
#ifdef KPATHSEA
      && !kpse_tex_hush ("checksum")
#endif
      ) {
     Warning ("Checksum mismatch in %s", name) ;
   }
}

double ActualFactor P1C(long4, unmodsize)
/* compute the actual size factor given the approximation */
/* actually factor * 1000 */
{
  double  realsize;     /* the actual magnification factor */
  realsize = (double)unmodsize / 1000.0;
  if (abs((int)(unmodsize - 1095l))<2)
    realsize = 1.095445115; /*stephalf*/
  else if (abs((int)(unmodsize - 1315l))<2)
    realsize = 1.31453414; /*stepihalf*/
  else if (abs((int)(unmodsize - 1577l))<2)
    realsize = 1.57744097; /*stepiihalf*/
  else if (abs((int)(unmodsize - 1893l))<2)
    realsize = 1.89292916; /*stepiiihalf*/
  else if (abs((int)(unmodsize - 2074l))<2)
    realsize = 2.0736;   /*stepiv*/
  else if (abs((int)(unmodsize - 2488l))<2)
    realsize = 2.48832;  /*stepv*/
  else if (abs((int)(unmodsize - 2986l))<2)
    realsize = 2.985984; /*stepvi*/
  /* the remaining magnification steps are represented with sufficient
     accuracy already */
  return(realsize);
}



void ReadFontDef P1C(long4, k)
{
  struct font_entry *tfontptr; /* temporary font_entry pointer   */
  long4   c, s, d;
  int     a, l;
  char n[STRSIZE];          /* FNT_DEF command parameters                */

  c = NoSignExtend(dvifp, 4); /* checksum */
  s = NoSignExtend(dvifp, 4); /* space size */
  d = NoSignExtend(dvifp, 4); /* design size */
  a = (int)NoSignExtend(dvifp, 1); /* length for font name */
  l = (int)NoSignExtend(dvifp, 1); /* device length */

  GetBytes(dvifp,n,a+l);
  n[a+l] = '\0';

  /* Find entry with this font number in use */
  tfontptr = hfontptr;
  while (tfontptr != NULL && (tfontptr->k != k || !tfontptr->in_use))
    tfontptr = tfontptr->next;

  /* If found, return if it is correct */
  if (tfontptr != NULL && tfontptr->s == s && tfontptr->d == d 
      && strcmp(tfontptr->n,n) == 0) {
    return;
  }

  /* otherwise mark it unused */
  if (tfontptr != NULL) {
    tfontptr->in_use=_FALSE;
#ifdef DEBUG
    if (Debug)
      printf("Font %ld moved to unused list\n",k);
#endif
  }

  /* Find font in unused list */
  tfontptr = hfontptr;
  while (tfontptr != NULL && ( tfontptr->in_use || tfontptr->s != s 
	   || tfontptr->d != d || strcmp(tfontptr->n,n) != 0 ) ) {
    tfontptr = tfontptr->next;
  }

  /* If found, set its number and return */
  if (tfontptr!=NULL) {
#ifdef DEBUG
    if (Debug)
      printf("Font %ld resurrected from unused list\n",k);
#endif
    tfontptr->k = k; 
    tfontptr->in_use=_TRUE;
    return;
  }

  /* No fitting font found, create new entry. */
#ifdef DEBUG
  if (Debug)
    printf("Mallocating %d Bytes)...\n", 
	   sizeof(struct font_entry ));
#endif

  if ((tfontptr = NEW(struct font_entry )) == NULL)
    Fatal("can't malloc space for font_entry");
  allocated_storage += sizeof(struct font_entry );

  tfontptr->next = hfontptr;
  tfontptr->font_file_id = FPNULL;
  hfontptr = tfontptr;
  tfontptr->k = k;
  tfontptr->in_use = _TRUE;
  tfontptr->c = c; /* checksum */
  tfontptr->s = s; /* space size */
  tfontptr->d = d; /* design size */
  tfontptr->a = a; /* length for font name */
  tfontptr->l = l; /* device length */
  strcpy(tfontptr->n,n);
  
  tfontptr->font_mag = 
    (long4)((ActualFactor((long4)(1000.0*tfontptr->s
				  /(double)tfontptr->d+0.5))
	     * ActualFactor(mag) * resolution * 5.0) + 0.5);
  
  tfontptr->name[0]='\0';
}


void FontFind P1C(struct font_entry *,tfontptr)
{
  long4    t;
  unsigned short i;
  struct char_entry *tcharptr; /* temporary char_entry pointer  */
  static int      plusid = 0;
  bool font_found = _FALSE;

#ifdef KPATHSEA
    {
      kpse_glyph_file_type font_ret;
      char *name;
      unsigned dpi
        = kpse_magstep_fix ((unsigned) (tfontptr->font_mag / 5.0 + .5),
                            resolution, NULL);
      tfontptr->font_mag = dpi * 5; /* save correct dpi */
      
      name = kpse_find_pk (tfontptr->n, dpi, &font_ret);
      if (name) {
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
      } else {
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

  /* sprintf(tfontptr->psname,"%s.%ld.%d",
       tfontptr->n, (long)tfontptr->font_mag, (long)tfontptr->plusid);*/

  /*  if (tfontptr != pfontptr) {*/
    if (font_found)
      OpenFontFile();
    else 
      pxlfp = NO_FILE;
    /*  }*/
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
    /*    unsigned char   temp_byte;*/
    register unsigned char  flag_byte;
    long4    hppp, vppp, packet_length;
    int     car, ii;

    /* read comment */
    for ( ii /*= temp_byte*/ = (unsigned char)NoSignExtend(pxlfp, 1);
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
      tcharptr->isloaded = _FALSE;
      FSEEK(pxlfp, (long)packet_length, SEEK_CUR);
      flag_byte = skip_specials();
    } /* end of while */
    /*tfontptr->max_height = max_depth ? tfontptr->max_yoff+max_depth :
      tfontptr->max_yoff+1;*/

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
void SetFntNum P1C(long4, k)
/*  this routine is used to specify the font to be used in printing future
    characters */
{
  fontptr = hfontptr;
  while ((fontptr != NULL) 
	 && (fontptr->k != k || fontptr->in_use == _FALSE))
    fontptr = fontptr->next;
  if (fontptr == NULL)
    Fatal("font %ld undefined", (long)k);

  if (fontptr->name[0]=='\0')
    FontFind(fontptr);
}


/*-->SkipFontDef*/
/**********************************************************************/
/****************************  SkipFontDef  ***************************/
/**********************************************************************/
void SkipFontDef P1H(void)
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

    
