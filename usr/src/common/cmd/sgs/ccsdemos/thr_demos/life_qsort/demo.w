#ident	"@(#)ccsdemos:thr_demos/life_qsort/demo.w	1.1"
#!/usr/bin/wksh -motif

# temporary next line due to test environment. remove line in final version
WKSH_API=MOOLIT

#
# This script will kick off process and threaded benchmarks,
# receive their measurements via a FIFO, and update gauges
# Note that the benchmarks (plife, life) and background process (pipslp)
# need to be in your search path for execution.
#

#
# FIFO for IPC: timings from benchmark progs to script to drive gauges
#
PFILE="/tmp/poutA$$"
#
# lock file for pipslp background daemon to hold open FIFO
#
SLOCK="/tmp/slock$$"

#
# set text input field behavior for LWP, THR, ITER values
#
TFOPTS="-f yellow -n lightblue -r -w -b -v [0-9] -c"

#
# change these values for s:scaling factor for results; g:gridsize of
# matrix for game of life; p:problem size for the quicksort program;
# f name of FIFO for IPC
#
INOPTS="-f $PFILE"

#
#initial values for number of LWPs, threads, and matrix iterations
#
INIT_SET="LWP=4
THR=4
ITER=10
SIZE=960
SCALE=20"

SINIT_SET="LWP=4
THR=4
ITER=10
SIZE=50000
SCALE=4"

#
FLAG=0

#
#default: run life
#
WHICH_DEMO=0

#
# run every second to get input from benchmarks from FIFO.
# if positive value, from process prog; if negative, from threaded
# prog. Make it positive. set gauges and text associated with them
# printout text as running average but move gauge with given iteration
#
updateCB() {
		RESULT="$2"
		if [ "$RESULT" -gt 0 ]
		then
			if [ "$RESULT" -gt 100 ]
			then
				RESULT=100
			fi
			let PCOUNT="$PCOUNT + 1"
			let PTOTAL="$PTOTAL + $RESULT"
			let PAVG="$PTOTAL / $PCOUNT"
			if [ "$TAVG" -gt 0 ]
			then
				RATIO=`awk "END { printf(\"%1.2f\", $PAVG / $TAVG) } " < /dev/null `
			fi
			sv $GAUGE1 sliderValue:"$RESULT"
			sv $TEXT string:"$PAVG"
			sv $CTEXT string:"$RATIO"
		fi
		if [ "$RESULT" -lt 0 ]
		then
			let RESULT="0 - $RESULT"
			if [ "$RESULT" -gt 100 ]
			then
				RESULT=100
			fi
			let TCOUNT="$TCOUNT + 1"
			let TTOTAL="$TTOTAL + $RESULT"
			let TAVG="$TTOTAL / $TCOUNT"
			if [ "$TAVG" -gt 0 ]
			then
				RATIO=`awk "
				END {
					printf (\"%1.2f\", $PAVG / $TAVG)
				}
				" </dev/null `
			fi
			sv $GAUGE2 sliderValue:"$RESULT"
			sv $TEXT2 string:"$TAVG"
			sv $CTEXT string:"$RATIO"
		fi
}


#
# stop background program holding open FIFO and remove
# lock file and FIFO. Stop any benchmarks currently running.
#
quit_demo() {
	stop_proc
	stop_thr
	kill "$SLP_PIDS"  2>/dev/null
	STOPPED=true;
	rm -f "$PFILE" "$SLOCK"
	exit 0
}

#
# stop process benchmark
#
stop_proc() {

	if [ "$PROC_PID" ]
	then
		kill "$PROC_PID" 2>/dev/null
	fi
	sv $GAUGE1 sliderValue:"0"
	sv $TEXT string:"0"
	sv $CTEXT string:"0"
}

#
# stop threaded benchmark
#
stop_thr() {

	if [ "$THR_PID" ]
	then
		kill "$THR_PID" 2>/dev/null
	fi
	sv $GAUGE2 sliderValue:"0"
	sv $TEXT2 string:"0"
	sv $CTEXT string:"0"
}

#
# start process benchmark
#
do_proc() {

	PAVG=0
	PCOUNT=0
	PTOTAL=0
	LOPTIONS="$INOPTS $NOPTIONS"
	SOPTIONS="$INOPTS $SNOPTIONS -l 1 -t 1"
	if [ "$WHICH_DEMO" -eq 0 ]
	then
		plife $LOPTIONS &
	fi
	if [ "$WHICH_DEMO" -gt 0 ]
	then
		qsort $SOPTIONS &
	fi
	PROC_PID=$!
}

#
# start threaded benchmark
#
do_th() {

	TAVG=0
	TCOUNT=0
	TTOTAL=0
	LOPTIONS="$INOPTS $NOPTIONS $POPTIONS"
	SOPTIONS="$INOPTS $SNOPTIONS $SPOPTIONS"
	if [ "$WHICH_DEMO" -eq 0 ]
	then
		life $LOPTIONS &
	fi
	if [ "$WHICH_DEMO" -gt 0 ]
	then
		quicksort $SOPTIONS &
	fi
	THR_PID=$!
}

#
# get properties
#
prop_apply() {

	SAVEPROP="`dataprint $PROP_UCA`"
	eval "$SAVEPROP"
	POPTIONS=""
	SPOPTIONS=""
	if [ $bind = set ]
	then POPTIONS="$POPTIONS -b"
	     SPOPTIONS="$SPOPTIONS -b"
	fi
	if [ $bindp = set ]
	then POPTIONS="$POPTIONS -p"
	fi
	cbclear $PROP_UCA
}

cd_apply() {
	CD_SAVEPROP="`dataprint $CD_UCA`"
	eval "$CD_SAVEPROP"
	if [ $life = set ]
	then
		WHICH_DEMO=0
		if [ "$FLAG" -eq 0 ]
		then
			datareset $ROW "$INIT_SET"
			adj_num
		fi
		if [ "$FLAG" -gt 0 ]
		then
   		 	prop_reset_factory
		fi
	fi
	if [ $sort = set ]
	then
		WHICH_DEMO=1
		if [ "$FLAG" -eq 0 ]
		then
			datareset $ROW "$SINIT_SET"
			adj_num
		fi
		if [ "$FLAG" -gt 0 ]
		then
   		 	prop_reset_factory
		fi
	fi
}
#
# get new values of LWP, threads, and/or iteration count
#
adj_num() {

	SAVENUM="`dataprint $ROW`"
	eval "$SAVENUM"
	NOPTIONS=""
	SNOPTIONS=""
	if [ $LWP != 0 ]
	then
		NOPTIONS="$NOPTIONS -l $LWP"
		SNOPTIONS="$SNOPTIONS -l $LWP"
	fi
	if [ $THR != 0 ]
	then
		NOPTIONS="$NOPTIONS -t $THR"
		SNOPTIONS="$SNOPTIONS -t $THR"
	fi
	if [ $ITER != 0  ]
	then
		NOPTIONS="$NOPTIONS -i $ITER"
		SNOPTIONS="$SNOPTIONS -i $ITER"
	fi
	if [ $SIZE != 0 ]
	then
		NOPTIONS="$NOPTIONS -g $SIZE"
		SNOPTIONS="$SNOPTIONS -p $SIZE"
	fi
	if [ $SCALE != 0 ]
	then
		NOPTIONS="$NOPTIONS -s $SCALE"
		SNOPTIONS="$SNOPTIONS -s $SCALE"
	fi
}

prop_reset() {
	datareset $PROP_UCA "$SAVEPROP"
	datareset $ROW "$SAVENUM"
	SAVEPROP="`dataprint $PROP_UCA`"
	SAVENUM="`dataprint $ROW`"
	cbclear $PROP_UCA
}

cd_reset() {
	datareset $CD_UCA "$CD_SAVEPROP"
	datareset $ROW "$SAVENUM"
	CD_SAVEPROP="`dataprint $CD_UCA`"
	SAVENUM="`dataprint $ROW`"
}
	

FACTORY_SETTINGS="bind=unset
bindp=unset"

CD_FACTORY_SETTINGS="life=set
sort=unset"

NCD_FACTORY_SETTINGS="life=unset
sort=set"

prop_reset_factory() {
	datareset $PROP_UCA "$FACTORY_SETTINGS"
	if [ "$WHICH_DEMO" -eq 0 ]
	then
		datareset $ROW "$INIT_SET"
	fi
	if [ "$WHICH_DEMO" -gt 0 ]
	then
		datareset $ROW "$SINIT_SET"
	fi
	prop_apply $1
	adj_num
}
cd_reset_factory() {
	datareset $CD_UCA "$CD_FACTORY_SETTINGS"
	datareset $ROW "$INIT_SET"
	cd_apply $1
	adj_num
}

do_properties() {
	if [ ! "$PROP" ]
	then
		cps PROP prop popupWindowShell $TOPLEVEL \
			apply:'prop_apply'	\
			reset:'prop_reset'	\
			resetFactory:'prop_reset_factory'
		sv $PROP_UCA allowChangeBars:true
		cmw BIND bind checkBox $PROP_UCA \
			label:"Bind Threads: "
		cmw BINDP bindp checkBox $PROP_UCA   \
			label:"Bind to Processors:  "
		mnsetup $PROP		# set up mnemonics
		cbsetup $PROP_UCA	# set up change bars
		datareset $PROP_UCA "$FACTORY_SETTINGS"
	fi
	SAVEPROP="`dataprint $PROP_UCA`"
	pu $PROP GrabNone
	FLAG=1
}

selectLIFE() {
	CD_SPROP="`dataprint $CD_UCA`"
	eval "$CD_SPROP"
	if [ $sort = set ]
	then
		datareset $CD_UCA "$CD_FACTORY_SETTINGS"
	fi
}

selectSORT() {
	CD_SPROP="`dataprint $CD_UCA`"
	eval "$CD_SPROP"
	if [ $life = set ]
	then
		datareset $CD_UCA "$NCD_FACTORY_SETTINGS"
	fi
}
	

	
choose_demo() {
	if [ ! "$CD" ]
	then 
		cps CD cd popupWindowShell $TOPLEVEL \
			apply:'cd_apply'	\
			reset:'cd_reset'	\
			resetFactory:'cd_reset_factory'
		sv $CD_UCA allowChangeBars:true
		cmw LIFE life checkBox $CD_UCA	\
			label:"Life "
		acb $LIFE select "selectLIFE $LIFE"
		cmw SORT sort checkBox $CD_UCA 	\
			label:"Quicksort "
		acb $SORT select "selectSORT $SORT"
		mnsetup $CD
		datareset $CD_UCA "$CD_FACTORY_SETTINGS"
	fi
	CD_SAVEPROP="`dataprint $CD_UCA`"
	pu $CD GrabNone
}
STOPPED=false
PAVG=0
TAVG=0
TTOTAL=0
PTOTAL=0
TCOUNT=0
PCOUNT=0

rm -f "$PFILE"  "$SLOCK"

#
# make FIFO
#
/etc/mknod "$PFILE" p

#
# start background process to hold FIFO open
#
pipslp "$PFILE" "$SLOCK" &
SLP_PIDS=$!

ai TOPLEVEL tdemo THREADS_DEMO "$@" -motif

cmw FORM form form $TOPLEVEL

cmw CA ca controlArea $FORM measure:3 

addbuttons $CA \
	"Choose Demo"   choose_demo \
	"Run Thread"	do_th \
	"Run   Proc"	do_proc \
	"Quit  Demo"	quit_demo \
	"Stop   Thr"	stop_thr \
	"Stop  Proc"	stop_proc \
	"Apply Vals"	adj_num  \
	"Properties"	do_properties 

cmw CA1 ca1 controlArea $FORM `rightof $CA 50`
addrows $CA1 COMP
cmw HDR1 hdr1 staticText $COMP \
	string:"thr speed factor:"
cmw CTEXT ctext textField $COMP string:0 maximumSize:4
XtSetSensitive $CTEXT false

cmw CA2 ca2 controlArea $FORM `under $CA 10` 
addrows $CA2 ROW

addfields $ROW \
	LWP "LWPS:" : 3 \
	THR "THR:" : 3 \
	ITER "ITER:" : 3 \
	SIZE "SIZE:" : 6 \
	SCALE "SCALE:" : 3

datareset $ROW "$INIT_SET"
adj_num

cmw CA3 ca3 controlArea $FORM `rightof $CA2 80`
addrows $CA3 ROW2
cmw GAUGE1 g1 gauge $ROW2 orientation:vertical height:200 x:5 y:5
cmw HDR hdr staticText $ROW2 \
	string:"p_avg"
cmw TEXT text textField $ROW2 string:0 maximumSize:4
XtSetSensitive $TEXT false

cmw CA4 ca4 controlArea $FORM `rightof $CA3 20`
addrows $CA4 ROW3
cmw GAUGE2 g2 gauge $ROW3 orientation:vertical height:200 x:5 y:5
cmw HDR hdr staticText $ROW3 \
	string:"t_avg"
cmw TEXT2 text2 textField $ROW3 string:0 maximumSize:4
XtSetSensitive $TEXT2 false

XtAddInput $PFILE "updateCB $TOPLEVEL"

rw $TOPLEVEL
ml
