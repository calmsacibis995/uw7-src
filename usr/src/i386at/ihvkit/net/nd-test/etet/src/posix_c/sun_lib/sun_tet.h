/*
 * Copyright 1992 SunSoft, Inc.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of SunSoft, Inc. not be used in 
 * advertising or publicity pertaining to distribution of the software 
 * without specific, written prior permission.  SunSoft, Inc. makes
 * no representations about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * SunSoft, Inc. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
 * EVENT SHALL SunSoft, Inc. BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* sun_tet.h */

/* TET result code definitions */
#define PASS 0
#define FAIL 1
#define FATAL 32

/* for functions that do comparision */
#define EQUAL 0
#define NOT_EQUAL 1

int 	err_cnt;

extern char *tet_pname;	/* test name */
