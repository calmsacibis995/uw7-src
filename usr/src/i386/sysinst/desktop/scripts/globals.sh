#ident	"@(#)globals.sh	15.1"

#
# defines for PROTO GLOBALS 
# Here for all to see!!!
# It is imperative that this file is imbibed only ONCE from inst.
#

#ubiquitously used variables
typeset -x UW_PRODUCTS="UW22 UW52"
typeset -x ODM_PRODUCTS="ODM1"
typeset UnixWare_PRODUCT_ID=UW 
PROMPTNODE=on
PROMPTKEYB=on
#typeset -i MEMSIZE=$(memsize)
typeset -lx SH_DEBUG= # set to null to turn off
typeset -x MEDIUM_ERR_FLAG=/tmp/med_err_flag
typeset -x HDROOT=/mnt	#Hard disk root filesystem mount point
set -A IHVHBAS END END
set -A IHVHBAMODS ""
set -A IHVHBASTUBS
set -A IHVHBAMEDIA
export IHVHBAS IHVHBAMODS IHVHBASTUBS IHVHBAMEDIA
typeset INSTWID SETUPWID NOT_EMPTY_MSG
typeset DO_REBOOT
typeset DO_HBA
typeset FLPY=/flpy2
typeset UNIX_REL=/etc/.release_ver
typeset PKGINSTALL_TYPE=NEWINSTALL
typeset AUTOMERGE=NULL
typeset UPGRADE=NO
typeset RAMROOT=/tmp
typeset PLATFORM=none
export TERM
typeset RELATED_HELPS="genhelp kbhelp"
typeset -x UNIX_INSTALL=Y SILENT_INSTALL GENERIC_HEADER
typeset -x SEC_MEDIUM_TYPE	#Secondary medium device type
typeset -x SEC_MEDIUM		#Secondary medium device node
typeset -x PKGDEV			#Secondary medium device node (pkgadd-compatible)
typeset -x PKGDEV_MOUNTPT	#Secondary medium mount point
typeset -x BACK_END_MANUAL=false
typeset -x INST_COMPAQ_FS=false #true if install fs copy of compaq package
typeset -x INST_JALE_FS=false #true if install fs copy of jale package
typeset -x ODM_UP=false		#set to true if upgrade of 1.1 with VxVm
typeset -x LOADED_MODS=""
typeset -x NOT_LOADED_MODS=""
typeset -i NDISKS
typeset -i MIN_HARDDISK=80
typeset -i MIN_SECDISK=40
typeset -i ONEMEG=1048576

#flag for DynaText package installation...
typeset FULL=NO

#global window identifiers
integer FSFID CUR_FSFIDX FSFIDX
set -A FDFID
integer DCHKFID
integer NODEFID
integer SPXFID
integer HWFID
integer SERIAL_FID

#used in fs support
typeset -i FSTOT=0
typeset VTOC_SLICES="1 2 3 4 6 8 10 11 12 13 14 15 16 17"
typeset MEMFS_SLICES="13 16"
set -A SLCHOICES
set -A SLTYPE 
set -A SLSIZE 
set -A SLDISK 
set -A SLDISK2 
set -A SLFS 
set -A SLNAME 
set -A SLINODES 
set -A SLBLKSIZE
set -A _SLTYPE 
set -A _SLSIZE 
set -A _SLDISK 
set -A _SLDISK2 
set -A _SLFS 
set -A _SLNAME 
set -A _SLINODES 
set -A _SLBLKSIZE
typeset -i MEMFSTOT=0
typeset WHICHDISK
typeset ATTRSLC
typeset CHOOSING_FSTYPE
typeset FSTOT_DISPLAY
typeset HD0OPT HD1OPT
typeset INSTDISKS
typeset -i ALTS1
set -A SLCHOICES

#used in fd support
typeset -i LEN_CHANGED
set -A CHAR_DISK_NODES 
set -A BLOCK_DISK_NODES
set -A DISK_DEVS 
set -A DISK_SIZE 
set -A DISKCHK
set -A RESETGEOM
set -A DISK_NUMCYL 
set -A DISK_CYLSIZE
set -A DISKCYLS
set -A PARTS_OPTS
set -A UNIX_PARTITION NONE NONE
set -A UNIX_PARTITION_SIZE 0 0
set -A FDchoices -- "$UNIX" "$DOS" "$other" "$unused"
set -A FDchoicesP1 -- "$UNIX" "$PRE5DOS" "$DOS" "$other" "$unused" "$SYSCONFIG"
DISK0_WAS_INSANE=NO
DISK1_WAS_INSANE=NO

#used in floppy2 support
typeset -i HBA_MEM_LIMIT=4*ONEMEG

typeset -x DCUDIR=${DCUDIR:-${ROOT}/etc/dcu.d}

typeset -x SerialNumber SerialID ActKey PatchSerial=AbCdptf

typeset -x SID_MODE=false

