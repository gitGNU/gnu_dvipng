#include "dvipng.h"

/*char    *GetKeyStr AA((char *, KeyWord *));
  bool    GetKeyVal AA((KeyWord *, KeyDesc[], int, int *));*/
/*bool    IsSame AA((char *, char *));*/


/*-->DoSpecial*/
/*********************************************************************/
/*****************************  DoSpecial  ***************************/
/*********************************************************************/

#define PSFILE 0
#define ORIENTATION 1
#define RESETPOINTS 2
#define DEFPOINT 3
#define FILL 4
#define GRAY 5
#define PATTERN 6
#define HPFILE 7
#define LLX 8
#define LLY 9
#define URX 10
#define URY 11
#define RWI 12
#define RHI 13


KeyDesc KeyTab[] = {
  { "psfile", String },
  { "orientation", Integer},
  { "resetpoints", String},
  { "defpoint", String},
  { "fill", String},
  { "gray", Integer},
  { "pattern", Integer},
  { "hpfile", String},
  { "llx", Integer},
  { "lly", Integer},
  { "urx", Integer},
  { "ury", Integer},
  { "rwi", Integer},
  { "rhi", Integer}
  /*,
    {"hsize", Dimension},
    {"vsize", Dimension},
    {"hoffset", Dimension},
    {"voffset", Dimension},
    {"hscale", Number},
    {"vscale", Number}*/
};

#define NKEYS (sizeof(KeyTab)/sizeof(KeyTab[0]))

#ifdef __riscos
# ifdef LJ

/* Compare two strings, ignoring case;
   s1 pointer to null-terminated keyword, s2 pointer to parseline;
   returns (if successful) pointer to character following keyword in s2 */
#if NeedFunctionPrototypes
bool StrCompare(char *s1, char *s2, char **end)
#else
bool StrCompare(s1, s2, end)
char *s1;
char *s2;
char **end;
#endif
{
  char *a,*b;
  
  a = s1; 
  b = s2;
  while (*a != '\0') {
    if (tolower(*a) != tolower(*b)) return(_FALSE);
    a++; 
    b++;
  }
  *end = b; 
  return(_TRUE);
}

/* Read <number> integer values from string and store results in
   <result>. Returns number + of arguments actually read, end =
   pointer to char following last number */
#if NeedFunctionPrototypes
int ParseNumbers(char *str, int *result, int number, char **end)
#else
int ParseNumbers(str, result, number, end)
char *str;
int *result;
int number;
char **end;
#endif
{
  char *s;
  int count = 0;
  
  s = str;
  while ((*s != '\0') && (count < number)) {
    while ((*s == ' ') || (*s == ',') || (*s == '=')) 
      s++;
    if (*s != '\0') {
      result[count++] = strtod(s,&s);
    }
  }
  while ((*s == ' ') || (*s == ',') || (*s == '=')) 
    s++;
  *end = s; 
  return(count);
}


/* Diagram commands are parsed separately since the format varies from the one
+    used by the other special commands */
#if NeedFunctionPrototypes
bool ParseDiagram(char *str)
#else
bool ParseDiagram(str)
char *str;
#endif
{
  diagtrafo dt;
  char *s,*sh;
  char diagname[STRSIZE];
  int results[4],no;

  s = str;
  while (*s == ' ') 
    s++;
  if ((StrCompare("drawfile",s,&s)) || (StrCompare("DVIview_diagram",s,&s))) {

    if (printdiag == _FALSE) 
      return(_TRUE); /* it's a diagram, but don't print */

    while ((*s == ' ') || (*s == '=')) 
      s++; /* space or '=' separates keyword/keyval */

    if (*s == '\0') {
      fprintf(ERR_STREAM,"No filename given for \\special-diagram!\n"); 
      return(_TRUE);
    }
    sh = diagname;
    while ((*s != ' ') && (*s != ',') && (*s != '\0')) 
      *sh++ = *s++;
    *sh = '\0';

    /* Set up default values */
    dt.scalex = dt.scaley = 100;
    dt.cropl = dt.cropb = dt.cropr = dt.cropt = 0;
    while (*s != '\0') {
      while ((*s == ' ') || (*s == ',')) 
        s++;
      if (*s != '\0') {
        if (StrCompare("scale",s,&s)) {
          if ((no = ParseNumbers(s,results,2,&s)) != 2) {
            fprintf(ERR_STREAM,
                   "Too few arguments (%d) given for <scale> - ignored.\n",no);
          }
          dt.scalex = results[0]; 
          dt.scaley = results[1];
        }
        else if (StrCompare("crop",s,&s)) {
          if ((no = ParseNumbers(s,results,4,&s)) != 4) {
            fprintf(ERR_STREAM,
                   "Too few arguments (%d) given for <crop> - ignored.\n",no);
          }
          dt.cropl = results[0]; 
          dt.cropr = results[1];
          dt.cropt = results[2]; 
          dt.cropb = results[3];
        }
        else {
          fprintf(ERR_STREAM,"Bad \\special keyword - <%s> ignored\n",s);
          /* skip over this word */
          while ((*s != ' ') && (*s != ',') && (*s != '=') && (*s != '\0')) 
            s++;
        }
      }
    }
    /* fprintf(ERR_STREAM,"Diagram: %s, scale %d %d, crop %d %d %d %d\n",
       diagname,dt.scalex,dt.scaley,dt.cropl,dt.cropb,dt.cropr,dt.cropt);*/
    diagram(diagname,&dt);
    return(_TRUE);
  }
  else 
    return(_FALSE);
}
# endif /* LJ */
#endif /* __riscos */


void DoSpecial P2C(char *,str, int, n)
/* interpret a \special command, made up of keyword=value pairs */
/* Color specials only for now. Warn otherwise. */
{
  char *p;

#ifdef DEBUG
  if (Debug)
    printf("'%.*s' ",n,str);
#endif

  p=str;
  while(*p==' ') p++;

  /* Fragile. Should compare with n */

  switch (*p) {
  case 'b':
    if (strncmp(p,"background",10) == 0 ) {
      p += 10 ;
      while ( *p <= ' ' ) p++ ;
      background(p);
    }
    break;
  case 'c':
    if (strncmp(p,"color",5) == 0 ) {
      p += 6 ;
      while ( *p <= ' ' ) p++ ;
       if (strncmp(p, "push", 4) == 0 ) {
	p += 5 ;
	while ( *p <= ' ' ) p++ ;
	pushcolor(p) ;
      } else if (strncmp(p, "pop", 3) == 0 ) {
	popcolor() ;
      } else {
	resetcolorstack(p) ;
      }
    } 
    break ;
  default:
    Warning("at (%ld,%ld) unimplemented \\special{%.*s}.",
	    PIXROUND(h,conv*shrinkfactor),
	    PIXROUND(v,conv*shrinkfactor),n,str);
  }
}



  /*  char    spbuf[STRSIZE], xs[STRSIZE], ys[STRSIZE];
  char    *psfile = NULL;
  float   x,y;
  long4   x_pos, y_pos;
  KeyWord k;
  int     i, j, j1;
  static  int   GrayScale = 10, Pattern = 1;
  static  bool  GrayFill = _TRUE;
  static  long4 p_x[80], p_y[80];
  int llx=0, lly=0, urx=0, ury=0, rwi=0, rhi=0;
#ifdef WIN32
  char    *gs_path;
#endif

  str[n] = '\0';
  spbuf[0] = '\0';


#ifdef __riscos
#ifdef LJ
  if (ParseDiagram(str)) 
    return;
#endif
#endif
 
while ( (str = GetKeyStr(str, &k)) != NULL ) {*/
    /* get all keyword-value pairs */
    /* for compatibility, single words are taken as file names */
/*    if ( GetKeyVal( &k, KeyTab, NKEYS, &i ) && i != -1 )
      switch (i) {
      case PSFILE:
        (void) strcpy(spbuf, k.Val);
        psfile = spbuf;
        break;
        
      case ORIENTATION:
#ifdef LJ
if ((k.v.i >= 0) && (k.v.i < 2)) {*/
          /*EMIT2("\033&l%dO\033*rF", (unsigned char)k.v.i);*/
/*        }
	  #endif*/
	/*        else*/
/*#ifdef KPATHSEA
           if (!kpse_tex_hush ("special"))
#endif
          Warning( "Invalid orientation (%d)given; ignored.", k.v.i);
        break;

      case RESETPOINTS:
        (void) strcpy(spbuf, k.Val);
        break;

      case DEFPOINT:
        (void) strcpy(spbuf, k.Val);
        i = sscanf(spbuf,"%d(%[^,],%s)",&j,xs,ys);
        if (i>0) {
          x_pos = h; 
          y_pos = v;
          if (i>1) {
            if (sscanf(xs,"%fpt",&x)>0) {
              fprintf(ERR_STREAM,"x = %f\n",x);
              x_pos = PT_TO_DVI(x);
            }
          }
          if (i>2) {
            if (sscanf(ys,"%fpt",&y)>0) {
              fprintf(ERR_STREAM,"y = %f\n",y);
              y_pos = PT_TO_DVI(y);
            }
          }
          p_x[j]=x_pos;
          p_y[j]=y_pos;
        } else
#ifdef KPATHSEA
              if (!kpse_tex_hush ("special"))
#endif
          Warning("invalid point definition\n");
        break;

      case FILL:
        (void) strcpy(spbuf, k.Val);
        i = sscanf(spbuf,"%d/%d %s",&j,&j1,xs);
        if (i>1) {
	#ifdef LJ*/
	  /*          SetPosn(p_x[j], p_y[j]); */
/*          x_pos = (long4)PIXROUND(p_x[j1]-p_x[j], conv*shrinkfactor);
          y_pos = (long4)PIXROUND(p_y[j1]-p_y[j], conv*shrinkfactor);
          if (labs(x_pos)<labs(y_pos)) x_pos = x_pos+3;
          else                         y_pos = y_pos+3;
          if (GrayFill) {*/
	    /*  EMIT4("\033*c%lda%ldb%dg2P", x_pos, y_pos, GrayScale);
          } else {
	  EMIT4("\033*c%lda%ldb%dg3P", x_pos, y_pos, Pattern);*/
/*        }
#endif
        }
        break;

      case GRAY:
        if ((k.v.i >= 0) && (k.v.i < 101)) {
          GrayScale = k.v.i;
          GrayFill = _TRUE;
        } else
#ifdef KPATHSEA
           if (!kpse_tex_hush ("special"))
#endif
          Warning( "Invalid gray scale (%d) given; ignored.", k.v.i);
        break;

      case PATTERN:
        if ((k.v.i >= 0) && (k.v.i < 7)) {
          Pattern = k.v.i;
          GrayFill = _FALSE;
        } else
#ifdef KPATHSEA
           if (!kpse_tex_hush ("special"))
#endif
          Warning( "Invalid pattern (%d) given; ignored.", k.v.i);
        break;

      case LLX: llx = k.v.i; break;
      case LLY: lly = k.v.i; break;
      case URX: urx = k.v.i; break;
      case URY: ury = k.v.i; break;
      case RWI: rwi = k.v.i; break;
      case RHI: rhi = k.v.i; break;

      default:
#ifdef KPATHSEA
           if (!kpse_tex_hush ("special"))
#endif
        Warning("Can't handle %s=%s command; ignored.", k.Key, k.Val);
        break;
      }
      
    else
#ifdef KPATHSEA
           if (!kpse_tex_hush ("special"))
#endif
      Warning("Invalid keyword or value in \\special - <%s> ignored", k.Key);
  }
#ifdef LJ
if (psfile) {*/
        /* int height = rwi * (urx - llx) / (ury - lly);*/
/*      int width  = urx - llx;
        int height = ury - lly;
        char cmd[255];
        int scale_factor    = 3000 * width / rwi;
        int adjusted_height = height * 300/scale_factor;
        int adjusted_llx    = llx    * 300/scale_factor;
        char *printer = "ljetplus"; *//* use the most stupid one */

/*
        char scale_file_name[255];
        char *scale_file = tmpnam(scale_file_name);
        char *pcl_file = tmpnam(NULL);  
        FILEPTR scalef;

        if ( (scalef = BOUTOPEN(scale_file)) == FPNULL ) {
          Warning("Unable to open file %s for writing", scale_file );
          return;
        }
        fprintf(scalef, "%.2f %.2f scale\n%d %d translate\n",  
                300.0/scale_factor, 300.0/scale_factor,
                0, adjusted_height == height ? 0 : ury);
        BCLOSE( scalef );

#ifdef WIN32
	gs_path = getenv("GS_PATH");
	if (!gs_path)
	  gs_path = "gswin32c.exe";
        sprintf(cmd,"%s -q -dSIMPLE -dSAFER -dNOPAUSE -sDEVICE=%s -sOutputFile=%s %s %s showpage.ps -c quit",
		gs_path, printer, pcl_file, scale_file, psfile);
#else
        sprintf(cmd,"gs -q -dSIMPLE -dSAFER -dNOPAUSE -sDEVICE=%s -sOutputFile=%s %s %s showpage.ps -c quit",
                printer, pcl_file, scale_file, psfile);
#endif
#ifdef DEBUGGS   
        fprintf(stderr,
          "PS-file '%s' w=%d, h=%d, urx=%d, ury=%d, llx=%d, lly=%d, rwi=%d\n",
                psfile, urx - llx, height, urx,ury,llx,lly, rwi);
        fprintf(stderr,"%s\n",cmd);
#endif
        if (system(cmd)) {
          Warning("execution of '%s' returned an error", cmd);
        } else {
#ifdef DEBUGGS   
          fprintf(stderr, "o=%d, h=%d, so=%d, sh=%d\n", 
                  llx, height, adjusted_llx, adjusted_height);
          
          fprintf(stderr, "OLD x=%d, y=%d\n", 
                  (int)PIXROUND(h, conv*shrinkfactor) + x_goffset,
                  (int)PIXROUND(v, conv*shrinkfactor) + y_goffset);
#endif  
          v -= 65536l*adjusted_height;*/ /**300/scale_factor;*/
/*        h -= 65536l*adjusted_llx; *//* *300/scale_factor;*/
	  /* SetPosn(h, v);*/
/*#ifdef DEBUGGS   
          fprintf(stderr, "NEW x=%d, y=%d\n", 
                  (int)PIXROUND(h, conv*shrinkfactor) + x_goffset,
                  (int)PIXROUND(v, conv*shrinkfactor) + y_goffset);
#endif
*/
          /*CopyHPFile( pcl_file );*/
          /* unlink(pcl_file); */
          /* unlink(scale_file); */
/*      }
      }
#endif*/ /* LJ */
/*}*/

/*-->GetKeyStr*/
/**********************************************************************/
/*****************************  GetKeyStr  ****************************/
/**********************************************************************/
/* extract first keyword-value pair from string (value part may be null)
 * return pointer to remainder of string
 * return NULL if none found
 */
char    KeyStr[STRSIZE];
char    ValStr[STRSIZE];
#if NeedFunctionPrototypes
char *GetKeyStr(char *str, KeyWord *kw )
#else
char    *GetKeyStr( str, kw )
char    *str;
KeyWord *kw;
#endif
{
  char    *s, *k, *v, t;
  if ( !str )
    return( NULL );
  for (s = str; *s == ' '; s++)
    ;          /* skip over blanks */
  if (*s == '\0')
    return( NULL );
  for (k = KeyStr; /* extract keyword portion */
       *s != ' ' && *s != '\0' && *s != '=';
       *k++ = *s++)
    ;
  *k = '\0';
  kw->Key = KeyStr;
  kw->Val = v = NULL;
  kw->vt = None;
  for ( ; *s == ' '; s++)
    ;            /* skip over blanks */
  if ( *s != '=' )         /* look for "=" */
    return( s );
  for (s++; *s == ' '; s++);      /* skip over blanks */
  if ( *s == '\'' || *s == '\"' )  /* get string delimiter */
    t = *s++;
  else
    t = ' ';
  for (v = ValStr; /* copy value portion up to delim */
       *s != t && *s != '\0';
       *v++ = *s++)
    ;
  if ( t != ' ' && *s == t )
    s++;
  *v = '\0';
  kw->Val = ValStr;
  kw->vt = String;
  return( s );
}



/*-->IsSame*/
/**********************************************************************/
/*******************************  IsSame  *****************************/
/**********************************************************************/
/* compare strings, ignore case */
#if NeedFunctionPrototypes
bool IsSame(char *a, char *b)
#else
bool IsSame(a, b)
char    *a, *b;
#endif
{
  char *x, *y;
  
  for (x = a, y = b; *a; a++, b++)
    if ( tolower(*a) != tolower(*b) )
      return( _FALSE );

  return( *a == *b ? _TRUE : _FALSE );
}


/*-->GetKeyVal*/
/**********************************************************************/
/*****************************  GetKeyVal  ****************************/
/**********************************************************************/
/* get next keyword-value pair decode value according to table entry  */
#if NeedFunctionPrototypes
bool GetKeyVal(KeyWord *kw, KeyDesc tab[], int nt, int *tno)
#else
bool    GetKeyVal( kw, tab, nt, tno)
KeyWord *kw;
KeyDesc tab[];
int     nt;
int     *tno;
#endif
{
  int     i;
  char    c = '\0';
  *tno = -1;
  for (i = 0; i < nt; i++)
    if ( IsSame(kw->Key, tab[i].Entry) ) {
      *tno = i;
      switch ( tab[i].Typ ) {
      case None:
        if ( kw->vt != None )
          return( _FALSE );
        break;
      case String:
        if ( kw->vt != String )
          return( _FALSE );
        break;
      case Integer:
        if ( kw->vt != String )
          return( _FALSE );
        if ( sscanf(kw->Val, "%d%c", &(kw->v.i), &c) != 1 || c != '\0' )
          return( _FALSE );
        break;
      }
      kw->vt = tab[i].Typ;
      return( _TRUE );
    }
  return( _TRUE );
}
