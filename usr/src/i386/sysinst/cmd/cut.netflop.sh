#!/usr/bin/ksh
#ident	"@(#)cut.netflop.sh	15.3	98/03/04"


# this script assumes that an entire copy of $ROOT/$MACH
# already exists as $ROOT/.$MACH
#

export MACH ROOT SPOOL WORK PROTO PATH

#  cut.netflop.sh:
#
#  Cut Network Installation Utilities floppies -
#        - Create staging directory ($PROTO/net.stage) structure
#        - Hand create/modify some network config files
#        - Build loadable modules for networking (tcp, spx)
#        - Build loadable modules for NICs
#        - Create zipped cpio archives for:
#        - netinstall scripts
#        - common networking code
#        - tcp-specific bits
#        - spx-specific bits
#        - each NIC driver & its attendant files
#        - config files from NICs pkg
#        - Determine which driver goes to which floppy
#        - Create driver to floppy map
#        - Cut the floppies
#

integer numflop=2  # Number of floppies it takes.... 


#####################################################################
#
#  Function: remove_tmp_work
#
#  Purpose: To remove any temporary files or directories
#
#####################################################################

remove_tmp_work ()
{

     rm -rf ${IITMP}
     rm -rf /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$
     rm -rf /tmp/track1.$$ /tmp/disk2.$$
     rm -rf /tmp/out1.$$ /tmp/out2.$$
     rm -rf /tmp/track1.$$ /tmp/disk2.$$
     rm -rf /tmp/netflop.label
}



#####################################################################
#
#  Function: cleanup
#
#  Purpose: To cleanup any files left behind when a failure occurs
#
#####################################################################
cleanup()
{
     echo "FAILURE has occured in $1"
     remove_tmp_work
     exit 1
}

#####################################################################
#
#  Function: cleanup_trap
#
#  Purpose: To cleanup any files left behind when a trap occurs
#
#####################################################################
cleanup_trap()
{
     echo "\nCleaning up temporary files...."
     remove_tmp_work
     exit 1
}



#####################################################################
#
#  Function: verify_env_set
#
#  Purpose: Verifies that the appropriate variables are set
#
#####################################################################
verify_env_set()
{
  echo "===> Verifing that enviornment variables are set ..."
#
#  Make sure tht appropriate environment variables are set
#
  [ -z "${PROTO}" ] && {
        echo "PROTO is not set."
        cleanup "verify_env_set"
  }

  [ -z "${MACH}" ] && {
        echo "MACH is not set."
        cleanup "verify_env_set"
  }

  [ -z "${ROOT}" ] && {
        echo "ROOT is not set."
        cleanup "verify_env_set"
  }

  [ -z "${SPOOL}" ] && {
        echo "SPOOL is not set."
        cleanup "verify_env_set"
  }

  [ -z "${WORK}" ] && {
        echo "WORK is not set."
        cleanup "verify_env_set"
  }

  [ -z "${PATH}" ] && {
        echo "PATH is not set."
        cleanup "verify_env_set"
  }

  return 0

}

#####################################################################
#
#  Function: set_env_variables
#
#  Purpose: Sets local environment variables
#
#####################################################################

set_env_variables()
{
  echo "===> Setting local environment variables..."
#
#  Variables for our convenience
#
  PATH=$PROTO/bin:$PROTO/cmd:$PATH export PATH

  NET_SCRIPTS="ii_do_netinst ii_hw_select ii_hw_config ii_net_config ii_spx_config"

  TCP_DRIVERS="arp icmp inet incf ip rip route sockmod tcp timod tirdwr udp"

  SPX_DRIVERS="uni ipx nspx ripx"

  NET_DRIVERS="net dlpi pf dlpibase net0"

  IITMP=/tmp/inetinst.$$                # Temporary directory per cut

  MINI=mini
  NETFLOP_ROOT=${PROTO}/net.stage       # Staging directory for netinstall only
  FLOPMAP=${NETFLOP_ROOT}/floppy_map    # Generated driver to floppy map
  rm -f ${FLOPMAP} >/dev/null 2>&1


  return 0

}


#####################################################################
#
#  Function: create_tmp_dirs
#
#  Purpose: Create temporary working directories
#
#####################################################################
create_tmp_dirs()
{
  echo "===> Creating temporary directories..."
  mkdir -p ${IITMP}/nics
  # also create an mdi directory for keeping track of added MDI drivers
  mkdir -p ${IITMP}/mdi
  return 0
}

#####################################################################
#
#  Function: verify_rel_fullname
#
#  Purpose: To verify that the environment variable REL_FULLNAME exists
#
#####################################################################
verify_rel_fullname()
{
  echo "===> Verifying that REL_FULLNAME is set..."
  . ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
  [ -z "${REL_FULLNAME}" ] && {
    echo "REL_FULLNAME not set.  Please set and reinvoke this command."
    return 1
  }
  return 0
}

#####################################################################
#
#  Function: set_medium
#
#  Purpose: To set the environment variable MEDIUM
#
#####################################################################
set_medium()
{
  # One can set "diskette1 or diskette2" as first argument
  [ "$1" = "diskette1" -o "$1" = "diskette2" ] && MEDIUM="$1"

  return 0
}


#####################################################################
#
#  Function: ask_drive
#
#  Purpose: To prompt a user what diskette drive they're going to use
#                   to cut this Network Install Floppy (lifted from cut.flop)
#
#####################################################################
ask_drive()
{
        #
        #  We need a high-capacity drive to fit the net install floppy
        #
  DONE=0
  while [ "${DONE}" = "0" ]
  do
    [ "$MEDIUM" = "" ] && {
      echo "Please enter diskette1 or diskette2 (default is diskette1): \c"
      read MEDIUM
      [ "$MEDIUM" = "" ] && MEDIUM="diskette1"
    }
    #FDRIVE=`devattr $MEDIUM fmtcmd|cut -f 3 -d " "|sed 's/t//'`
    FDRIVE=`devattr $MEDIUM fmtcmd|cut -f 3 -d " "`
    BLKS=`devattr $MEDIUM capacity`
    FDRVIE_TMP=`basename $FDRIVE`
    if [ "${BLKS}" -le "2800" ]
    then
      echo "You must choose a high-capacity drive.  The drive"
      echo "you selected will only hold ${BLKS} blocks."
    else
      DONE=1
    fi
  done

  BLKCYLS=`devattr $MEDIUM mkfscmd|cut -f 7 -d " "`
  echo "${FDRIVE_TMP}\t${BLKCYLS}\t${BLKS}" >&2
}


#####################################################################
#
#  Function: mod_reg_updt
#
#  Purpose:  Update mod_register file with each module's info
#
#####################################################################
mod_reg_updt()
{
  DRIVER=$1
  DIR=$2
  MDEV="${DIR}/../mdevice.d"
  MOD_OUT="${NETFLOP_ROOT}/etc/conf/mod_register"

  #
  #  Yank the major dev range out of the Master file
  #
  DDI_VERSION=`grep '^\$interface ddi' ${MDEV}/${DRIVER} | cut -d ' ' -f 3`
  MAJORRANGE=`tail -1 ${MDEV}/${DRIVER} | cut -f 6`

  if [ "${MAJORRANGE}" = "0" ]
  then
  #
  #  Streams modules (type 3) are not 'real' devices
  #
    echo "3:1:${DRIVER}:${DRIVER}" >> ${MOD_OUT}
  else
    if [ "$DDI_VERSION" = "8" ]
      then
        :
      else
        #
        #  Real devices are type 5
        #
        echo "${MAJORRANGE}" | sed "s/-/ /" > /tmp/rm.$$
        read START END < /tmp/rm.$$
        rm /tmp/rm.$$
        [ -z "${END}" ] && END=${START}

        # kludge workaround for netinstall of drivers with
        # ranges. (it's a long story...)
        [ "${END}" -ne "${START}" ] && END=${START}
        while [ ${START} -le ${END} ]
        do
          echo "5:1:${DRIVER}:${START}" >> ${MOD_OUT}
          START=`expr ${START} + 1`
        done
    fi
  fi
}

#####################################################################
#
#  Function: switch_driver
#
#  Purpose:
#     This function allows Networking drivers to be turned off and
#     on so that loadable modules may be built without collision.
#     Usage:
#       switch_driver driver_name turnonnic      or
#       switch_driver driver_name turnon         or
#       switch_driver driver_name turnoff
#      (turnonnic does some extra work to munge a NIC driver, like
#      making sure that it's at unused IO/RAM/ Addresses & IRQ,
#      so driver can be idbuilt)
#
#####################################################################
switch_driver()
{

  BEG_MAJOR=72
  MAX_MAJOR=79
  DRIVER=$1
  ACTION=$2
  SW_TMP="/tmp/${DRIVER}.sys.$$"
  MDEV_ORIG="${ROOT}/${MACH}/${MINI}/etc/conf/mdevice.d"
  SDEV_ORIG="${ROOT}/${MACH}/${MINI}/etc/conf/sdevice.d"
  PACK_ORIG="${ROOT}/${MACH}/${MINI}/etc/conf/pack.d"
  NODE_ORIG="${ROOT}/${MACH}/${MINI}/etc/conf/node.d"
  NODE="${ROOT}/.${MACH}/etc/conf/node.d"
  PACK="${ROOT}/.${MACH}/etc/conf/pack.d"
  SDEV="${ROOT}/.${MACH}/etc/conf/sdevice.d"
  MDEV="${ROOT}/.${MACH}/etc/conf/mdevice.d"
  MOD="${ROOT}/.${MACH}/etc/conf/mod.d"

  #
  #  The real sdevice file from the tree is in SDEV_ORIG, so no matter
  #  what we're doing here, we copy that into our working directory.
  #  (Same for the mdevice file!)
  #
  cp ${SDEV_ORIG}/${DRIVER} ${SDEV}/${DRIVER}
  if [ $? != 0 ]
  then
    cp ${SDEV_ORIG}/.${DRIVER} ${SDEV}/${DRIVER}
    if [ $? != 0 ]
    then
      echo "Could not find ${SDEV_ORIG}/${DRIVER}"
      return 1
    fi
  fi

  cp ${MDEV_ORIG}/${DRIVER} ${MDEV}/${DRIVER}
  if [ $? != 0 ]
  then
    cp ${MDEV_ORIG}/.${DRIVER} ${MDEV}/${DRIVER}
    if [ $? != 0 ]
    then
      echo "Could not find ${MDEV_ORIG}/${DRIVER}"
      return 1
    fi
  fi

  # - if we just copied the "net0" or "dlpi" Master file, mangle its
  # character major number.  we do this here to treat these
  # as a real driver but we want them in common.z not in net0.z and
  # dlpi.z -- possibly on a second disk.  They will still get idbuilt
  # into a DLM and entered into mod_register elsewhere
  [ "$DRIVER" = "net0" ] && {

  MAJOR_NUMBER=79

  export MAJOR_NUMBER

  awk 'BEGIN {
            maj="'$MAJOR_NUMBER'";
            OFS="\t";
          }
          NF==6 {
            $6 = maj;
            print $0
            next
          }
          { print $0 }' ${MDEV}/${DRIVER} > ${MDEV}/${DRIVER}+

          mv -f ${MDEV}/${DRIVER}+ ${MDEV}/${DRIVER}
        }


  #
  #  Make mods to the Master file - make sure that the Major
  #  numbers are between 72 and 79 as defined in the res_major
  #  file for lan.  Hard code for now (linh).
  #  adds support for a single major number and not range (MDI)
  #  since net0's master file only has one number it won't fall into
  #  the "if (end > max)" section and get lowered to 73(new MAX_MAJOR)
  #  ii_do_netinst depends on the major number for net0 always being 79.
  #  the moral of the story is that netX should never have a range in its
  #  major number column.
  #
  awk 'BEGIN       {
            condition="'$ACTION'";
            beg="'$BEG_MAJOR'";
            max="'$MAX_MAJOR'";
            OFS="\t";
          }
          NF==6 {
            if (condition == "turnonnic" )
            {
              n = split($6, x, "-");
              if (n == 1) {
                end = beg;
              } else {
                end = beg + (x[2] - x[1]);
                if (end > max) { end = max; }
              }
              $6 = beg "-" end;
              }
            print $0
            next
            }
            { print $0 }' \
            ${MDEV}/${DRIVER} > ${SW_TMP} && \
            mv ${SW_TMP} ${MDEV}/${DRIVER}
  #
  #  Make mods to the System file - turn on (Y) or off (N),
  #  if it's an NIC driver, also make sure that we give it
  #  nonconflicting values for RAM, IO, IRQ
  #
  awk 'BEGIN  {
            condition="'$ACTION'";
            OFS="\t";
          }
	    {
            if ($1 == "'$DRIVER'") {
            if (condition == "turnonnic" )
            {
              $2="Y";
              #
              #  If itype=0, then this card should not have
              #  IRQ/Addresses set, or else idbuild fails.
              #
              if ( $5 != "0" )
              {
                $6="9";
                $7="280";
                $8="29f";
                $9="D0000";
                $10="D1fff";
              }
              $11="-1";
              condition = "turnoff";
            }
            else
            if (condition == "turnon" )
              {
                $2="Y";
              }
            else
              {
                $2="N";
              }
            print $0
            next
          }
	  }
          { print $0 }' \
          ${SDEV}/${DRIVER} > ${SW_TMP} && \
          mv ${SW_TMP} ${SDEV}/${DRIVER}

#
#  We need to play a bit with the networking driver space.c here
#  so that odm can modify it successfully.  We redefine a static
#  char string to be a usable external.
#
  if [ "${ACTION}" = "turnonnic" ]
  then
    [ ! -d ${PACK}/${DRIVER} ] && {
      mkdir -p ${PACK}/${DRIVER} || {
       echo "Couldnt mkdir ${PACK}/${DRIVER}"
       return 1
      }
    }
    if [ ${PACK_ORIG}/${DRIVER}/Driver.o -nt \
      ${PACK}/${DRIVER}/Driver.o ]
    then
      cp ${PACK_ORIG}/${DRIVER}/Driver.o ${PACK}/${DRIVER}
    fi

# - don't try to copy the space.c file if it doesn't exist.
    [ -f ${PACK_ORIG}/${DRIVER}/space.c ] && {
      cp ${PACK_ORIG}/${DRIVER}/space.c ${PACK}/${DRIVER}/space.c
    }
    cp ${NODE_ORIG}/${DRIVER} ${NODE}/${DRIVER}
  fi

  return 0
}


#####################################################################
#
#  Function: create_nic_list
#
#  Purpose: To determine the list of NIC drivers we need, start off with a initial
#                   list of "popular" drivers and take the rest from the config
#                   directory of the NICS package. Go through the config files,
#                   creating a list of what files to put into cpio archives for each
#                   DRIVER_NAME, and a list of DRIVER_NAMEs.
#
#####################################################################
create_nic_list()
{

echo "===> Creating list of NICs..."

### - since we will be obsoleting ODI drivers soon get the list
### of ODI drivers solely from the config directory of the NICS package.
### this way when we retire the files later this script won't have to
### be changed.
###
### NOTE:  After commenting out the following list the following 3
### ODI drivers were not included on the netinstall floppies because
### they do not have bcfg files.  Conversely, these drivers could never
### be reached since there aren't any bcfg files for them!
### nics/NE1500T/etc/conf/mod.d/NE1500T  - ISA board
###  - Novell NE1500T Version 3.28 Revision 0 9/29/1994
### nics/NE3300/etc/conf/mod.d/NE3300    - EISA board with boardid "ETI1001"
###  - Microdyne NE3300 Ethernet v2.20 (940514) Revision 0 5/14/1994
### nics/SMCPWR/etc/conf/mod.d/SMCPWR    -
###  - Digital DC21X4 Ethernet EISA/PCI driver v1.02a V 1.2 Revision 0 2/5/1995
###     |PCI|0x10110002|SMC EtherPower (8432)|menus/nics/config/SMC_8432
###     |PCI|0x10110014|SMC EtherPower (8432e)|menus/nics/config/SMC_8432
###     |PCI|0x10110009|SMC EtherPower (9432)|menus/nics/config/SMC_9332
###     ^^^^^^^note that the Drvmap boardids point to the _DLPI_ smpw0 driver!
###    I think this was obsoleted since it was written by Digital and a
###    DLPI driver was available instead.
###
### The bottom line is that there are 3 drivers wasting space on the
### netinstall floppies (the drivers aren't in final product) that cannot
### be chosen since there's no bcfg file for it.  By not including them
### (by simply not using the list below) we free up exactly 27.5K on
### the floppy(ooooh aahhh).
###
> ${IITMP}/nics/nic_list
### cat > ${IITMP}/nics/nic_list << !!
### CPQNF3
### E100
### EMPCI
### EPRO
### ES3210
### HPFEODI
### IBM164
### INT32
### NE1000
### NE1500T
### NE2
### NE2000
### NE2100
### NE2_32
### NE3200
### NE3300
### NTR2000
### SMC8100
### SMC8232
### SMC8332
### SMC8K
### SMC9232
### SMC9K
### SMCPWR
### TCM503
### TCM523
### TCM59X
### TCM5X9
### TCTOKH
### nflxe
### pnt
### ee16
### el16
### nflxt
### !!


nl='
'
  OIFS=${IFS}
  IFS="${IFS}${nl}"

# for each (possibly compressed) bcfg file, source it in and see if we've
# already got it in nic_list.  Determine if driver has any EXTRA_FILES= and
# if so then add to extra files list.
#

# nathan added 9 may 97
# don't get bcfgs from packaging directory as they won't exist there
# any more.

  for NICS in ${ROOT}/${MACH}/etc/inst/locale/C/menus/nics/config/*
  do

        # as we obsolete ODI drivers this directory will eventually be empty
        # and the shell will not expand the astericks above but return them
        # as the single variable.  The test below will handle this by
        # ensuring that the file exists.
        [ ! -f $NICS ] && continue

        # nathan added 9 may 97
        # don't handle anything ending in .bcfg as we know these will
        # be MDI .bcfg files.  usr/src/i386at/cmd/niccfg/config convention
        # is that all ODI and DLPI bcfg files will not end in .bcfg
        [ -f `dirname $NICS`/`basename $NICS .bcfg`.bcfg ] && continue
        # end of nathan's mods on 9 may 97



        uncompress < $NICS > /tmp/$$.cfile 2>/dev/null || cp $NICS /tmp/$$.cfile

        unset DRIVER_NAME EXTRA_FILES
        . /tmp/$$.cfile

        grep "^${DRIVER_NAME}[ 	]*$" ${IITMP}/nics/nic_list >/dev/null
        [ $? != 0 ] && echo ${DRIVER_NAME} >> ${IITMP}/nics/nic_list

        for FILE in ${EXTRA_FILES}
        do
                echo $FILE | sed -e "s,^/,," >> ${IITMP}/nics/${DRIVER_NAME}.files
        done
  done
  rm -f /tmp/$$.cfile

  return 0

}


#####################################################################
#
#  Function: process_mdi_drivers
#
#  Purpose: Process the MDI driver bcfg files from WORK.
#                   "If mdi.mk did an IDINSTALL of this driver (i.e. it was packaged for
#                   distribution), then include it as a possible netinstall driver"
#                   so don't use  ${ROOT}/usr/src/${WORK}/uts/io/nd/mdi/*/*.cf/*.bcfg
#
#####################################################################
process_mdi_drivers()
{
  echo "===> Creating list of MDIs..."
  for NICS in ${ROOT}/${MACH}/etc/conf/bcfg.d/*/*.bcfg
  do

        unset DRIVER_NAME EXTRA_FILES
        . $NICS

        # added by hah by request of nathanp on 5/7/97
        # if driver does WAN we do not want driver on netinstall floppies
        echo "$TOPOLOGY" | egrep -qi "isdn|x25|serial" && continue


        grep "^${DRIVER_NAME}[ 	]*$" ${IITMP}/nics/nic_list >/dev/null
        [ $? != 0 ] && echo ${DRIVER_NAME} >> ${IITMP}/nics/nic_list

        for FILE in ${EXTRA_FILES}
        do
                echo $FILE | sed -e "s,^/,," >> ${IITMP}/nics/${DRIVER_NAME}.files
        done

        touch ${IITMP}/mdi/${DRIVER_NAME}   # remember this for later drv.z

  done

  IFS=${OIFS}

  for FILE in ${IITMP}/nics/*.files
  do
         sort -u -o ${FILE} ${FILE}
  done

  return 0
}

#####################################################################
#
#  Function: create_staging_dirs
#
#  Purpose: Create the staging directories and sub directories if needed
#
#####################################################################
create_staging_dirs ()
{

 echo "===> Creating  ${NETFLOP_ROOT} area and subdirectories..."
#
#  Make sure that our staging directory exists
#
  [ ! -d "${NETFLOP_ROOT}" ] && {
        mkdir -p ${NETFLOP_ROOT}
        RET=$?
        [ ${RET} != 0 ] && {
                echo "Couldn't create directory ${NETFLOP_ROOT}."
                return 1
        }
  }

#
#  Create the necessary directory structure under NETFLOP_ROOT
#  if it's not there already
#
  for DIR in etc/conf/mod.d etc/conf/bin etc/inst/scripts \
           usr/bin usr/lib usr/sbin bin nics sbin
  do
        [ ! -d "${NETFLOP_ROOT}/${DIR}" ] && {
                mkdir -p ${NETFLOP_ROOT}/${DIR}
                RET=$?
                [ ${RET} != 0 ] && {
                  echo "Couldn't create directory ${NETFLOP_ROOT}/${DIR}."
                  return 1
                }

        }
  done
  return 0
}

#####################################################################
#
#  Function: set_final_list_drivers
#
#  Purpose: To create the final lis of dirvers
#
#####################################################################
set_final_list_drivers()
{
  echo "===> Setting final list of drivers ..."
  > ${NETFLOP_ROOT}/etc/conf/mod_register

#
#  sdev_list is used by netinst script so that the DCU can be updated
#  with the selected networking hardware driver
#

#
# Set the final list of drivers
#
  NIC_DRIVERS=`cat ${IITMP}/nics/nic_list`

  > ${NETFLOP_ROOT}/etc/conf/sdev_list

  for MODULE in ${NIC_DRIVERS}
  do
        grep "^${MODULE}[ 	]" ${ROOT}/${MACH}/etc/conf/sdevice.d/${MODULE} \
                >> ${NETFLOP_ROOT}/etc/conf/sdev_list
  done
  return 0
}

#####################################################################
#
#  Function: populate_staging_area
#
#  Purpose: To copy some thins from the tree that we need to modify in this
#                   script
#
#####################################################################
populate_staging_area ()
{

  for CONFIG in services strcf
  do
        [ ! -f ${ROOT}/${MACH}/etc/inet/${CONFIG} ] && {
                echo "ERROR: ${ROOT}/${MACH}/etc/inet/${CONFIG} not found"
                return 1
        }
        cp ${ROOT}/${MACH}/etc/inet/${CONFIG} ${NETFLOP_ROOT}/etc/
  done

#
#  Write an identifiable string into an id file so that netinst
#  knows this is the right floppy!
#
  echo "${REL_FULLNAME} 1" > ${NETFLOP_ROOT}/id_1

#
#  Append "ii_boot" clause to strcf; it's the same as boot but does not
#  include a 'rawip' interface, which we don't need for Net Install.
#
  cat >> ${NETFLOP_ROOT}/etc/strcf << EONET
ii_boot {
        #
        # queue params
        #
        initqp /dev/ip muxrq 40960 64386 rq 8192 40960
        initqp /dev/tcp muxrq 8192 40960
        initqp /dev/udp hdrq 32768 64512
        #
        # transport
        #
        tp /dev/tcp
        tp /dev/udp
        tp /dev/icmp
}

EONET

  cat > ${NETFLOP_ROOT}/etc/netconfig << EONET
ticlts	   tpi_clts	  v	loopback	-	/dev/ticlts	/usr/lib/straddr.so
ticots	   tpi_cots	  v	loopback	-	/dev/ticots	/usr/lib/straddr.so
ticotsord  tpi_cots_ord	  v	loopback	-	/dev/ticotsord	/usr/lib/straddr.so
tcp	tpi_cots_ord	v	inet	tcp	/dev/tcp	/usr/lib/tcpip.so,/usr/lib/resolv.so
udp	tpi_clts  	v	inet	udp	/dev/udp	/usr/lib/tcpip.so,/usr/lib/resolv.so
icmp	tpi_raw  	-	inet	icmp	/dev/icmp	/usr/lib/tcpip.so,/usr/lib/resolv.so
rawip	tpi_raw  	-	inet	-	/dev/rawip	/usr/lib/tcpip.so,/usr/lib/resolv.so
ipx tpi_clts     v netware ipx /dev/ipx   /usr/lib/novell.so
spx tpi_cots_ord v netware spx /dev/nspx2 /usr/lib/novell.so
EONET

#
#  Assemble a cpio archive of message catalogs for SPX.  (The SPX commands
#  will not run if message catalogs are not in place)
#
  echo "===> Creating archive message catalogs for SPX"
  cd ${ROOT}/${MACH}
  sh ${PROTO}/desktop/buildscripts/cpioout > \
        ${NETFLOP_ROOT}/usr/lib/msgcat.cpio.z << EONET
usr/lib/locale/C/LC_MESSAGES/npsmsgs.cat
usr/lib/locale/C/LC_MESSAGES/npsmsgs.cat.m
usr/lib/locale/C/LC_MESSAGES/utilmsgs.cat
usr/lib/locale/C/LC_MESSAGES/utilmsgs.cat.m
usr/lib/locale/C/LC_MESSAGES/nwcmmsgs.cat
usr/lib/locale/C/LC_MESSAGES/nwcmmsgs.cat.m
EONET

 return 0
}

#####################################################################
#
#  Function: build_loadable_modules
#
#  Purpose: We need to build all of the loadable modules that go on the
#                   Network Installation Utilities floppy.  Here we construct one
#                   idbuild command line to build all of the drivers that do not
#                   driver hardware (anything that's not a NIC driver).
#                  This command line will contain a list of all drivers that are
#                  older than the Driver.o in the integration tree.
#
#####################################################################
build_loadable_modules ()
{

  echo "===> Building protocol loadable modules"
  COMMAND_LINE=""
  for DRIVER_NAME in ${TCP_DRIVERS} ${SPX_DRIVERS} ${NET_DRIVERS}
  do
        if [ ${ROOT}/${MACH}/${MINI}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o \
                -nt ${ROOT}/.${MACH}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o ]
        then
                cp ${ROOT}/${MACH}/${MINI}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o \
                   ${ROOT}/.${MACH}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o
        fi

        if [ -f ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ]
        then
                if [ ${ROOT}/.${MACH}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o \
                -nt ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ]
                then
                COMMAND_LINE="${COMMAND_LINE}-M ${DRIVER_NAME} "
                switch_driver ${DRIVER_NAME} turnon || return $?
                fi
        else
                COMMAND_LINE="${COMMAND_LINE}-M ${DRIVER_NAME} "
                switch_driver ${DRIVER_NAME} turnon || return $?
        fi
  done

#
#  Build the drivers (if there are any to build)
#
  if [ ! -z "${COMMAND_LINE}" ]
  then
        MACH=.${MACH} idbuild ${COMMAND_LINE}
  fi

#
#  Turn all of the drivers back off, and update mod_register so that
#  we can load the drivers on the machine doing the network install.
#
  for DRIVER_NAME in ${TCP_DRIVERS} ${SPX_DRIVERS}
  do
        mod_reg_updt ${DRIVER_NAME} ${ROOT}/.${MACH}/etc/conf/mod.d
        cp ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ${NETFLOP_ROOT}/etc/conf/mod.d
        switch_driver ${DRIVER_NAME} turnoff || return $?
  done

  for DRIVER_NAME in ${NET_DRIVERS}
  do
        cp ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ${NETFLOP_ROOT}/etc/conf/mod.d
        switch_driver ${DRIVER_NAME} turnoff || return $?
  done


#
#  For each of the Networking card drivers, we do the following:
#       turn it on (in the sdevice file)
#       idbuild -M it
#       update mod_register for it
#       turn it off (in the sdevice file)
#       compress it into the target directory
#
#  Just for a belt and suspenders, make sure all NIC drivers are turned off.
#
  echo "===> Turning off network card drivers"
  for DRIVER_NAME in ${NIC_DRIVERS}
  do
        switch_driver ${DRIVER_NAME} turnoff || return $?
  done

#
#  Now build the NIC drivers
#  The only drivers that will actually get built are those where the
#  driver is older than the Driver.o in the integration tree.
#
  echo "===> Building network card drivers"
  echo "===> Copying network drivers into staging area.."
  for DRIVER_NAME in ${NIC_DRIVERS}
  do
        echo "${DRIVER_NAME} \c"

        [ ! -d ${ROOT}/${MACH}/${MINI}/etc/conf/pack.d/${DRIVER_NAME} ] && {
                echo "WARNING: ${ROOT}/${MACH}/${MINI}/etc/conf/pack.d/${DRIVER_NAME} does not exist, continue with next driver ..."
                continue
        }

        switch_driver ${DRIVER_NAME} turnonnic || return $?

        if [ -f ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ]
        then
                if [ ${ROOT}/.${MACH}/etc/conf/pack.d/${DRIVER_NAME}/Driver.o \
                -nt ${ROOT}/.${MACH}/etc/conf/mod.d/${DRIVER_NAME} ]
                then
                MACH=.${MACH} idbuild -M ${DRIVER_NAME}
                fi
        else
                MACH=.${MACH} idbuild -M ${DRIVER_NAME}
        fi

        #
        #  Any "extra" files first
        #
        [ -f ${IITMP}/nics/${DRIVER_NAME}.files ] && {

                cd ${ROOT}/${MACH}
                cat ${IITMP}/nics/${DRIVER_NAME}.files | \
                cpio -pdum ${NETFLOP_ROOT} > /dev/null 2>&1

        }


        #
        #  Then the driver itself
        #
        cd ${ROOT}/.${MACH}

        echo "etc/conf/mod.d/${DRIVER_NAME}" | \
              cpio -pdum ${NETFLOP_ROOT} > /dev/null 2>&1

        switch_driver ${DRIVER_NAME} turnoff || return $?
  done
  echo
  return 0
}

#####################################################################
#
#  Function: remove_white_spaces
#
#  Purpose: Remove whitespaces from netinstall scripts and copy them to
#                   staging directory.
#
#####################################################################
remove_white_spaces  ()
{
  echo "===> Copying netinstall scripts into net.stage"
  . ${PROTO}/bin/rmwhite
  cd ${PROTO}/desktop/menus
  for i in $NET_SCRIPTS
  do
        rmwhite $i ${NETFLOP_ROOT}/etc/inst/scripts/$i
  done
  chmod 555 ${NETFLOP_ROOT}/etc/inst/scripts/*
  return 0
}


#####################################################################
#
#  Function: copy_relevant_files_into_netstage
#
#  Purpose: Get executables/libs config files out of $ROOT/$MACH and stage in
#                   net.stage area for cpio into compressed archives.
#
#####################################################################
copy_relevant_files_into_netstage ()
{
  echo "===> Copying relevant files into net.stage"


  echo "3:1:dlpi:dlpi
3:1:dlpibase:dlpibase
5:1:net0:79" >> ${NETFLOP_ROOT}/etc/conf/nics_register

cd ${ROOT}/${MACH}
cpio -pdu ${NETFLOP_ROOT} <<EOF
etc/conf/bin/idkname
etc/conf/bin/idmodreg
etc/netware/conf/nwnet.bin
usr/sbin/pkgcat
usr/sbin/mknod
usr/sbin/npsd
usr/sbin/nwcm
usr/sbin/nwdiscover
usr/sbin/bootp
usr/sbin/ifconfig
usr/sbin/ping
usr/sbin/route
usr/sbin/slink
usr/lib/libNwCal.so
usr/lib/libNwClnt.so
usr/lib/libNwLoc.so
usr/lib/libNwNcp.so
usr/lib/libcrypt.so
usr/lib/libiaf.so
usr/lib/libnsl.so
usr/lib/libnwnetval.so
usr/lib/libsocket.so.2
usr/lib/libresolv.so.2
usr/lib/libthread.so
usr/lib/novell.so
usr/lib/libnwutil.so
usr/lib/tcpip.so
sbin/resmgr
usr/bin/dd
usr/bin/sort
usr/bin/uname
usr/sbin/dlpid
usr/bin/ndstat
EOF
  cp ${PROTO}/cmd/sap_nearest ${NETFLOP_ROOT}/bin/
  return 0
}




#####################################################################
#
#  Function: build_new_lists
#
#  Purpose: The list of files going onto the floppy(s)
#
#####################################################################
build_new_lists()
{

  echo "===> Building file lists..."
  common_list="usr/bin/dd
usr/lib/libcrypt.so
usr/lib/libiaf.so
usr/lib/libnsl.so
usr/lib/libresolv.so.2
usr/lib/libsocket.so.2
usr/lib/libthread.so
usr/sbin/pkgcat
usr/sbin/mknod
etc/netconfig
etc/services
etc/conf/mod_register
etc/conf/nics_register
etc/conf/sdev_list
etc/conf/bin/idmodreg
etc/conf/mod.d/net
etc/conf/mod.d/sockmod
etc/conf/mod.d/timod
etc/conf/mod.d/tirdwr
etc/conf/mod.d/dlpi
etc/conf/mod.d/net0
etc/conf/mod.d/dlpibase
etc/conf/mod.d/pf
usr/bin/uname
usr/sbin/dlpid
usr/bin/ndstat"

  tcp_list="usr/lib/tcpip.so
usr/sbin/bootp
usr/sbin/ifconfig
usr/sbin/ping
usr/sbin/route
usr/sbin/slink
etc/strcf
etc/conf/mod.d/arp
etc/conf/mod.d/icmp
etc/conf/mod.d/inet
etc/conf/mod.d/incf
etc/conf/mod.d/ip
etc/conf/mod.d/rip
etc/conf/mod.d/route
etc/conf/mod.d/tcp
etc/conf/mod.d/udp"

  spx_list="bin/sap_nearest
usr/sbin/npsd
usr/sbin/nwcm
usr/sbin/nwdiscover
usr/lib/libNwCal.so
usr/lib/libNwClnt.so
usr/lib/libNwLoc.so
usr/lib/libNwNcp.so
usr/lib/libnwnetval.so
usr/lib/libnwutil.so
usr/lib/msgcat.cpio.z
usr/lib/novell.so
etc/conf/mod.d/ipx
etc/conf/mod.d/nspx
etc/conf/mod.d/ripx
etc/conf/mod.d/uni
etc/netware/conf/nwnet.bin"

  cet_list="etc/conf/mod.d/cet
etc/inst/nd/mdi/cet/unieth.bin
etc/inst/nd/mdi/cet/uniethf.bin
etc/inst/nd/mdi/cet/unitok.bin
etc/inst/nd/mdi/cet/unitokf.bin"

  cnet_list="etc/conf/mod.d/cnet"

  dcxe_list="etc/conf/mod.d/dcxe"
  dcxf_list="etc/conf/mod.d/dcxf"
  dfx_list="etc/conf/mod.d/dfx"
  dex_list="etc/conf/mod.d/dex"
  e3B_list="etc/conf/mod.d/e3B"
  e3C_list="etc/conf/mod.d/e3C"
  e3D_list="etc/conf/mod.d/e3D"
  e3E_list="etc/conf/mod.d/e3E"
  e3G_list="etc/conf/mod.d/e3G"
  e3H_list="etc/conf/mod.d/e3H"
  ee16_list="etc/conf/mod.d/ee16"
  eeE_list="etc/conf/mod.d/eeE"
  lanosm_list="etc/conf/mod.d/lanosm"
  nat_list="etc/conf/mod.d/nat"
  ne_list="etc/conf/mod.d/ne"
  pnt_list="etc/conf/mod.d/pnt"
  sme_list="etc/conf/mod.d/sme"
  smpw_list="etc/conf/mod.d/smpw"
  spwr_list="etc/conf/mod.d/spwr"
  stbg_list="etc/conf/mod.d/stbg"
  tok_list="etc/conf/mod.d/tok"
  trps_list="etc/conf/mod.d/trps"
  wdn_list="etc/conf/mod.d/wdn"
  wwdu_list="etc/conf/mod.d/wwdu"

  nics_list="cet cnet dcxe dcxf dex dfx e3B e3C e3E e3G e3H ee16 eeE lanosm nat ne pnt sme spwr stbg wdn wwdu" 

  nics_list2="e3D smpw tok trps "

  netflop_files[0]="${common_list}
${tcp_list}
${spx_list}"

  > ${FLOPMAP}

  for nic in $nics_list
  do
        eval 'for name in '\${${nic}_list}'
        do
                netflop_files[0]="${netflop_files[0]}
${name}"
        done'
        DDI_VERSION=`grep '^\$interface ddi' ${ROOT}/.${MACH}/etc/conf/mdevice.d/${nic} | cut -d ' ' -f 3`
        echo "1 ${nic} ${DDI_VERSION}" >> ${FLOPMAP}

  done

  netflop_files[1]=""
  for nic in $nics_list2
  do
        eval 'for name in '\${${nic}_list}'
        do
                netflop_files[1]="${netflop_files[1]}
${name}"
        done'
        DDI_VERSION=`grep '^\$interface ddi' ${ROOT}/.${MACH}/etc/conf/mdevice.d/${nic} | cut -d ' ' -f 3`
        echo "2 ${nic} ${DDI_VERSION}" >> ${FLOPMAP}
  done

}

#####################################################################
#
#  Function: cut_netflop
#
#  Purpose: Floppy two is a little unusual. The first track is a
#           small cpio archive, and the rest of the disk is a
#           weird format. It might be cpio. It is created by a cpio,
#           then bzip-ed, then wrt is used. Mayby it puts a cpio header
#           back on the archive? The first archive is a disk label,
#           the second is the files that didn't fit on boot1.
#           Anyway, the device /dev/dsk/f0 does not include the first
#           track. The device /dev/dsk/f0t does include the first
#           track. This function used to write to both. Now it
#           creates an image of the disk, and does one write. This
#           lets us cut floppies over the network.
#
#####################################################################
function cut_netflop
{

  echo "===> Cutting new floppy images..."

  integer count
  integer flopnum=$1
  integer trksize=${TRKSIZE:-36}

  set -- $(ls -l netflop.image)
  (( count = $5 / 512 ))
  . ${ROOT}/${MACH}/var/sadm/dist/rel_fullname
  echo "Creating netinstall floppy ${flopnum} image..."
#
# Create one track's worth of zeros
#
  rm -f /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$
  rm -f /tmp/track1.$$ /tmp/disk2.$$
  /usr/bin/dd if=/dev/zero of=/tmp/zeros.$$ bs=512 count=$trksize
#
# Create the cpio header
#
  rm -f /tmp/netflop.label
  if [ -f /tmp/netflop.label ]
  then
    echo "Error: cannot remove /tmp/netflop.label"
    rm -f /tmp/zeros.$$
    return 1
  fi
  echo  "${REL_FULLNAME} ${flopnum}" >/tmp/netflop.label
  if [ ${flopnum} -eq 1 ]
  then
    cp ${FLOPMAP} /tmp
    LIST="/tmp/netflop.label /tmp/floppy_map"
    for list in common tcp spx
    do
      rm -f /tmp/${list}.list
      eval 'for name in '\${${list}_list}'
        do
        echo ${name} >> /tmp/${list}.list
        done'
        LIST="$LIST /tmp/${list}.list"
    done
    for nic in $nics_list $nics_list2
    do
      rm -f /tmp/${nic}.list
      eval 'for name in '\${${nic}_list}'
          do
          echo ${name} >> /tmp/${nic}.list
          done'
          LIST="$LIST /tmp/${nic}.list"
    done
  else
    LIST="/tmp/netflop.label"
  fi

  ls ${LIST} | cpio -oLDVH crc -O /tmp/out1.$$ || retun $?


  $PROTO/bin/bzip -s32k /tmp/out1.$$ > /tmp/out2.$$
  $PROTO/bin/wrt -s /tmp/out2.$$ > /tmp/cpio.$$
  chmod uog+w /tmp/cpio.$$
  rm -f ${LIST} > /dev/null 2>&1
#
# Merge them
#
  set -- $(ls -l /tmp/cpio.$$)
  (( tracksize = $trksize * 512 ))
  [[ $5 -gt $tracksize ]] && {
    echo "first track for netfloppy image ${flopnum} too big"
    echo "$5 > $tracksize"
    rm -f /tmp/both.$$ /tmp/zeros.$$
    rm -f /tmp/out*.$$
    return 1
  }
  cat /tmp/cpio.$$ /tmp/zeros.$$ > /tmp/both.$$
#
# Extract one track's worth
#
  /usr/bin/dd if=/tmp/both.$$ of=/tmp/track1.$$ bs=512 count=$trksize || return $?
#
# Add on the rest of the disk
#
  cat /tmp/track1.$$ netflop.image > netflop.$$
  ls -l netflop.$$
  rm -f /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$ /tmp/track1.$$
  (( flopsize = $trksize * 80 * 512 ))
  set -- $(ls -l netflop.$$)
  [[ $5 -gt $flopsize ]] && {
    echo "netfloppy image ${flopnum} too big"
    echo "$5 > $flopsize"
    rm -f netflop.$$
    return 1
  }
  (( flopsize = $trksize * 80 ))
  /usr/bin/dd if=/dev/zero of=/tmp/zeros.$$ bs=512 count=$flopsize
  cat netflop.$$ /tmp/zeros.$$ > /tmp/both.$$
  /usr/bin/dd if=/tmp/both.$$ of=netinstall.image.${flopnum} bs=512 count=$flopsize || {
    rm -r /tmp/zeros.$$ /tmp/both.$$
    rm -r netflop.$$
    return $?
  }
#
# Dump to the disk
#
  print Writing to netinstall floppy ${flopnum}...
  print "Wrote $count blocks to netinstall floppy ${flopnum}."
  print "Done with netinstall floppy ${flopnum}.\007"
  rm -f /tmp/zeros.$$ /tmp/cpio.$$ /tmp/both.$$
  rm -f /tmp/track1.$$ /tmp/disk2.$$
  rm -f netflop.$$
}




#####################################################################
#
#  Function: create_new_images
#
#  Purpose: To cut the images to floppy
#
#####################################################################
create_new_images()
{
echo "===> Creating new floppy images..."
> ${NETFLOP_ROOT}/sumrs
idx=0
while [ $idx -lt ${#netflop_files[@]} ]
do
        echo "Creating netinstall floppy $(( idx + 1 )) archive..."
        typeset OIFS=$IFS
        IFS="
"
        cd ${NETFLOP_ROOT}
        ls ${netflop_files[idx]} | cpio -oLDV -H crc -O /tmp/out1.$$

        cp /tmp/out1.$$  ${NETFLOP_ROOT}/netinstall.cpio.image.$(( idx + 1 ))

        IFS=$OIFS
        (( idx += 1 ))
        cd ..
        print "Compressing image for netinstall floppy $idx."
        $PROTO/bin/bzip -s32k /tmp/out1.$$ > /tmp/out2.$$
        $PROTO/bin/wrt -s /tmp/out2.$$ > netflop.image
        cut_netflop $idx || return $?
        rm -f /tmp/out?.$$
        rm -f netflop.image
        sum -r netinstall.image.$idx >> ${NETFLOP_ROOT}/sumrs
        mv netinstall.image.$idx ${NETFLOP_ROOT} 
      
done


}

#####################################################################
#
#  Function: cut_floppies
#
#  Purpose: Cut the floppies
#
#####################################################################
cut_floppies ()
{

  echo ""
  ask_drive 2> $PROTO/stage/drive_info

  cd ${NETFLOP_ROOT}

  cnt=1
  while (( cnt <= numflop ))
  do
        echo "\nInsert Net Install Floppy #$cnt into $MEDIUM drive and press"
        echo "         <RETURN>       to write floppy,"
        echo "         F              to format and write floppy,"
        echo "         D              to write raw cpio format to device,"
        echo "         d              to change output device and write to it,"
        echo "         o              to change output device,"
        echo "         s              to skip,"
        echo "         q              to quit: \c"
        read a

    
        [ "$a" = "o" ] && {
                echo
                echo -n "Enter new device to write to :"
                read FDRIVE
	        continue
		
        }

        [ "$a" = "d" ] && {
                echo
                echo -n "Enter new device to write to :"
                read FDRIVE
        }

        [ "$a" = "D" ] && {
          /sbin/dd if=${NETFLOP_ROOT}/netinstall.cpio.image.$cnt of=${FDRIVE} bs=36b
          RET=$?
          if [ $RET != 0 ]
          then
                echo "ERROR: dd returns $RET, exit."
                return 1
          fi
                continue
        }


        [ "$a" = "s" ] && {
                let cnt+=1
                continue
        }

        [ "$a" = "q" ] && break

        [ \( ! -z "$a" -a "$a" != "F" \) ] && {
                echo "Invalid input, try again"
                continue
        }

        [ "$a" = "F" ] && {
                /usr/sbin/format ${FDRIVE} || return $?
        }

        /sbin/dd if=${NETFLOP_ROOT}/netinstall.image.$cnt of=${FDRIVE} bs=36b
        RET=$?
        if [ $RET != 0 ]
        then
                echo "ERROR: dd returns $RET, exit."
                return 1
        fi
        echo "Done with Network Install Diskette ${cnt}."

        let cnt+=1
  done

 return 0
}


# main()

echo "===> Creating Network Install Diskettes ..."


# Need to set up traps to cleanup if we del out....
trap 'cleanup_trap ' 1 2 3 15
#

set_env_variables                     || cleanup "set_env_variables"
verify_env_set                        || cleanup "set_env_variables"
create_tmp_dirs                       || cleanup "create_tmp"
verify_rel_fullname                   || cleanup "verify_rel_fullname"
set_medium $1                         || cleanup "set_medium"
create_nic_list                       || cleanup "create_nic_list"
process_mdi_drivers                   || cleanup "process_mdi_drivers"
create_staging_dirs                   || cleanup "create_staging_dirs"
set_final_list_drivers                || cleanup "set_final_list_drivers"
populate_staging_area                 || cleanup "populate_staging_area"
build_loadable_modules                || cleanup "build_loadable_moduels"
remove_white_spaces                   || cleanup "remove_white_spaces"
copy_relevant_files_into_netstage     || cleanup "copy_relevant_files_into_netstage"
build_new_lists                       || cleanup "build_new_lists"
create_new_images                     || cleanup "create_new_images"
cut_floppies                          || cleanup "cut_floppies"
remove_tmp_work                       || cleanup "remove_tmp_work"
exit 0


