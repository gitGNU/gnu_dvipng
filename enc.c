#include "dvipng.h"
#include <fcntl.h> // open/close
#include <sys/mman.h>
#include <sys/stat.h>

struct encoding* encodingp=NULL;

struct encoding* InitEncoding(char* encoding) 
{
  char *pos,*max,*buf,*enc_file =
    kpse_find_file(encoding,kpse_dvips_config_format,false);
  int encfd,i;
  struct encoding* encp=NULL;
  struct stat stat;
  unsigned char* encmmap;
  
  if (enc_file == NULL)
    Warning("encoding file %s could not be found",encoding);
  DEBUG_PRINT(((DEBUG_FT|DEBUG_ENC),"\n  OPEN ENCODING:\t'%s'", enc_file));
  if ((encfd = open(enc_file,O_RDONLY)) == -1) 
    Warning("encoding file %s could not be opened", enc_file);
  fstat(encfd,&stat);
  encmmap = mmap(NULL,stat.st_size, PROT_READ, MAP_SHARED,encfd,0);
  if (encmmap == (unsigned char *)-1)
    Warning("cannot mmap encoding <%s> !\n",enc_file);
  if ((encp = calloc(sizeof(struct encoding)+stat.st_size,1))==NULL)
    Warning("cannot alloc space for encoding",enc_file);
  pos=encmmap;
  max=encmmap+stat.st_size;
  buf=(char*)encp+sizeof(struct encoding);
#define SKIPCOMMENT(x) if (*x=='%') while (x<max && *x!='\n') x++;
  while (pos < max && *pos!='[') {
    SKIPCOMMENT(pos);
    pos++;
  }
  while(pos<max && *pos!='/') {
    SKIPCOMMENT(pos);
    pos++;
  }
  i=0;
  while(pos<max && *pos!=']') {
    pos++;
    encp->charname[i++]=buf;
    while(pos<max && *pos!=' ' && *pos!='\t' && *pos!='\n' && *pos!='%') 
      *buf++=*pos++;
    *buf++='\0';
    DEBUG_PRINT((DEBUG_ENC,"\n  PS ENCODING %d '%s'",
		 i-1,encp->charname[i-1])); 
    while(pos<max && *pos!='/' && *pos!=']') {
      SKIPCOMMENT(pos);
      pos++;
    }
  }
  return(encp);
}


