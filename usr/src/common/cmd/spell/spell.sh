#!/sbin/sh
#ident	"@(#)spell:spell.sh	1.11.2.5"
#	copyright	"%c%"


#	spell program
# B_SPELL flags, D_SPELL dictionary, F_SPELL input files, H_SPELL history, S_SPELL stop, V_SPELL data for -v
# L_SPELL sed script, I_SPELL -i option to deroff

# Check for MAC, setting H_SPELL accordingly

if [ ! -w "/var/adm/spellhist" ]
then
  H_SPELL=/dev/null
else
	H_SPELL=${H_SPELL-/var/adm/spellhist}
fi

EXIT_CODE=0
OPTS_DONE=false
V_SPELL=/dev/null
F_SPELL=
B_SPELL=
L_SPELL="sed -e \"/^[.'].*[.'][ 	]*nx[ 	]*\/usr\/lib/d\" -e \"/^[.'].*[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*so[ 	]*\/usr\/lib/d\" -e \"/^[.'][ 	]*nx[ 	]*\/usr\/lib/d\" "
USAGE="usage:\n\tspell [-v] [-b] [-x] [-l] [-i] [+local_file] [ files ]\n"
trap "rm -f /tmp/spell.$$; exit \$EXIT_CODE" 0 1 2 13 15
for A in $*
do
	if $OPTS_DONE
	then
		F_SPELL="$F_SPELL $A"
	else
	case $A in
	-v)	if /bin/pdp11
		then	pfmt -l UX:spell -s error \
			     -g "uxspell:1" "-v option not supported on pdp11\n"
			EXIT_CODE=1
			EXIT_SPELL=exit
		else	B_SPELL="$B_SPELL -v"
			V_SPELL=/tmp/spell.$$
		fi ;;
	-a)	: ;;
	-b) 	D_SPELL=${D_SPELL-/usr/share/lib/spell/hlistb}
		B_SPELL="$B_SPELL -b" ;;
	-x)	B_SPELL="$B_SPELL -x" ;;
	-l)	L_SPELL="cat" ;;
	+*)	if [ "$FIRSTPLUS" = "+" ]
		then	pfmt -l UX:spell -s warn -g "uxspell:2" \
			    "multiple + options in spell, all but the last are ignored\n"
			EXIT_CODE=2;
		fi;
		FIRSTPLUS="$FIRSTPLUS"+
		if  LOCAL=`expr $A : '+\(.*\)' 2>/dev/null`;
		then if test ! -r $LOCAL;
		     then pfmt -l UX:spell -s error \
			       -g "uxspell:3" "spell cannot read %s\n" $LOCAL
			  EXIT_CODE=1
			  EXIT_SPELL=exit;
		     fi
		else
		     pfmt -l UX:spell -s error \
			-g "uxspell:4" "spell cannot identify local spell file\n"
		     EXIT_CODE=1
		     EXIT_SPELL=exit;
		fi ;;
	-i)	I_SPELL="-i" ;;
	--)	OPTS_DONE=true ;;
	-*)	pfmt -l UX:spell -s error -g "uxspell:5" "Invalid option: %s\n" $A
		pfmt -l UX:spell -s action -g "uxspell:6" "$USAGE"
		EXIT_CODE=1
		exit ;;
	*)	F_SPELL="$F_SPELL $A"
		OPTS_DONE=true ;;
	esac
	fi
done
${EXIT_SPELL-:}
( MSGOFF=ls export MSGOFF
 cat -- $F_SPELL | eval "$L_SPELL" |\
 deroff -w $I_SPELL |\
 sort -u +0 |\
 /usr/lib/spell/spellprog ${S_SPELL-/usr/share/lib/spell/hstop} 1 |\
 /usr/lib/spell/spellprog ${D_SPELL-/usr/share/lib/spell/hlista} $V_SPELL $B_SPELL |\
 comm -23 - ${LOCAL-/dev/null} |\
 tee -a $H_SPELL )
( who am i >>$H_SPELL ) 2>/dev/null
case $V_SPELL in
/dev/null)	exit
esac
sed '/^\./d' $V_SPELL | sort -u +1f +0
