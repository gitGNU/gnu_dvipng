#include "dvipng.h"

#define VF_ID 202
#define LONG_CHAR 242

#define MACRO_BATCHSIZE 256

void InitVF  P1C(struct font_entry *,tfontp)
{
  long4    t;

  OpenFont(tfontp);
  t = NoSignExtend(tfontp->filep, 1);
  if (t == PRE) {
    unsigned char   temp_byte;
    temp_byte = (unsigned char) NoSignExtend(tfontp->filep, 1);
    if (temp_byte != VF_ID) 
      Fatal( "wrong version of vf file!  (%d should be 202)\n",
	     (int)temp_byte);
  } else
    Fatal("unknown font format in file <%s> !\n",vfstack[vfstackptr]->name);

  { /* VF 202 format */
    register unsigned char  temp_byte;
    unsigned char  command[STRSIZE];
    unsigned char*   current;           
    int length;

    temp_byte = (unsigned char)NoSignExtend(tfontp->filep, 1);
    fread(command,1,temp_byte,tfontp->filep); /* vf comment */
#ifdef DEBUG
    if (Debug) fprintf(ERR_STREAM, "VF COMMENT: %s\n", command );
#endif
    t = NoSignExtend(tfontp->filep, 4);
    CheckChecksum (tfontp->c, t, tfontp->name);

    tfontp->designsize = NoSignExtend(tfontp->filep, 4);
#ifdef DEBUG
    if (Debug)
      printf("VF DESIGNSIZE:\t%ld\n", tfontp->designsize );
#endif
    tfontp->is_vf = _TRUE;

    /* Read font definitions */
    tfontp->hfontnump=NULL;

    *command = fgetc(tfontp->filep);
    current = command+1;
#ifdef DEBUG
    if (Debug)
      printf("READ VF FNT@%ld:\t%d\n", (long) ftell(tfontp->filep) - 1, *command);
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
	printf("READ VF FNT@%ld:\t%d\n", (long) ftell(tfontp->filep) - 1, *command);
#endif
    }

    
    /* Read char definitions */
    while(*command < FNT_DEF1) {
      struct vf_char *tcharptr;    /* temporary vf_char pointer          */
      long4 macro_length=0,c=0;
      int bytes_left=0;            /* bytes left in current macro 'batch'*/
      unsigned char *macro=NULL;
      
      tcharptr=xmalloc(sizeof(struct char_entry));
      /*#ifdef DEBUG
	if (Debug)
	printf("VF CHAR@%ld\n", tcharptr);
	#endif*/
      switch (*command)  {
      case LONG_CHAR:
	fread(command+1,1,12,tfontp->filep);
	tcharptr->macro_length = 
	  macro_length = UNumRead(command+1,4);
	if ((c = UNumRead(command+5,4))>NFNTCHARS)
	  Fatal("vf character exceeds numbering limit");
#ifdef DEBUG
	if (Debug)
	  printf("VF CHAR@%ld:\t%ld\n", (long) ftell(tfontp->filep) - 13, c);
#endif
	tfontp->ch[c] = tcharptr;
	tcharptr->tfmw = (long4) ((long long) UNumRead(command+9,4) * tfontp->d / (1 << 20));
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
	  printf("VF CHAR@%ld:\t%ld\n", (long) ftell(tfontp->filep) - 5, c);
#endif
	tfontp->ch[c] = tcharptr;
	tcharptr->tfmw = (long4) 
	  ((long long) UNumRead(command+2,3) * tfontp->d / (1 << 20));
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
    printf("VF FONTNUMP@%ld\n", tfontp->hfontnump);
#endif
#ifdef DEBUG
  if (Debug)
    printf("VF FONT@%ld\n", tfontp);
#endif
  */
}

