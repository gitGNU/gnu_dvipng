#include "dvipng.h"

struct stack_entry {  
  long4    h, v, w, x, y, z; /* stack entry                           */
} stack[STACK_SIZE];       /* stack                                   */
int     sp = 0;            /* stack pointer                           */
long4   w = 0;             /* current horizontal spacing              */
long4   x = 0;             /* current horizontal spacing              */
long4   y = 0;             /* current vertical spacing                */
long4   z = 0;             /* current vertical spacing                */

#define DO_VFCONV(a) (long4)((long long) a * vfstack[vfstackptr-1]->d / 65535l)

void DrawCommand P2C(unsigned char*, command, int, PassNo)
     /* To be used both in plain DVI drawing and VF drawing */
{
  int     k=0;             /* temporary parameter                     */
  long4   val, val2;       /* temporarys to hold command information  */

  if (/*command >= SETC_000 &&*/ *command <= SETC_127) {
    val=SetChar((long4)*command,PassNo);
    MoveOver(val);
  } else if (*command >= FONT_00 && *command <= FONT_63) {
    SetFntNum((long4)*command - FONT_00);
  } else switch (*command)  {
  case PUT1: case PUT2: case PUT3: case PUT4:
    val = UNumRead(command+1, (int)*command - PUT1 + 1);
    (void) SetChar(val,PassNo);
    break;
  case SET1: case SET2: case SET3: case SET4:
    val = UNumRead(command+1, (int)*command - SET1 + 1);
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
    val = SNumRead(command+1, (int)*command - RIGHT1 + 1);
    MoveOver(DO_VFCONV(val));
    break;
  case W1: case W2: case W3: case W4:
    w = SNumRead(command+1, (int)*command - W1 + 1);
  case W0:
    MoveOver(DO_VFCONV(w));
      break;
  case X1: case X2: case X3: case X4:
    x = SNumRead(command+1, (int)*command - X1 + 1);
  case X0:
    MoveOver(DO_VFCONV(x));
    break;
  case DOWN1: case DOWN2: case DOWN3: case DOWN4:
    val = SNumRead(command+1, (int)*command - DOWN1 + 1);
    MoveDown(DO_VFCONV(val));
    break;
  case Y1: case Y2: case Y3: case Y4:
    y = SNumRead(command+1, (int)*command - Y1 + 1);
  case Y0:
    MoveDown(DO_VFCONV(y));
    break;
  case Z1: case Z2: case Z3: case Z4:
    z = SNumRead(command+1, (int)*command - Z1 + 1);
  case Z0:
    MoveDown(DO_VFCONV(z));
    break;
  case FNT1: case FNT2: case FNT3: case FNT4:
    k = UNumRead(command+1, (int)* command - FNT1 + 1);
    SetFntNum(k);
    break;
  case XXX1: case XXX2: case XXX3: case XXX4:
    k = UNumRead(command+1, (int)*command - XXX1 + 1);
    if (PassNo == PASS_DRAW)
      DoSpecial(command + 1 + *command - XXX1 + 1, k);
    break;
  case FNT_DEF1: case FNT_DEF2: case FNT_DEF3: case FNT_DEF4:
    FontDef(command, &dvi); /* Not allowed within a VF macro */
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
}

void DrawPage P1C(int, PassNo) 
     /* To be used after having read BOP and will exit cleanly when
      * encountering EOP.
      */
{
  unsigned char  command[STRSIZE];  /* current command                  */

  h = v = w = x = y = z = 0;
  vfstack[1] = NULL;                /* No default font                  */

  ReadCommand(command);
#ifdef DEBUG
  if (Debug)
    printf("DRAW CMD:\t%d\n", *command);
#endif
  while (*command != EOP)  {
    DrawCommand(command,PassNo);
    ReadCommand(command);
#ifdef DEBUG
    if (Debug)
      printf("DRAW CMD:\t%d\n", *command);
#endif
  } 
}

unsigned char push=PUSH;
unsigned char pop=POP;

long4 SetVF P2C(long4, c, int, PassNo) 
{
  struct vf_char* ptr;
  unsigned char *command;
  long4 bytes_left, length;
  struct font_num *tfontnump;  /* temporary font_num pointer   */

  DrawCommand(&push,PassNo);
  w = x = y = z = 0;
  ptr = vfstack[vfstackptr]->ch[c];
  command = ptr->macro;
  bytes_left = ptr->macro_length;

  tfontnump = vfstack[vfstackptr]->hfontnump;
  while (tfontnump->next != NULL) {
    tfontnump = tfontnump->next;
  }
  vfstack[++vfstackptr] = tfontnump->fontp;
  if (vfstack[vfstackptr]->name[0]=='\0')
    FontFind(vfstack[vfstackptr]);

#ifdef DEBUG
  if (Debug)
    printf("VF CMD:\t%d\n", *command);
#endif
  while (bytes_left > 0)  {
    DrawCommand(command,PassNo);
    length = CommandLength(command)+1;
    command += length;
    bytes_left -= length;
#ifdef DEBUG
    if (Debug)
      printf("VF CMD:\t%d\n", *command);
#endif
  } /* while  */
  --vfstackptr;
  DrawCommand(&pop,PassNo);
  return(ptr->tfmw);
}

void DoPages P1H(void)
{
  struct page_list *tpagelistp;

  while((tpagelistp=FindPage(TodoPage()))!=NULL) {
    if (PassDefault == PASS_BBOX) {
      DrawPage(PASS_BBOX);
      /* Reset to after BOP of current page */
      FSEEK(dvi.filep, tpagelistp->offset+45, SEEK_SET); 
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
#ifdef TIMING
    ++ndone;
#endif
    qfprintf(ERR_STREAM,"] ");
    if (tpagelistp->next==NULL) 
      tpagelistp->next=InitPage();
  }
  qfprintf(ERR_STREAM,"\n");
}
