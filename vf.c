#include "dvipng.h"
#include <sys/mman.h>
#include <sys/stat.h>


#define VF_ID 202
#define LONG_CHAR 242

unsigned char push=PUSH;
unsigned char pop=POP;

int32_t SetVF(int32_t c, int PassNo) 
{
  struct font_entry* currentvf;
  unsigned char *command,*end;

  currentvf=currentfont;
  SetFntNum(currentvf->defaultfont,(struct dvi_vf_entry*)currentvf);
  DrawCommand(&push,PassNo,(struct dvi_vf_entry*)NULL);
  w = x = y = z = 0;
  command = currentvf->vf_ch[c]->mmap;
  end = command + currentvf->vf_ch[c]->length;
  while (command < end)  {
    DEBUG_PRINTF(DEBUG_DVI,"\n  VF MACRO:\t%s ", dvi_commands[*command]);
    DrawCommand(command,PassNo,(struct dvi_vf_entry*)currentfont);
    command += CommandLength(command);
  } 
  DrawCommand(&pop,PassNo,(struct dvi_vf_entry*)NULL);
  currentfont=currentvf;
  return(currentvf->vf_ch[c]->tfmw);
}



void InitVF(struct font_entry * tfontp)
{
  struct stat stat;
  unsigned char* position;
  int length;
  struct vf_char *tcharptr;  
  uint32_t c=0;
  struct font_num *tfontnump;  /* temporary font_num pointer   */
  
  DEBUG_PRINTF((DEBUG_DVI|DEBUG_VF),"\n  OPEN FONT:\t'%s'", tfontp->name);
  if ((tfontp->filedes = open(tfontp->name,O_RDONLY)) == -1) 
    Warning("font file %s could not be opened", tfontp->name);
  fstat(tfontp->filedes,&stat);
  tfontp->mmap = mmap(NULL,stat.st_size,
      PROT_READ, MAP_SHARED,tfontp->filedes,0);
  if (*(tfontp->mmap) != PRE) 
    Fatal("unknown font format in file <%s> !\n",currentfont->name);
  if (*(tfontp->mmap+1) != VF_ID) 
      Fatal( "wrong version of vf file!  (%d should be 202)\n",
	     (int)*(tfontp->mmap+1));
  DEBUG_PRINTF2(DEBUG_VF,"\n  VF_PRE:\t'%.*s'", 
		(int)*(tfontp->mmap+2), tfontp->mmap+3);
  position = tfontp->mmap+3 + *(tfontp->mmap+2);
  c=UNumRead(position, 4);
  DEBUG_PRINTF(DEBUG_VF," %d", c);
  CheckChecksum (tfontp->c, c, tfontp->name);
  tfontp->designsize = UNumRead(position+4,4);
  DEBUG_PRINTF(DEBUG_VF," %d", tfontp->designsize);
  tfontp->type = FONT_TYPE_VF;
  tfontp->vffontnump=NULL;
  /* Read font definitions */
  position += 8;
  while(*position >= FNT_DEF1 && *position <= FNT_DEF4) {
    DEBUG_PRINTF2(DEBUG_VF,"\n  @%ld VF:\t%s", 
		  (long)(position - tfontp->mmap), 
		  dvi_commands[*position]);
    FontDef(position,(struct dvi_vf_entry*)tfontp);	
    length = dvi_commandlength[*position];
    position += length + *(position + length-1) + *(position+length-2);
  }
  /* Default font is the first defined */
  tfontnump = tfontp->vffontnump;
  while (tfontnump->next != NULL) {
    tfontnump = tfontnump->next;
  }
  tfontp->defaultfont=tfontnump->k;
  
  /* Read char definitions */
  while(*position < FNT_DEF1) {
    DEBUG_PRINTF(DEBUG_VF,"\n@%ld VF CHAR:\t", 
		 (long)(position - tfontp->mmap));
    tcharptr=xmalloc(sizeof(struct vf_char));
    switch (*position) {
    case LONG_CHAR:
      tcharptr->length = UNumRead(position+1,4);
      c = UNumRead(position+5,4);
      tcharptr->tfmw = UNumRead(position+9,4);
      position += 13;
      break;
    default:
      tcharptr->length = UNumRead(position,1);
      c = UNumRead(position+1,1);
      tcharptr->tfmw = UNumRead(position+2,3);
      position += 5;
    }
    DEBUG_PRINTF2(DEBUG_VF,"%d %d",tcharptr->length,c);
    DEBUG_PRINTF(DEBUG_VF," %d",tcharptr->tfmw);
    tcharptr->tfmw = (int32_t) 
      ((int64_t) tcharptr->tfmw * tfontp->d / (1 << 20));
    DEBUG_PRINTF(DEBUG_VF," (%d)",tcharptr->tfmw);
    if (c > NFNTCHARS) /* Only positive for now */
      Fatal("vf character exceeds numbering limit");
    tfontp->vf_ch[c] = tcharptr;
    tcharptr->mmap=position;
    position += tcharptr->length;
  }
}
  

