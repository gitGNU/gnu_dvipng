#include "dvipng.h"

/*-->OpenFont*/
/**********************************************************************/
/**************************** OpenFont  *******************************/
/**********************************************************************/
void OpenFont P1C(struct font_entry*, tfontp)
{
  if (tfontp->filep != FPNULL)
    return;         /* we need not have been called */

#ifdef DEBUG
  if (Debug)
    printf("OPEN FONT FILE %s\n", tfontp->name);
#endif

  if ((tfontp->filep = fopen(tfontp->name,"rb")) == FPNULL) {
    Warning("font file %s could not be opened", tfontp->name);
  } 
}


/*-->ReadFontDef*/
/**********************************************************************/
/****************************  ReadFontDef  ***************************/
/**********************************************************************/
/* Report a warning if both checksums are nonzero, they don't match, and
   the user hasn't turned it off.  */

void CheckChecksum P3C(unsigned, c1, unsigned, c2, const char*, name)
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


void FontDef P2C(unsigned char*, command, 
		 struct font_entry*, vfparent)
{
  long4 k;
  unsigned char* current;
  struct font_entry *tfontptr; /* temporary font_entry pointer   */
  struct font_num *tfontnump;  /* temporary font_num pointer   */
  long4   c, s, d;
  int     a, l;
  char n[STRSIZE];          /* FNT_DEF command parameters                */
  unsigned short i;

  current = command + 1;
  k = UNumRead(current, (int)*command - FNT_DEF1 + 1);
  current += (int)*command - FNT_DEF1 + 1;
  c = UNumRead(current, 4); /* checksum */
  s = UNumRead(current+4, 4); /* space size */
  d = UNumRead(current+8, 4); /* design size */
  /*  if (vfparent->d > 0) { */
     s = (long4)((long long) s * vfparent->d / 65536);
	/* 
	   According to some docs I read it should rather be 
	   d / (1 << 20) ? Whatever. Can anyone inform me how this is
	   _really_ calculated?  designsize / (1 << 20) seems to work
	   also, but I have a feeling the sizing should be guided by
	   the size in which the virtual font is actually used, not
	   the designsize.
	*/
     /*  }*/
  a = UNumRead(current+12, 1); /* length for font name */
  l = UNumRead(current+13, 1); /* device length */

  if (a+l > STRSIZE-1)
    Fatal("too long font name for font %ld\n",k);
  strncpy(n,current+14,a+l);
  n[a+l] = '\0';

  /* Find entry with this font number in use */
  tfontnump = vfparent->hfontnump;
  while (tfontnump != NULL && tfontnump->k != k) {
    tfontnump = tfontnump->next;
  }
  /* If found, return if it is correct */
  if (tfontnump!=NULL 
      && tfontnump->fontp->s == s 
      && tfontnump->fontp->d == d 
      && strcmp(tfontnump->fontp->n,n) == 0) {
    return;
  }
  /* If not found, create new */
  if (tfontnump==NULL) {
    if ((tfontnump=malloc(sizeof(struct font_num)))==NULL) 
      Fatal("cannot allocate memory for new font number");
    tfontnump->next=vfparent->hfontnump;
    tfontnump->k=k;
    vfparent->hfontnump=tfontnump;
  }

  if (Debug)
    printf("DEF FONT%ld: %s\n",k,n);

  /* Search font list for possible match */
  tfontptr = hfontptr;
  while (tfontptr != NULL 
	 && (tfontptr->s != s 
	     || tfontptr->d != d 
	     || strcmp(tfontptr->n,n) != 0 ) ) {
    tfontptr = tfontptr->next;
  }
  /* If found, set its number and return */
  if (tfontptr!=NULL) {
#ifdef DEBUG
    if (Debug)
      printf("FOUND FONT%ld in unused list\n",k);
#endif
    tfontnump->fontp = tfontptr; 
    return;
  }

  /* No fitting font found, create new entry. */
#ifdef DEBUG
  if (Debug)
    printf("Mallocating %d bytes...\n", 
	   sizeof(struct font_entry ));
#endif

  if ((tfontptr = NEW(struct font_entry )) == NULL)
    Fatal("can't malloc space for font_entry");

  tfontptr->next = hfontptr;
  hfontptr = tfontptr;
  tfontnump->fontp = tfontptr;
  tfontptr->filep = FPNULL;
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
  /*  if (vf_size > 0) {
    Fatal("FONT_MAG %d/%d=%d\n",tfontptr->s,tfontptr->d,tfontptr->font_mag);
    }*/
  tfontptr->name[0]='\0';
  for (i = FIRSTFNTCHAR; i <= LASTFNTCHAR; i++) {
    tfontptr->ch[i] = NULL;
  }
}

void FontFind P1C(struct font_entry *,tfontptr)
{
#ifdef KPATHSEA
  kpse_glyph_file_type font_ret;
  char *name;
  unsigned dpi
    = kpse_magstep_fix ((unsigned) (tfontptr->font_mag / 5.0 + .5),
			resolution, NULL);
  tfontptr->font_mag = dpi * 5; /* save correct dpi */

#ifdef DEBUG
  if (Debug)
    printf("FONT FIND: %s %d\n",tfontptr->n,dpi);
#endif

  name = kpse_find_vf (tfontptr->n);
  if (name) {
    strcpy (tfontptr->name, name);
    free (name);
    InitVF(tfontptr);
  } else {
    name = kpse_find_pk (tfontptr->n, dpi, &font_ret);
    if (name) {
      strcpy (tfontptr->name, name);
      free (name);
      
      if (!FILESTRCASEEQ (tfontptr->n, font_ret.name)) {
	Warning("font %s not found, using %s at %d instead.\n",
		tfontptr->n, font_ret.name, font_ret.dpi);
	tfontptr->c = 0; /* no checksum warning */
      } else if (!kpse_bitmap_tolerance ((double)font_ret.dpi, (double) dpi))
	Warning("font %s at %d not found, using %d instead.\n",
		tfontptr->name, dpi, font_ret.dpi);
      if (G_verbose)
	printf("Using font <%s>\n",tfontptr->name);
      
      InitPK(tfontptr);
    } else {
      Warning("font %s at %u not found, characters will be left blank.\n",
	      tfontptr->n, dpi);
      tfontptr->filep = NO_FILE;
      tfontptr->magnification = 0;
      tfontptr->designsize = 0;
    }
  }
#else /* not KPATHSEA */
      /* Ouch time! findfile not implemented (old cruft from dvilj) */
  /* Total argh, since none of this is adapted to vf and the like */
  if (!(findfile(PXLpath,
		 tfontptr->n,
		 tfontptr->font_mag,
		 tfontptr->name,
		 _FALSE,
		 0))) {
    Warning(tfontptr->name); /* contains error messsage */
    tfontptr->filep = NO_FILE;
#ifdef __riscos
    MakeMetafontFile(PXLpath, tfontptr->n, tfontptr->font_mag);
#endif
  } else {
    if (G_verbose)
      printf("Using font <%s>\n", tfontptr->name);
  }
#endif 
}



/*-->SetFntNum*/
/**********************************************************************/
/****************************  SetFntNum  *****************************/
/**********************************************************************/
void SetFntNum P1C(long4, k)
/*  this routine is used to specify the font to be used in printing future
    characters */
{
  struct font_num *tfontnump;  /* temporary font_num pointer   */

  tfontnump = vfstack[vfstackptr-1]->hfontnump;
  while (tfontnump != NULL && tfontnump->k != k)
    tfontnump = tfontnump->next;
  if (tfontnump == NULL)
    Fatal("font %ld undefined", (long)k);

  vfstack[vfstackptr] = tfontnump->fontp;
  if (vfstack[vfstackptr]->name[0]=='\0')
    FontFind(vfstack[vfstackptr]);
}


/*-->SkipFontDef*/
/**********************************************************************/
/****************************  SkipFontDef  ***************************/
/**********************************************************************/
void SkipFontDef P1H(void)
{
  int     a, l;
  char    n[STRSIZE];
  
  (void) NoSignExtend(dvi.filep, 4);
  (void) NoSignExtend(dvi.filep, 4);
  (void) NoSignExtend(dvi.filep, 4);
  a = (int)NoSignExtend(dvi.filep, 1);
  l = (int)NoSignExtend(dvi.filep, 1);
  GetBytes(dvi.filep, n, a + l);
}

    
