include $(CMDRULES)

OWN=		bin
GRP=		bin
INSDIR1=	${TOOLS}/usr/bin
INSDIR2=	${TOOLS}/usr/lib

all:		docats mkcatdefs

docats:

mkcatdefs:

install_xenv:	all
		$(INS) -f $(INSDIR1) -m 0755 -u $(OWN) -g $(GRP) mkcatdefs
		$(INS) -f $(INSDIR2) -m 0644 -u $(OWN) -g $(GRP) mkcatdefs.aw
		$(INS) -f $(INSDIR1) -m 0755 -u $(OWN) -g $(GRP) docats

clean:
		rm -f *.o a.out core

clobber:	clean
