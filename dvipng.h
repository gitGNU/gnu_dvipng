/* dvipng.h */

/************************************************************************

  Part of the dvipng distribution

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
  02111-1307, USA.

  Copyright (C) 2002-2005 Jan-�ke Larsson

************************************************************************/

#ifndef DVIPNG_H
#define DVIPNG_H
#include "config.h"

#define  STRSIZE         255     /* stringsize for file specifications  */

#define  FIRSTFNTCHAR  0
#define  LASTFNTCHAR   255
#define  NFNTCHARS     LASTFNTCHAR+1

#define  STACK_SIZE      100     /* DVI-stack size                     */

#define DEFAULT_GAMMA 1.0

/* Name of the program which is called to generate missing pk files */
#define MAKETEXPK "mktexpk"

#ifdef HAVE_INTTYPES_H
#  include <inttypes.h>
#else /* HAVE_INTTYPES_H */
# ifndef __sgi
/* IRIX has the following types typedef'd in sys/types.h already,
 * i.e., _old_ IRIX systems; newer ones have a working inttypes.h */
typedef signed char               int8_t;
typedef short                    int16_t;
typedef int                      int32_t;
#endif /* ! __sgi */
typedef unsigned char            uint8_t;
typedef unsigned short          uint16_t;
typedef unsigned int            uint32_t;
typedef long long                int64_t;
typedef unsigned long long      uint64_t;
#endif /* HAVE_INTTYPES_H */

#ifndef INT32_MIN
#define INT32_MIN   (-2147483647-1)
#endif
#ifndef INT32_MAX
#define INT32_MAX   (2147483647)
#endif

#include <gd.h>

#ifdef HAVE_KPATHSEA_KPATHSEA_H
# include <kpathsea/kpathsea.h>
#else
# error: kpathsea/kpathsea.h is missing from your system
#endif

#ifdef HAVE_FT2
#define HAVE_FT2_OR_LIBT1
#include <ft2build.h>  
#include FT_FREETYPE_H
#endif

#ifdef HAVE_LIBT1
#define HAVE_FT2_OR_LIBT1
#include <t1lib.h>
#endif

#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#else
# ifdef HAVE_KPATHSEA_KPATHSEA_H
/* boolean is an enum type from kpathsea/types.h loaded in
   kpathsea/kpathsea.h, use it as fallback */
#  define bool boolean
# else
typedef int bool;
#  define true (bool) 1
#  define false (bool) 0
# endif
#endif

#ifndef HAVE_VPRINTF
# ifdef HAVE_DOPRNT
#  define   vfprintf(stream, message, args)  _doprnt(message, args, stream)
# else
#  error: vfprintf AND _doprnt are missing!!!
   /* If we have neither, should fall back to fprintf with fixed args.  */
# endif
#endif

/* Autoconf may define malloc to rpl_malloc, if the system does not
 * have a GNU Libc-compatible malloc (for which malloc(0) gives a
 * valid pointer). We don't need that (yet) */
#ifdef malloc
# undef malloc
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

#if HAVE_ALLOCA_H
# include <alloca.h>
#endif
#define TEMPSTR(s,a) { char* tmp=a; \
               if (tmp!=NULL) {\
                 s=alloca(strlen(tmp)+1);strcpy(s,tmp);free(tmp);\
               } else \
                 s=NULL;}

/*************************************************************************/


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

struct dvi_color {
  int red,green,blue;
};

struct page_list {
  struct page_list* next;
  int               offset;           /* file offset to BOP */
  int32_t           count[11];        /* 10 dvi counters + absolute pagenum in file */
  int               csp;              /* color stack pointer at BOP */
  struct dvi_color  cstack[2];        /* color stack at BOP, may be longer */
};




struct dvi_data* DVIOpen(char*,char*);
void             DVIClose(struct dvi_data*);
bool             DVIReOpen(struct dvi_data*);
struct page_list*FindPage(struct dvi_data*, int32_t, bool);
struct page_list*NextPage(struct dvi_data*, struct page_list*);
struct page_list*PrevPage(struct dvi_data*, struct page_list*);
int              SeekPage(struct dvi_data*, struct page_list*);
bool             DVIFollowToggle(void);
unsigned char*   DVIGetCommand(struct dvi_data*);
uint32_t         CommandLength(unsigned char*); 

/********************************************************/
/**********************  misc.h  ************************/
/********************************************************/

struct filemmap {
#ifdef MIKTEX
  HANDLE hFile;
  HANDLE hMap;
#else  /* MIKTEX */
  int fd;
#endif	/* MIKTEX */
  char* mmap;
  size_t size;
};

bool    DecodeArgs(int, char *[]);
void    DecodeString(char *);
bool    MmapFile (char *filename,struct filemmap *fmmap);
void    UnMmapFile(struct filemmap* fmmap);

void    Message(int, char *fmt, ...);
void    Warning(char *fmt, ...);
void    Fatal(char *fmt, ...);

int32_t   SNumRead(unsigned char*, register int);
uint32_t   UNumRead(unsigned char*, register int);

bool MmapFile (char *filename,struct filemmap *fmmap);
void UnMmapFile(struct filemmap* fmmap);


/********************************************************/
/***********************  font.h  ***********************/
/********************************************************/
struct encoding {
  struct encoding* next;
  char*            name;
  char*            charname[257];
};

#ifdef HAVE_FT2_OR_LIBT1
struct psfontmap {
  struct psfontmap *next;
  char *line,*psfile,*tfmname,*encname,*end;
  struct encoding* encoding;
#ifdef HAVE_FT2
  FT_Matrix* ft_transformp;
  FT_Matrix ft_transform;
#endif
#ifdef HAVE_LIBT1
  T1_TMATRIX* t1_transformp;
  T1_TMATRIX t1_transform;
#endif
};
#endif

#define FONT_TYPE_PK            1
#define FONT_TYPE_VF            2
#define FONT_TYPE_FT            3
#define FONT_TYPE_T1            4
struct char_entry {                /* PK/FT/T1 Glyph/VF Macro             */
  dviunits       tfmw;             /* TFM width                           */
  unsigned char *data;             /* glyph data, either pixel data
				    * (0=transp, 255=max ink) or VF macro */
  uint32_t       length;           /* Length of PK data or VF macro       */
  /* Only used in pixel fonts */
  pixels         w,h;              /* width and height in pixels          */
  subpixels      xOffset, yOffset; /* x offset and y offset in subpixels  */
  /* Only used in PK fonts */
  unsigned char *pkdata;           /* Points to beginning of PK data      */
  unsigned char  flag_byte;        /* PK flagbyte                         */
};

struct font_entry {    /* font entry */
  int          type;            /* PK/VF/Type1 ...                   */
  struct font_entry *next;
  uint32_t     c, s, d;                                                
  uint8_t      a, l;                                                   
  char         n[STRSIZE];      /* FNT_DEF command parameters        */
  int          dpi;             /* computed from s and d             */
  char         name[STRSIZE];   /* full name of PK/VF file           */
  struct filemmap fmmap;        /* file memory map                   */
  uint32_t     magnification;   /* magnification read from font file */
  uint32_t     designsize;      /* design size read from font file   */
  void *       chr[NFNTCHARS];  /* character information             */ 
#ifdef HAVE_FT2
  FT_Face      face;            /* Freetype2 face                    */
#endif
#ifdef HAVE_LIBT1
  int          T1id;            /* T1lib font id                     */
#endif
#ifdef HAVE_FT2_OR_LIBT1
  struct psfontmap* psfontmap;  /* Font transformation               */
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

void    CheckChecksum(uint32_t, uint32_t, const char*);
void    InitPK (struct font_entry *newfontp);
void    DonePK(struct font_entry *oldfontp);
void    InitVF (struct font_entry *newfontp);
void    DoneVF(struct font_entry *oldfontp);

void    FontDef(unsigned char*, void* /* dvi/vf */);
void    ClearFonts(void);
void    SetFntNum(int32_t, void* /* dvi/vf */);      
void    FreeFontNumP(struct font_num *hfontnump);

#ifdef HAVE_FT2_OR_LIBT1
void    InitPSFontMap(void);
void    ClearPSFontMap(void);
struct psfontmap* FindPSFontMap(char*);
struct encoding* FindEncoding(char*);
void    ClearEncoding(void);
bool    ReadTFM(struct font_entry *, char*);
#endif

#ifdef HAVE_FT2
bool    InitFT(struct font_entry *);
void    DoneFT(struct font_entry *tfontp);
void    LoadFT(int32_t, struct char_entry *);
#endif

#ifdef HAVE_LIBT1
bool    InitT1(struct font_entry *);
void    DoneT1(struct font_entry *tfontp);
void    LoadT1(int32_t, struct char_entry *);
#endif

/********************************************************/
/*********************  pplist.h  ***********************/
/********************************************************/

bool    ParsePages(char*);
void    FirstPage(int32_t,bool);
void    LastPage(int32_t,bool);
void    ClearPpList(void);
void    Reverse(bool);
struct page_list*   NextPPage(void* /* dvi */, struct page_list*);




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

void      CreateImage(pixels width, pixels height);
void      DestroyImage(void);
void      DrawCommand(unsigned char*, void* /* dvi/vf */); 
void      DrawPages(void);
void      WriteImage(char*, int);
void      LoadPK(int32_t, register struct char_entry *);
int32_t   SetChar(int32_t);
dviunits  SetGlyph(int32_t c, int32_t hh,int32_t vv);
void      Gamma(double gamma);
int32_t   SetVF(int32_t);
int32_t   SetRule(int32_t, int32_t, int32_t, int32_t);
void      SetSpecial(char *, int32_t, int32_t, int32_t);
void      BeginVFMacro(struct font_entry*);
void      EndVFMacro(void);

/**************************************************/
void handlepapersize(char*,int32_t*,int32_t*);

void stringrgb(char* colorstring,int *r,int *g,int *b);
void background(char *);
void initcolor(void);
void popcolor(void);
void pushcolor(char *);
void resetcolorstack(char *);
void StoreColorStack(struct page_list *tpagep);
void ReadColorStack(struct page_list *tpagep);
void StoreBackgroundColor(struct page_list *tpagep);
void ClearColorNames(void);


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
} current_state;

#define BE_NONQUIET                  1
#define BE_VERBOSE                   (1<<1)
#define PARSE_STDIN                  (1<<2)
#define EXPAND_BBOX                  (1<<3)
#define TIGHT_BBOX                   (1<<4)
#define CACHE_IMAGES                 (1<<5)
#define RENDER_TRUECOLOR             (1<<6)
#define USE_FREETYPE                 (1<<7)
#define USE_LIBT1                    (1<<8)
#define REPORT_HEIGHT                (1<<9)
#define REPORT_DEPTH                 (1<<10)
#define DVI_PAGENUM                  (1<<11)
#define MODE_PICKY                   (1<<12)
#define PAGE_GAVE_WARN               (1<<13)
#define PREVIEW_LATEX_TIGHTPAGE      (1<<14)
#define GIF_OUTPUT                   (1<<15)
#define MODE_STRICT                  (1<<16)
#define NO_GHOSTSCRIPT               (1<<17)
#define NO_GSSAFER                   (1<<18)
#define BG_TRANSPARENT               (1<<19)
#define BG_TRANSPARENT_ALPHA         (1<<20)
EXTERN uint32_t flags INIT(BE_NONQUIET | USE_FREETYPE | USE_LIBT1);

#ifdef DEBUG
EXTERN unsigned int debug INIT(0);
#define DEBUG_PRINT(a,b) if (debug & a) { printf b; fflush(stdout); }
#define DEBUG_DVI                    1
#define DEBUG_VF                     (1<<1)
#define DEBUG_PK                     (1<<2)
#define DEBUG_TFM                    (1<<3)
#define DEBUG_GLYPH                  (1<<4)
#define DEBUG_FT                     (1<<5)
#define DEBUG_ENC                    (1<<6)
#define DEBUG_COLOR                  (1<<7)
#define DEBUG_T1                     (1<<8)
#define DEBUG_GS                     (1<<9)
#define LASTDEBUG                    DEBUG_GS
#define DEBUG_DEFAULT                DEBUG_DVI
#else
#define DEBUG_PRINT(a,b)
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

EXTERN char*  user_mfmode          INIT(NULL);
EXTERN int    user_bdpi            INIT(0);
EXTERN int    dpi                  INIT(100);

#ifdef HAVE_GDIMAGEPNGEX
EXTERN int   compression INIT(1);
#endif
#ifdef MIKTEX
#undef min
#undef max
#endif        /* MIKTEX */
# define  max(x,y)       if ((y)>(x)) x = y
# define  min(x,y)       if ((y)<(x)) x = y

/* These are in pixels*/
EXTERN  int x_min INIT(0); 
EXTERN  int y_min INIT(0);
EXTERN  int x_max INIT(0);
EXTERN  int y_max INIT(0);

/* Page size: default set by -T */
EXTERN  int x_width_def INIT(0); 
EXTERN  int y_width_def INIT(0);

/* Offset: default set by -O and -T bbox */
EXTERN  int x_offset_def INIT(0);
EXTERN  int y_offset_def INIT(0);

/* Preview-latex's tightpage */
EXTERN  int x_width_tightpage INIT(0); 
EXTERN  int y_width_tightpage INIT(0);
EXTERN  int x_offset_tightpage INIT(0);
EXTERN  int y_offset_tightpage INIT(0);

/* Paper size: set by -t, for cropmark purposes only */
/* This has yet to be written */
EXTERN  int x_pwidth INIT(0); 
EXTERN  int y_pwidth INIT(0);

/* The transparent border preview-latex desires */
EXTERN  int borderwidth INIT(0);

/* fallback color for transparent background */
EXTERN bool userbordercolor INIT(FALSE); /* if true, use user-supplied color */
EXTERN struct dvi_color bordercolor;


EXTERN gdImagePtr page_imagep INIT(NULL);
EXTERN int32_t shrinkfactor INIT(4);

EXTERN struct dvi_color cstack[STACK_SIZE];
EXTERN int csp INIT(1);

EXTERN struct font_entry* currentfont;
EXTERN struct dvi_data* dvi INIT(NULL);

#ifdef HAVE_FT2
EXTERN FT_Library libfreetype INIT(NULL);
#endif

#ifdef HAVE_LIBT1
EXTERN void* libt1 INIT(NULL);
#endif

#define  EXIT_FATAL    EXIT_FAILURE+1
EXTERN int exitcode INIT(EXIT_SUCCESS);

#endif /* DVIPNG_H */
