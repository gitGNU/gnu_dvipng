#include "dvipng.h"

struct stack_entry {  
  int32_t    h, v, w, x, y, z; /* stack entry                           */
} stack[STACK_SIZE];       /* stack                                   */
int       sp = 0;            /* stack pointer                           */

#define DO_VFCONV(a) (parent->type==DVI_TYPE)?a:\
    (int32_t)((int64_t) a * parent->d / 65535l)

void DrawCommand P3C(unsigned char*, command, int, PassNo, 
		     struct dvi_vf_entry*, parent)
     /* To be used both in plain DVI drawing and VF drawing */
{
  int32_t   val, val2;       /* temporarys to hold command information  */

  if (/*command >= SETC_000 &&*/ *command <= SETC_127) {
    val=SetChar((int32_t)*command,PassNo);
    MoveOver(val);
  } else if (*command >= FONT_00 && *command <= FONT_63) {
    SetFntNum((int32_t)*command - FONT_00,parent);
  } else switch (*command)  {
  case PUT1: case PUT2: case PUT3: case PUT4:
    val = UNumRead(command+1, dvi_commandlength[*command]-1);
    (void) SetChar(val,PassNo);
    break;
  case SET1: case SET2: case SET3: case SET4:
    val = UNumRead(command+1, dvi_commandlength[*command]-1);
    val2 = SetChar(val,PassNo);
    MoveOver(val2);
    break;
  case SET_RULE:
    val = UNumRead(command+1, 4);
    val2 = UNumRead(command+5, 4);
    val = SetRule(DO_VFCONV(val), DO_VFCONV(val2), PassNo);
    MoveOver(val);
    break;
  case PUT_RULE:
    val = UNumRead(command+1, 4);
    val2 = UNumRead(command+5, 4);
    (void) SetRule(DO_VFCONV(val), DO_VFCONV(val2), PassNo);
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
    val = SNumRead(command+1, dvi_commandlength[*command]-1);
    MoveOver(DO_VFCONV(val));
    break;
  case W1: case W2: case W3: case W4:
    w = SNumRead(command+1, dvi_commandlength[*command]-1);
  case W0:
    MoveOver(DO_VFCONV(w));
      break;
  case X1: case X2: case X3: case X4:
    x = SNumRead(command+1, dvi_commandlength[*command]-1);
  case X0:
    MoveOver(DO_VFCONV(x));
    break;
  case DOWN1: case DOWN2: case DOWN3: case DOWN4:
    val = SNumRead(command+1, dvi_commandlength[*command]-1);
    MoveDown(DO_VFCONV(val));
    break;
  case Y1: case Y2: case Y3: case Y4:
    y = SNumRead(command+1, dvi_commandlength[*command]-1);
  case Y0:
    MoveDown(DO_VFCONV(y));
    break;
  case Z1: case Z2: case Z3: case Z4:
    z = SNumRead(command+1, dvi_commandlength[*command]-1);
  case Z0:
    MoveDown(DO_VFCONV(z));
    break;
  case FNT1: case FNT2: case FNT3: case FNT4:
    val = UNumRead(command+1, dvi_commandlength[*command]-1);
    SetFntNum(val,parent);
    break;
  case XXX1: case XXX2: case XXX3: case XXX4:
    val = UNumRead(command+1, dvi_commandlength[*command]-1);
    if (PassNo == PASS_DRAW)
      DoSpecial(command + dvi_commandlength[*command], val);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
    /* Not really allowed within a VF macro, but... */
    FontDef(command, parent); 
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

void DrawPage P1C(int, PassNo) 
     /* To be used after having read BOP and will exit cleanly when
      * encountering EOP.
      */
{
  unsigned char*  command;  /* current command                  */

  h = v = w = x = y = z = 0;
  currentfont = NULL;                /* No default font                  */

  command=DVIGetCommand(dvi);
#ifdef DEBUG
  if (Debug)
    printf("DRAW CMD:\t%s ", dvi_commands[*command]);
#endif
  while (*command != EOP)  {
    DrawCommand(command,PassNo,(struct dvi_vf_entry*)dvi);
    command=DVIGetCommand(dvi);
#ifdef DEBUG
    if (Debug)
      printf("DRAW CMD:\t%s ", dvi_commands[*command]);
#endif
  } 
}

void DoPages P1H(void)
{
  struct page_list *tpagelistp=NULL;

  while((tpagelistp=FindPage(TodoPage()))!=NULL
	&& tpagelistp->count[10]!=-1) {
    if (PassDefault == PASS_BBOX) {
      DrawPage(PASS_BBOX);
      /* Reset to after BOP of current page */
      fseek(dvi->filep, tpagelistp->offset+45, SEEK_SET); 
      x_width = x_max-x_min;
      y_width = y_max-y_min;
      x_offset = -x_min; /* offset by moving topleft corner */
      y_offset = -y_min; 
      x_max = x_min = -x_offset_def; /* reset BBOX */
      y_max = y_min = -y_offset_def;
    }
    qfprintf(ERR_STREAM,"[%d",  tpagelistp->count[0]);
#ifdef DEBUG
    if (Debug)
      printf("@%d PAGE START:\tBOP ",tpagelistp->offset);
#endif
    DoBop();
    DrawPage(PASS_DRAW);
    FormFeed(dvi,tpagelistp->count[0]);
#ifdef TIMING
    ++ndone;
#endif
    qfprintf(ERR_STREAM,"] ");
    if (tpagelistp->next==NULL) 
      tpagelistp->next=InitPage();
  }
  qfprintf(ERR_STREAM,"\n");
}
