#include "dvipng.h"

struct stack_entry {  
  int32_t    h, v, w, x, y, z; /* stack entry                           */
} stack[STACK_SIZE];           /* stack                                 */
int       sp = 0;              /* stack pointer                         */

int32_t     h;                   /* current horizontal position     */
int32_t     v;                   /* current vertical position       */
int32_t     w INIT(0);           /* current horizontal spacing      */
int32_t     x INIT(0);           /* current horizontal spacing      */
int32_t     y INIT(0);           /* current vertical spacing        */
int32_t     z INIT(0);           /* current vertical spacing        */

int PassNo;

#define MoveRight(b)  h += (int32_t) b
#define MoveDown(a)   v += (int32_t) a
#define DO_VFCONV(a) (((struct font_entry*) parent)->type==DVI_TYPE)?a:\
    (int32_t)((int64_t) a *  ((struct font_entry*) parent)->s / (1 << 20))


int32_t SetChar(int32_t c)
{
#ifdef DEBUG
  if (currentfont->type==FONT_TYPE_VF)
    DEBUG_PRINT((DEBUG_DVI,"\n  VF CHAR:\t"));
  else
    DEBUG_PRINT((DEBUG_DVI,"\n  PK CHAR:\t"));
  if (isprint(c))
    DEBUG_PRINT((DEBUG_DVI,"'%c' ",c));
  DEBUG_PRINT((DEBUG_DVI,"%d at (%d,%d) offset (%d,%d)",
	       (int)c, 
	       PIXROUND(h, dvi->conv*shrinkfactor),
	       PIXROUND(v, dvi->conv*shrinkfactor),
	       x_offset,y_offset));
#endif

  if (currentfont->type==FONT_TYPE_VF) { 
    DEBUG_PRINT((DEBUG_DVI," tfmw %d", 
		 ((struct vf_char*)currentfont->chr[c])->tfmw));
    return(SetVF(c));
  } else {
    struct pk_char* ptr = currentfont->chr[c];
    if (ptr) {
      if (ptr->glyph.data == NULL) 
	LoadAChar(c, ptr);
      DEBUG_PRINT((DEBUG_DVI," tfmw %d", ptr->tfmw));
      if (PassNo==PASS_DRAW)
	return(SetPK(c, h, v));
      else {
	/* Expand bounding box if necessary */
	min(x_min,PIXROUND(h, dvi->conv*shrinkfactor)
	    -PIXROUND(ptr->xOffset,shrinkfactor));
	min(y_min,PIXROUND(v, dvi->conv*shrinkfactor)
	    -PIXROUND(ptr->yOffset,shrinkfactor));
	max(x_max,PIXROUND(h, dvi->conv*shrinkfactor)
	    -PIXROUND(ptr->xOffset,shrinkfactor)
	    +ptr->glyph.w);
	max(y_max,PIXROUND(v, dvi->conv*shrinkfactor)
	    -PIXROUND(ptr->yOffset,shrinkfactor)
	    +ptr->glyph.h);
	return(ptr->tfmw);
      }
    }
  }
  return(0);
}

void DrawCommand(unsigned char* command, void* parent /* dvi/vf */)
     /* To be used both in plain DVI drawing and VF drawing */
{
  if (/*command >= SETC_000 &&*/ *command <= SETC_127) {
    MoveRight(SetChar((int32_t)*command));
  } else if (*command >= FONT_00 && *command <= FONT_63) {
    SetFntNum((int32_t)*command - FONT_00,parent);
  } else switch (*command)  {
  case PUT1: case PUT2: case PUT3: case PUT4:
    DEBUG_PRINT((DEBUG_DVI," %d",
		 UNumRead(command+1, dvi_commandlength[*command]-1)));
    (void) SetChar(UNumRead(command+1, dvi_commandlength[*command]-1));
    break;
  case SET1: case SET2: case SET3: case SET4:
    DEBUG_PRINT((DEBUG_DVI," %d",
		 UNumRead(command+1, dvi_commandlength[*command]-1)));
    MoveRight(SetChar(UNumRead(command+1, dvi_commandlength[*command]-1)));
    break;
  case SET_RULE:
    DEBUG_PRINT((DEBUG_DVI," %d %d",
		  UNumRead(command+1, 4), UNumRead(command+5, 4)));
    MoveRight(SetRule(DO_VFCONV(UNumRead(command+1, 4)),
		      DO_VFCONV(UNumRead(command+5, 4)),
		      h,v, PassNo));
    break;
  case PUT_RULE:
    DEBUG_PRINT((DEBUG_DVI," %d %d",
		  UNumRead(command+1, 4), UNumRead(command+5, 4)));
    (void) SetRule(DO_VFCONV(UNumRead(command+1, 4)),
		   DO_VFCONV(UNumRead(command+5, 4)),
		   h,v, PassNo);
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
    DEBUG_PRINT((DEBUG_DVI," %d",
		 SNumRead(command+1, dvi_commandlength[*command]-1)));
    MoveRight(DO_VFCONV(SNumRead(command+1, dvi_commandlength[*command]-1)));
    break;
  case W1: case W2: case W3: case W4:
    w = SNumRead(command+1, dvi_commandlength[*command]-1);
    DEBUG_PRINT((DEBUG_DVI," %d",w));
  case W0:
    MoveRight(DO_VFCONV(w));
    break;
  case X1: case X2: case X3: case X4:
    x = SNumRead(command+1, dvi_commandlength[*command]-1);
    DEBUG_PRINT((DEBUG_DVI," %d",x));
  case X0:
    MoveRight(DO_VFCONV(x));
    break;
  case DOWN1: case DOWN2: case DOWN3: case DOWN4:
    DEBUG_PRINT((DEBUG_DVI," %d",
		 SNumRead(command+1, dvi_commandlength[*command]-1)));
    MoveDown(DO_VFCONV(SNumRead(command+1, dvi_commandlength[*command]-1)));
    break;
  case Y1: case Y2: case Y3: case Y4:
    y = SNumRead(command+1, dvi_commandlength[*command]-1);
    DEBUG_PRINT((DEBUG_DVI," %d",y));
  case Y0:
    MoveDown(DO_VFCONV(y));
    break;
  case Z1: case Z2: case Z3: case Z4:
    z = SNumRead(command+1, dvi_commandlength[*command]-1);
    DEBUG_PRINT((DEBUG_DVI," %d",z));
  case Z0:
    MoveDown(DO_VFCONV(z));
    break;
  case FNT1: case FNT2: case FNT3: case FNT4:
    DEBUG_PRINT((DEBUG_DVI," %d",
		 UNumRead(command+1, dvi_commandlength[*command]-1)));
    SetFntNum(UNumRead(command+1, dvi_commandlength[*command]-1),parent);
    break;
  case XXX1: case XXX2: case XXX3: case XXX4:
    DEBUG_PRINT((DEBUG_DVI," %d",
		 UNumRead(command+1, dvi_commandlength[*command]-1)));
    SetSpecial(command + dvi_commandlength[*command], 
	       UNumRead(command+1, dvi_commandlength[*command]-1),h,v,PassNo);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
    if (((struct font_entry*)parent)->type==DVI_TYPE) {
      FontDef(command, parent); 
    } else {
      Fatal("%s within VF macro from %s",dvi_commands[*command],
	    ((struct font_entry*)parent)->name);
    }
    break;
  case PRE: case POST: case POST_POST:
    Fatal("%s occurs within page",dvi_commands[*command]);
    break;
  case NOP:
    break;
  default:
    Fatal("%s is an undefined command",dvi_commands[*command]);
    break;
  }
}

void DrawPage(void) 
     /* To be used after having read BOP and will exit cleanly when
      * encountering EOP.
      */
{
  unsigned char*  command;  /* current command                  */

  h = v = w = x = y = z = 0;
  currentfont = NULL;    /* No default font                  */

  command=DVIGetCommand(dvi);
  DEBUG_PRINT((DEBUG_DVI,"DRAW CMD:\t%s", dvi_commands[*command]));
  while (*command != EOP)  {
    DrawCommand(command,dvi);
    command=DVIGetCommand(dvi);
    DEBUG_PRINT((DEBUG_DVI,"DRAW CMD:\t%s", dvi_commands[*command]));
  } 
}

void DrawPages(void)
{
  struct page_list *dvi_pos;
  
  dvi_pos=NextPPage(dvi,NULL);
  if (dvi_pos!=NULL) {
    while(dvi_pos!=NULL) {
      SeekPage(dvi,dvi_pos);
      if (PassDefault == PASS_BBOX || PassDefault == PASS_TIGHT_BBOX) {
	if (PassDefault == PASS_TIGHT_BBOX) {
	  x_max = y_max = INT32_MIN;
	  x_min = y_min = INT32_MAX;
	}
	PassNo=PASS_BBOX;
	DrawPage();
	SeekPage(dvi,dvi_pos);
	x_width = x_max-x_min;
	y_width = y_max-y_min;
	x_offset = -x_min; /* offset by moving topleft corner */ 
	y_offset = -y_min; /* offset by moving topleft corner */
	x_max = x_min = -x_offset_def; /* reset BBOX */
	y_max = y_min = -y_offset_def;
      }
      DEBUG_PRINT((DEBUG_DVI,"\n  IMAGE:\t%dx%d",x_width,y_width));
      DEBUG_PRINT((DEBUG_DVI,"\n@%d PAGE START:\tBOP",dvi_pos->offset));
#ifdef DEBUG
      { 
	int i;
	for (i=0;i<10;i++) 
	  DEBUG_PRINT((DEBUG_DVI," %d",dvi_pos->count[i]));
	DEBUG_PRINT((DEBUG_DVI," (%d)\n",dvi_pos->count[10]));
      }
#endif
      CreateImage();
      Message(BE_NONQUIET,"[%d", dvi_pos->count[0]);
      PassNo=PASS_DRAW;
      DrawPage();
      WriteImage(dvi->outname,dvi_pos->count[0]);
#ifdef TIMING
      ++ndone;
#endif
      Message(BE_NONQUIET,"] ");
      fflush(ERR_STREAM);
      dvi_pos=NextPPage(dvi,dvi_pos);
    }
    Message(BE_NONQUIET,"\n");
    ClearPpList();
  }
}
