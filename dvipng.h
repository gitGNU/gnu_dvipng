#ifndef DVIPNG_H
#define DVIPNG_H

#define VERSION "0.0 (dvipngk)"
#define TIMING
/* #define DRAWGLYPH */

#include <stdint.h>

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

/*#include "config.h"*/
#define  STRSIZE         255     /* stringsize for file specifications  */

#define  FIRSTFNTCHAR  0
#define  LASTFNTCHAR  255

typedef  int     bool;
#define  _TRUE      (bool) 1
#define  _FALSE     (bool) 0
#define  UNKNOWN     -1

/* name of the program which is called to generate missing pk files */
#define MAKETEXPK "mktexpk"

# define ERR_STREAM stdout   /* ???? */

/* end of "config.h" */

/*************************************************************/
/*************************  protos.h  ************************/
/*************************************************************/

#ifdef __STDC__
#define NeedFunctionPrototypes 1
#include <stdarg.h>
#else
#define NeedFunctionPrototypes 0
#include <varargs.h>
#endif

#if NeedFunctionPrototypes
#define NeedVarargsPrototypes 1
#endif

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
#define  PT_TO_PXL(x)   (int)((int32_t)((x)*resolution*100l)/7224)
#define  PT_TO_DVI(x)   (int32_t)((x)*65536l)

/*#define PIXROUND(x,c) ((((double)x+(double)(c>>1))/(double)c)+0.5)*/
/*#define PIXROUND(x,c) (((x)+c)/(c))*/
#define PIXROUND(x,c) ((x+c-1)/(c))

/*************************************************************************/

#define  MoveOver(b)  h += (int32_t) b
#define  MoveDown(a)  v += (int32_t) a
#define  qfprintf if (!G_quiet) fprintf
#define  qprintf  if (!G_quiet) printf

/********************************************************/
/********************** special.h ***********************/
/********************************************************/

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

void    DoSpecial AA((char *, int));

/********************************************************/
/***********************  font.h  ***********************/
/********************************************************/

#define FONT_TYPE_PK            1
struct pk_char {                   /* PK character */
  unsigned char* mmap;             /* Points to beginning of PK data    */
  uint32_t       length;           /* Length of PK data                 */
  int32_t        tfmw;             /* TFM width                         */
  unsigned char  flag_byte;        /* PK flagbyte                       */
  gdFont         glyph;            /* contains width and height in
		                    * pixels, number of greylevels and
		                    * the bitmaps for the greylevels
				    */
  short          xOffset, yOffset; /* x offset and y offset in pixels   */
};

#define FONT_TYPE_VF            2
struct vf_char {                   /* VF character                     */
  unsigned char* mmap;             /* Points to beginning of VF macro  */
  uint32_t       length;           /* Length of VF macro               */
  int32_t        tfmw;             /* TFM width                        */
};

struct font_entry {    /* font entry */
  int          type;            /* PK/VF/Type1 ...                   */
  struct font_entry *next;
  uint32_t     c, s, d;                                                
  uint8_t      a, l;                                                   
  char         n[STRSIZE];      /* FNT_DEF command parameters        */
  uint32_t     font_mag;        /* computed from s and d             */
  char         name[STRSIZE];   /* full name of PK/VF file           */
  int          filedes;         /* file descriptor                   */
  unsigned char* mmap;          /* memory map                        */
  uint32_t     magnification;   /* magnification read from font file */
  uint32_t     designsize;      /* design size read from font file   */
  union {                       /* character information             */ 
    struct pk_char *pk_ch[NFNTCHARS];  
    struct vf_char *vf_ch[NFNTCHARS]; 
  };
  struct font_num *vffontnump;  /* VF local font numbering           */
  int32_t      defaultfont;     /* VF default font number            */
};

struct font_num {    /* Font number. Different for VF/DVI, and several
			font_num can point to one font_entry */
  struct font_num   *next;
  int32_t            k;
  struct font_entry *fontp;
};

void    CheckChecksum AA((unsigned, unsigned, const char*));
void    InitPK  AA((struct font_entry *));
void    InitVF  AA((struct font_entry *));

/********************************************************/
/***********************  dvi.h  ************************/
/********************************************************/

#define DVI_TYPE            0
struct dvi_data {    /* dvi entry */
  int          type;            /* This is a DVI                    */
  struct dvi_data *next;
  uint32_t     num, den, mag;   /* PRE command parameters            */
  int32_t     conv;             /* computed from num and den         */
  char         name[STRSIZE];   /* full name of DVI file             */
  char         outname[STRSIZE];/* output filename (basename)        */
  FILE *       filep;           /* file pointer                      */
  struct font_num  *fontnump;   /* DVI font numbering                */
  struct page_list *pagelistp;  /* DVI page list                     */
};

struct dvi_vf_entry {
  union {
    struct dvi_data;
    struct font_entry;
  };
};

struct page_list {
  struct page_list* next;
  int     offset;           /* file offset to BOP */
  int32_t count[11];        /* 10 dvi counters + absolute pagenum in file */
};

uint32_t         CommandLength AA((unsigned char*)); 
struct dvi_data* DVIOpen AA((char*));
unsigned char*   DVIGetCommand AA((struct dvi_data*));
void             DVIClose AA((struct dvi_data*));


/***************** general crap ******************/

void    CloseFiles AA((void));
void    DecodeArgs AA((int, char *[]));
/*#ifdef __riscos
void    diagram AA((char *, diagtrafo *));
void   *xosfile_set_type AA((char *, int));
void    MakeMetafontFile AA((char *, char *, int));
#endif*/
void    DoBop AA((void));
void    DrawCommand AA((unsigned char*, int, struct dvi_vf_entry*));
void    DoPages AA((void));
void    Fatal VA();
struct page_list *FindPage AA((int32_t));
void    FormFeed AA((struct dvi_data*,int));
void    FontDef AA((unsigned char*, struct dvi_vf_entry*));
void    FontFind AA((struct font_entry *));
void    GetFontDef AA((void));
struct page_list *InitPage AA((void));
void    LoadAChar AA((int32_t, register struct pk_char *));
uint32_t   NoSignExtend AA((FILE*, int));
void       OpenFont AA((struct font_entry *));
bool       QueueParse AA((char*,bool));
bool       QueueEmpty AA((void));
void       QueuePage AA((int,int,bool));
int32_t    SetChar AA((int32_t, int));
int32_t    SetPK AA((int32_t, int));
int32_t    SetVF AA((int32_t, int));
void    SetFntNum AA((int32_t, struct dvi_vf_entry*));
int32_t   SetRule AA((int32_t, int32_t, int));
int32_t   SignExtend AA((FILE*, int));
void    SkipPage AA((void));
int32_t   SNumRead AA((unsigned char*, register int));
int32_t   TodoPage AA((void));
uint32_t   UNumRead AA((unsigned char*, register int));
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

#include "commands.h"

#define MAXPAGE INT32_MAX               /* assume no pages out of this range */
#define MINPAGE INT32_MIN
EXTERN int32_t    FirstPage  INIT(MINPAGE);  /* first page to print (count0) */
EXTERN int32_t    LastPage   INIT(MAXPAGE);  /* last page to print           */
EXTERN bool    FirstPageSpecified INIT(_FALSE);
EXTERN bool    LastPageSpecified INIT(_FALSE);
#ifndef KPATHSEA
EXTERN char   *PXLpath INIT(FONTAREA);
#endif
EXTERN char    G_progname[STRSIZE];     /* program name                     */
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
EXTERN int32_t     h;                   /* current horizontal position     */
EXTERN int32_t     v;                   /* current vertical position       */
EXTERN int32_t     w INIT(0);           /* current horizontal spacing      */
EXTERN int32_t     x INIT(0);           /* current horizontal spacing      */
EXTERN int32_t     y INIT(0);           /* current vertical spacing        */
EXTERN int32_t     z INIT(0);           /* current vertical spacing        */
EXTERN uint32_t    usermag INIT(0);     /* user specified magstep          */
EXTERN struct font_entry *hfontptr INIT(NULL); /* font list pointer        */

EXTERN struct internal_state {
  struct font_entry* currentfont;
  int                passno;
} current_state;

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
EXTERN int      ndone INIT(0);          /* number of pages converted       */
#else
#define TIC()
#define TOC()
#endif /* TIMING */

EXTERN int   resolution INIT(300);
EXTERN char *MFMODE     INIT("cx");
# define  max(x,y)       if ((y)>(x)) x = y
# define  min(x,y)       if ((y)<(x)) x = y

/* These are in pixels*/
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
EXTERN int32_t shrinkfactor INIT(3);
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
EXTERN uint32_t abspagenumber INIT(0);

EXTERN struct font_entry* currentfont;
EXTERN struct dvi_data* dvi INIT(NULL);

#endif /* DVIPNG_H */
