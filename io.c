#include "dvipng.h"
/* This function closes all open files */
void CloseFiles P1H(void)
{
  struct font_entry *fe;
  FILEPTR f;

  /*
    if (dvi.filep != FPNULL) {
    BCLOSE(dvi.filep);
    }*/
#ifdef __riscos
  if (metafile != FPNULL) {
    BCLOSE(metafile);
  }
#endif
  /* Now all open font files */
  fe = hfontptr;
  while (fe != NULL) {
    f = fe->filep;
    if ((f != FPNULL) && (f != NO_FILE)) {
      BCLOSE(f);
    }
    fe = fe->next;
  }
}

/*-->NoSignExtend*/
/**********************************************************************/
/***************************  NoSignExtend  ***************************/
/**********************************************************************/
long4 NoSignExtend P2C(FILEPTR, fp, register int, n)
{
  long4 x = 0;      /* number being constructed */
  unsigned char h;
  while (n--) {
    x <<= 8; 
    h=fgetc(fp);
    x |= h;
  }
  /* fprintf(stderr,"[%ld] ",(long)x);*/
  return(x);
}


#ifndef ARITHMETIC_RIGHT_SHIFT
long4   signTab[5] = {0,0x00000080,0x00008000,0x00800000,0x00000000};
long4 extendTab[5] = {0,~0^0xff,~0^0xffff,~0^0xffffff,~0^0xffffffff};
#endif

/*-->SignExtend*/
/**********************************************************************/
/****************************  SignExtend  ****************************/
/**********************************************************************/
long4 SignExtend P2C(FILEPTR, fp, register int, n)
{
  int     n1;       /* number of bytes      */
  long4   x;        /* number being constructed */
  unsigned char    h;
#ifdef SIGN_DEBUG
  long4    x0;      /* copy of x  */
#endif
  h=fgetc(fp); 
  x = h; /* get first (high-order) byte */
  n1 = n--;
  while (n--) {
    x <<= 8;
    h=fgetc(fp); 
    x |= h;
  }
  /*
   *   NOTE: This code assumes that the right-shift is an arithmetic, rather
   *   than logical, shift which will propagate the sign bit right.   According
   *   to Kernighan and Ritchie, this is compiler dependent!
   */

#ifdef SIGN_DEBUG
  x0 = x;
#endif

#ifdef ARITHMETIC_RIGHT_SHIFT
  x <<= 32 - 8 * n1;
  x >>= 32 - 8 * n1; /* sign extend */
#else
  if (x & signTab[n1]) x |= extendTab[n1];
#endif

#ifdef SIGN_DEBUG
  fprintf(ERR_STREAM,"\tSignExtend(fp,%d)=%lX, was=%lX,%d\n",
          n1,x,x0,x0&signTab[n1]);
#endif

#ifdef DEBUG
  if (Debug > 1)
    fprintf(ERR_STREAM,"\tSignExtend(fp,%d)=%lX\n", n1, x);
#endif
  return(x);
}

long4 UNumRead P2C(unsigned char*, current, register int, n)
{
  long4 x = *(current)++;      /* number being constructed */

  while(--n) 
    x = (x << 8) | *(current)++;
  return(x);
}

long4 SNumRead P2C(unsigned char*, current, register int, n)
{
  long4	x;
  
#if	__STDC__
  x = (signed char) *(current)++;      /* number being constructed */
#else
  x = *(current)++;      /* number being constructed */
  if (x & 0x80) x -= 0x100;
#endif
  while(--n) 
    x = (x << 8) | *(current)++;
  return(x);
}

void ReadCommand P1C(unsigned char*, command)
     /* This function reads in and stores the next dvi command. */
{ 
  unsigned char*   current;           
  int length;

  *command = fgetc(dvi.filep);
  current = command+1;
#ifdef DEBUG
  if (Debug)
    printf("READ CMD@%ld:\t%d\n", (long) ftell(dvi.filep) - 1, *command);
#endif
  if ((*command > SETC_127 && *command < FONT_00) 
      || *command > FONT_63) {
    switch (*command)  {
    case SET_RULE: case PUT_RULE:
      *(current)++ = fgetc(dvi.filep);
      *(current)++ = fgetc(dvi.filep);
      *(current)++ = fgetc(dvi.filep);
      *(current)++ = fgetc(dvi.filep);
    case PUT4: case SET4: case RIGHT4: case W4: case X4:
    case DOWN4: case Y4: case Z4: case FNT4:
      *(current)++ = fgetc(dvi.filep);
    case PUT3: case SET3: case RIGHT3: case W3: case X3:
    case DOWN3: case Y3: case Z3: case FNT3:
      *(current)++ = fgetc(dvi.filep);
    case PUT2: case SET2: case RIGHT2: case W2: case X2:
    case DOWN2: case Y2: case Z2: case FNT2:
      *(current)++ = fgetc(dvi.filep);
    case PUT1: case SET1: case RIGHT1: case W1: case X1:
    case DOWN1: case Y1: case Z1: case FNT1:
      *(current)++ = fgetc(dvi.filep);
    case EOP: case PUSH: case POP: case W0: case X0:
    case Y0: case Z0: case NOP: 
      break;
    case XXX4:
      *(current)++ = fgetc(dvi.filep);
    case XXX3: 
      *(current)++ = fgetc(dvi.filep);
    case XXX2: 
      *(current)++ = fgetc(dvi.filep);
    case XXX1: 
      *(current)++ = fgetc(dvi.filep);
      length = UNumRead((char*)command+1,(int)*command - XXX1 + 1);
      if (length >  (int)STRSIZE - 1 - *command + XXX1 - 1)
	Fatal("\\special string longer than internal string size");
      if (fread(current,1,length,dvi.filep) < length) 
        Fatal("\\special string shorter than expected");
      break;
    case FNT_DEF4:
      *(current)++ = fgetc(dvi.filep);
    case FNT_DEF3: 
      *(current)++ = fgetc(dvi.filep);
    case FNT_DEF2: 
      *(current)++ = fgetc(dvi.filep);
    case FNT_DEF1: 
      *(current)++ = fgetc(dvi.filep);
      if (fread(current,1,14,dvi.filep) < 14) 
	Fatal("font definition ends prematurely");
      current += 14;
      length = *(current-1) + *(current-2);
      if (length > (int)STRSIZE - 1 - *command + FNT_DEF1 - 1 - 14)
	Fatal("font name longer than internal string size");
      if (fread(current,1,length,dvi.filep) < length) 
        Fatal("font name shorter than expected");
      break;
    case PRE: 
      if (fread(current,1,14,dvi.filep) < 14) 
	Fatal("preamble ends prematurely");
      current += 14;
      length = *(current-1);
      if (length > (int)STRSIZE - 1 - 14)
	Fatal("preamble comment longer than internal string size");
      if (fread(current,1,length,dvi.filep) < length) 
        Fatal("preamble comment shorter than expected");
      break;
    case BOP: 
      if (fread(current,1,44,dvi.filep) < 44) 
	Fatal("BOP ends prematurely");
      break;
    case POST:
      if (fread(current,1,28,dvi.filep) < 28) 
	Fatal("postamble ends prematurely");
      break;
    case POST_POST:
      if (fread(current,1,9,dvi.filep) < 9) 
	Fatal("postpostamble ends prematurely");
      break;
    default:
      Fatal("%d is an undefined command", *command);
      break;
    }
  } /* if */
}

long4 CommandLength P1C(unsigned char*, command)
{ 
  unsigned char*   current;           
  long4 length=0;

  current = command+1;
  if ((*command > SETC_127 && *command < FONT_00) 
      || *command > FONT_63) {
    switch (*command)  {
    case SET_RULE: case PUT_RULE:
      length=8;
      break;
    case PUT4: case SET4: case RIGHT4: case W4: case X4:
    case DOWN4: case Y4: case Z4: case FNT4:
      length=4;
      break;
    case PUT3: case SET3: case RIGHT3: case W3: case X3:
    case DOWN3: case Y3: case Z3: case FNT3:
      length=3;
      break;
    case PUT2: case SET2: case RIGHT2: case W2: case X2:
    case DOWN2: case Y2: case Z2: case FNT2:
      length=2;
      break;
    case PUT1: case SET1: case RIGHT1: case W1: case X1:
    case DOWN1: case Y1: case Z1: case FNT1:
      length=1;
      break;
    case EOP: case PUSH: case POP: case W0: case X0:
    case Y0: case Z0: case NOP: 
      length=0;
      break;
    case XXX4:
      length++;
    case XXX3: 
      length++;
    case XXX2:
      length++;
    case XXX1: 
      length++;
      length += UNumRead((char*)command + 1,length);
      break;
    case FNT_DEF4:
      length++;
    case FNT_DEF3: 
      length++;
    case FNT_DEF2:   
      length++;
    case FNT_DEF1: 
      length += 15;
      length += *(command + length - 1) + *(command + length - 2);
      break;
    case PRE: 
      length = 14;
      length += *(command + length - 1);
      break;
    case BOP: 
      length = 44;
      break;
    case POST:
      length = 28;
      break;
    case POST_POST:
      length = 9; /* minimum */
      break;
    default:
      Fatal("%d is an undefined command", *command);
      break;
    }
  } /* if */
  return(length);
}
