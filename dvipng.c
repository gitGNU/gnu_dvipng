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

struct stack_entry {  /* stack entry */
  long4    h, v, w, x, y, z;  /* what's on stack */
};
short   command;           /* current command                         */
long4   count[10];         /* the 10 counters at begining of each page*/
long4   cpagep = 0;        /* current page pointer                    */
long4   w = 0;             /* current horizontal spacing              */
long4   x = 0;             /* current horizontal spacing              */
long4   y = 0;             /* current vertical spacing                */
long4   z = 0;             /* current vertical spacing                */
int     i;                 /* command parameter; loop index           */
int     k;                 /* temporary parameter                     */
char    n[STRSIZE];        /* command parameter                       */
int     sp = 0;            /* stack pointer                           */
struct  stack_entry stack[STACK_SIZE];  /* stack                      */
char    SpecialStr[STRSIZE]; /* "\special" strings                    */
long4   val, val2;         /* temporarys to hold command information  */
long4 abspagenumber=0;

struct page_list *InitPage AA((void));

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
  int     PassNo = 0;        /* which pass over the DVI page are we on? */

  /* Initialize pixel_files */   
  for (i = 0; i <= MAXOPEN; i++)
    pixel_files[i].pixel_file_id = FPNULL;

  setbuf(ERR_STREAM, NULL);
  (void) strcpy(G_progname, argv[0]);
#ifdef KPATHSEA
  kpse_set_progname(argv[0]);
  kpse_set_program_enabled (kpse_pk_format, makeTexPK, kpse_src_compile);
#endif

  initcolor();

  DecodeArgs(argc, argv);

#ifdef KPATHSEA
  kpse_init_prog("DVIPNG", resolution, MFMODE, "cmr10");
#endif


#ifdef TIMING
#ifdef BSD_TIME_CALLS
  ftime(&timebuffer);
  start_time = timebuffer.time + (float)(timebuffer.millitm) / 1000.0;
#else
  gettimeofday(&Tp, NULL);
  start_time = Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0;
#endif
#endif

  if (!ParseStdin) {
    /* If pages not given on commandline, insert all pages */
    if (QueueEmpty()) {
      if (!Reverse) {
	QueuePage(1,MAXPAGE,_TRUE);
      } else {
	struct page_list *tpagelistp;
	
	tpagelistp=hpagelistp;
	while(tpagelistp!=NULL) {
	  SkipPage();
	  tpagelistp->next=InitPage();
	  tpagelistp=tpagelistp->next;
	}
	QueuePage(1,abspagenumber,_TRUE);
      }
    }
    DoPages();
  } else {
    char    line[STRSIZE];
    char    *linev[3];
    
    fgets(line,STRSIZE,stdin);
    while(!FEOF(stdin)) {
      linev[0]=line;  /* OBSERVE linev[0] is never used in DecodeArgs */
      while( *linev[0]!='\n' ) linev[0]++;
      *linev[0]='\0';
      linev[1]=line;
      linev[2]=line+2;
      while( *linev[2]!=' ' && *linev[2]!='\0' ) linev[2]++;
      while( *linev[2]==' ') *(linev[2]++)='\0';
      if (*linev[2]=='\0') {
	DecodeArgs(2,linev);
      } else {
	DecodeArgs(3,linev);
      }
      if (!QueueEmpty()) 
	DoPages();
      fgets(line,STRSIZE,stdin);
    }
  }

  AllDone(_FALSE);
  qfprintf(ERR_STREAM,"\n");
  exit(0);
}


void DoPages P1H(void)
{
  struct page_list *tpagelistp;

  while((tpagelistp=FindPage(TodoPage()))!=NULL) {
    if (PassDefault == PASS_BBOX) {
      DrawPage(PASS_BBOX);
      /* Reset to after BOP of current page */
      FSEEK(dvifp, tpagelistp->offset+45, SEEK_SET); 
      x_width = x_max-x_min;
      y_width = y_max-y_min;
      x_offset = -x_min; /* offset by moving topleft corner */
      y_offset = -y_min; 
      x_max = x_min = -x_offset_def; /* reset BBOX */
      y_max = y_min = -y_offset_def;
    }
    qfprintf(ERR_STREAM,"[%ld",  tpagelistp->count[0]);
    DoBop();
    DrawPage(PASS_DRAW);
    FormFeed(tpagelistp->count[0]);
    ++ndone;
    qfprintf(ERR_STREAM,"] ");
    if (tpagelistp->next==NULL) 
      tpagelistp->next=InitPage();
  }
}


struct page_list* FindPage P1C(long4,pagenum)
{
  struct page_list* tpagelistp;
  int index;
  
  if (pagenum==MAXPAGE)
    return(NULL);

  index = Abspage ? 10 : 0 ;

  tpagelistp = hpagelistp;
  /* Check if page is in list (stop on last page in list anyway) */
  while(tpagelistp->next!=NULL && tpagelistp->count[index]!=pagenum) {
    tpagelistp = tpagelistp->next;
  }
  FSEEK(dvifp, tpagelistp->offset+45L, SEEK_SET);
#ifdef DEBUG
  if (Debug)
    printf("FIND PAGE@%ld:\t%ld\t(%ld)\n", tpagelistp->offset,
	     tpagelistp->count[0],tpagelistp->count[10]);
#endif

  /* If page not yet found, skip in file until found */
  while(tpagelistp!=NULL && tpagelistp->count[index]!=pagenum) {
    SkipPage();
    tpagelistp->next=InitPage();
    tpagelistp = tpagelistp->next;
  }
  return(tpagelistp);
}

struct page_list* InitPage P1H(void)
{
  /* Find page start, return pointer to page_list entry if found */
  struct page_list* tpagelistp=NULL;


  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("INIT CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif

  while((command != BOP) && (command != POST)) {
    switch(command) {
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      k = (int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1);
      ReadFontDef(k);
      break;
    case NOP: case BOP: case POST:
      break;
    default:
      Fatal("DVI file has non-allowed command %d between pages.",command);
    }
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("INIT CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  }
  if ( command == BOP ) {  /*  Init page */
    if ((tpagelistp = malloc(sizeof(struct page_list)))==NULL)
      Fatal("cannot allocate memory for new page entry");
    tpagelistp->offset = (long) FTELL(dvifp) - 1;
    for (i = 0; i <= 9; i++) {
      tpagelistp->count[i] = NoSignExtend(dvifp, 4);
    }
    tpagelistp->count[10] = ++abspagenumber;
#ifdef DEBUG
    if (Debug)
      printf("INIT PAGE@%ld:\tcount0=%ld\t(absno=%ld)\n", tpagelistp->offset,
	     tpagelistp->count[0],abspagenumber);
#endif
    (void)NoSignExtend(dvifp, 4);
    h = v = w = x = y = z = 0;
    sp = 0;
    fontptr = NULL;
    prevfont = NULL;
    tpagelistp->next = NULL;
  }
  return(tpagelistp);
}

void EmptyPageList P1H(void)
{
  struct page_list* temp;

  temp=hpagelistp;
  while(hpagelistp!=NULL) {
    hpagelistp=hpagelistp->next;
    free(temp);
    temp=hpagelistp;
  }
  abspagenumber=0;
}

void SkipPage P1H(void)
{
  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("SKIP CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  while (command != EOP)  {
    if ((command > SETC_127 && command < FONT_00) 
	|| command > FONT_63) {
      switch (command)  {
      case PUT1: case SET1: case RIGHT1: case W1: case X1:
      case DOWN1: case Y1: case Z1: case FNT1:
	FSEEK(dvifp,1,SEEK_CUR);
	break;
      case PUT2: case SET2: case RIGHT2: case W2: case X2:
      case DOWN2: case Y2: case Z2: case FNT2:
	FSEEK(dvifp,2,SEEK_CUR);
	break;
      case PUT3: case SET3: case RIGHT3: case W3: case X3:
      case DOWN3: case Y3: case Z3: case FNT3:
	FSEEK(dvifp,3,SEEK_CUR);
	break;
      case PUT4: case SET4: case RIGHT4: case W4: case X4:
      case DOWN4: case Y4: case Z4: case FNT4:
	FSEEK(dvifp,4,SEEK_CUR);
	break;
      case SET_RULE: case PUT_RULE:
	FSEEK(dvifp,8,SEEK_CUR);
	break;
      case EOP: case PUSH: case POP: case W0: case X0:
      case Y0: case Z0: case NOP:
	break;
      case XXX1: case XXX2: case XXX3: case XXX4:
	k = (int)NoSignExtend(dvifp, (int)command - XXX1 + 1);
	FSEEK(dvifp, k,SEEK_CUR);
	break;
      case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
	k = (int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1);
	ReadFontDef(k);
	break;
      case BOP:
	Fatal("BOP occurs within page");
	break;
      case PRE:
	Fatal("PRE occurs within page");
	break;
      case POST:
	Fatal("POST occurs within page");
	break;
      case POST_POST:
	Fatal("POST_POST occurs within page");
	break;
      default:
	Fatal("%d is an undefined command", command);
	break;
      }
    } /* if */
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("SKIP CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  } /* while */
}

void DrawPage P1C(int, PassNo) 
{
  command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
  if (Debug)
    printf("DRAW CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  while (command != EOP)  {
    if (/*command >= SETC_000 &&*/ command <= SETC_127) {
      val=SetChar(command,PassNo);
      MoveOver(val);
    } else if (command >= FONT_00 && command <= FONT_63) {
      SetFntNum((long4)command - FONT_00);
    } else switch (command)  {
    case PUT1: case PUT2: case PUT3: case PUT4:
      val = NoSignExtend(dvifp, (int)command - PUT1 + 1);
      (void) SetChar(val,PassNo);
      break;
    case SET1: case SET2: case SET3: case SET4:
      val = NoSignExtend(dvifp, (int)command - SET1 + 1);
      val2 = SetChar(val,PassNo);
      MoveOver(val2);
      break;
    case SET_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      val = SetRule(val, val2, PassNo);
      MoveOver(val);
      break;
    case PUT_RULE:
      val = NoSignExtend(dvifp, 4);
      val2 = NoSignExtend(dvifp, 4);
      SetRule(val, val2, PassNo);
      break;
    case BOP:
      Fatal("BOP occurs within page");
      break;
    case EOP:
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
    case RIGHT1: case RIGHT2: case RIGHT3: case RIGHT4:
      val = SignExtend(dvifp, (int)command - RIGHT1 + 1);
      MoveOver(val);
      break;
    case W1: case W2: case W3: case W4:
      w = SignExtend(dvifp, (int)command - W1 + 1);
    case W0:
      MoveOver(w);
      break;
    case X1: case X2: case X3: case X4:
      x = SignExtend(dvifp, (int)command - X1 + 1);
    case X0:
      MoveOver(x);
      break;
    case DOWN1: case DOWN2: case DOWN3: case DOWN4:
      val = SignExtend(dvifp, (int)command - DOWN1 + 1);
      MoveDown(val);
      break;
    case Y1: case Y2: case Y3: case Y4:
      y = SignExtend(dvifp, (int)command - Y1 + 1);
    case Y0:
      MoveDown(y);
      break;
    case Z1: case Z2: case Z3: case Z4:
      z = SignExtend(dvifp, (int)command - Z1 + 1);
    case Z0:
      MoveDown(z);
      break;
    case FNT1: case FNT2: case FNT3: case FNT4:
      k = NoSignExtend(dvifp, (int) command - FNT1 + 1);
      SetFntNum(k);
      break;
    case XXX1: case XXX2: case XXX3: case XXX4:
      k = (int)NoSignExtend(dvifp, (int)command - XXX1 + 1);
      GetBytes(dvifp, SpecialStr, k);
      if ( PassNo == PASS_DRAW)
	DoSpecial(SpecialStr, k);
      break;
    case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
      k = (int)NoSignExtend(dvifp, (int)command - FNT_DEF1 + 1);
      ReadFontDef(k);
      break;
    case PRE:
      Fatal("PRE occurs within page");
      break;
    case POST:
      Fatal("POST occurs within page");
      break;
    case POST_POST:
      Fatal("POST_POST occurs within page");
      break;
    case NOP:
      break;
    default:
      Fatal("%d is an undefined command", command);
      break;
    }
    command = (short) NoSignExtend(dvifp, 1);
#ifdef DEBUG
    if (Debug)
      printf("DRAW CMD@%ld:\t%d\n", (long) ftell(dvifp) - 1, command);
#endif
  } /* while  */
  /* end of page */
}
