%{
#include "confdata.h"
#include "y.tab.h"
int linecount = 1;
%}
%%
[ \t]*			;
\n			linecount++;
card			return CARD;
device			return DEVICE;
option			return OPTION;
single			return SINGLE;
fixed			return FIXED;
static			return STATIC;
touchonly		return TOUCHONLY;
conflict		return CONFLICT;
nostatic		return NOSTATIC;
control			return CONTROL;
PnP			return PNP;
pseudoPnP		return PSEUDOPNP;
optional		return OPTIONAL;
nosingle		return NOSINGLE;
nofixed			return NOFIXED;
requires		return REQUIRES;
nooptional		return NOOPTIONAL;
port			return PORT;
irq			return IRQ;
dma			return DMA;
download		return DOWNLOAD;
lenght			return LENGTH;
length			return LENGTH;
choice			return CHOICE;
name			return NAME;
default			return DEFAULT;
range			return RANGE;
shadow			return SHADOW;
like			return LIKE;
bus			return BUS;
text			return TEXT;
ISA			return ISA;
ISAX			return ISAX;
MCA			return MCA;
PCI			return PCI;
detect			return DETECT;
include			return INCLUDE;
arch			return ARCH;
menu			return MENU;
type			return TYPE;
BIN			return BIN;
HEX			return HEX;
align			return ALIGN;

[a-zA-Z][a-zA-Z0-9]*	return IDENTIFIER;
0x[0-9a-fA-F]*		return HEXADECIMAL;
[0-9][0-9]*		return DECIMAL;
\,			return ',';
;			return ';';
-			return '-';
\{			return '{';
\}			return '}';
\"[^"]*\"		return STR;
\/\/[^\n]*		;
.			fprintf(stderr, "Unknown char %s\n", yytext);
%%
int yywrap(void){return 1;}
