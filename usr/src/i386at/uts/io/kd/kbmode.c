#ident	"@(#)kbmode.c	1.3"
#ident	"$Header$"

#ifdef AT_KEYBOARD

/*
 * kbscr defines for the PC/AT in XT mode
 */

#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <io/ansi/at_ansi.h>
#include <io/kd/kd.h>
#include <io/ascii.h>
#include <io/termios.h>
#include <io/stream.h>
#include <io/strtty.h>
#include <io/stropts.h>
#include <proc/proc.h>
#include <io/xque/xque.h>
#include <io/ws/ws.h>
#include <svc/errno.h>
#include <util/cmn_err.h>

#ifdef DEBUG
STATIC int kbmode_debug = 0;
#define DEBUG1(a)	if (kbmode_debug == 1) printf a
#define DEBUG2(a)	if (kbmode_debug == 2) printf a
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */


/*
 * default key map table
 *
 * note: mapping of scan codes to characters is done within
 *	 screen driver
 */

#define K_NXSC		10	/* Switch to next screen */
#define	KS		K_SCRF
#define K_SCRF		11	/* Switch to first screen */
#define	IS_SCRKEY(x)	(((x) >= K_SCRF) && ((x) <= K_SCRL))
#define	K_RCTL		123	/* Right Ctrl */
#define	K_RALT		124	/* Right Alt */

#define	KS		K_SCRF
#define	KF		K_FUNF

int	kbscr_error = 0;

keymap_t cn_keymap = { SCO_TABLE_SIZE, {	/* number of scan codes */
/*  XT                                                     ALT     SPECIAL    */
/* SCAN                        CTRL          ALT    ALT    CTRL    FUNC       */
/* CODE   BASE   SHIFT  CTRL   SHIFT  ALT    SHIFT  CTRL   SHIFT   FLAGS  LOCK*/
/*  0*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*  1*/ {{A_ESC, A_ESC, K_NOP, K_NOP, A_ESC, A_ESC, K_NOP, K_NOP}, 0x33, L_O },
/*  2*/ {{'1',   '!',   K_NOP, K_NOP, '1',   '!',   K_NOP, K_NOP}, 0x33, L_O },
/*  3*/ {{'2',   '@',   A_NUL, A_NUL, '2',   '@',   A_NUL, A_NUL}, 0x00, L_O },
/*  4*/ {{'3',   '#',   K_NOP, K_NOP, '3',   '#',   K_NOP, K_NOP}, 0x33, L_O },
/*  5*/ {{'4',   '$',   K_NOP, K_NOP, '4',   '$',   K_NOP, K_NOP}, 0x33, L_O },
/*  6*/ {{'5',   '%',   K_NOP, K_NOP, '5',   '%',   K_NOP, K_NOP}, 0x33, L_O },
/*  7*/ {{'6',   '^',   A_RS,  A_RS,  '6',   '^',   A_RS,  A_RS }, 0x00, L_O },
/*  8*/ {{'7',   '&',   K_NOP, K_NOP, '7',   '&',   K_NOP, K_NOP}, 0x33, L_O },
/*  9*/ {{'8',   '*',   K_NOP, K_NOP, '8',   '*',   K_NOP, K_NOP}, 0x33, L_O },
/* 10*/ {{'9',   '(',   K_NOP, K_NOP, '9',   '(',   K_NOP, K_NOP}, 0x33, L_O },
/* 11*/ {{'0',   ')',   K_NOP, K_NOP, '0',   ')',   K_NOP, K_NOP}, 0x33, L_O },
/* 12*/ {{'-',   '_',   A_US,  A_US,  '-',   '_',   A_US,  A_US }, 0x00, L_O },
/* 13*/ {{'=',   '+',   K_NOP, K_NOP, '=',   '+',   K_NOP, K_NOP}, 0x33, L_O },
/* 14*/ {{A_BS,  A_BS,  A_DEL, A_DEL, A_BS,  A_BS,  A_DEL, A_DEL}, 0x00, L_O },
/* 15*/ {{A_HT,  K_BTAB,K_NOP, K_NOP, A_HT,  K_BTAB,K_NOP, K_NOP}, 0x77, L_O },
/* 16*/ {{'q',   'Q',   A_DC1, A_DC1, 'q',   'Q',   A_DC1, A_DC1}, 0x00, L_C },
/* 17*/ {{'w',   'W',   A_ETB, A_ETB, 'w',   'W',   A_ETB, A_ETB}, 0x00, L_C },
/* 18*/ {{'e',   'E',   A_ENQ, A_ENQ, 'e',   'E',   A_ENQ, A_ENQ}, 0x00, L_C },
/* 19*/ {{'r',   'R',   A_DC2, A_DC2, 'r',   'R',   A_DC2, A_DC2}, 0x00, L_C },
/* 20*/ {{'t',   'T',   A_DC4, A_DC4, 't',   'T',   A_DC4, A_DC4}, 0x00, L_C },
/* 21*/ {{'y',   'Y',   A_EM,  A_EM,  'y',   'Y',   A_EM,  A_EM }, 0x00, L_C },
/* 22*/ {{'u',   'U',   A_NAK, A_NAK, 'u',   'U',   A_NAK, A_NAK}, 0x00, L_C },
/* 23*/ {{'i',   'I',   A_HT,  A_HT,  'i',   'I',   A_HT,  A_HT }, 0x00, L_C },
/* 24*/ {{'o',   'O',   A_SI,  A_SI,  'o',   'O',   A_SI,  A_SI }, 0x00, L_C },
/* 25*/ {{'p',   'P',   A_DLE, A_DLE, 'p',   'P',   A_DLE, A_DLE}, 0x00, L_C },
/* 26*/ {{'[',   '{',   A_ESC, A_ESC, '[',   '{',   A_ESC, A_ESC}, 0x00, L_O },
/* 27*/ {{']',   '}',   A_GS,  A_GS,  ']',   '}',   A_GS,  A_GS }, 0x00, L_O },
/* 28*/ {{A_CR,  A_CR,  A_LF,  A_LF,  A_CR,  A_CR,  A_LF,  A_LF }, 0x00, L_O },
/* 29*/ {{K_CTL, K_CTL, K_CTL, K_CTL, K_CTL, K_CTL, K_CTL, K_CTL}, 0xff, L_O },
/* 30*/ {{'a',   'A',   A_SOH, A_SOH, 'a',   'A',   A_SOH, A_SOH}, 0x00, L_C },
/* 31*/ {{'s',   'S',   A_DC3, A_DC3, 's',   'S',   A_DC3, A_DC3}, 0x00, L_C },
/* 32*/ {{'d',   'D',   A_EOT, A_EOT, 'd',   'D',   A_EOT, A_EOT}, 0x00, L_C },
/* 33*/ {{'f',   'F',   A_ACK, A_ACK, 'f',   'F',   A_ACK, A_ACK}, 0x00, L_C },
/* 34*/ {{'g',   'G',   A_BEL, A_BEL, 'g',   'G',   A_BEL, A_BEL}, 0x00, L_C },
/* 35*/ {{'h',   'H',   A_BS,  A_BS,  'h',   'H',   A_BS,  A_BS }, 0x00, L_C },
/* 36*/ {{'j',   'J',   A_LF,  A_LF,  'j',   'J',   A_LF,  A_LF }, 0x00, L_C },
/* 37*/ {{'k',   'K',   A_VT,  A_VT,  'k',   'K',   A_VT,  A_VT }, 0x00, L_C },
/* 38*/ {{'l',   'L',   A_FF,  A_FF,  'l',   'L',   A_FF,  A_FF }, 0x00, L_C },
/* 39*/ {{';',   ':',   K_NOP, K_NOP, ';',   ':',   K_NOP, K_NOP}, 0x33, L_O },
/* 40*/ {{'\'',  '"',   K_NOP, K_NOP, '\'',  '"',   K_NOP, K_NOP}, 0x33, L_O },
/* 41*/ {{'`',   '~',   K_NOP, K_NOP, '`',   '~',   K_NOP, K_NOP}, 0x33, L_O },
/* 42*/ {{K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH, K_LSH}, 0xff, L_O },
/* 43*/ {{'\\',  '|',   A_FS,  A_FS,  '\\',  '|',   A_FS,  A_FS }, 0x00, L_O },
/* 44*/ {{'z',   'Z',   A_SUB, A_SUB, 'z',   'Z',   A_SUB, A_SUB}, 0x00, L_C },
/* 45*/ {{'x',   'X',   A_CAN, A_CAN, 'x',   'X',   A_CAN, A_CAN}, 0x00, L_C },
/* 46*/ {{'c',   'C',   A_ETX, A_ETX, 'c',   'C',   A_ETX, A_ETX}, 0x00, L_C },
/* 47*/ {{'v',   'V',   A_SYN, A_SYN, 'v',   'V',   A_SYN, A_SYN}, 0x00, L_C },
/* 48*/ {{'b',   'B',   A_STX, A_STX, 'b',   'B',   A_STX, A_STX}, 0x00, L_C },
/* 49*/ {{'n',   'N',   A_SO,  A_SO,  'n',   'N',   A_SO,  A_SO }, 0x00, L_C },
/* 50*/ {{'m',   'M',   A_CR,  A_CR,  'm',   'M',   A_CR,  A_CR }, 0x00, L_C },
/* 51*/ {{',',   '<',   K_NOP, K_NOP, ',',   '<',   K_NOP, K_NOP}, 0x33, L_O },
/* 52*/ {{'.',   '>',   K_NOP, K_NOP, '.',   '>',   K_NOP, K_NOP}, 0x33, L_O },
/* 53*/ {{'/',   '?',   K_NOP, K_NOP, '/',   '?',   K_NOP, K_NOP}, 0x33, L_O },
/* 54*/ {{K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH, K_RSH}, 0xff, L_O },
/* 55*/ {{'*',   '*',   K_NXSC,K_NXSC,'*',   '*',   K_NXSC,K_NXSC},0x33, L_O },
/* 56*/ {{K_ALT, K_ALT, K_ALT, K_ALT, K_ALT, K_ALT, K_ALT, K_ALT}, 0xff, L_O },
/* 57*/ {{' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' ',   ' '  }, 0x00, L_O },
/* 58*/ {{K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK, K_CLK}, 0xff, L_O },
/* 59*/ {{KF+0,  KF+12, KF+24, KF+36, KS+0,  KS+10, KS+0,  KS+10}, 0xff, L_O },
/* 60*/ {{KF+1,  KF+13, KF+25, KF+37, KS+1,  KS+11, KS+1,  KS+11}, 0xff, L_O },
/* 61*/ {{KF+2,  KF+14, KF+26, KF+38, KS+2,  KS+12, KS+2,  KS+12}, 0xff, L_O },
/* 62*/ {{KF+3,  KF+15, KF+27, KF+39, KS+3,  KS+13, KS+3,  KS+13}, 0xff, L_O },
/* 63*/ {{KF+4,  KF+16, KF+28, KF+40, KS+4,  KS+14, KS+4,  KS+14}, 0xff, L_O },
/* 64*/ {{KF+5,  KF+17, KF+29, KF+41, KS+5,  KS+15, KS+5,  KS+15}, 0xff, L_O },
/* 65*/ {{KF+6,  KF+18, KF+30, KF+42, KS+6,  KS+6,  KS+6,  KS+6 }, 0xff, L_O },
/* 66*/ {{KF+7,  KF+19, KF+31, KF+43, KS+7,  KS+7,  KS+7,  KS+7 }, 0xff, L_O },
/* 67*/ {{KF+8,  KF+20, KF+32, KF+44, KS+8,  KS+8,  KS+8,  KS+8 }, 0xff, L_O },
/* 68*/ {{KF+9,  KF+21, KF+33, KF+45, KS+9,  KS+9,  KS+9,  KS+9 }, 0xff, L_O },
/* 69*/ {{K_NLK, K_NLK, A_DC3, A_DC3, K_NLK, K_NLK, A_DC3, A_DC3}, 0xcc, L_O },
/* 70*/ {{K_SLK, K_SLK, A_DEL, A_DEL, K_SLK, K_SLK, A_DEL, A_DEL}, 0xcc, L_O },
/* 71*/ {{KF+48, '7',   '7',   '7',   '7',   '7',   '7',   '7'  }, 0x80, L_N },
/* 72*/ {{KF+49, '8',   '8',   '8',   '8',   '8',   '8',   '8'  }, 0x80, L_N },
/* 73*/ {{KF+50, '9',   '9',   '9',   '9',   '9',   '9',   '9'  }, 0x80, L_N },
/* 74*/ {{KF+51, '-',   '-',   '-',   '-',   '-',   '-',   '-'  }, 0x80, L_N },
/* 75*/ {{KF+52, '4',   '4',   '4',   '4',   '4',   '4',   '4'  }, 0x80, L_N },
/* 76*/ {{KF+53, '5',   '5',   '5',   '5',   '5',   '5',   '5'  }, 0x80, L_N },
/* 77*/ {{KF+54, '6',   '6',   '6',   '6',   '6',   '6',   '6'  }, 0x80, L_N },
/* 78*/ {{KF+55, '+',   '+',   '+',   '+',   '+',   '+',   '+'  }, 0x80, L_N },
/* 79*/ {{KF+56, '1',   '1',   '1',   '1',   '1',   '1',   '1'  }, 0x80, L_N },
/* 80*/ {{KF+57, '2',   '2',   '2',   '2',   '2',   '2',   '2'  }, 0x80, L_N },
/* 81*/ {{KF+58, '3',   '3',   '3',   '3',   '3',   '3',   '3'  }, 0x80, L_N },
/* 82*/ {{KF+59, '0',   '0',   '0',   '0',   '0',   '0',   '0'  }, 0x80, L_N },
/* 83*/ {{A_DEL, '.',   A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL}, 0x00, L_N },
/* 84*/ {{0x1f,  0x1f,  0x1f,  0x1f,  0x1f,  0x1f,  0x1f,  0x1f }, 0x00, L_O },
/* 85*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 86*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 87*/ {{KF+10, KF+22, KF+34, KF+46, KS+10, KS+10, KS+10, KS+10}, 0xff, L_O },
/* 88*/ {{KF+11, KF+23, KF+35, KF+47, KS+11, KS+11, KS+11, KS+11}, 0xff, L_O },
/* 89*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 90*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 91*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 92*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 93*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 94*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 95*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/* 96*/ {{KF+49, KF+49, KF+49, KF+49, KF+49, KF+49, KF+49, KF+49}, 0xff, L_O },
/* 97*/ {{KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52}, 0xff, L_O },
/* 98*/ {{KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57}, 0xff, L_O },
/* 99*/ {{KF+54, KF+54, KF+54, KF+54, KF+54, KF+54, KF+54, KF+54}, 0xff, L_O },
/*100*/ {{KF+48, KF+48, KF+48, KF+48, KF+48, KF+48, KF+48, KF+48}, 0xff, L_O },
/*101*/ {{KF+50, KF+50, KF+50, KF+50, KF+50, KF+50, KF+50, KF+50}, 0xff, L_O },
/*102*/ {{KF+56, KF+56, KF+56, KF+56, KF+56, KF+56, KF+56, KF+56}, 0xff, L_O },
/*103*/ {{KF+58, KF+58, KF+58, KF+58, KF+58, KF+58, KF+58, KF+58}, 0xff, L_O },
/*104*/ {{KF+59, KF+59, KF+59, KF+59, KF+59, KF+59, KF+59, KF+59}, 0xff, L_O },
/*105*/ {{A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL}, 0x00, L_O },
/*106*/ {{KF+53, KF+53, KF+53, KF+53, KF+53, KF+53, KF+53, KF+53}, 0xff, L_O },
/*107*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*108*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*109*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*110*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*111*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*112*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*113*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*114*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*115*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*116*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*117*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*118*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*119*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*120*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*121*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*122*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*123*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*124*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*125*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*126*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*127*/ {{K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP, K_NOP}, 0xff, L_O },
/*128*/ {{K_RCTL,K_RCTL,K_RCTL,K_RCTL,K_RCTL,K_RCTL,K_RCTL,K_RCTL},0xff, L_O },
/*129*/ {{K_RALT,K_RALT,K_RALT,K_RALT,K_RALT,K_RALT,K_RALT,K_RALT},0xff, L_O },
/*130*/ {{KF+59, KF+59, KF+59, KF+59, KF+59, KF+59, KF+59, KF+59}, 0xff, L_N },
/*131*/ {{A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL, A_DEL}, 0x00, L_N },
/*132*/ {{KF+48, KF+48, KF+48, KF+48, KF+48, KF+48, KF+48, KF+48}, 0xff, L_N },
/*133*/ {{KF+56, KF+56, KF+56, KF+56, KF+56, KF+56, KF+56, KF+56}, 0xff, L_N },
/*134*/ {{KF+50, KF+50, KF+50, KF+50, KF+50, KF+50, KF+50, KF+50}, 0xff, L_N },
/*135*/ {{KF+58, KF+58, KF+58, KF+58, KF+58, KF+58, KF+58, KF+58}, 0xff, L_N },
/*136*/ {{KF+54, KF+54, KF+54, KF+54, KF+54, KF+54, KF+54, KF+54}, 0xff, L_N },
/*137*/ {{KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52, KF+52}, 0xff, L_N },
/*138*/ {{KF+49, KF+49, KF+49, KF+49, KF+49, KF+49, KF+49, KF+49}, 0xff, L_N },
/*139*/ {{KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57, KF+57}, 0xff, L_N },
/*140*/ {{'/',   K_NOP, K_NOP, K_NOP, '/',   K_NOP, K_NOP, K_NOP}, 0x77, L_N },
/*141*/ {{A_CR,  A_CR,  A_LF,  A_LF,  A_CR,  A_CR,  A_LF,  A_LF }, 0x00, L_O },
}};


/*
 * Table indexed by AT scan codes yielding XT scan codes 
 */
char at2xt[][2] = {
	/*AT	XT	Ext	Key*/
	/*00*/	0,	0,
	/*01*/	67,	0,	/* F9 */
	/*02*/	65,	0,	/* F7 */
	/*03*/	63,	0,	/* F5 */
	/*04*/	61,	0,	/* F3 */
	/*05*/	59,	0,	/* F1 */
	/*06*/	60,	0,	/* F2 */
	/*07*/	88,	0,	/* F12 */
	/*08*/	0,	0,
	/*09*/	68,	0,	/* F10 */
	/*10*/	66,	0,	/* F8 */
	/*11*/	64,	0,	/* F6 */
	/*12*/	62,	0,	/* F4 */
	/*13*/	15,	0,	/* TAB */
	/*14*/	41,	0,	/* ~ ` */
	/*15*/	0,	0,
	/*16*/	0,	0,
	/*17*/	56,	129,	/* Left Alt, Right Alt */
	/*18*/	42,	0,	/* Left Shift */
	/*19*/	0,	0,
	/*20*/	29,	128,	/* Left Control, Right Control */
	/*21*/	16,	0,	/* Q */
	/*22*/	2,	0,	/* 1 */
	/*23*/	0,	0,
	/*24*/	0,	0,
	/*25*/	0,	0,
	/*26*/	44,	0,	/* Z */
	/*27*/	31,	0,	/* S */
	/*28*/	30,	0,	/* A */
	/*29*/	17,	0,	/* W */
	/*30*/	3,	0,	/* 2 */
	/*31*/	0,	0,
	/*32*/	0,	0,
	/*33*/	46,	0,	/* C */
	/*34*/	45,	0,	/* X */
	/*35*/	32,	0,	/* D */
	/*36*/	18,	0,	/* E */
	/*37*/	5,	0,	/* 4 */
	/*38*/	4,	0,	/* 3 */
	/*39*/	0,	0,
	/*40*/	0,	0,
	/*41*/	57,	0,	/* Space */
	/*42*/	47,	0,	/* V */
	/*43*/	33,	0,	/* F */
	/*44*/	20,	0,	/* T */
	/*45*/	19,	0,	/* R */
	/*46*/	6,	0,	/* 5 */
	/*47*/	0,	0,
	/*48*/	0,	0,
	/*49*/	49,	0,	/* N */
	/*50*/	48,	0,	/* B */
	/*51*/	35,	0,	/* H */
	/*52*/	34,	0,	/* G */
	/*53*/	21,	0,	/* Y */
	/*54*/	7,	0,	/* 6 */
	/*55*/	0,	0,
	/*56*/	0,	0,
	/*57*/	0,	0,
	/*58*/	50,	0,	/* M */
	/*59*/	36,	0,	/* J */
	/*60*/	22,	0,	/* U */
	/*61*/	8,	0,	/* 7 */
	/*62*/	9,	0,	/* 8 */
	/*63*/	0,	0,
	/*64*/	0,	0,
	/*65*/	51,	0,	/* , < */
	/*66*/	37,	0,	/* K */
	/*67*/	23,	0,	/* I */
	/*68*/	24,	0,	/* O */
	/*69*/	11,	0,	/* 0 */
	/*70*/	10,	0,	/* 9 */
	/*71*/	0,	0,
	/*72*/	0,	0,
	/*73*/	52,	0,	/* . > */
	/*74*/	53,	140,	/* / ? */
	/*75*/	38,	0,	/* L */
	/*76*/	39,	0,	/* ; : */
	/*77*/	25,	0,	/* P */
	/*78*/	12,	0,	/* - _ */
	/*79*/	0,	0,
	/*80*/	0,	0,
	/*81*/	0,	0,
	/*82*/	40,	0,	/* ' " */
	/*83*/	0,	0,
	/*84*/	26,	84,	/* [ { or Extended Sys Request/PrtScrn */
	/*85*/	13,	0,	/* = + */
	/*86*/	0,	0,
	/*87*/	0,	0,
	/*88*/	58,	0,	/* Caps Lock */
	/*89*/	54,	0,	/* Right Shift */
	/*90*/	28,	141,	/* Enter */
	/*91*/	27,	0,	/* ] } */
	/*92*/	0,	0,
	/*93*/	43,	0,	/* \ | */
	/*94*/	0,	0,
	/*95*/	0,	0,
	/*96*/	0,	0,
	/*97*/	86,	0,	/* S005 102 foreign keyboards need this map */
	/*98*/	0,	0,	
	/*99*/	0,	0,	
	/*100*/	0,	0,	
	/*101*/	0,	0,	
	/*102*/	14,	0,	/* Backspace */
	/*103*/	0,	0,	
	/*104*/	0,	0,	
	/*105*/	79,	133,	/* End Extended End */
	/*106*/	0,	0,	
	/*107*/	75,	137,	/* Left Arrow Extended Left Arrow */
	/*108*/	71,	132,	/* Home Extended Home */
	/*109*/	0,	0,	
	/*110*/	0,	0,	
	/*111*/	0,	0,	
	/*112*/	82,	130,	/* Insert Extended Insert */
	/*113*/	83,	131,	/* DEL Extended DEL */
	/*114*/	80,	139,	/* Arrow Down Extended Arrow Down */
	/*115*/	76,	0,	/* 5 */
	/*116*/	77,	136,	/* Arrow Right Extended Arrow Right */
	/*117*/	72,	138,	/* Arrow Up Extended Arrow Up */
	/*118*/	1,	0,	/* Escape */
	/*119*/	69,	0,	/* Num Lock */
	/*120*/	87,	0,	/* F11 */
	/*121*/	78,	0,	/* Num + */
	/*122*/	81,	135,	/* Num PgDn Extended Page Down */
	/*123*/	74,	0,	/* Num - */
	/*124*/	55,	0,	/* PrtScr * */
	/*125*/	73,	134,	/* Num PgUp Extended Page Up */
	/*126*/	70,	0,	/* Scroll Lock */
	/*127*/	0,	0,	
	/*128*/	0,	0,	
	/*129*/	0,	0,	
	/*130*/	0,	0,	
	/*131*/	65,	0,	/* extra F7	S003 */
	/*132*/	142,	0,	
	/*133*/	0,	0,	
	/*134*/	0,	0,	
	/*135*/	79,	0,	
	/*136*/	0,	0,	
	/*137*/	0,	0,	
	/*138*/	0,	0,	
	/*139*/	0,	0,	
	/*140*/	0,	140,	/* Extended Num '/' */
};

#define	KBD_RCMD	0x20	/* Command to read the current cmd byte */
#define	KCMD_PCC	0x40	/* cmd bit for PC compatibility */
#define	KBD_WCMD	0x60	/* Command to write a new cmd byte */
#define	KAT_BREAK	0xf0	/* first byte in two byte break sequence */
#define	KAT_EXTEND	0xe0	/* first byte in two byte extended sequence */
#define	KAT_BREAK	0xf0	/* first byte in two byte break sequence */
#define	KAT_EXTEND	0xe0	/* first byte in two byte extended sequence */

extern int     kb_raw_mode;


/*
 * void
 * switch_kb_mode(int)
 *
 * Calling/Exit State:
 *
 * Description:
 *	Put the keyboard into AT or XT mode.
 *
 *	Read the 8042's current command byte. If it's PC compatibility
 *	bit doesn't agree with our new mode, write a new command byte.
 */
void
switch_kb_mode(int kbmode)
{
	int	i;
	pl_t	s;


	if (kb_raw_mode == kbmode)
		return;

	s = spl1(); 

	kb_raw_mode = kbmode;

	DEBUG1(("Statement 1\n"));
	/* read the current command byte */
	xmit8042(3, KBD_RCMD, &i);				/* L006 */

	/*
	 * Write a new command byte if and only if its necessary	S000
	 */
	if (kbmode == KBM_XT && (i & KCMD_PCC) == 0 ||
	    kbmode == KBM_AT && (i & KCMD_PCC)) {
		DEBUG1(("Statement 2\n"));
		xmit8042(3, KBD_WCMD, (unsigned char *) 0); /* L006 */
		if (kbmode == KBM_XT) {
			DEBUG1(("Statement 3\n");
			/* turn on PC compatibility */	/* L006 */
			xmit8042(0, i|KCMD_PCC, (unsigned char *) 0);
		} else {
			DEBUG1(("Statement 4\n"));
			/* turn off PC compatibility */	/* L006 */
			xmit8042(0, i&~KCMD_PCC, (unsigned char *) 0);
		}
	}

	splx(s);
	DEBUG1(("Statement 5\n"));
}


/*
 * unchar
 * kd_xlate_at2xt(register unchar)
 *
 * Calling/Exit State:
 */
unchar
kd_xlate_at2xt(register unchar c)
{
	static int  saw_break = 0;
	static int  extend = 0;


	if (c == KAT_BREAK) {
		saw_break = KBD_BREAK;
		return 0;
	}

	if (c == KAT_EXTEND) {	/* AT extend prefix is 0xE0 */
		extend = 1;
		return 0;
	}

	c = at2xt[c][extend] & 0xff | saw_break;

	saw_break = 0;
	extend = 0;

	return c;
}


/*
 * unchar
 * char_xlate_at2xt(unsigned char)
 *
 * Calling/Exit State:
 */
unchar
char_xlate_at2xt(unsigned char c)
{
	static int  saw_break = 0;
	static int  extend = 0;


	if (c == KAT_BREAK) {
		saw_break = KBD_BREAK;
		return 0;
	}

	if (c == KAT_EXTEND) {	/* AT extend prefix is 0xE0 */
		extend = 1;
		return 0;
	}

	c = at2xt[c][extend] & 0xff | saw_break;

	saw_break = 0;
	extend = 0;

	return c;
}

#else

/*
 * void
 * switch_kb_mode(int)
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
void
switch_kb_mode(int kbmode)
{
}

#endif	/* AT_KEYBOARD */
