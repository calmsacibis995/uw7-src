/*
 *	@(#) lexer.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *	S000	Tue Sep 03 14:55:04 PDT 1991	mikep@sco.com
 *	- Added an extra malloc rather than fixing a
 *	stupid parsing bug.  This probably wasn't a problem with OMF.
 *
 */
#if defined(SCCS_ID)
static char Sccs_Id[] =
	 "@(#) lexer.c 11.1 97/10/22
#endif

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
   lexer.c - Token lexer
*/

#include <stdio.h>
#include <string.h>

#include "lexer.h"
#include "alloc.h"

typedef enum State {
    InWhite, InToken, InString
}State;
       
typedef enum CharType {
    IsEnd, IsWhite, IsBreak, IsQuote, IsToken
} CharType;
    
static char *    
CheckQuote(quotes, c)
char *quotes;
int c;
{
    char *cp;
    
    for (cp=quotes; *cp; cp+=2) {
	if (*cp == c)
	    return cp+1;
    }
    return NULL;
}
       
Lexeme *
LexStream(white, breaks, quotes, nextChar)
char *white;
char *breaks;
char *quotes;
int (*nextChar)();
{
    State state;
    char *curToken;
    int curChar;
    CharType curCharType;
    int curTokLen;
    int curTokMax;
    int closeQuote;
    Lexeme *lexList;
    Lexeme *curLex;
    int maxLex;
    int curLexIndex;
    char *cp;
    int addChar;
    int addLexeme;
    int ungot;
    long num;
    char hack_space;

    /*
       Initialize
    */
    state = InWhite;
    curTokLen = 0;
    curTokMax = 0;
    curToken = NULL;
    addChar = 0;
    addLexeme = 0;
    ungot = 0;
    curLexIndex = 0;
    maxLex = 10;
    lexList = (Lexeme *)AllocMem(sizeof(Lexeme) * maxLex);
    curLex = lexList;
    curLex->type = TokEnd;
    
    /*
       Process characters until we hit eof
    */
    
    do {
	   
	/*
	   Get next character
	*/
	if (ungot) {
	    curChar = ungot;
	    ungot = 0;
	}
	else {
	    curChar = (*nextChar)();
	}
	
	/*
	   Find type of new character
	*/
	if (curChar == EOF) {
	    curCharType = IsEnd;
	}
	else if (cp = strchr(white, curChar)) {
	    curCharType = IsWhite;
	}
	else if (cp = strchr(breaks, curChar)) {
	    curCharType = IsBreak;
	}
	else if (cp = CheckQuote(quotes, curChar)) {
	    curCharType = IsQuote;
	}
	else {
	    curCharType = IsToken;
	}
	
	/*
	   Grind....
	*/
	switch (state) {
	    
	    case InWhite: 
		
		switch (curCharType) {
		    
		    case IsEnd:
		    case IsWhite:
			break;
			
		    case IsBreak:
			curLex->type = TokBreak;
			addLexeme++;
			break;
			
		    case IsQuote:
			closeQuote = *cp;
			curLex->type = TokString;
			curLex->lead = curChar;
			state = InString;
			break;
			
		    case IsToken:
			curLex->type = TokWord;
			state = InToken;
			addChar++;
			break;
		}
		
		break;			/* InWhite */
		
	    case InToken:
		switch (curCharType) {
		    
		    case IsEnd:
		    case IsWhite:
		    case IsBreak:
		    case IsQuote:
			curLex->term = curChar;
			addLexeme++;
			ungot = curChar;
			state = InWhite;
			break;
			
		    case IsToken:
			addChar++;
			break;
		}
			
		break;			/* InToken */
		
	    case InString:
		switch (curCharType) {
		    
		    case IsEnd:
			curLex->term = curChar;
			addLexeme++;
			state = InWhite;
			ungot = curChar;
			break;
			
		    case IsQuote:
		    case IsWhite:
		    case IsBreak:
		    case IsToken:
			if (curChar == closeQuote) {
			    curLex->term = curChar;
			    addLexeme++;
			    state = InWhite;
			}
			else {
			    addChar++;
			}
			break;
			
		}
			
		break;			/* InString */
	}
	
	if (addChar) {
	    if (curTokLen >= curTokMax) {
		curTokMax += 128;
		curToken = ReallocMem(curToken, curTokMax+1);
	    }
	    curToken[curTokLen++] = curChar;
	    addChar = 0;
	}
	    
	if (addLexeme) {
	    if(curToken == NULL)				/* S000 */
	       curToken = ReallocMem(curToken, 1);		/* S000 */
	    curToken[curTokLen] = '\0';
	    switch (curLex->type) {
		
		case TokBreak:
		    curLex->value.brk = curChar;
		    break;
		    
		case TokWord:
		    num = strtol(curToken, &cp, 0);
		    if (*cp) {
			curLex->value.str = StrDup(curToken);
		    }
		    else {
			curLex->type = TokNumber;
			curLex->value.num = num;
		    }
		    break;
		    
		case TokString:
		    curLex->value.str = StrDup(curToken);
		    break;
	    }
	    
	    curLexIndex++;
	    if (curLexIndex >= maxLex) {
		maxLex += 10;
		lexList = (Lexeme *)ReallocMem(lexList, sizeof(Lexeme) * maxLex);
	    }
	    curLex = &lexList[curLexIndex];
	    curLex->type = TokEnd;
	    curTokLen = 0;
	    addLexeme = 0;
	    
	}
	    
	
    } while (curChar != EOF || state != InWhite);
    
    FreeMem(curToken);
    
    return lexList;
}

void
FreeLex(lexList)
Lexeme *lexList;
{
    Lexeme *lex;
    
    for (lex=lexList; lex->type != TokEnd; lex++) {
	if (lex->type == TokString || lex->type == TokWord)
	    FreeMem(lex->value.str);
    }
    
    FreeMem(lexList);
}

void
dispLex(list)
Lexeme *list;
{
    Lexeme *lex;
    char *type;
    
    for (lex=list; lex->type != TokEnd; lex++) {
	switch (lex->type) {
	
	    case TokBreak:
		printf("%10s: %c\n", "Break", lex->value.brk);
		break;
		    
	    case TokWord:
		printf("%10s: %s\n", "Word", lex->value.str);
		break;
		    
	    case TokString:
		printf("%10s: %c %s %c\n", "String", 
				lex->lead, lex->value.str, lex->term);
		break;
		    
	    case TokNumber:
		printf("%10s: %ld\n", "Number", lex->value.num);
		break;
		    
	}
    }
}

