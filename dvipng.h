#ifndef DVIPNG_H
#define DVIPNG_H
/**********************************************************************
 ************************  Global Definitions  ************************
 **********************************************************************/
/* #define DRAWGLYPH */
#define TIMING

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

#define  DVIFORMAT     2
#ifndef UNKNOWN
#define  UNKNOWN      -1
#endif
#define  FIRSTFNTCHAR  0

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
#undef CopyFile
#define CopyFile LJCopyFile
#define ResetPrinter LJResetPrinter
#endif

/**********************************************************************/
/********************** Special Data Structures ***********************/
/**********************************************************************/

typedef enum  { None, String, Integer /*, Number, Dimension*/ }


ValTyp;
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
    long4    k, c, s, d;
    int     a, l;
    char n[STRSIZE];          /* FNT_DEF command parameters                */
    long4    font_mag;         /* computed from FNT_DEF s and d parameters  */
    /*char psname[STRSIZE];*/ /* PostScript name of the font               */
    char    name[STRSIZE];    /* full name of PXL file                     */
    FILEPTR font_file_id;      /* file identifier (NO_FILE if none)         */
    long4    magnification;    /* magnification read from PXL file          */
    long4    designsize;       /* design size read from PXL file            */
    struct char_entry ch[NFNTCHARS];   /* character information            */
    struct font_entry *next;
    unsigned short ncdl;      /* #of different chars actually downloaded   */
    unsigned short plusid;    /* Font id in Printer                        */
    bool used_on_this_page;
    enum PxlId {
        id1001, id1002, pk89    } id;
    unsigned short max_width, max_height, max_yoff;
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
/*************************  Global Procedures  ************************/
/**********************************************************************/
/* Note: Global procedures are declared here in alphabetical order, with
   those which do not return values typed "void".  Their bodies occur in
   alphabetical order following the main() procedure.  The names are
   kept unique in the first 6 characters for portability. */

#if NeedFunctionPrototypes
# define DVIPROTO(x) x
#if NeedVarargsPrototypes
# define DVIELI() (char *fmt, ...)
#else
# define DVIELI() ()
#endif
#else
# define DVIPROTO(x) ()
# define DVIELI() ()
#endif

double  ActualFactor DVIPROTO((long4));
void    AllDone DVIPROTO((bool));
void    CloseFiles DVIPROTO((void));
void    DecodeArgs DVIPROTO((int, char *[]));
#ifdef __riscos
void    diagram DVIPROTO((char *, diagtrafo *));
void   *xosfile_set_type DVIPROTO((char *, int));
void    MakeMetafontFile DVIPROTO((char *, char *, int));
#endif
void    DoBop DVIPROTO((void));
long4   DoConv DVIPROTO((long4, long4, int));
void    DoSpecial DVIPROTO((char *, int));
void    EmitChar DVIPROTO((long4, struct char_entry *));
void    Fatal DVIELI();
void    FindPostAmblePtr DVIPROTO((long *));
void    FormFeed DVIPROTO((int));
void    GetFontDef DVIPROTO((void));
char    *GetKeyStr DVIPROTO((char *, KeyWord *));
bool    GetKeyVal DVIPROTO((KeyWord *, KeyDesc[], int, int *));
bool    IsSame DVIPROTO((char *, char *));
void    LoadAChar DVIPROTO((long4, register struct char_entry *));
long4   NoSignExtend DVIPROTO((FILEPTR, int));
void    OpenFontFile DVIPROTO((void));
void    ReadFontDef DVIPROTO((long4));
void    ReadPostAmble DVIPROTO((bool));
void    SetChar DVIPROTO((long4, short, int));
void    SetFntNum DVIPROTO((long4, bool));
void    SetRule DVIPROTO((long4, long4, int, int));
long4   SignExtend DVIPROTO((FILEPTR, int));
void    SkipFontDef DVIPROTO((void));
void    Warning DVIELI();
unsigned char   skip_specials DVIPROTO((void));

void background DVIPROTO((char *));
void bopcolor DVIPROTO((void));
void initcolor DVIPROTO((void));
void popcolor DVIPROTO((void));
void pushcolor DVIPROTO((char *));
void resetcolorstack DVIPROTO((char *));

/* buffer IO */
char   b_read DVIPROTO((FILEPTR));
#ifdef RISC_BUFFER
void   b_write DVIPROTO((FILEPTR, char));
void   b_wrtmult DVIPROTO((FILEPTR, char *, int));
void   b_oflush DVIPROTO((FILEPTR));
#endif


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
EXTERN bool    ManualFeed INIT(_FALSE);
EXTERN long4    FirstPage  INIT(-1000000);  /* first page to print (uses count0)   */
EXTERN long4    LastPage   INIT(1000000);   /* last page to print                  */
EXTERN long4    PrintPages INIT(1000000);   /* nr of pages to print                */
EXTERN bool    FirstPageSpecified INIT(_FALSE);
EXTERN bool    LastPageSpecified INIT(_FALSE);
#ifndef KPATHSEA
EXTERN char   *PXLpath INIT(FONTAREA);
#endif
EXTERN char    G_progname[STRSIZE];     /* program name                        */
EXTERN char    filename[STRSIZE];       /* DVI file name                       */
EXTERN char    rootname[STRSIZE];       /* DVI filename without extension      */
EXTERN char    pngname[STRSIZE];        /* current PNG filename                */
EXTERN char   *HeaderFileName INIT("");     /* file name & path of Headerfile      */
EXTERN char   *EmitFileName INIT("");       /* file name & path for output         */
EXTERN bool    Reverse INIT(_FALSE);        /* process DVI pages in reverse order?   */
EXTERN bool    Landscape INIT(_FALSE);      /* print document in ladscape mode       */
EXTERN bool    ResetPrinter INIT(_TRUE);    /* reset printer at the begin of the job */
EXTERN bool    DoublePage INIT(_FALSE);     /* print on both sides of a paper        */
EXTERN bool    PrintSecondPart INIT(_TRUE); /* print First Part when DoublePage      */
EXTERN bool    PrintFirstPart  INIT(_TRUE); /* print Second Part when DoublePage     */
EXTERN bool    PrintEmptyPages INIT(_TRUE); /* print Empty pages in DoublePage mode  */
EXTERN short   PageParity INIT(1);
#ifdef MAKETEXPK
#ifdef KPATHSEA
EXTERN bool    makeTexPK INIT(MAKE_TEX_PK_BY_DEFAULT);
#else
EXTERN bool    makeTexPK INIT(_TRUE);
#endif
#endif

#ifndef vms
EXTERN short   G_errenc INIT(0);           /* has an error been encountered?      */
#else
EXTERN long4    G_errenc INIT(SS$_NORMAL);  /* has an error been encountered?      */
#endif
EXTERN bool    G_header INIT(_FALSE);      /* copy header file to output?         */
EXTERN bool    G_quiet INIT(_FALSE);       /* for quiet operation                 */
EXTERN bool    G_verbose INIT(_FALSE);     /* inform user about pxl-files used    */
EXTERN bool    G_nowarn INIT(_FALSE);      /* don't print out warnings            */
EXTERN short   x_origin;               /* x-origin in dots                    */
EXTERN short   y_origin;               /* y-origin in dots                    */
EXTERN short   x_goffset;              /* global x-offset in dots             */
EXTERN short   y_goffset;              /* global y-offset in dots             */
EXTERN unsigned short ncopies INIT(1);     /* number of copies to print           */
EXTERN long4    hconv, vconv;           /* converts DVI units to pixels        */
EXTERN long4    den;                    /* denominator specified in preamble   */
EXTERN long4    num;                    /* numerator specified in preamble     */
EXTERN long4    h;                      /* current horizontal position         */
EXTERN long4    hh INIT(0);                 /* current h on device                 */
EXTERN long4    v;                      /* current vertical position           */
EXTERN long4    vv INIT(0);                 /* current v on device                 */
EXTERN long4    mag;                    /* magnification specified in preamble */
EXTERN long     usermag INIT(0);            /* user specified magnification        */
EXTERN int      ndone INIT(0);              /* number of pages converted           */
EXTERN int      nopen INIT(0);              /* number of open PXL files            */
#ifdef vms
EXTERN int	kk;			 /* loop variable for EMITB	       */
#endif
EXTERN FILEPTR outfp INIT(FPNULL);          /* output file                         */
EXTERN FILEPTR pxlfp;                   /* PXL file pointer                    */
EXTERN FILEPTR dvifp  INIT(FPNULL);         /* DVI file pointer                    */
EXTERN struct font_entry *prevfont INIT(NULL); /* font_entry pointer previous font*/
EXTERN struct font_entry *fontptr;      /* font_entry pointer                  */
EXTERN struct font_entry *hfontptr INIT(NULL); /* font_entry pointer              */
EXTERN struct font_entry *pfontptr INIT(NULL); /* previous font_entry pointer     */
EXTERN struct pixel_list pixel_files[MAXOPEN+1]; /* list of open PXL files    */
EXTERN long   postambleptr;            /* Pointer to the postamble            */
EXTERN long   ppagep;                  /* previous page pointer               */
EXTERN long4  StartPrintPages;         /* notpad for double paged output      */
EXTERN int    WouldPrint    INIT(0);
EXTERN bool   ZeroPage INIT(_FALSE);       /* Document starts with a Zero Page    */
EXTERN bool   EvenPage INIT(_FALSE);       /* Document starts with an even Page   */
EXTERN long4  LastPtobePrinted INIT(0);
EXTERN int    G_ncdl INIT(0);

EXTERN long     allocated_storage INIT(0); /* size of mallocated storage (statistics) */
EXTERN long4    power[32] ;
EXTERN long4    gpower[33] ;

EXTERN unsigned char buffin[BUFFSIZE]; /* Input buffer; always used for Copy[HP]File */
EXTERN int binumber INIT(0);            /* number of valid bytes in input buffer */
EXTERN int biact INIT(0);               /* number of next byte to read from input buffer */
#ifdef RISC_BUFFER
EXTERN char buffout[BUFFSIZE];    /* Output buffer; used if RISC_BUFFER defined */
EXTERN int boact INIT(0);               /* number of next byte to write to output buffer */
#endif

#ifdef RISC_USE_OSL
EXTERN char   embuf[STRSIZE];         /* Buffer for emitting stuff */
EXTERN int    emsize;                 /* Number of bytes written in buffer */
#else
# ifdef RISC_BUFFER
EXTERN char   embuf[STRSIZE];
EXTERN int    emsize;
# endif
#endif

#ifdef __riscos
#define DIAGDIRSIZE 32
EXTERN char diagdir[DIAGDIRSIZE] INIT("LJdiag"); /* Prefix name of directory for
					 cached printouts */
EXTERN bool cachediag INIT(_FALSE);       /* cache PDriver's output in document folder */
EXTERN bool printdiag INIT(_TRUE);        /* printf diagrams */
EXTERN FILEPTR metafile INIT(FPNULL);     /* Filepointer of file containing
				  metafont directives*/

EXTERN char MFFileName[STRSIZE];
EXTERN int RasterMultipass INIT(0);
#endif

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
EXTERN double  start_time;
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
# define  XDEFAULTOFF   resolution     /* x default offset on page 1in */
# define  YDEFAULTOFF   resolution     /* y default offset on page 1in */
# define  hconvRESOLUTION   resolution
# define  vconvRESOLUTION   resolution
# define  max(x,y)       if ((y)>(x)) x = y
# define  min(x,y)       if ((y)<(x)) x = y

EXTERN unsigned short v_pgsiz_dots INIT(1169); /* A4 at 100 dpi */
EXTERN unsigned short h_pgsiz_dots INIT(826);  /* A4 at 100 dpi */

EXTERN  short x_min INIT(0); /* Include topleft corner of page */
EXTERN  short y_min INIT(0);
EXTERN  short x_max INIT(0);
EXTERN  short y_max INIT(0);

#define VERSION "0.0 (dvipngk)"

EXTERN gdImagePtr page_imagep INIT(NULL);
EXTERN int shrinkfactor INIT(3);
struct dvi_page {
  struct dvi_page* next;
  int              filepos;
};
EXTERN char TeXcomment[STRSIZE];  

EXTERN int Red    INIT(0);
EXTERN int Green  INIT(0);
EXTERN int Blue   INIT(0);
EXTERN int bRed   INIT(255);
EXTERN int bGreen INIT(255);
EXTERN int bBlue  INIT(255);

#endif /* DVIPNG_H */
