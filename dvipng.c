/* dvipng.c
 */
/**********************************************************************
 ****************************  Intro  *********************************
 **********************************************************************
 * This program translates TeX's DVI-Code into Portable Network Graphics.
 *
 * Copyright (C) Jan-Åke Larsson 2002
 *
 * Recipe:
 * Take a simple dvi2?? converter, dvilj is suitable here.
 * Read and relish the code of xdvi and dvips
 * Choose a png-drawing library with a simple interface and support
 *  on many platforms: gd
 * Stir, sprinkle with some insights of your own, and enjoy
 *
 **********************************************************************
 * Preprocessor switches:
 *      #define DEBUG    for massive printing of trace information
 *               when -d cmdline option specified
 *      #define DRAWGLYPH draws PK-Glyphs on stdout
 **********************************************************************
 */

#define MAIN
#include "dvipng.h"

/**********************************************************************/
/*******************************  main  *******************************/
/**********************************************************************/
int
#if NeedFunctionPrototypes
main(int argc, char *argv[])
#else
main(argc, argv)
int     argc;
char    *argv[];
#endif
{
  struct stack_entry {  /* stack entry */
    long4    h, v, w, x, y, z;  /* what's on stack */
  };
  short   command;           /* current command                         */
  long4   count[10];         /* the 10 counters at begining of each page*/
  long4   cpagep = 0;        /* current page pointer                    */
  bool    Emitting = _FALSE; /* outputting typsetting instructions?     */
  int     i;                 /* command parameter; loop index           */
  int     k;                 /* temporary parameter                     */
  char    n[STRSIZE];        /* command parameter                       */
  int     PassNo = 0;        /* which pass over the DVI page are we on? */
  bool    SkipMode = _FALSE; /* in skip mode flag                       */
  int     sp = 0;            /* stack pointer                           */
  struct  stack_entry stack[STACK_SIZE];  /* stack                      */
  char    SpecialStr[STRSIZE]; /* "\special" strings                    */
  long4   val, val2;         /* temporarys to hold command information  */
  long4   w = 0;             /* current horizontal spacing              */
  long4   x = 0;             /* current horizontal spacing              */
  long4   y = 0;             /* current vertical spacing                */
  long4   z = 0;             /* current vertical spacing                */


# if !defined (__riscos) && !defined (KPATHSEA)
  extern  char *sys_errlist[];
  extern  int   errno;
# endif

  /* Initialize pixel_files */   
  for (i = 0; i <= MAXOPEN; i++)
    pixel_files[i].pixel_file_id = FPNULL;

  x_origin = XDEFAULTOFF; /* x-origin in dots                    */
  y_origin = YDEFAULTOFF; /* y-origin in dots                    */

  setbuf(ERR_STREAM, NULL);
  (void) strcpy(G_progname, argv[0]);
#ifdef KPATHSEA
  kpse_set_progname(argv[0]);
  kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_compile);
#endif
  DecodeArgs(argc, argv);

  initcolor();

#ifdef KPATHSEA
  kpse_init_prog("DVIPNG", resolution, MFMODE, "cmr10");
#endif

  if ((i = (int)NoSignExtend(dvifp, 1)) != PRE) {
    Fatal("%s: PRE doesn't occur first--are you sure this is a DVI file?\n\n",
          G_progname);
  }
  i = (int)SignExtend(dvifp, 1);
  if (i != DVIFORMAT) {
    Fatal( "%s: DVI format = %d, can only process DVI format %d files\n\n",
           G_progname, i, DVIFORMAT);
  }

#ifdef TIMING
#ifdef BSD_TIME_CALLS
  ftime(&timebuffer);
  start_time = timebuffer.time + (float)(timebuffer.millitm) / 1000.0;
#else
  gettimeofday(&Tp, NULL);
  start_time = Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0;
#endif
#endif

  if (Reverse) {
#ifdef DEBUG
    if (Debug)
      fprintf(ERR_STREAM, "reverse\n");
#endif
    ReadPostAmble(_TRUE);
    FSEEK(dvifp, ppagep, SEEK_SET);
  } else {
    ReadPostAmble(_TRUE);
    FSEEK(dvifp,  14l, SEEK_SET);
    k = (int)NoSignExtend(dvifp, 1);
    GetBytes(dvifp, n, k);
  }
  PassNo = 0;
  while (_TRUE)  {
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
    if (/*command >= SETC_000 &&*/ command <= SETC_127) {
      SetChar(command, command, PassNo);
    } else if (command >= FONT_00 && command <= FONT_63) {
        /*if (!SkipMode)*/
      SetFntNum((long4)command - FONT_00, Emitting);
    } else switch (command)  {
    case PUT1:
    case PUT2:
    case PUT3:
    case PUT4:
      val = NoSignExtend(dvifp, (int)command - PUT1 + 1);
      /*if (!SkipMode)*/
      SetChar(val, command, PassNo);
      break;
    case SET1:
    case SET2:
    case SET3:
    case SET4:
      val = NoSignExtend(dvifp, (int)command - SET1 + 1);
      /*      if (!SkipMode)*/
      SetChar(val, command, PassNo);
      break;
    case SET_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      /*if (Emitting)*/
      SetRule(val, val2, PassNo, 1);
      break;
    case PUT_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      /*if (Emitting)*/
      SetRule(val, val2, PassNo, 0);
      break;
    case BOP:
      cpagep = FTELL(dvifp) - 1;
      for (i = 0; i <= 9; i++) {
        count[i] = NoSignExtend(dvifp, 4);
      }
      ppagep = (long)NoSignExtend(dvifp, 4);
      h = v = w = x = y = z = 0;
      sp = 0;
      fontptr = NULL;
      prevfont = NULL;
      /*if ( !SkipMode ) {*/
      if (PassNo == 0) {
	qfprintf(ERR_STREAM,"[%ld",  (long)count[0]);
      } else {
	DoBop();	  /*}*/
      }
      break;
    case EOP:
      /*if ( !SkipMode ) {*/
      if (PassNo == 0) {
	/* start second pass on current page */
	FSEEK(dvifp, cpagep, SEEK_SET);
	PassNo = 1;
#ifdef DEBUG
	if (Debug)
	  fprintf(ERR_STREAM,"\nStarting second pass\n");
#endif

      } else {
	/* end of second pass, and of page processing */
	FormFeed(count[0]);
	++ndone;

	qfprintf(ERR_STREAM,"] ");
	PassNo = 0;
      }
      /*}  else
        PassNo = 0;

      if ( PassNo == 0 && Reverse ) {
        if ( ppagep > 0 )
          FSEEK(dvifp, ppagep, SEEK_SET);
        else {
          AllDone(_FALSE);
        }
	}*/
      break;
    case PUSH:
      if (sp >= STACK_SIZE)
        Fatal("stack overflow");
      stack[sp].h = h;
      stack[sp].v = v;
      stack[sp].w = w;
      stack[sp].x = x;
      stack[sp].y = y;
      stack[sp].z = z;
      sp++;
      break;
    case POP:
      --sp;
      if (sp < 0)
        Fatal("stack underflow");
      h = stack[sp].h;
      v = stack[sp].v;
      w = stack[sp].w;
      x = stack[sp].x;
      y = stack[sp].y;
      z = stack[sp].z;
      break;
    case RIGHT1:
    case RIGHT2:
    case RIGHT3:
    case RIGHT4:
      val = SignExtend(dvifp, (int)command - RIGHT1 + 1);
      /*if (Emitting)*/
        MoveOver(val);
      break;
    case W1:
    case W2:
    case W3:
    case W4:
      w = SignExtend(dvifp, (int)command - W1 + 1);
    case W0:
      /*if (Emitting)*/
        MoveOver(w);
      break;
    case X1:
    case X2:
    case X3:
    case X4:
      x = SignExtend(dvifp, (int)command - X1 + 1);
    case X0:
      /*if (Emitting)*/
        MoveOver(x);
      break;
    case DOWN1:
    case DOWN2:
    case DOWN3:
    case DOWN4:
      val = SignExtend(dvifp, (int)command - DOWN1 + 1);
      /*if (Emitting)*/
        MoveDown(val);
      break;
    case Y1:
    case Y2:
    case Y3:
    case Y4:
      y = SignExtend(dvifp, (int)command - Y1 + 1);
    case Y0:
      /*if (Emitting)*/
        MoveDown(y);
      break;
    case Z1:
    case Z2:
    case Z3:
    case Z4:
      z = SignExtend(dvifp, (int)command - Z1 + 1);
      /*if (Emitting)*/
    case Z0:
        MoveDown(z);
      break;
    case FNT1:
    case FNT2:
    case FNT3:
    case FNT4:
      k = NoSignExtend(dvifp, (int) command - FNT1 + 1);
      /*if (!SkipMode) {*/
        SetFntNum(k, Emitting);
	/*}*/
      break;
    case XXX1:
    case XXX2:
    case XXX3:
    case XXX4:
      k = (int)NoSignExtend(dvifp, (int)command - XXX1 + 1);
      GetBytes(dvifp, SpecialStr, k);
      if (PassNo==1)
	DoSpecial(SpecialStr, k);
      break;
    case FNT_DEF1:
    case FNT_DEF2:
    case FNT_DEF3:
    case FNT_DEF4:
      k = (int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1);
      SkipFontDef();    /* SkipFontDef(k); */
      break;
    case PRE:
      Fatal("PRE occurs within file");
      break;
    case POST:
      AllDone(_FALSE);
      PassNo = 0;
      qfprintf(ERR_STREAM,"\n");
      break;
    case POST_POST:
      Fatal("POST_POST with no preceding POST");
      break;
    case NOP:
      break;
    default:
      Fatal("%d is an undefined command", command);
      break;
    }
  } /* while _TRUE */
}



