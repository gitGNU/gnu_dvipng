#include "dvipng.h"

#define VF_ID 202
#define LONG_CHAR 242

#define MACRO_BATCHSIZE 256

unsigned char push=PUSH;
unsigned char pop=POP;

int32_t SetVF P2C(int32_t, c, int, PassNo) 
{
  struct vf_char* ptr;
  unsigned char *command;
  uint32_t bytes_left, length;
  struct font_num *tfontnump;  /* temporary font_num pointer   */

  DrawCommand(&push,PassNo);
  w = x = y = z = 0;
  ptr = vfstack[vfstackptr]->ch[c];
  command = ptr->macro;
  bytes_left = ptr->macro_length;

  tfontnump = vfstack[vfstackptr]->hfontnump;
  while (tfontnump->next != NULL) {
    tfontnump = tfontnump->next;
  }
  vfstack[++vfstackptr] = tfontnump->fontp;
  if (vfstack[vfstackptr]->name[0]=='\0')
    FontFind(vfstack[vfstackptr]);

  while (bytes_left > 0)  {
#ifdef DEBUG
    if (Debug)
      printf("\n VF CMD:\t%s ", dvi_commands[*command]);
#endif
    DrawCommand(command,PassNo);
    length = CommandLength(command);
    command += length;
    bytes_left -= length;
  } /* while  */
#ifdef DEBUG
  if (Debug)
    printf("\n");
#endif
  --vfstackptr;
  DrawCommand(&pop,PassNo);
  return(ptr->tfmw);
}



void InitVF  P1C(struct font_entry *,tfontp)
{
  uint32_t    t;

  OpenFont(tfontp);
  t = fgetc(tfontp->filep);
  if (t == PRE) {
    t = fgetc(tfontp->filep);
    if (t != VF_ID) 
      Fatal( "wrong version of vf file!  (%d should be 202)\n",
	     (int)t);
  } else
    Fatal("unknown font format in file <%s> !\n",vfstack[vfstackptr]->name);

  { /* VF 202 format */
    unsigned char  command[STRSIZE];
    unsigned char*   current;           
    int length;

    t = fgetc(tfontp->filep);
    fread(command,1,t,tfontp->filep); /* vf comment */
#ifdef DEBUG
    if (Debug) fprintf(ERR_STREAM, "VF COMMENT: %.*s\n", (int)t,command );
#endif

    fread(command,1,8,tfontp->filep);
    t = UNumRead(command, 4);
    CheckChecksum (tfontp->c, t, tfontp->name);

    tfontp->designsize = UNumRead(command+4,4);
#ifdef DEBUG
    if (Debug)
      printf("VF DESIGNSIZE:\t%d\n", tfontp->designsize );
#endif
    tfontp->is_vf = _TRUE;

    /* Read font definitions */
    tfontp->hfontnump=NULL;

    *command = fgetc(tfontp->filep);
    current = command+1;
#ifdef DEBUG
    if (Debug)
      printf("READ VF FNT@%ld:\t%d\n", 
	     ftell(tfontp->filep) - 1, *command);
#endif
    while(*command >= FNT_DEF1 && *command <= FNT_DEF4) {
      switch (*command)  {
      case FNT_DEF4:
	*(current)++ = fgetc(tfontp->filep);
      case FNT_DEF3: 
	*(current)++ = fgetc(tfontp->filep);
      case FNT_DEF2: 
	*(current)++ = fgetc(tfontp->filep);
      case FNT_DEF1: 
	*(current)++ = fgetc(tfontp->filep);
	if (fread(current,1,14,tfontp->filep) < 14) 
	  Fatal("vf internal font definition ends prematurely");
	current += 14;
	length = *(current-1) + *(current-2);
	if (length > (int)STRSIZE - 1 - *command + FNT_DEF1 - 1 - 14)
	  Fatal("vf internal font name longer than internal string size");
	if (fread(current,1,length,tfontp->filep) < length) 
	  Fatal("vf internal font name shorter than expected");
	FontDef(command,tfontp);
      }
      *command = fgetc(tfontp->filep);
      current = command+1;
#ifdef DEBUG
      if (Debug)
	printf("READ VF FNT@%ld:\t%d\n", ftell(tfontp->filep) - 1, *command);
#endif
    }

    
    /* Read char definitions */
    while(*command < FNT_DEF1) {
      struct vf_char *tcharptr;    /* temporary vf_char pointer          */
      uint32_t macro_length=0,c=0;
      int bytes_left=0;            /* bytes left in current macro 'batch'*/
      unsigned char *macro=NULL;
      
      tcharptr=xmalloc(sizeof(struct char_entry));
      /*#ifdef DEBUG
	if (Debug)
	printf("VF CHAR@%d\n", tcharptr);
	#endif*/
      switch (*command)  {
      case LONG_CHAR:
	fread(command+1,1,12,tfontp->filep);
	tcharptr->macro_length = 
	  macro_length = UNumRead(command+1,4);
	if ((c = UNumRead(command+5,4))>NFNTCHARS) /* Only positive for now */
	  Fatal("vf character exceeds numbering limit");
#ifdef DEBUG
	if (Debug)
	  printf("VF CHAR@%ld:\t%d\n", ftell(tfontp->filep) - 13, c);
#endif
	tfontp->ch[c] = tcharptr;
	tcharptr->tfmw = (int32_t) 
	  ((int64_t) UNumRead(command+9,4) * tfontp->d / (1 << 20));
	if (macro_length > MACRO_BATCHSIZE) {
	  Warning("vf macro too long; ignored");
	  fseek(tfontp->filep,macro_length,SEEK_CUR);
	  tcharptr->macro=NULL;
	  tcharptr->macro_length=0;
	} else {
	  if (macro == NULL || bytes_left < macro_length) {
	    macro=xmalloc(MACRO_BATCHSIZE);
	    tcharptr->free_me=_TRUE;
	    bytes_left=MACRO_BATCHSIZE;
	  } else {
	    tcharptr->free_me=_FALSE;
	  }
	  tcharptr->macro=macro;
	  if (fread(macro,1,macro_length,tfontp->filep) < macro_length) 
	    Fatal("vf char ends prematurely");
	  macro += macro_length;
	  bytes_left -= macro_length;
	}
	break;
      default:
	fread(command+1,1,4,tfontp->filep);
	tcharptr->macro_length = 
	  macro_length = *command;
	if ((c = *(command+1)) > NFNTCHARS)
	  Fatal("vf character exceeds numbering limit");
#ifdef DEBUG
	if (Debug)
	  printf("VF CHAR@%ld:\t%d\n", ftell(tfontp->filep) - 5, c);
#endif
	tfontp->ch[c] = tcharptr;
	tcharptr->tfmw = (int32_t) 
	  ((int64_t) UNumRead(command+2,3) * tfontp->d / (1 << 20));
	if (macro_length > MACRO_BATCHSIZE) {
	  Warning("vf macro too long; ignored");
	  fseek(tfontp->filep,macro_length,SEEK_CUR);
	  tcharptr->macro=NULL;
	  tcharptr->macro_length=0;
	} else {
	  if (macro == NULL || bytes_left < macro_length) {
	    macro=xmalloc(MACRO_BATCHSIZE);
	    tcharptr->free_me=_TRUE;
	    bytes_left=MACRO_BATCHSIZE;
	  } else {
	    tcharptr->free_me=_FALSE;
	  }
	  tcharptr->macro=macro;
	  if (fread(macro,1,macro_length,tfontp->filep) < macro_length) 
	    Fatal("vf char ends prematurely");
	  macro += macro_length;
	  bytes_left -= macro_length;
	}
	break;
      }
      *command = fgetc(tfontp->filep);
      current = command+1;
    }
  }
  /*#ifdef DEBUG
  if (Debug)
    printf("VF FONTNUMP@%d\n", tfontp->hfontnump);
#endif
#ifdef DEBUG
  if (Debug)
    printf("VF FONT@%d\n", tfontp);
#endif
  */
}

