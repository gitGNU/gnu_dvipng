#ifndef DVIPNG_H
#define DVIPNG_H
#include "config.h"

#define  STRSIZE         255     /* stringsize for file specifications  */

#define  FIRSTFNTCHAR  0
#define  LASTFNTCHAR   255
#define  NFNTCHARS     LASTFNTCHAR+1

#define  STACK_SIZE      100     /* DVI-stack size                     */

/* Name of the program which is called to generate missing pk files */
#define MAKETEXPK "mktexpk"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#else
#include <inttypes.h>
#endif

/* For some reason FreeBSD does not follow the C standard */
/* It has int32_t but not INT32_MAX                       */
#ifndef INT32_MIN
#define INT32_MIN   (-2147483647-1)
#endif
#ifndef INT32_MAX
#define INT32_MAX   (2147483647)
#endif
              
#include <gd.h>

#ifdef HAVE_LIBKPATHSEA
#include <kpathsea/kpathsea.h>
#endif

#ifdef HAVE_FT2
#include <freetype/freetype.h>
FT_Library  libfreetype;
#endif


typedef  int     bool;
#define  _TRUE      (bool) 1
#define  _FALSE     (bool) 0
#define  UNKNOWN     -1

#ifndef HAVE_VPRINTF
# ifdef HAVE_DOPRNT
#  define   vfprintf(stream, message, args)  _doprnt(message, args, stream)
# else
#  error: vprintf AND _doprnt are missing!!!
   /* If we have neither, should fall back to fprintf with fixed args.  */
# endif
#endif

/*************************************************************/
/*************************  protos.h  ************************/
/*************************************************************/

typedef int pixels;
typedef int32_t subpixels;
typedef int32_t dviunits;

#define  MM_TO_PXL(x)   (int)(((x)*resolution*10)/254)
#define  PT_TO_PXL(x)   (int)((int32_t)((x)*resolution*100l)/7224)
#define  PT_TO_DVI(x)   (int32_t)((x)*65536l)

/*#define PIXROUND(x,c) ((((double)x+(double)(c>>1))/(double)c)+0.5)*/
/*#define PIXROUND(x,c) (((x)+c)/(c))*/
/*#define PIXROUND(x,c) ((x+c-1)/(c))*/
/*#define PIXROUND(x,c) ((x)/(c))*/
/* integer round to the nearest number, not towards zero */
#define PIXROUND(num,den) ((num)>0 ? ((num)+(den)/2)/(den) : -(((den)/2-(num))/(den)))


/*************************************************************************/

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

void    SetSpecial(char *, int, int32_t, int32_t,bool);

/********************************************************/
/***********************  dvi.h  ************************/
/********************************************************/

#define DVI_TYPE            0
struct dvi_data {    /* dvi entry */
  int          type;            /* This is a DVI                    */
  struct dvi_data *next;
  uint32_t     num, den, mag;   /* PRE command parameters            */
  int32_t      conv;            /* computed from num and den         */
  /* divide dvi units (sp) with conv to get mf device resolution */
  /* divide further with shrinkfactor to get true resolution */
  char *       name;            /* full name of DVI file             */
  char *       outname;         /* output filename (basename)        */
  FILE *       filep;           /* file pointer                      */
  time_t       mtime;           /* modification time                 */
  struct font_num  *fontnump;   /* DVI font numbering                */
  struct page_list *pagelistp;  /* DVI page list                     */
};

#define PAGE_POST      INT32_MAX
#define PAGE_LASTPAGE  INT32_MAX-1
#define PAGE_MAXPAGE   INT32_MAX-2    /* assume no pages out of this range */
#define PAGE_FIRSTPAGE INT32_MIN  
#define PAGE_MINPAGE   INT32_MIN+1    /* assume no pages out of this range */

struct page_list {
  struct page_list* next;
  int     offset;           /* file offset to BOP */
  int32_t count[11];        /* 10 dvi counters + absolute pagenum in file */
};

struct dvi_data* DVIOpen(char*,char*);
void             DVIClose(struct dvi_data*);
void             DVIReOpen(struct dvi_data*);
struct page_list*FindPage(struct dvi_data*, int32_t, bool);
struct page_list*NextPage(struct dvi_data*, struct page_list*);
struct page_list*PrevPage(struct dvi_data*, struct page_list*);
int              SeekPage(struct dvi_data*, struct page_list*);
bool             DVIFollowToggle(void);
unsigned char*   DVIGetCommand(struct dvi_data*);
uint32_t         CommandLength(unsigned char*); 

/********************************************************/
/***********************  font.h  ***********************/
/********************************************************/
struct encoding {
  struct encoding* next;
  char*            name;
  char*            charname[256];
};


#define FONT_TYPE_PK            1
struct pk_char {                   /* PK character */
  dviunits       tfmw;             /* TFM width                         */
  unsigned char  greys;            /* ink is black at this value        */
  pixels         w,h;              /* width height in pixels            */
  char          *data;             /* glyph data                        */
  pixels         xOffset, yOffset; /* x offset and y offset in pixels   */
  unsigned char *mmap;             /* Points to beginning of PK data    */
  uint32_t       length;           /* Length of PK data                 */
  unsigned char  flag_byte;        /* PK flagbyte                       */
};

#define FONT_TYPE_VF            2
struct vf_char {                   /* VF character                     */
  dviunits       tfmw;             /* TFM width                        */
  unsigned char* mmap;             /* Points to beginning of VF macro  */
  uint32_t       length;           /* Length of VF macro               */
};

#ifdef HAVE_FT2
#define FONT_TYPE_FT            3
struct ft_char {                   /* FT character */
  dviunits       tfmw;             /* TFM width                         */
  unsigned char  greys;            /* ink is black at this value        */
  pixels         w,h;              /* width height in pixels            */
  unsigned char *data;             /* glyph data                        */
  pixels         xOffset, yOffset; /* x offset and y offset in pixels   */
};
#endif

struct font_entry {    /* font entry */
  int          type;            /* PK/VF/Type1 ...                   */
  struct font_entry *next;
  uint32_t     c, s, d;                                                
  uint8_t      a, l;                                                   
  char         n[STRSIZE];      /* FNT_DEF command parameters        */
  uint32_t     font_mag;        /* computed from s and d             */
  char         name[STRSIZE];   /* full name of PK/VF file           */
  int          filedes;         /* file descriptor                   */
  unsigned char* mmap,*end;     /* memory map                        */
  //  int          length;          /* its length                        */
  uint32_t     magnification;   /* magnification read from font file */
  uint32_t     designsize;      /* design size read from font file   */
  void *       chr[NFNTCHARS];  /* character information             */ 
#ifdef HAVE_FT2
  FT_Face      face;            /* Type1/Truetype font               */
  struct encoding *enc;         /* custom encoding array             */
#endif
  struct font_num *vffontnump;  /* VF local font numbering           */
  int32_t      defaultfont;     /* VF default font number            */
};

struct font_num {    /* Font number. Different for VF/DVI, and several
			font_num can point to one font_entry */
  struct font_num   *next;
  int32_t            k;
  struct font_entry *fontp;
};

void    CheckChecksum(unsigned, unsigned, const char*);
void    InitPK (struct font_entry *newfontp);
void    DonePK(struct font_entry *oldfontp);
void    InitVF (struct font_entry *newfontp);
void    DoneVF(struct font_entry *oldfontp);

void    FontDef(unsigned char*, void* /* dvi/vf */);
void    ClearFonts(void);
void    SetFntNum(int32_t, void* /* dvi/vf */);      
void    FreeFontNumP(struct font_num *hfontnump);

#ifdef HAVE_FT2
void    InitPSFontMap(void);
char*   FindPSFontMap(char*, char**, FT_Matrix**);
bool    InitFT(struct font_entry *, unsigned, char*, FT_Matrix* );
void    DoneFT(struct font_entry *tfontp);
int32_t SetFT(int32_t, int32_t, int32_t);
void    LoadFT(int32_t, struct ft_char *);
struct encoding* FindEncoding(char*);
bool    ReadTFM(struct font_entry *, char*);
#endif

/********************************************************/
/*********************  pplist.h  ***********************/
/********************************************************/

bool    ParsePages(char*);
void    FirstPage(int32_t,bool);
void    LastPage(int32_t,bool);
void    ClearPpList(void);
bool    Reverse(void);
struct page_list*   NextPPage(void* /* dvi */, struct page_list*);

/********************************************************/
/**********************  misc.h  ************************/
/********************************************************/

bool    DecodeArgs(int, char *[]);
void    DecodeString(char *);

void    Message(int, char *fmt, ...);
void    Warning(char *fmt, ...);
void    Fatal(char *fmt, ...);

int32_t   SNumRead(unsigned char*, register int);
uint32_t   UNumRead(unsigned char*, register int);

#ifdef MAIN
#define EXTERN
#define INIT(x) =x
#else
#define EXTERN extern
#define INIT(x)
#endif

/********************************************************/
/**********************  draw.h  ************************/
/********************************************************/
#include "commands.h"

void      CreateImage(void);
void      DrawCommand(unsigned char*, void* /* dvi/vf */); 
void      DrawPages(void);
void      WriteImage(char*, int);
void      LoadPK(int32_t, register struct pk_char *);
//void      LoadFT(int32_t, struct ft_char *);
int32_t   SetChar(int32_t);
int32_t   SetPK(int32_t,int32_t, int32_t);
int32_t   SetVF(int32_t);
int32_t   SetRule(int32_t, int32_t, int32_t, int32_t, int);
void      BeginVFMacro(struct font_entry*);
void      EndVFMacro(void);

/**************************************************/
void handlepapersize(char*,int*,int*);

void background(char *);
void initcolor(void);
void popcolor(void);
void pushcolor(char *);
void resetcolorstack(char *);

/**********************************************************************/
/*************************  Global Variables  *************************/
/**********************************************************************/

#ifdef MAKETEXPK
#ifdef HAVE_LIBKPATHSEA
EXTERN bool    makeTexPK INIT(MAKE_TEX_PK_BY_DEFAULT);
#else
EXTERN bool    makeTexPK INIT(_TRUE);
#endif
#endif

EXTERN uint32_t    usermag INIT(0);     /* user specified magstep          */
EXTERN struct font_entry *hfontptr INIT(NULL); /* font list pointer        */

EXTERN struct internal_state {
  struct font_entry* currentfont;
  int                passno;
} current_state;

#define BE_NONQUIET     1
#define BE_VERBOSE      2
#define PARSE_STDIN     4
#define REPORT_BASELINE 8
#define LASTFLAG        REPORT_BASELINE

#ifdef DEBUG
/*EXTERN unsigned int Debug INIT(0);*/
#define DEBUG_PRINT(a) Message a
#define DEBUG_DEFAULT LASTFLAG * 2
#define DEBUG_DVI     DEBUG_DEFAULT
#define DEBUG_VF      DEBUG_DEFAULT * 2
#define DEBUG_PK      DEBUG_DEFAULT * 4
#define DEBUG_TFM     DEBUG_DEFAULT * 8
#define DEBUG_GLYPH   DEBUG_DEFAULT * 16
#define DEBUG_FT      DEBUG_DEFAULT * 32
#define DEBUG_ENC     DEBUG_DEFAULT * 64
#define DEBUG_COLOR   DEBUG_DEFAULT * 128
#define DEBUG_GS      DEBUG_DEFAULT * 256
#define LASTDEBUG     DEBUG_GS
#else
#define DEBUG_PRINT(a)
#endif

/************************timing stuff*********************/
#ifdef TIMING
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
EXTERN double timer INIT(0);
EXTERN double my_tic,my_toc INIT(0);
EXTERN int      ndone INIT(0);          /* number of pages converted       */
# ifdef HAVE_GETTIMEOFDAY
EXTERN struct timeval Tp;
#  define TIC { gettimeofday(&Tp, NULL); \
    my_tic= (float)Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0;}
#  define TOC { gettimeofday(&Tp, NULL); \
    my_toc += ((float)Tp.tv_sec + ((float)(Tp.tv_usec))/ 1000000.0) - my_tic;}
# else
#  ifdef HAVE_FTIME
EXTERN struct timeb timebuffer;
#   define TIC() { ftime(&timebuffer); \
 my_tic= timebuffer.time + (float)(timebuffer.millitm) / 1000.0;
#   define TOC() { gettimeofday(&Tp, NULL); \
 my_toc += (timebuffer.time + (float)(timebuffer.millitm) / 1000.0) - my_tic;}
#  else
#   define TIC()
#   define TOC()
#  endif
# endif
#endif /* TIMING */

EXTERN int   resolution INIT(300);
EXTERN char *MFMODE     INIT("cx");
#ifdef HAVE_FT2
EXTERN bool   freetype INIT(true);
#endif
#ifdef HAVE_GDIMAGEPNGEX
EXTERN int   compression INIT(1);
#endif
#ifdef  HAVE_GDIMAGECREATETRUECOLOR
EXTERN bool truecolor   INIT(FALSE);
#endif
EXTERN bool cacheimages INIT(FALSE);
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

EXTERN int Red    INIT(0);
EXTERN int Green  INIT(0);
EXTERN int Blue   INIT(0);
EXTERN int bRed   INIT(255);
EXTERN int bGreen INIT(255);
EXTERN int bBlue  INIT(255);

#define PASS_SKIP 0
#define PASS_BBOX 1
#define PASS_TIGHT_BBOX 2
#define PASS_DRAW 4
EXTERN int PassDefault INIT(PASS_BBOX);

EXTERN struct font_entry* currentfont;
EXTERN struct dvi_data* dvi INIT(NULL);

#endif /* DVIPNG_H */
