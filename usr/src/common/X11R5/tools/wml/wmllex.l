#ident	"@(#)mmisc:tools/wml/wmllex.l	1.1"
%{
/*
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
 */
/*
 * Motif Release 1.2.1
 */
#ifdef YY_BUFFER_SIZE
#define BUF_SIZE YY_BUFFER_SIZE
#else
#define BUF_SIZE 8192
#endif
char            yystringval[BUF_SIZE];      /* any string value */
char            yynameval[BUF_SIZE];         /* any identifier (name) */
int             yytknval1;              /* terminal token value 1 */
int             yytknval2;              /* terminal token value 2 */

%}

%p 3000
%%

[ \t\f]		{}

[\n]		{ wml_line_count += 1; }

"Class"			{ return CLASS; }
"Resource"		{ return RESOURCE; }
"DataType"		{ return DATATYPE; }
"ControlList"		{ return CONTROLLIST; }
"EnumerationSet"	{ return ENUMERATIONSET; }
"EnumerationValue"	{ return ENUMERATIONVALUE; }
"CharacterSet"		{ return CHARACTERSET; }
"Child"			{ return CHILD; }

"DocName"		{ return DOCNAME; }
"ConvenienceFunction"	{ return CONVFUNC; }
"Alias"			{ return ALIAS; }
"Type"			{ return TYPE; }
"ResourceLiteral"	{ return RESOURCELITERAL; }
"Related"		{ return RELATED; }
"InternalLiteral"	{ return INTERNALLITERAL; }
"Constraint"		{ return CONSTRAINT; }
"Exclude"		{ return EXCLUDE;}
"Resources"		{ return RESOURCES; }
"SuperClass"		{ return SUPERCLASS; }
"ParentClass"		{ return PARENTCLASS; }
"Controls"		{ return CONTROLS; }
"WidgetClass"		{ return WIDGETCLASS; }
"DialogClass"		{ return DIALOGCLASS; }
"Default"		{ return DEFAULT; }
"EnumLiteral"		{ return ENUMLITERAL; }
"XmStringCharsetName"	{ return XMSTRINGCHARSETNAME; }
"Direction"		{ return DIRECTION; }
"ParseDirection"	{ return PARSEDIRECTION; }
"CharacterSize"		{ return CHARACTERSIZE; }
"ControlsMapToResource"	{ return CTRLMAPSRESOURCE; }
"Children"		{ return CHILDREN; }

"MetaClass"		{ return METACLASS;}
"Widget"		{ return WIDGET;}
"Gadget"		{ return GADGET;}
"Argument"		{ return ARGUMENT;}
"Reason"		{ return REASON;}
"Constraint"		{ return CONSTRAINT;}
"SubResource"		{ return SUBRESOURCE;}
"True"			{ return ATTRTRUE; }
"False"			{ return ATTRFALSE; }
"LeftToRight"		{ return LEFTTORIGHT; }
"RightToLeft"		{ return RIGHTTOLEFT; }
"OneByte"		{ return ONEBYTE; }
"TwoByte"		{ return TWOBYTE; }
"MixedOneAndTwoByte"	{ return MIXED1_2BYTE; }

":"			{ return COLON; }
";"			{ return SEMICOLON; }
"="			{ return EQUALS; }
"{"			{ return LBRACE; }
"}"			{ return RBRACE; }

"!"[^\n]*	{}

[a-zA-Z][a-zA-Z0-9$_]* {	/* string without quotes	*/
	strcpy (yystringval, yytext);
	return STRING;
	}

\"[^"\n]*\\	{	/* escaped character in the string 	*/
		yymore();
		}

\"[^"\n]*\"	{	/* String in quotes	*/
		strncpy(yystringval,yytext+1, yyleng - 2);
		yystringval[yyleng-2] = '\0' ;
		return STRING;
		}
\"[^"\n]*	{
		printf ("\nUnterminated string near %s, line %d",
			yytext, wml_line_count);
		return ERRORTOKEN;
		}

.	{
	printf ("\nEncountered illegal character '%c', line %d",
		yytext[0], wml_line_count);
	return ERRORTOKEN;
	}
