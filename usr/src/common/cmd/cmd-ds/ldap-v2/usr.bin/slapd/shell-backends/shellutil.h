/*
 shellutil.h

 Copyright (c) 1995 Regents of the University of Michigan.
 All rights reserved.

 Redistribution and use in source and binary forms are permitted
 provided that this notice is preserved and that due credit is given
 to the University of Michigan at Ann Arbor. The name of the University
 may not be used to endorse or promote products derived from this
 software without specific prior written permission. This software
 is provided ``as is'' without express or implied warranty.
*/


#define MAXLINELEN	512

#define STR_OP_SEARCH	"SEARCH"


struct inputparams {
    int		ip_type;
#define IP_TYPE_SUFFIX		0x01
#define IP_TYPE_BASE		0x02
#define IP_TYPE_SCOPE		0x03
#define IP_TYPE_ALIASDEREF	0x04
#define IP_TYPE_SIZELIMIT	0x05
#define IP_TYPE_TIMELIMIT	0x06
#define IP_TYPE_FILTER		0x07
#define IP_TYPE_ATTRSONLY	0x08
#define IP_TYPE_ATTRS		0x09
    char	*ip_tag;
};


struct ldsrchparms {
    int		ldsp_scope;
    int		ldsp_aliasderef;
    int		ldsp_sizelimit;
    int		ldsp_timelimit;
    int		ldsp_attrsonly;
    char	*ldsp_filter;
    char	**ldsp_attrs;
};


struct ldop { 
    int		ldop_op;
#define LDOP_SEARCH	0x01
    char	**ldop_suffixes;
    char	*ldop_dn;
    union {
		    struct ldsrchparms LDsrchparams;
	  }	ldop_params;
#define ldop_srch	ldop_params.LDsrchparams
};


struct ldattr {
    char	*lda_name;
    char	**lda_values;
};


struct ldentry {
    char		*lde_dn;
    struct ldattr	**lde_attrs;
};


#ifdef LDAP_DEBUG
void debug_printf();
#else /* LDAP_DEBUG */
#define debug_printf()
#endif /* LDAP_DEBUG */

/*
 * function prototypes
 */
void write_result( FILE *fp, int code, char *matched, char *info );
void write_entry( struct ldop *op, struct ldentry *entry, FILE *ofp );
int test_filter( struct ldop *op, struct ldentry *entry );
void free_entry( struct ldentry *entry );
int attr_requested( char *name, struct ldop *op );
int parse_input( FILE *ifp, FILE *ofp, struct ldop *op );
struct inputparams *find_input_tag( char **linep );
void add_strval( char ***sp, char *val );
char *ecalloc( unsigned nelem, unsigned elsize );
void *erealloc( void *s, unsigned size );
char *estrdup( char *s );

/*
 * global variables
 */
extern int	debugflg;
extern char	*progname;
