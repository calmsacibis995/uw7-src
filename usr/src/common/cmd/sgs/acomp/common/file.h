#ident	"@(#)acomp:common/file.h	1.4"
/* file.h */

/* The function available in file.c. */
#ifdef __STDC__
extern int in_system_header(void);
extern void record_incdir(char *, int);
extern int record_start_of_src_file(int    seq_number,
				    int    line_number,
				    char  *file_name,
				    int    is_include_file);
extern void record_end_of_src_file(int	seq_number);
#ifndef LINT
extern void conv_seq_to_line_and_file_index(int    sequence_number,
					    int   *line_number,
					    int   *file_index);

#endif  /* idndef LINT */
#else

extern int in_system_header();
extern void record_incdir();
extern int record_start_of_src_file();
extern void record_end_of_src_file();
#ifndef LINT
extern void conv_seq_to_line_and_file_index();
#endif  /* idndef LINT */

#endif  /* __STDC__ */
