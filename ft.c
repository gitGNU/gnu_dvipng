#include "dvipng.h"

#define PROPER_OVERSTRIKE

int32_t SetFT(int32_t c, int32_t hh, int32_t vv) 
{
  register struct ft_char *ptr = currentfont->chr[c];
                                      /* temporary ft_char pointer */
  int red,green,blue;
  int *Color=alloca(sizeof(int)*ptr->greys);
  int x,y;
  int pos=0;
  int bgColor,pixelcolor;
  hh -= ptr->xOffset;
  vv -= ptr->yOffset;
  
  for( x=1; x<=ptr->greys ; x++) {
    red = bRed-(bRed-Red)*x/ptr->greys;
    green = bGreen-(bGreen-Green)*x/ptr->greys;
    blue = bBlue-(bBlue-Blue)*x/ptr->greys;
    Color[x] = gdImageColorResolve(page_imagep,red,green,blue);
  }  
  for( y=0; y<ptr->h; y++) {
    for( x=0; x<ptr->w; x++) {
      if (ptr->data[pos]>0) {
	bgColor = gdImageGetPixel(page_imagep, hh + x, vv + y);
	if (bgColor > 0) {
	  red=gdImageRed(page_imagep, bgColor);
	  green=gdImageGreen(page_imagep, bgColor);
	  blue=gdImageBlue(page_imagep, bgColor);
	  red = red-(red-Red)*ptr->data[pos]/ptr->greys;
	  green = green-(green-Green)*ptr->data[pos]/ptr->greys;
	  blue = blue-(blue-Blue)*ptr->data[pos]/ptr->greys;
	  pixelcolor = gdImageColorResolve(page_imagep, red, green, blue);
	  gdImageSetPixel(page_imagep, hh + x, vv + y, pixelcolor);
	} else
	  gdImageSetPixel(page_imagep, hh + x, vv + y, 
			  Color[(int)ptr->data[pos]]);
      }
      pos++;
    }
  }
  return(ptr->tfmw);
}

void LoadFT(int32_t c, struct ft_char * ptr)
{
  FT_Bitmap  bitmap;
  FT_UInt    glyph_i;
  int i,j,k;
  char* bit;

  DEBUG_PRINT((DEBUG_FT,"\n  LOAD FT CHAR\t%d (%d)",c,ptr->tfmw));
  if (currentfont->enc == NULL)
    glyph_i = FT_Get_Char_Index( currentfont->face, c );
  else 
    glyph_i = FT_Get_Name_Index( currentfont->face,
				 currentfont->enc->charname[c]);
  if (FT_Load_Glyph( currentfont->face,    /* handle to face object */
		     glyph_i,              /* glyph index           */
		     FT_LOAD_RENDER ))     /* load flags            */
    Fatal("can't load ft_char %d",c);
  ptr->xOffset = -currentfont->face->glyph->bitmap_left;
  ptr->yOffset = currentfont->face->glyph->bitmap_top-1;
  bitmap=currentfont->face->glyph->bitmap;
  DEBUG_PRINT((DEBUG_FT," (%dx%d)",bitmap.width,bitmap.rows));
    
  if ((ptr->data 
       = (char*) calloc(bitmap.width*bitmap.rows,sizeof(char))) == NULL)
    Fatal("Unable to allocate image space for char <%c>\n", (char)c);
  ptr->w = bitmap.width;
  ptr->h = bitmap.rows;
  ptr->greys = 15;
  
  DEBUG_PRINT((DEBUG_GLYPH, "\nDRAW GLYPH %d\n", (int)c));
  bit=ptr->data;
  for(i=0;i<bitmap.rows;i++) {
    for(j=0;j<bitmap.width;j++) {
      k=bitmap.buffer[i*bitmap.pitch+j]/16;
      DEBUG_PRINT((DEBUG_GLYPH,k>0?"*":" "));
      bit[i*bitmap.width+j]=k;
    }
    DEBUG_PRINT((DEBUG_GLYPH,"|\n"));
  }
}

void InitFT(struct font_entry * tfontp, unsigned dpi,
	    char* encoding, FT_Matrix* transform)
{
  int error;

  DEBUG_PRINT(((DEBUG_DVI|DEBUG_VF),"\n  OPEN FONT:\t'%s'", tfontp->name));
  Message(BE_VERBOSE,"<%s>", tfontp->name);
  error = FT_New_Face( libfreetype, tfontp->name, 0, &tfontp->face );
  if (error == FT_Err_Unknown_File_Format)
    Warning("font file %s has unknown format", tfontp->name);
  else if (error)
    Warning("font file %s could not be opened", tfontp->name);
  if (encoding == NULL) {
    tfontp->enc=NULL;
    if (FT_Select_Charmap( tfontp->face, FT_ENCODING_ADOBE_CUSTOM )
	&& FT_Select_Charmap( tfontp->face, FT_ENCODING_ADOBE_STANDARD ))
      Warning("unable to set font encoding for %s", tfontp->name);
  }
  else
    tfontp->enc=InitEncoding(encoding);
  if (FT_Set_Char_Size( tfontp->face, /* handle to face object           */
			0,            /* char_width in 1/64th of points  */
			tfontp->d/65536*64,
			/* char_height in 1/64th of points */
			dpi/shrinkfactor,   /* horizontal resolution */
			dpi/shrinkfactor )) /* vertical resolution   */
    Warning("unable to set font size for %s", tfontp->name);
  FT_Set_Transform(tfontp->face, transform, NULL);
  tfontp->type = FONT_TYPE_FT;
}
