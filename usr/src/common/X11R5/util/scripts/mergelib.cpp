XCOMM!/bin/sh
XCOMM
XCOMM $XConsortium: mergelib.cpp,v 1.3 91/08/22 11:08:08 rws Exp $
XCOMM 
XCOMM Copyright 1989 Massachusetts Institute of Technology
XCOMM 
XCOMM 
XCOMM 
XCOMM Author:  Jim Fulton, MIT X Consortium
XCOMM 
XCOMM mergelib - merge one library into another; this is commonly used by X
XCOMM     to add the extension library into the base Xlib.
XCOMM

usage="usage:  $0  to-library from-library [object-filename-prefix]"
objprefix=_

case $# in
    2) ;;
    3) objprefix=$3 ;;
    *) echo "$usage" 1>&2; exit 1 ;;
esac

tolib=$1
fromlib=$2

if [ ! -f $fromlib ]; then
    echo "$0:  no such from-library $fromlib" 1>&2
    exit 1
fi

if [ ! -f $tolib ]; then
    echo "$0:  no such to-library $tolib" 1>&2
    exit 1
fi


XCOMM
XCOMM Create a temp directory, and figure out how to reference the 
XCOMM object files from it (i.e. relative vs. absolute path names).
XCOMM

tmpdir=tmp.$$
origdir=..

mkdir $tmpdir

if [ ! -d $tmpdir ]; then
    echo "$0:  unable to create temporary directory $tmpdir" 1>&2
    exit 1
fi

case "$fromlib" in
    /?*) upfrom= ;;
    *)  upfrom=../ ;;
esac

case "$tolib" in
    /?*) upto= ;;
    *)  upto=../ ;;
esac


XCOMM
XCOMM In the temp directory, extract all of the object files and prefix
XCOMM them with some symbol to avoid name clashes with the base library.
XCOMM
cd $tmpdir
ar x ${upfrom}$fromlib
for i in *.o; do
    mv $i ${objprefix}$i
done


XCOMM
XCOMM Merge in the object modules, ranlib (if appropriate) and cleanup
XCOMM
ARCMD ${upto}$tolib *.o
RANLIB ${upto}$tolib
cd $origdir
rm -rf $tmpdir



