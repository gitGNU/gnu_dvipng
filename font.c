#include "dvipng.h"

#if 0
/*-->OpenFont*/
/**********************************************************************/
/**************************** OpenFont  *******************************/
/**********************************************************************/
void OpenFont P1C(struct font_entry*, tfontp)
{
  if (tfontp->filep != NULL)
    return;         /* we need not have been called */
  DEBUG_PRINTF(DEBUG_DVI,"(OPEN %s) ", tfontp->name);
  if ((tfontp->filep = fopen(tfontp->name,"rb")) == NULL) {
    Warning("font file %s could not be opened", tfontp->name);
  } 
}
#endif


void CheckChecksum P3C(uint32_t, c1, uint32_t, c2, const char*, name)
{
  /* Report a warning if both checksums are nonzero, they don't match,
     and the user hasn't turned it off.  */
  if (c1 && c2 && c1 != c2
#ifdef KPATHSEA
      && !kpse_tex_hush ("checksum")
#endif
      ) {
     Warning ("Checksum mismatch in %s", name) ;
   }
}


double ActualFactor P1C(uint32_t, unmodsize)
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
		 struct dvi_vf_entry*, parent)
{
  int32_t k;
  uint32_t   c, s, d;
  uint8_t    a, l;
  unsigned char* current;
  struct font_entry *tfontptr; /* temporary font_entry pointer   */
  struct font_num *tfontnump = NULL;  /* temporary font_num pointer   */
  unsigned short i;

  current = command + 1;
  k = UNumRead(current, (int)*command - FNT_DEF1 + 1);
  DEBUG_PRINTF(DEBUG_DVI," %d",k)
  current += (int)*command - FNT_DEF1 + 1;
  c = UNumRead(current, 4); /* checksum */
  DEBUG_PRINTF(DEBUG_DVI," %d %d",c)
  s = UNumRead(current+4, 4); /* space size */
  DEBUG_PRINTF(DEBUG_DVI," %d",s)
  if (parent->type==FONT_TYPE_VF) {
    s = (uint32_t)((uint64_t) s * parent->d / 65536);
    DEBUG_PRINTF(DEBUG_DVI," (%d)",s)
    /* 
     * According to some docs I read it should be d / (1 << 20) ?
     * Whatever. Can anyone inform me how this is _really_ calculated?
     * designsize / (1 << 20) seems to work also, but I have a feeling
     * the sizing should be guided by the size in which the virtual
     * font is actually used, not the designsize.
    */
  }
  d = UNumRead(current+8, 4); /* design size */
  DEBUG_PRINTF(DEBUG_DVI," %d",d)
  a = UNumRead(current+12, 1); /* length for font name */
  l = UNumRead(current+13, 1); /* device length */
  DEBUG_PRINTF2(DEBUG_DVI," %d %d",a,l)
  DEBUG_PRINTF2(DEBUG_DVI," '%.*s'",a+l,current+14);
  if (a+l > STRSIZE-1)
    Fatal("too long font name for font %ld\n",k);

  /* Find entry with this font number in use */
  switch (parent->type) {
  case FONT_TYPE_VF:
    tfontnump = parent->vffontnump;
    break;
  case DVI_TYPE:
    tfontnump = parent->fontnump;
  }
  while (tfontnump != NULL && tfontnump->k != k) {
    tfontnump = tfontnump->next;
  }
  /* If found, return if it is correct */
  if (tfontnump!=NULL 
      && tfontnump->fontp->s == s 
      && tfontnump->fontp->d == d 
      && strncmp(tfontnump->fontp->n,current+14,a+l) == 0) {
    DEBUG_PRINTF(DEBUG_DVI,"\n  FONT %d:\tMatch found",k)
    return;
  }
  /* If not found, create new */
  if (tfontnump==NULL) {
    if ((tfontnump=malloc(sizeof(struct font_num)))==NULL) 
      Fatal("cannot allocate memory for new font number");
    tfontnump->k=k;
    switch (parent->type) {
    case FONT_TYPE_VF:
      tfontnump->next=parent->vffontnump;
      parent->vffontnump=tfontnump;
      break;
    case DVI_TYPE:
      tfontnump->next=parent->fontnump;
      parent->fontnump=tfontnump;
    }
  }

  /* Search font list for possible match */
  tfontptr = hfontptr;
  while (tfontptr != NULL 
	 && (tfontptr->s != s 
	     || tfontptr->d != d 
	     || strncmp(tfontptr->n,current+14,a+l) != 0 ) ) {
    tfontptr = tfontptr->next;
  }
  /* If found, set its number and return */
  if (tfontptr!=NULL) {
    DEBUG_PRINTF(DEBUG_DVI,"\n  FONT %d:\tFound in unused list",k);
    tfontnump->fontp = tfontptr; 
    return;
  }

  DEBUG_PRINTF(DEBUG_DVI,"\n  FONT %d:\tNew entry created",k);
  /* No fitting font found, create new entry. */
  if ((tfontptr = NEW(struct font_entry )) == NULL)
    Fatal("can't malloc space for font_entry");
  tfontptr->next = hfontptr;
  hfontptr = tfontptr;
  tfontnump->fontp = tfontptr;
  tfontptr->filedes = 0;
  tfontptr->c = c; /* checksum */
  tfontptr->s = s; /* space size */
  tfontptr->d = d; /* design size */
  tfontptr->a = a; /* length for font name */
  tfontptr->l = l; /* device length */
  strncpy(tfontptr->n,current+14,a+l); /* full font name */
  tfontptr->n[a+l] = '\0';
  
  tfontptr->name[0] = '\0';
  for (i = FIRSTFNTCHAR; i <= LASTFNTCHAR; i++) {
    tfontptr->pk_ch[i] = NULL;
  }

  tfontptr->font_mag = 
    (uint32_t)((ActualFactor((uint32_t)(1000.0*tfontptr->s
				  /(double)tfontptr->d+0.5))
	     * ActualFactor(dvi->mag) * resolution * 5.0) + 0.5);
}

void FontFind P1C(struct font_entry *,tfontptr)
{
#ifdef KPATHSEA
  kpse_glyph_file_type font_ret;
  char *name;
  unsigned dpi;  

  dpi = kpse_magstep_fix ((unsigned) (tfontptr->font_mag / 5.0 + .5),
			resolution, NULL);
  tfontptr->font_mag = dpi * 5; /* save correct dpi */
  DEBUG_PRINTF2(DEBUG_DVI,"\n  FIND FONT:\t%s %d",tfontptr->n,dpi);
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
      tfontptr->filedes = 0;
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
    tfontptr->filedes = 0;
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
void SetFntNum P2C(int32_t, k, struct dvi_vf_entry*, parent)
/*  this routine is used to specify the font to be used in printing future
    characters */
{
  struct font_num *tfontnump=NULL;  /* temporary font_num pointer   */

  switch (parent->type) {
  case FONT_TYPE_VF:
    tfontnump = parent->vffontnump;
    break;
  case DVI_TYPE:
    tfontnump = parent->fontnump;
  }
  while (tfontnump != NULL && tfontnump->k != k)
    tfontnump = tfontnump->next;
  if (tfontnump == NULL)
    Fatal("font %d undefined", k);

  currentfont = tfontnump->fontp;
  if (currentfont->name[0]=='\0')
    FontFind(currentfont);
}


    
