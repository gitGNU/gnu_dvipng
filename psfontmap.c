#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

static unsigned char* psfont_mmap = (unsigned char *)-1;
static int psfont_filedes = -1;
static struct stat psfont_stat;

void InitPSFontMap(void)
{
  char* psfont_name =
    kpse_find_file("ps2pk.map",kpse_dvips_config_format,false);

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
}

char* FindPSFontMap(char* fontname, char** encoding, FT_Matrix** transform)
{
  unsigned char* position=NULL;

  *encoding=NULL;

  if (psfont_mmap != (unsigned char *)-1) {
    position = psfont_mmap;
    while(position !=NULL && strncmp(++position,fontname,strlen(fontname))!=0)
      position=memchr(position,'\n',
		      psfont_mmap+psfont_stat.st_size-position-1);
    if (position!=NULL) {
      unsigned char* end=memchr(position,'\n',
				psfont_mmap+psfont_stat.st_size-position);
      unsigned char* cur=memchr(position,'"', 
				psfont_mmap+psfont_stat.st_size-position)+1;
      unsigned char* cur2,*str,*psfile=NULL;

      if (end == NULL) 
	end=psfont_mmap+psfont_stat.st_size;
      DEBUG_PRINT((DEBUG_FT,"\n  PS FONT MAP: %.*s ",end-position,position));
      if (cur != NULL && cur < end) {    /* Reencoding/tranformation exists */
	FT_Fixed xx=0,xy=0,val=0;

	while(*cur!='"' && cur < end) {
	  val=strtod(cur,(char**)&cur)*0x10000; 
	  /*                     Small risk, but we're inside double quotes */
	  while(*cur==' ' && cur < end)
	    cur++;
	  if (cur<end-10 && strncmp(cur,"ExtendFont",10)==0) 
	    xx=val;
	  if (cur<end-9 && strncmp(cur,"SlantFont",9)==0)
	    xy=val;
	  while( *cur!=' ' && *cur!='"' && cur < end)
	    cur++;
	}
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
      }
      cur = memchr(position,'<',psfont_mmap+psfont_stat.st_size-position);
      while (cur != NULL && cur < end) {    /* we have "<filename" */
	while((*cur==' ' || *cur=='<') && cur < end)
	  cur++;
	cur2 = memchr(cur,' ',psfont_mmap+psfont_stat.st_size-cur);
	if (cur2>end)
	  cur2=end;
	str = malloc(cur2-cur);
	strncpy(str,cur,cur2-cur);
	str[cur2-cur]='\0';
	if (strncmp(cur2-4,".enc",4)==0)    /* encoding */
	  *encoding = str;
	else                                /* postcript filename */
	  psfile = str;
	cur = memchr(cur2,'<',psfont_mmap+psfont_stat.st_size-cur2);
      }
      if (psfile==NULL) {                   /* postscript filename not given */
	cur=cur2=position;
	cur2 = memchr(cur,' ',psfont_mmap+psfont_stat.st_size-cur);
	if (cur2>end)
	  cur2=end;
	str = malloc(cur2-cur);
	strncpy(str,cur,cur2-cur);
	str[cur2-cur]='\0';
	psfile = str;
      }
      DEBUG_PRINT((DEBUG_FT,"\n  PS FONT MAP DATA: '%s' ",psfile));

      return(psfile);
    }
  }
  return(NULL);
}
