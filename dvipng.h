#ifndef DVIPNG_H
#define DVIPNG_H

#define VERSION "0.0 (dvipngk)"
#define TIMING
/* #define DRAWGLYPH */

#ifdef KPATHSEA
#include <kpathsea/config.h>
#include <kpathsea/c-auto.h>
#include <kpathsea/c-limits.h>
#include <kpathsea/c-memstr.h>
#include <kpathsea/magstep.h>
#include <kpathsea/proginit.h>
#include <kpathsea/progname.h>
#include <kpathsea/tex-glyph.h>
#include <kpathsea/tex-hush.h>
#include <kpathsea/tex-make.h>
#include <kpathsea/c-vararg.h>
#else
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef  unix
#include <limits.h>
#endif
#endif

#include <gd.h>

#include <signal.h>
#include <ctype.h>
#ifdef vms
#include <file.h>
#else
# ifndef __riscos
# include <fcntl.h>
# endif
#endif
#ifdef MSC5
#include <dos.h>     /* only for binaryopen on device  */
#endif
#if defined (unix) && !defined (KPATHSEA)
#include <limits.h>
#endif

#include "config.h"
#include "commands.h"

#define  FIRSTFNTCHAR  0

/* Is any of the below really necessary? */
#ifdef __riscos
# ifdef RISC_USE_OSL
#  define MAXOPEN_OS    16
# else
#  define MAXOPEN_OS    8      /* don't know if this IS the maximum */
# endif
#else
# ifdef   OPEN_MAX                    /* ... in a friendly unix system  */
#  ifndef vms
#   define MAXOPEN_OS (OPEN_MAX - 8)
#  else
#   define  MAXOPEN_OS 12     /* OPEN_MAX seems to be 8 on VMS systems */
#  endif
# else
#  ifdef __DJGPP__
#   if __DJGPP_MINOR__ <= 1
    /* DJGPP needs few handles free in the first 20, or else child programs
       (called by MakeTeX... scripts) won't run, since the stub loader
       cannot open the .exe program.  This is because DOS only copies the
       first 20 handles to the child program.  */
#    define MAXOPEN_OS   5
#   else
    /* DJGPP v2.02 and later works around this.  Assume they have at least
       FILES=30 in their CONFIG.SYS (everybody should).  */
#    define MAXOPEN_OS  20
#   endif
#  else
#   define  MAXOPEN_OS  12     /* limit on number of open font files */
#  endif
# endif
#endif

#define  MAXOPEN       30 /*MAXOPEN_OS*/

#define  NFNTCHARS       LASTFNTCHAR+1
#define  STACK_SIZE      100     /* DVI-stack size                     */
#define  NONEXISTANT     -1      /* offset for PXL files not found     */
#ifdef RISC_USE_OSL
# define  NO_FILE        (FPNULL-1)
#else
# define  NO_FILE        ((FILE *)-1)
#endif
#define  NEW(A) ((A *)  malloc(sizeof(A)))
#define  EQ(a,b)        (strcmp(a,b)==0)
#define  MM_TO_PXL(x)   (int)(((x)*resolution*10)/254)
#define  PT_TO_PXL(x)   (int)((long4)((x)*resolution*100l)/7224)
#define  PT_TO_DVI(x)   (long4)((x)*65536l)

#define PK_POST 245
#define PK_PRE 247
#define PK_ID 89

/*#define PIXROUND(x,c) ((((double)x+(double)(c>>1))/(double)c)+0.5)*/
/*#define PIXROUND(x,c) (((x)+c)/(c))*/
#define PIXROUND(x,c) ((x+c-1)/(c))

/*************************************************************************/

#define  MoveOver(b)  h += (long4) b
#define  MoveDown(a)  v += (long4) a
#define  qfprintf if (!G_quiet) fprintf
#define  qprintf  if (!G_quiet) printf

#define GetBytes(fp,buf,n) read_multi(buf,1,n,fp) /* used to be a function */

/**********************************************************************/
/***********************  external definitions  ***********************/
/**********************************************************************/

#ifndef WIN32
#ifndef _AMIGA
# ifndef unix
#  if NeedFunctionPrototypes
long    access(char *, int);      /* all the other ones known under RISC OS */
#  else
long    access();
#  endif
#  ifndef __riscos
void    exit();
int     fclose();
int     fprintf();
int     fseek();
/*char   *index();*/
int     printf();
int     sscanf();
int     strcmp();
char   *strcpy();
#   ifdef MSC5
unsigned int strlen();
#   endif
void    free();
void    setbuf();
#  endif

#  ifdef MSC5
int     intdos();
#  endif
# endif
#endif
#else /* WIN32 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#endif

/**********************************************************************/
/********************** Special Data Structures ***********************/
/**********************************************************************/

typedef enum  { None, String, Integer /*, Number, Dimension*/ } ValTyp;

typedef struct {
  char    *Key;       /* the keyword string */
  char    *Val;       /* the value string */
  ValTyp  vt;         /* the value type */
  union {         /* the decoded value */
    int     i;
    float   n;
  } v;
} KeyWord;

typedef struct {
  char    *Entry;
  ValTyp  Typ;
} KeyDesc;

/**********************************************************************/
/***********************  Font Data Structures  ***********************/
/**********************************************************************/

struct char_entry {             /* character entry */
  unsigned short  width, height;      /* width and height in pixels */
  short   xOffset, yOffset, yyOffset; /* x offset and y offset in pixels*/
  bool isloaded;
  long4    fileOffset;
  long4    tfmw;             /* TFM width                 */
  long4    cw;               /* character width in pixels */
  unsigned char   flag_byte;          /* for PK-files    */
  unsigned char   charsize;
  gdFont  glyph;
};

struct font_entry {    /* font entry */
  struct font_entry *next;
  bool in_use;
  long4  k, c, s, d;
  int     a, l;
  char n[STRSIZE];          /* FNT_DEF command parameters                */
  long4    font_mag;         /* computed from FNT_DEF s and d parameters  */
  /*char psname[STRSIZE];*/ /* PostScript name of the font               */
  char    name[STRSIZE];    /* full name of PXL file                     */
  FILEPTR font_file_id;      /* file identifier (NO_FILE if none)         */
  long4    magnification;    /* magnification read from PXL file          */
  long4    designsize;       /* design size read from PXL file            */
  struct char_entry ch[NFNTCHARS];   /* character information            */
  enum PxlId {
    id1001, id1002, pk89    } id;
};


struct pixel_list {
    FILEPTR pixel_file_id;    /* file identifier  */
    int     use_count;        /* count of "opens" */
};

#ifdef __riscos
typedef struct {
  int scalex;
  int scaley;
  int cropl;
  int cropb;
  int cropr;
  int cropt;
} diagtrafo;                  /* to be passed to diagrams */
#endif

/**********************************************************************/
/***********************  Page Data Structures  ***********************/
/**********************************************************************/

struct page_list {
  struct page_list* next;
  long4 offset;            /* file offset to BOP */
  long4 count[11];        /* 10 dvi counters + absolute pagenum in file */
};

/**********************************************************************/
/*************************  Global Procedures  ************************/
/**********************************************************************/
/* Note: Global procedures are declared here in alphabetical order, with
   those which do not return values typed "void".  Their bodies occur in
   alphabetical order following the main() procedure.  The names are
   kept unique in the first 6 characters for portability. */

 /* To Prototype or not to prototype ? */
#ifdef NeedFunctionPrototypes

#define AA(args) args /* For an arbitrary number; ARGS must be in parens.  */
#if NeedVarargsPrototypes
# define VA() (char *fmt, ...)
#else
# define VA() ()
#endif

#define P1H(p1) (p1)
#define P2H(p1,p2) (p1, p2)
#define P3H(p1,p2,p3) (p1, p2, p3)
#define P4H(p1,p2,p3,p4) (p1, p2, p3, p4)
#define P5H(p1,p2,p3,p4,p5) (p1, p2, p3, p4, p5)
#define P6H(p1,p2,p3,p4,p5,p6) (p1, p2, p3, p4, p5, p6)

#define P1C(t1,n1)(t1 n1)
#define P2C(t1,n1, t2,n2)(t1 n1, t2 n2)
#define P3C(t1,n1, t2,n2, t3,n3)(t1 n1, t2 n2, t3 n3)
#define P4C(t1,n1, t2,n2, t3,n3, t4,n4)(t1 n1, t2 n2, t3 n3, t4 n4)
#define P5C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5) \
  (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5)
#define P6C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6) \
  (t1 n1, t2 n2, t3 n3, t4 n4, t5 n5, t6 n6)

#else /* no function prototypes */

#define AA(args) ()
#define VA() ()

#define P1H(p1) ()
#define P2H(p1, p2) ()
#define P3H(p1, p2, p3) ()
#define P4H(p1, p2, p3, p4) ()
#define P5H(p1, p2, p3, p4, p5) ()
#define P6H(p1, p2, p3, p4, p5, p6) ()

#define P1C(t1,n1) (n1) t1 n1;
#define P2C(t1,n1, t2,n2) (n1,n2) t1 n1; t2 n2;
#define P3C(t1,n1, t2,n2, t3,n3) (n1,n2,n3) t1 n1; t2 n2; t3 n3;
#define P4C(t1,n1, t2,n2, t3,n3, t4,n4) (n1,n2,n3,n4) \
  t1 n1; t2 n2; t3 n3; t4 n4;
#define P5C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5) (n1,n2,n3,n4,n5) \
  t1 n1; t2 n2; t3 n3; t4 n4; t5 n5;
#define P6C(t1,n1, t2,n2, t3,n3, t4,n4, t5,n5, t6,n6) (n1,n2,n3,n4,n5,n6) \
  t1 n1; t2 n2; t3 n3; t4 n4; t5 n5; t6 n6;

#endif /* function prototypes */

void    CloseFiles AA((void));
void    DecodeArgs AA((int, char *[]));
/*#ifdef __riscos
void    diagram AA((char *, diagtrafo *));
void   *xosfile_set_type AA((char *, int));
void    MakeMetafontFile AA((char *, char *, int));
#endif*/
void    DoBop AA((void));
long4   DoConv AA((long4, long4, int));
void    DoPages AA((void));
void    DoSpecial AA((char *, int));
void    DelPageList AA((void));
void    Fatal VA();
struct page_list *FindPage AA((long4));
void    FormFeed AA((int));
void    GetFontDef AA((void));
struct page_list *InitPage AA((void));
void    LoadAChar AA((long4, register struct char_entry *));
long4   NoSignExtend AA((FILEPTR, int));
void    OpenFontFile AA((void));
bool    QueueParse AA((char*,bool));
bool    QueueEmpty AA((void));
void    QueuePage AA((int,int,bool));
void    ReadFontDef AA((long4));
long4   SetChar AA((long4, int));
void    SetFntNum AA((long4));
long4   SetRule AA((long4, long4, int));
void    SkipPage AA((void));
long4   SignExtend AA((FILEPTR, int));
int     TodoPage AA((void));
void    Warning VA();
/*unsigned char   skip_specials AA((void));*/

void handlepapersize AA((char*,int*,int*));

void background AA((char *));
void initcolor AA((void));
void popcolor AA((void));
void pushcolor AA((char *));
void resetcolorstack AA((char *));

/**********************************************************************/
/*************************  Global Variables  *************************/
/**********************************************************************/
#ifdef MAIN
#define EXTERN
#define INIT(x) =x
#else
#define EXTERN extern
#define INIT(x)
#endif
#define MAXPAGE 1000000000 /* assume no pages out of this range */
EXTERN long4    FirstPage  INIT(-MAXPAGE);  /* first page to print (count0) */
EXTERN long4    LastPage   INIT(MAXPAGE);   /* last page to print           */
EXTERN bool    FirstPageSpecified INIT(_FALSE);
EXTERN bool    LastPageSpecified INIT(_FALSE);
#ifndef KPATHSEA
EXTERN char   *PXLpath INIT(FONTAREA);
#endif
EXTERN char    G_progname[STRSIZE];     /* program name                     */
EXTERN char    filename[STRSIZE];       /* DVI file name                    */
EXTERN char    rootname[STRSIZE];       /* DVI filename without extension   */
EXTERN char    pngname[STRSIZE];        /* current PNG filename             */

EXTERN bool    Reverse INIT(_FALSE);    /* process DVI in reverse order?    */
EXTERN bool    Landscape INIT(_FALSE);  /* print document in ladscape mode  */
EXTERN bool    Abspage INIT(_FALSE);    /* use absolute page numbers        */
EXTERN int     Firstseq INIT(0);        /* FIXME       */
EXTERN int     Lastseq INIT(1000);      /* FIXME       */
#ifdef MAKETEXPK
#ifdef KPATHSEA
EXTERN bool    makeTexPK INIT(MAKE_TEX_PK_BY_DEFAULT);
#else
EXTERN bool    makeTexPK INIT(_TRUE);
#endif
#endif

EXTERN short   G_errenc INIT(0);        /* has an error been encountered?  */
EXTERN bool    G_quiet INIT(_FALSE);    /* for quiet operation             */
EXTERN bool    G_verbose INIT(_FALSE);  /* inform user about pxl-files used*/
EXTERN bool    G_nowarn INIT(_FALSE);   /* don't print out warnings        */
EXTERN long4    hconv, vconv;           /* converts DVI units to pixels    */
EXTERN long4    den;                    /* denominator specified in preamble*/
EXTERN long4    num;                    /* numerator specified in preamble */
EXTERN long4    h;                      /* current horizontal position     */
EXTERN long4    v;                      /* current vertical position       */
EXTERN long4    mag;                    /* magstep specified in preamble   */
EXTERN long     usermag INIT(0);        /* user specified magstep          */
EXTERN int      ndone INIT(0);          /* number of pages converted       */
EXTERN int      nopen INIT(0);          /* number of open PXL files        */
EXTERN FILEPTR outfp INIT(FPNULL);      /* output file                     */
EXTERN FILEPTR pxlfp;                   /* PXL file pointer                */
EXTERN FILEPTR dvifp  INIT(FPNULL);     /* DVI file pointer                */
EXTERN struct font_entry *hfontptr INIT(NULL); /* font list pointer        */
EXTERN struct font_entry *fontptr;      /* font_entry pointer              */
EXTERN struct font_entry *pfontptr INIT(NULL); /* previous font_entry      */
EXTERN struct pixel_list pixel_files[MAXOPEN+1]; /* list of open PXL files */

EXTERN int    G_ncdl INIT(0);

EXTERN long     allocated_storage INIT(0); /* size of mallocated storage (statistics) */

#ifdef DEBUG
EXTERN int Debug INIT(0);
#define DEBUG_START() do { if (Debug) {
#define DEBUG_END()        fflush (stdout); } } while (0)
#define DEBUG_PRINT(str)						\
  DEBUG_START (); fputs (str, stdout); DEBUG_END ()
#define DEBUG_PRINT1(str, e1)						\
  DEBUG_START (); printf (str, e1); DEBUG_END ()
#else
#define DEBUG_PRINT(str)
#define DEBUG_PRINT1(str, e1)
#endif

EXTERN long     used_fontstorage INIT(0);

/************************timing stuff*********************/
#ifdef TIMING
# ifdef __riscos
#  include <sys/times.h>
# else
#  include <sys/time.h>
EXTERN struct timeval Tp;
EXTERN double timer;
# endif
EXTERN double my_tic,my_toc INIT(0);
#define TIC() { gettimeofday(&Tp, NULL); \
    my_tic= (float)Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0;}
#define TOC() { gettimeofday(&Tp, NULL); \
    my_toc += ((float)Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0) - my_tic;}
#else
#define TIC()
#define TOC()
#endif /* TIMING */

EXTERN int   resolution INIT(300);
EXTERN char *MFMODE     INIT("cx");
# define  hconvRESOLUTION   resolution
# define  vconvRESOLUTION   resolution
# define  max(x,y)       if ((y)>(x)) x = y
# define  min(x,y)       if ((y)<(x)) x = y

/* Used in PASS_BBOX: Include topleft corner of page */
EXTERN  int x_min INIT(0); 
EXTERN  int y_min INIT(0);
EXTERN  int x_max INIT(0);
EXTERN  int y_max INIT(0);

/* Page size: set by -T _or_ in PASS_BBOX */
EXTERN  int x_width INIT(0); 
EXTERN  int y_width INIT(0);

/* Offset: default set by -O, or/and in PASS_BBOX */
EXTERN  int x_offset INIT(0);
EXTERN  int y_offset INIT(0);
EXTERN  int x_offset_def INIT(0);
EXTERN  int y_offset_def INIT(0);

/* Paper size: set by -t, for cropmark purposes only */
/* This has yet to be written */
EXTERN  int x_pwidth INIT(0); 
EXTERN  int y_pwidth INIT(0);

/* The transparent border preview-latex desires */
EXTERN  int borderwidth INIT(0);


EXTERN gdImagePtr page_imagep INIT(NULL);
EXTERN int shrinkfactor INIT(3);
EXTERN char TeXcomment[STRSIZE];  

EXTERN int Red    INIT(0);
EXTERN int Green  INIT(0);
EXTERN int Blue   INIT(0);
EXTERN int bRed   INIT(255);
EXTERN int bGreen INIT(255);
EXTERN int bBlue  INIT(255);

#define PASS_SKIP 0
#define PASS_BBOX 1
#define PASS_DRAW 2
EXTERN int PassDefault INIT(PASS_BBOX);

EXTERN bool ParseStdin INIT(_FALSE);

EXTERN struct page_list* hpagelistp INIT(NULL);
EXTERN long4 abspagenumber INIT(0);

#endif /* DVIPNG_H */
