/*
 *	@(#) xserror.c 11.1 97/10/22
 *
 *	Copyright (C) The Santa Cruz Operation, 1991.
 *	This Module contains Proprietary Information of
 *	The Santa Cruz Operation, and should be treated as Confidential.
 */
/*
 *	SCO MODIFICATION HISTORY
 *
 *
 */
#if defined(SCCS_ID)
static char Sccs_Id[] = 
	 "@(#) xserror.c 11.1 97/10/22
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
   xserror.c - error messages for Xsight configuration compiler
*/

char errFile[] = "%s: %s\n";
char errHeap[] = "Out of heap space\n";

char errSyntax[] = "Syntax error\n";
char errBadSection[] = "Invalid section name: %s\n";
char errLeftOvers[] = "Extra characters on line\n";
char errParamName[] = "Unrecognized parameter: %s\n";
char errParamType[] = "Value does not match parameter type\n";
char errModName[] = "Unrecognized modifier name\n";
char errScanCode[] = "Invalid scan code  (Must be <= %d)\n";
char errKeysymName[] = "Unrecognized keysym name: %s\n";
char errNumKeysyms[] = "Too many symbols for key  (Must be <= %d)\n";
char errMaxButtons[] = "Too many button numbers  (Must be <= %d)\n";
char errRGBValue[] = "Bad RGB value (must be between 0 and 255): %d\n";
char errRGBName[] = "Missing color name\n";
char errColorName[] = "Unrecognized color name: %s\n";
char errBadKeycode[] = "Invalid key code.  (Must be 0xnn, 0xe0nn or 0xe1nn\n";
char errBadKeyCtrl[] = "Unrecognized keyboard control: %s\n";
