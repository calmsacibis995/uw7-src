/*
 *	@(#) lexer.h 11.1 97/10/22
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
   @(#) lexer.h 11.1 97/10/22
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
   lexer.h - definitions for token lexer
*/

typedef enum TokenType {
    TokEnd,
    TokBreak,
    TokWord,
    TokString,
    TokNumber
} TokenType;
    
typedef struct Lexeme {    
    TokenType type;
    int lead;
    int term;
    union {
	char brk;
	char *str;
	long num;
    } value;
} Lexeme;


extern  struct Lexeme *LexStream();
extern  void FreeLex();
extern  void dispLex();

/* <<< Function Prototypes >>> */

#if !defined(NOPROTO)

static  char *CheckQuote(char *,int );
extern  struct Lexeme *LexStream(char *,char *,char *,int (*)());
extern  void FreeLex(struct Lexeme *);
extern  void dispLex(struct Lexeme *);

#endif
