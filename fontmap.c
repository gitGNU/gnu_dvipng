#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

static unsigned char* psfont_mmap = (unsigned char *)-1;
static int psfont_filedes = -1;
static struct stat psfont_stat;
static struct psfontmap {
  struct psfontmap *next;
  char *line,*tfmname,*end;
} *psfontmap=NULL;

inline char* newword(char** buffer, char* end) 
{
  char *word,*pos=*buffer;

  while(pos<end && *pos!=' ' && *pos!='\t' && *pos!='"') pos++;
  if ((word=malloc(pos-*buffer+1))==NULL)
    Fatal("cannot malloc space for string");
  strncpy(word,*buffer,pos-*buffer);
  word[pos-*buffer]='\0';
  if (*pos=='"') pos++;
  *buffer=pos;
  return(word);
}

void InitPSFontMap(void)
{
  char *pos,*end,*psfont_name =
    kpse_find_file("ps2pk.map",kpse_dvips_config_format,false);
  struct psfontmap *entry;

  if (psfont_name==NULL)
    psfont_name =
      kpse_find_file("psfonts.map",kpse_dvips_config_format,false);

  if (psfont_name==NULL) {
    Warning("cannot find ps2pk.map, nor psfonts.map");
    return;
  }
  DEBUG_PRINT((DEBUG_FT,"\n  OPEN PSFONT MAP:\t'%s'", psfont_name));  
  if ((psfont_filedes = open(psfont_name,O_RDONLY)) == -1) { 
    Warning("psfonts map %s could not be opened", psfont_name);
    return;
  }
  fstat(psfont_filedes,&psfont_stat);
  psfont_mmap = mmap(NULL,psfont_stat.st_size,
		     PROT_READ, MAP_SHARED,psfont_filedes,0);
  if (psfont_mmap == (unsigned char *)-1) 
    Warning("cannot mmap psfonts map %s !\n",psfont_name);
  else {
    pos = psfont_mmap;
    end = psfont_mmap+psfont_stat.st_size;
    while(pos<end) {
      while(pos < end && (*pos=='\n' || *pos==' ' || *pos=='\t' 
			  || *pos=='%' || *pos=='*' || *pos==';' || *pos=='#')) {
	while(pos < end && *pos!='\n') pos++; /* skip comments/empty rows */
	pos++;
      }
      if (pos < end) {
	if ((entry=malloc(sizeof(struct psfontmap)))==NULL)
	  Fatal("cannot malloc psfontmap space");
	entry->line = pos;
	while(pos < end && (*pos=='<' || *pos=='"')) {
	  if (*pos=='<') 
	    while(pos < end && *pos!=' ' && *pos!='\t') pos++;
	  else 
	    while(pos < end && *pos!='"') pos++;
	  while(pos < end && (*pos==' ' || *pos=='\t')) pos++;
	}
 	entry->tfmname = pos;
	while(pos < end && *pos!='\n') pos++;
	entry->end = pos;
	entry->next=psfontmap;
	psfontmap=entry;
      }
      pos++;
    }
  }
}


char* FindPSFontMap(char* fontname, char** encoding, FT_Matrix** transform)
{
  char *pos,*tfmname,*psname,*psfile=NULL;
  struct psfontmap *entry;

  *encoding=NULL;
  *transform=NULL;

  entry=psfontmap;
  while(entry!=NULL && strncmp(entry->tfmname,fontname,strlen(fontname))!=0) 
    entry=entry->next;
  if (entry!=NULL) {
    int nameno = 0;
    
    tfmname=fontname;
    DEBUG_PRINT((DEBUG_FT,"\n  PSFONTMAP: %s ",fontname));
    pos=entry->line;
    while(pos < entry->end) { 
      if (*pos=='<') {                               /* filename follows */
	pos++;
	if (pos<entry->end && *pos=='<') {           /* <<download.font */
	  pos++;
	  psfile = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT,"<%s ",psfile));
	} else if (pos<entry->end && *pos=='[') {    /* <[encoding.file */
	  pos++;
	  *encoding = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT,"<[%s ",*encoding));
	} else {                                     /* <some.file      */
	  char* word =newword((char**)&pos,entry->end); 
	  if (strncmp(word+strlen(word)-4,".enc",4)==0) {   /* encoding */
	    *encoding=word;
	    DEBUG_PRINT((DEBUG_FT,"<[%s ",*encoding));
	  } else {
	    psfile=word;
	    DEBUG_PRINT((DEBUG_FT,"<%s ",psfile));
	  }
	}
      } else if (*pos=='"') { /* PS code: reencoding/tranformation exists */
	FT_Fixed xx=0,xy=0,val=0;
	
	pos++;
	DEBUG_PRINT((DEBUG_FT,"\""));
	while(pos < entry->end && *pos!='"') {
	  val=strtod(pos,(char**)&pos)*0x10000; 
	  /* Small buffer overrun risk, but we're inside double quotes */
	  while(pos < entry->end && (*pos==' ' || *pos=='\t')) pos++;
	  if (pos<entry->end-10 && strncmp(pos,"ExtendFont",10)==0) {
	    xx=val;
	    DEBUG_PRINT((DEBUG_FT,"%f ExtendFont ",(float)xx/0x10000));
	  }
	  if (pos<entry->end-9 && strncmp(pos,"SlantFont",9)==0) {
	    xy=val;
	    DEBUG_PRINT((DEBUG_FT,"%f SlantFont ",(float)xy/0x10000));
	  }
	  while(pos<entry->end && *pos!=' ' && *pos!='\t' && *pos!='"') pos++;
	}
	DEBUG_PRINT((DEBUG_FT,"\" "));
	pos++;
	if (xx!=0 || xy!=0) {
	  *transform=malloc(sizeof(FT_Matrix));
	  (**transform).yx=0;
	  (**transform).yy=0x10000;
	  if (xx!=0)
	    (**transform).xx=xx;
	  else
	    (**transform).xx=0x10000;
	  if (xy!=0)
	    (**transform).xy=xy;
	  else
	    (**transform).xy=0;
	}
      } else {                                      /* bare word */
	switch (++nameno) {
	case 1:
	  while(pos<entry->end && *pos!=' ' && *pos!='\t') pos++;
	  psname=tfmname;
	  break;
	case 2:
	  psname = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT,"(%s) ",psname));
	  break;
	case 3:
	  *encoding = newword((char**)&pos,entry->end);
	  DEBUG_PRINT((DEBUG_FT,"<[%s ",*encoding));
	  break;
	default:
	  Warning("more than three bare words in font map entry");
	}
      }
      while(pos < entry->end && (*pos==' ' || *pos=='\t')) pos++;
    }
    if (psfile==NULL) {
      psfile=tfmname;
      DEBUG_PRINT((DEBUG_FT," <%s ",psfile));
    }
  }
  return(psfile);
}
