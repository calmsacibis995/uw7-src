#ident  "@(#)ptosm.h	1.2"
#ident	"$Header$"

#define		I2O_IOCTL		('I'<<24)|('2'<<16)|('O'<<8)
#define		I2O_PT_NUMIOPS		I2O_IOCTL | 0
#define		I2O_PT_MSGTFR		I2O_IOCTL | 1

typedef struct {

void 		*Data;
unsigned int	Length;
unsigned int	Flags;

#define	I2O_PT_DATA_READ	1
#define	I2O_PT_DATA_WRITE	2
#define	I2O_PT_DATA_MASK	(I2O_PT_DATA_READ|I2O_PT_DATA_WRITE)

} I2OPtData;


#define MAX_PT_SGL_BUFFERS	4

#define I2O_PT_VERSION_1	1

typedef struct {

unsigned char	Version;
unsigned char	IopNum;
unsigned char	Pad[2];
void		*Message;
unsigned int	MessageLength;
void		*Reply;
unsigned int	ReplyLength;
I2OPtData	Data[MAX_PT_SGL_BUFFERS];

} I2OptUserMsg_t;

