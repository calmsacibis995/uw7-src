/*
 *	@(#) xsconfig.h 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 */

   
/*
   @(#) xsconfig.h 11.1 97/10/22
*/


/*
   (c) Copyright 1989 by Locus Computing Corporation.  ALL RIGHTS RESERVED.

   This material contains valuable proprietary products and trade secrets
   of Locus Computing Corporation, embodying substantial creative efforts
   and confidential information, ideas and expressions.  No part of this
   material may be used, reproduced or transmitted in any form or by any
   means, electronic, mechanical, or otherwise, including photocopying or
   recording, or in connection with any information storage or retrieval
   system without permission in writing from Locus Computing Corporation.
*/

/*
   xsconfig.h - Definitions for xsconfig main module
*/

/* <<< Function Prototypes >>> */

#if !defined(NOPROTO)

extern  int main(int ,char * *);
extern  int newChar(void );
extern  void ErrorMsg(char *,...);
extern  void ProcessFile(char *);
extern  void InitDummy(void );
extern  struct Lexeme *ProcDummy(struct Lexeme *);
extern  void InitDefines(void );
extern  struct Lexeme *ProcDefines(struct Lexeme *);
extern  struct Lexeme *ProcParams(struct Lexeme *);
extern  void InitModifiers(void );
extern  struct Lexeme *ProcModifiers(struct Lexeme *);
extern  void InitKeysyms(void );
extern  struct Lexeme *ProcKeysyms(struct Lexeme *);
extern  void InitButtons(void );
extern  struct Lexeme *ProcButtons(struct Lexeme *);
extern  void InitXlate(void );
extern  struct Lexeme *ProcXlate(struct Lexeme *);
extern  void InitRGBDef(void );
extern  struct Lexeme *ProcRGBDef(struct Lexeme *);
extern  void InitPixels(void );
extern  void AddPixel(struct PixelValue *);
extern  struct Lexeme *ProcPixels(struct Lexeme *);
extern  void WriteConfigFile(char *);

#endif
