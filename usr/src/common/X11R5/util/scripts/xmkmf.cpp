XCOMM!/bin/sh

XCOMM
XCOMM generate a Makefile from an Imakefile from inside or outside the sources
XCOMM 
XCOMM $XConsortium: xmkmf.cpp,v 1.18 91/08/22 11:08:01 rws Exp $

usage="usage:  $0 [-a] [top_of_sources_pathname [current_directory]]"

topdir=
curdir=.
do_all=

case "$1" in
-a)
    do_all="yes"
    shift
    ;;
esac

case $# in 
    0) ;;
    1) topdir=$1 ;;
    2) topdir=$1  curdir=$2 ;;
    *) echo "$usage" 1>&2; exit 1 ;;
esac

case "$topdir" in
    -*) echo "$usage" 1>&2; exit 1 ;;
esac

if [ -f Makefile ]; then 
    echo mv Makefile Makefile.bak
    mv Makefile Makefile.bak
fi

if [ "$topdir" = "" ]; then
    args="-DUseInstalled "CONFIGDIRSPEC
else
    args="-I$topdir/config -DTOPDIR=$topdir -DCURDIR=$curdir"
fi

echo imake $args
case "$do_all" in
yes)
    imake $args && 
    echo "make Makefiles" &&
    make Makefiles &&
    echo "make includes" &&
    make includes &&
    echo "make depend" &&
    make depend
    ;;
*)
    imake $args
    ;;
esac
