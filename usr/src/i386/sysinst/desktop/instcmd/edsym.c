#ident	"@(#)edsym.c	15.1"

/*		copyright	"%c%" 	*/

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<string.h>

#define	READ_VALUE	1
#define	WRITE_VALUE	2
#define	VAR_LENGTH	4
#define	VAR_OFFSET	8

#define	VAR_INTEGER	0x10
#define	VAR_STRING	0x20
#define	VAR_DOUBLE	0x40
#define	VAR_FLOAT	0x80

#define	VAR_HEX		0x100

#define USAGE_MSG \
"Usage: %s [-f inputfile] [-v var_value ] [-r|w] [-l] [-o] [-ISDF] [-X] loc execfile\n\
\n	Where LOCATION is of the format \"data-section-address offset var-address\"\n"

int	option_flags = READ_VALUE|VAR_STRING;

long	var_length = -1;
long	var_offset = 0;
char	*input_file = NULL;
char	*input_string = NULL;

char options_string[] = "f:v:l:o:ISDFH";

extern char	*optarg;
extern int	optind;
int		errflg = 0;

char	*program = NULL;	

unsigned long Ascii2Long();
Usage()
{
	fprintf(stderr, USAGE_MSG, program);
	exit(1);
}

main(argc, argv)
int	argc;
char	*argv[];
{
	unsigned long	where, dsection, offset, addr;
	int	fdinput = -1, fdexecfile = -1;
	int	n, c, inlength;
	char	*var_value;
	double	d;
	char	buf[512];

	var_value = (char *) &d;
	program = argv[0];

	if(argc < 5)
		Usage();

	while ((c = getopt(argc, argv, options_string)) != -1) {
		switch (c) {
		case 'S':
			option_flags |= VAR_STRING;
			break;
		case 'I':
			option_flags |= VAR_INTEGER;
			break;
		case 'F':
			option_flags |= VAR_FLOAT;
			break;
		case 'D':
			option_flags |= VAR_DOUBLE;
			break;
		case 'H':
			option_flags |= VAR_HEX;
			break;
		case 'l':
			var_length = atol(optarg);
			break;
		case 'o':
			var_offset = atol(optarg);
			break;
		case 'f':
			option_flags |= WRITE_VALUE;
			input_file =  optarg;
			break;
		case 'v':
			option_flags |= WRITE_VALUE;
			input_string = optarg;
			break;
		}
	}

	if (argc - optind != 4)
		++errflg;
	if (errflg)
		Usage();

	dsection = Ascii2Long(argv[optind]);
	offset = Ascii2Long(argv[optind+1]);
	addr = Ascii2Long(argv[optind+2]);
	where = addr - dsection + offset;

#ifdef DEBUG
	fprintf(stderr, "addr %s  %s + %s \n",
			argv[optind], argv[optind+1], argv[optind+2]);
	fprintf(stderr, "%s, addr %x - %x + %x where is %x\n",
			argv[optind], addr, dsection, offset, where);
	fprintf(stderr, "where is %x\n", where);
#endif

	if(input_string) {
		if(option_flags & VAR_INTEGER) {
			inlength = sizeof(int);
			sscanf(input_string, "%d", (int *) var_value);	
		} else if (option_flags & VAR_FLOAT) {
			inlength = sizeof(float);
			sscanf(input_string, "%f", (float *) var_value);	
		} else if (option_flags & VAR_DOUBLE) {
			inlength = sizeof(double);
			sscanf(input_string, "%lf", (double *) var_value);	
		} else {
			inlength = strlen(input_string);
			var_value = input_string;
		}
	} else if(input_file) {
		struct stat statbuf;
		if(stat(input_file, &statbuf) < 0) {
			fprintf(stderr, "stat failed on file %s\n", input_file);
			perror("");
			exit(1);
		}
		inlength = statbuf.st_size;
		fdinput = open(input_file, O_RDONLY);
		if(fdinput < 0) {
			fprintf(stderr, "Cannot open file %s\n", input_file);
			perror("");
			exit(1);
		}
	}
	
#ifdef DEBUG
	fprintf(stderr, "offset %x length %x where is %x\n",
		var_offset, var_length, where);
#endif
	if( var_offset > 0)
		where += var_offset;
#ifdef DEBUG
	fprintf(stderr, "offset %x length %x where is %x\n",
		var_offset, var_length, where);
#endif
	if((fdexecfile = open(argv[optind + 3], O_RDWR)) < 0) {
		fprintf(stderr, "Cannot open %s\n", argv[optind + 1]);
		exit(1);
	}
	if(lseek(fdexecfile, where, 0) != where) {
		fprintf(stderr, "lseek failed\n");
		exit(1);
	}

	if(option_flags & WRITE_VALUE){
		if(var_length > 0 && ((var_length - offset) < inlength)) {
			fprintf(stderr, "Input is much larger than field length\n");
			exit(1);
		}
		if(fdinput >= 0) {
			while((n = read(fdinput, buf, 512)) > 0)
				write(fdexecfile, buf, n);
		}
		else	write(fdexecfile, var_value, inlength);
		close(fdinput);
		close(fdexecfile);
		exit(0);
		
	}
#ifdef DEBUG
	fprintf(stderr, "offset %d length %d where is %d\n",
		var_offset, var_length, where);
#endif
	do {
		int	r;
		if(var_length < 512)
			n = var_length;
		else	n = 512;
		var_length -= n;
		r = read(fdexecfile, buf, n);
#ifdef DEBUG
		writeout(buf, r);
#endif
	} while (var_length > 0);
	exit(0);
}
#ifdef DEBUG
writeout(buf, r)
char buf[];
int	r;
{
	struct i {
		int	i;
	};
	struct f {
		float	f;
	};
	struct d {
		double	d;
	};
	if(option_flags & VAR_INTEGER) {
		printf("%d", ((struct i *) buf)->i);	
	} else if(option_flags & VAR_FLOAT) {
		printf("%f", ((struct f *) buf)->f);	
	} else if(option_flags & VAR_DOUBLE) {
		printf("%lf", ((struct d *) buf)->d);	
	} else {
		write(1, buf, r);
	}
}
#endif

unsigned long 
Ascii2Long(string)
char	  *string;
{
	char	ch;
	long  n, rv = 0;
	char	*p = &string[2];

	if(string[0] != '0' || (string[1] != 'x' && string[1] != 'X'))
			return(0);
	while((ch = *p++) != '\0') {
		if(ch >= '0' && ch <= '9')
                        n = ch - '0';
		else if(ch >= 'a' && ch <= 'f')
                        n = ch - 'a' + 10;
		else if(ch >= 'A' && ch <= 'F')
                        n = ch - 'A' + 10;
		else {
                      	fprintf(stderr, "Unexepected char (0%o) %c\n", ch);
                        continue;
		}
#ifdef DEBUG
  		fprintf(stderr, "char (0%o) %c: n = %d, rv %ld\n", 
						ch, ch, n, rv);
#endif
  		rv = rv * 16 + n;
	}
#ifdef DEBUG
  	fprintf(stderr, "Value of (%s) is (0x%x) \n", string, rv);
#endif
  	return(rv);
}

