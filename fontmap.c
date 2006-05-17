/* fontmap.c */

/************************************************************************

  Part of the dvipng distribution

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  Copyright (C) 2002-2005 Jan-Åke Larsson

************************************************************************/

#include "dvipng.h"

static char* psfont_name=NULL;
static struct filemmap psfont_mmap;
static struct psfontmap *psfontmap=NULL;

char* newword(char** buffer, char* end) 
{
  char *word,*pos=*buffer;

  while(pos<end && *pos!=' ' && *pos!='\t' && *pos!='"') pos++;
  if ((word=malloc(pos-*buffer+1))==NULL)
    Fatal("cannot malloc space for string");
  strncpy(word,*buffer,pos-*buffer);
  word[pos-*buffer]='\0';
  *buffer=pos;
  return(word);
}

char* find_format(char* name)
{
  /* Cater for both new (first case) and old (second case) kpathsea */
  char* format =
    kpse_find_file(name,kpse_fontmap_format,false);

  if (format==NULL)
    format = kpse_find_file(name,kpse_dvips_config_format,false);
  return(format);
}

void InitPSFontMap(void)
{
  psfont_name=find_format("ps2pk.map");
  if (psfont_name==NULL)
    psfont_name=find_format("psfonts.map");

  if (psfont_name==NULL) {
    Warning("cannot find ps2pk.map, nor psfonts.map");
    return;
  }
  DEBUG_PRINT(DEBUG_FT,("\n  OPEN PSFONT MAP:\t'%s'", psfont_name));  
  if (MmapFile(psfont_name,&psfont_mmap)) {
    Warning("psfonts map %s could not be opened", psfont_name);
    return;
  }
}

struct psfontmap *SearchPSFontMap(char* fontname)
{
  char *pos,*end;
  struct psfontmap *entry=NULL;

  if (psfontmap==NULL)
    pos = psfont_mmap.mmap;
  else
    pos = psfontmap->end;
  end = psfont_mmap.mmap+psfont_mmap.size;
  while(pos<end && (entry==NULL || strcmp(entry->tfmname,fontname)!=0)) {
    while(pos < end 
	  && (*pos=='\n' || *pos==' ' || *pos=='\t' 
	      || *pos=='%' || *pos=='*' || *pos==';' || *pos=='#')) {
      while(pos < end && *pos!='\n') pos++; /* skip comments/empty rows */
      pos++;
    }
    if (pos < end) {
      if ((entry=malloc(sizeof(struct psfontmap)))==NULL)
	Fatal("cannot malloc psfontmap space");
      entry->line = pos;
      /* skip <something and quoted entries */
      while(pos < end && (*pos=='<' || *pos=='"')) {
	if (*pos=='<') 
	  while(pos < end && *pos!=' ' && *pos!='\t') pos++;
	else 
	  while(pos < end && *pos!='"') pos++;
	while(pos < end && (*pos==' ' || *pos=='\t')) pos++;
      }
      /* first word is font name */
      entry->tfmname = newword(&pos,end);
      entry->psfile = NULL;
      entry->encname = NULL;
      entry->encoding = NULL;
      while(pos < end && *pos!='\n') pos++;
      entry->end = pos;
      entry->next=psfontmap;
      psfontmap=entry;
    }
    pos++;
  }
  if (strcmp(entry->tfmname,fontname)==0)
    return(entry);
  else
    return(NULL);
}

void ClearPSFontMap(void)
{
  struct psfontmap *entry;

  while(psfontmap!=NULL) {
    entry=psfontmap;
    psfontmap=psfontmap->next;
    free(entry->tfmname);
    if (entry->psfile!=NULL && entry->psfile!=entry->tfmname)
      free(entry->psfile);
    if (entry->encname!=NULL)
      free(entry->encname);
    free(entry);
  }
  UnMmapFile(&psfont_mmap);
  if (psfont_name != NULL)
    free(psfont_name);
  psfont_name=NULL;
}

void ReadPSFontMap(struct psfontmap *entry)
{
  char *pos,*end,*psname;
  int nameno = 0;
    
  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\n  PSFONTMAP: %s ",entry->tfmname));
  pos=entry->line;
  end=entry->end;
#if HAVE_LIBT1
  entry->t1_transformp=NULL;
#endif
#if HAVE_FT2
  entry->ft_transformp=NULL;
#endif
  while(pos < end) { 
    if (*pos=='<') {                               /* filename follows */
      pos++;
      if (pos<end && *pos=='<') {           /* <<download.font */
	pos++;
	entry->psfile = newword((char**)&pos,end);
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<%s ",entry->psfile));
      } else if (pos<end && *pos=='[') {    /* <[encoding.file */
	pos++;
	entry->encname = newword((char**)&pos,end);
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
      } else {                                     /* <some.file      */
	char* word =newword((char**)&pos,end); 
	if (strncmp(word+strlen(word)-4,".enc",4)==0) {/* <some.enc */
	  entry->encname=word;
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
	} else {                                   /* <font    */  
	  entry->psfile=word;
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<%s ",entry->psfile));
	}
      }
    } else if (*pos=='"') { /* PS code: reencoding/tranformation exists */
      char *word;
      double cxx=1.0,cxy=0.0;
      pos++;
      DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\""));
      while(pos < end && *pos!='"') {
	word=newword((char**)&pos,end);
	while(pos < end && (*pos==' ' || *pos=='\t')) pos++;
	if (pos+10<end && strncmp(pos,"ExtendFont",10)==0) {
	  cxx=strtod(word,NULL);
	  pos+=10;
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("%f ExtendFont ",cxx));
	} else if (pos+9<end && strncmp(pos,"SlantFont",9)==0) {
	  pos+=9;
	  cxy=strtod(word,NULL);
	  DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("%f SlantFont ",cxy));
	} else if (pos+12<end && strncmp(pos,"ReEncodeFont",12)==0) {
	    pos+=12;
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("%s ReEncodeFont ",word));
	} else {
	    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("(?:%s) ",word));
	}
	free(word);
      }
#if HAVE_FT2
      entry->ft_transformp=&(entry->ft_transform);
      entry->ft_transform.xx=(FT_Fixed)(cxx*0x10000);
      entry->ft_transform.xy=(FT_Fixed)(cxy*0x10000);
      entry->ft_transform.yx=0;
      entry->ft_transform.yy=0x10000;
#endif
#if HAVE_LIBT1
      entry->t1_transformp=&(entry->t1_transform);
      entry->t1_transform.cxx=cxx;
      entry->t1_transform.cxy=cxy;
      entry->t1_transform.cyx=0.0;
      entry->t1_transform.cyy=1.0;
#endif
      DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("\" "));
      pos++;
    } else {                                      /* bare word */
      switch (++nameno) {
      case 1: /* first word is tfmname and perhaps psname, NOT psfile */
	while(pos<end && *pos!=' ' && *pos!='\t') pos++;
	psname=entry->tfmname;
	break;
      case 2:                    /* second word is psname, NOT psfile */
	psname = newword((char**)&pos,end);
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("(%s) ",psname));
	free(psname);
	break;
      case 3:                             /* third word is encoding */
	entry->encname = newword((char**)&pos,end);
	DEBUG_PRINT((DEBUG_FT|DEBUG_T1),("<[%s ",entry->encname));
	break;
      default:
	while(pos<end && *pos!=' ' && *pos!='\t') pos++;
	Warning("more than three bare words in font map entry");
      }
    }
    while(pos < end && (*pos==' ' || *pos=='\t')) pos++;
  }
  if (entry->psfile==NULL) { 
    /* No psfile-name given, use tfmname */
    entry->psfile=entry->tfmname;
    DEBUG_PRINT((DEBUG_FT|DEBUG_T1),(" <%s ",entry->psfile));
  }
  if (entry->encname!=NULL 
      && (entry->encoding=FindEncoding(entry->encname))==NULL) 
    Warning("unable to load font encoding '%s' for %s",
	    entry->encname,entry->tfmname);
}


struct psfontmap* FindPSFontMap(char* fontname)
{
  struct psfontmap *entry;
	
  entry=psfontmap;
  while(entry!=NULL && strcmp(entry->tfmname,fontname)!=0)
    entry=entry->next;
  if(entry==NULL)
    entry=SearchPSFontMap(fontname);
  if (entry!=NULL && entry->psfile==NULL) 
    ReadPSFontMap(entry);
  if (entry!=NULL && entry->encname!=NULL && entry->encoding==NULL) 
    /* Encoding given but it cannot be found. Unusable font */
    return(NULL);
  return(entry);
}
