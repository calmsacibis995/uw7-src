#ident	"@(#)dcu:locale/C/config.sh	1.10"

# For use in 'boards' script
max BD_WIDTH[1] 1
echo BD_TITLE[1]="\"\""
max BD_WIDTH[2] boardname 20
center -C boardname BD_WIDTH[2]
echo BD_TITLE[2]="\"$boardname\""
max BD_WIDTH[3] boardirq 3
echo BD_TITLE[3]="\"$boardirq\""
max BD_WIDTH[4] boardios 7
echo BD_TITLE[4]="\"$boardios\""
max BD_WIDTH[5] boardioe 5
echo BD_TITLE[5]="\"$boardioe\""
max BD_WIDTH[6] boardmems 8
echo BD_TITLE[6]="\"$boardmems\""
max BD_WIDTH[7] boardmeme 8
echo BD_TITLE[7]="\"$boardmeme\""
max BD_WIDTH[8] boarddma 2
echo BD_TITLE[8]="\"$boarddma\""
echo BD_TITLE[9]="\"\""

max XBD_WIDTH[1] boardname 16
center -C boardname BD_WIDTH[1]
echo XBD_TITLE[1]="\"$boardname\""
max XBD_WIDTH[2] boardid 8
center -C boardid XBD_WIDTH[1]
echo XBD_TITLE[2]="\"$boardid\""

# For use in boardxpnd
OIFS="$IFS"
IFS="$nl"
set -A a -- ${BDXNDTEXT}
max -s BDxpndCols ${a[@]}
let i=$(echo "$BDXNDTEXT" | wc -l)
echo BDxpndLines=$i
IFS="$OIFS"
unset OIFS

# For use in 'category' script
max CATMAINWIDTH CATMAIN_TITLE network_interface_cards host_bus_adapters communications_cards video_cards sound_boards miscellaneous All_Software_Device_Drivers Return_to_Main_menu

# For use in 'dcu -C' scripts
OIFS="$IFS"
IFS="$nl"
set -A a -- ${CnflWait}
max -s CnflWaitCols ${a[@]}
let i=$(echo "$CnflWait" | wc -l)
echo CnflWaitLines=$i
IFS="$OIFS"
unset OIFS

# For use in 'dcumain' scripts
OIFS="$IFS"
IFS="$nl"
set -A a -- ${CnfgFloppy2}
max -s CnfgFloppy2Cols ${a[@]}
let i=$(echo "$CnfgFloppy2" | wc -l)
echo CnfgFloppy2Lines=$i
IFS="$OIFS"
unset OIFS

OIFS="$IFS"
IFS="$nl"
set -A a -- ${CnfgWait}
max -s CnfgWaitCols ${a[@]}
let i=$(echo "$CnfgWait" | wc -l)
echo CnfgWaitLines=$i
IFS="$OIFS"
unset OIFS

OIFS="$IFS"
IFS="$nl"
set -A a -- ${HdcWait}
max -s HdcWaitCols ${a[@]}
let i=$(echo "$HdcWait" | wc -l)
echo HdcWaitLines=$i
IFS="$OIFS"
unset OIFS

# For use in 'dcumain' script
max DCUMAINWIDTH BOARD DRIVER CANCEL SAVE RESTART CNFG_HBA LOAD_HBA

OIFS="$IFS"
IFS="$nl"
set -A a -- ${DCUFloppy2}
max -s DCUFloppy2Cols ${a[@]}
let i=$(echo "$DCUFloppy2" | wc -l)
echo DCUFloppy2Lines=$i
IFS="$OIFS"

