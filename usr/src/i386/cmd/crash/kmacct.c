#ident	"@(#)crash:i386/cmd/crash/kmacct.c	1.1"
#ident "$Header$"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/kmacct.h>

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

char *device   = "/dev/kmacct";		/* Default kma device 		*/
char *myname;

typedef struct {
	int       kma_size;
	int       kma_ndict;
	int       kma_depth;
	int       kma_state;
	kmasym_t *kma_sym;
	char     *kma_stack;

	/*
	 * Evaluation
	 */
	long kma_reqa[KMSIZES];
	long kma_reqf[KMSIZES];
	long kma_ra, kma_rf;
} kma_t;

kma_t _kma, *kma = &_kma;

extern int   opterr, optind;
extern char *optarg;

main(argc,argv)
int argc; char **argv;
{
	int fd, c, mode = 0;

	if((myname = (char *)strrchr(argv[0],'/')) == NULL)
		myname = argv[0];
	else myname++;

	if((fd = open(device,O_RDWR)) == -1){
		fprintf(stderr,"%s: can't open %s, %s\n",
			myname,device,strerror(errno));
		exit(1);
	}

	opterr = 0;
	while((c = getopt(argc,argv,"n:s:d:")) != EOF){
		switch(c){
		case 'n': 
			fprintf(stderr,"Obsolete option - using getksym(2)\n");
			break;

		case 's':
			switch(optarg[1]){
			case 'n': /* on  */ ioctl(fd,KMACCT_ON,0);  break;
			case 'f': /* off */ ioctl(fd,KMACCT_OFF,0); break;
			}
			break;

		case 'd':
			mode = atoi(optarg); break;
			break;

		default :
			fprintf(stderr,
				"Usage: %s [-s {on,off}][-d level]\n",
				argv[0]);
			exit(1);
		}
	}


	if(KmaSetup(fd,kma,mode) < 0){
		fprintf(stderr,"%s: can't setup kma, %s\n",
			myname,strerror(errno));
		exit(1);
	}

	if(mode > 0){
		KmaSymDump(kma,mode);
	}else{
		printf("Size %d, ndict %d, depth %d, state %d\n",
			kma->kma_size,kma->kma_ndict,
			kma->kma_depth,kma->kma_state);
	}

	exit(0);
}

KmaSetup(fd,kma,mode)
int fd; kma_t *kma; int mode;
{
	int   r, i, j;
	register kmasym_t *sym;

	kma->kma_size  = ioctl(fd,KMACCT_SIZE,0);
	kma->kma_ndict = ioctl(fd,KMACCT_NDICT,0);
	kma->kma_depth = ioctl(fd,KMACCT_DEPTH,0);
	kma->kma_state = ioctl(fd,KMACCT_STATE,0);

	if(mode <= 0)
		return(0);

	if((kma->kma_sym = (kmasym_t *)malloc(kma->kma_size)) == NULL){
		fprintf(stderr,"%s: out of memory, %s\n",
			myname,strerror(errno));
		return(-1);
	}

	if((r = read(fd,kma->kma_sym,kma->kma_size)) == -1){
		fprintf(stderr,"%s: Read Error %d/%d, %s\n",
			myname,r,kma->kma_size,strerror(errno));
		return(-1);
	}
	kma->kma_stack = (char *)kma->kma_sym +
				 kma->kma_ndict * sizeof(kmasym_t);


	for(i=0, sym = kma->kma_sym; i<kma->kma_ndict; i++, sym++){
		if(sym->pc == 0)
			continue;

		for(j=0; j<KMSIZES; j++){
			if(sym->reqa[j] == 0 && sym->reqf[j] == 0)
				continue;

			kma->kma_reqa[j] += sym->reqa[j];
			kma->kma_reqf[j] += sym->reqf[j];
		}
	}

	for(j=0; j<KMSIZES; j++){
		if(kma->kma_reqa[j] == 0 && kma->kma_reqf[j] == 0)
			continue;

		kma->kma_ra += kma->kma_reqa[j];
		kma->kma_rf += kma->kma_reqf[j];
	}

	return(0);
}

/*
 *
 */
#define ChunkSize(i) (16 << (i))

KmaSymDump(kma,mode)
kma_t *kma; int mode;
{
	register kmasym_t *sym;
	register int i;

	if(mode > 2)
		for(i=0, sym = kma->kma_sym; i<kma->kma_ndict; i++, sym++)
			if(sym->pc != 0)
				SymDump(kma,sym);

	if(mode > 1){
		printf("Total:\n");
		for(i=0; i<KMSIZES; i++){
			if(kma->kma_reqa[i] == 0 && kma->kma_reqf[i] == 0)
				continue;

			printf("%4d B [%6d,%6d,%6d]\n",ChunkSize(i),
				kma->kma_reqa[i],kma->kma_reqf[i],
				kma->kma_reqa[i]-kma->kma_reqf[i]);
		}
	}

	printf("%7s[%6d,%6d,%6d]\n","",
		kma->kma_ra,kma->kma_rf,
		kma->kma_ra-kma->kma_rf);
}

SymDump(kma,sym)
kma_t *kma; kmasym_t *sym;
{
	register int i, l;
	caddr_t *pc;

	if(sym->pc == 0)
		return;

	i  = (sym - kma->kma_sym) * kma->kma_depth;
	pc = (caddr_t *)((char *)kma->kma_stack+i);
	l  = MAX(KMSIZES,kma->kma_depth);

	printf("%d:      Alloc-Chunk-Size      Stack\n",sym - kma->kma_sym);

	for(i=0; i<l; i++, pc++){
		if(i >= KMSIZES || (sym->reqa[i] == 0 && sym->reqf[i] == 0)){
			if(i >= kma->kma_depth)
				continue;

			printf("%4d B [%5s,%5s,%5s]%4s",ChunkSize(i),
				"-","-","-","");
		}else{
			printf("%4d B [%5d,%5d,%5d]%4s",ChunkSize(i),
				sym->reqa[i],sym->reqf[i],
				sym->reqa[i]-sym->reqf[i],"");
		}

		if(i < kma->kma_depth)
			printf("%8x %s",*pc,Addr2Symbol(*pc));

		printf("\n");
	}

	printf("\n");
}
