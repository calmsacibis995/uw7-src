#!/usr/bin/wksh -openlook

#ident	"@(#)wksh:xlibexamples/graph.sh	1.1.1.1"

GRAPH_TEXTHEIGHT=13
GRAPH_TEXTWIDTH=8
GRAPH_FONT="8x13bold"

DrawBarChart() { # $1=parent $2=nbars $3-$3+n=colors rest are [label data ...]
	typeset w=$1 C L DATA
	integer width height scale nbars=$2
	integer i c d l gs x y zero_y
	integer totbars maxdata mindata margin=$((GRAPH_TEXTWIDTH*5))
	integer spacing=10 gspacing=24
	integer shadow=$((spacing*2/3))

	if [ "$w" = POSTSCRIPT ]
	then width=$((8.5*72))
	     height=$((11*72))
	else gv $1 width:width height:height
	     XDraw -clearall $w
	fi
	shift 2

	# Get the bar colors

	i=0
	while (( i < nbars ))
	do
		C[i]="$1"
		i=i+1
		shift
	done

	# The rest of the args are of the form: label data

	i=0
	while (( $# > 0 ))
	do
		L[i]="$1"
		DATA[i]="$2"

		shift 2
		i=i+1
	done

	# run through the data, find the maximum

	maxdata=-1
	mindata=0
	for i in "${DATA[@]}"
	do
		if (( i > maxdata ))
		then maxdata=i
		fi
		if (( i < mindata ))
		then mindata=i
		fi
	done
	totbars=${#DATA[@]}
	scale='((height - 2*margin - shadow)*100)/(maxdata-mindata)'
	zero_y='height - margin + mindata*scale/100'
	barwidth='((width - 2*margin)/totbars) - spacing - gspacing/nbars'

	if (( barwidth < 8 ))
	then
		barwidth=8
		shadow=3
		width='(barwidth + spacing + gspacing/3)*totbars + 2*margin'
		if [ "$w" != POSTSCRIPT ]
		then sv $w width:$width; return
		fi
	fi

	# Draw the background

	XDraw -fillrect -fg ${GRAPHBACKGROUND:-grey} $w \
		$((margin)) \
		$((margin)) \
		$((width-2*margin+shadow)) \
		$((height-2*margin+shadow))

	# Draw the bars

	i=0
	gs=margin
	while (( i < ${#DATA[@]} ))
	do
		c='i%nbars'
		if (( c == 0 ))
		then gs=gs+gspacing
		fi

		d='DATA[i]*scale/100'
		if (( d < 0 ))
		then let d=0-d
		fi

		x='i * (barwidth+spacing) + gs'

		if ((DATA[i] >= 0))
		then
			y='zero_y - d'
		else
			y='zero_y'
		fi

		# Draw the shadow

		if (( DATA[i] > 0 ))
		then
			XDraw -fillrect -fg black "$w" \
				$(( x + shadow )) \
				$(( y + shadow )) \
				$((barwidth)) \
				$((d-shadow))
		else
			XDraw -fillrect -fg black "$w" \
				$(( x + shadow )) \
				$(( y )) \
				$((barwidth)) \
				$((d+shadow))
		fi

		# Draw the actual data

		XDraw -fillrect -fg "${C[c]}" "$w" \
			$((x)) \
			$((y)) \
			$((barwidth)) \
			$((d))

		# Draw outline around data

		XDraw -rect "$w" \
			$((x)) \
			$((y)) \
			$((barwidth)) \
			$((d))

		# Draw the label, centered in the middle of the bar

		XDraw -string -fn $GRAPH_FONT "$w" \
			$((x+(barwidth-${#L[$i]}*GRAPH_TEXTWIDTH)/2)) \
			$((height-margin+shadow+GRAPH_TEXTHEIGHT+6)) \
			"${L[i]}"

		i=i+1
	done

	# Draw the border

	XDraw -rect $w \
		$((margin)) \
		$((margin)) \
		$((width-2*margin+shadow)) \
		$((height-2*margin+shadow))

	# Draw the title

	XDraw -string -fn $GRAPH_FONT $w $((margin)) 10 "$GRAPHTITLE"

	#
	# Draw the X Axis Labels
	#
	
	# figure the max number of lines displayable

	integer maxlines='(height-2*margin-shadow)/(GRAPH_TEXTHEIGHT+2)'

	if (( maxlines < 2 ))
	then return
	fi

	# figure the X unit corresponding to this maximum
	integer xdelta='(GRAPH_TEXTHEIGHT*100)/scale'

	# round this delta to a nice value by doubling and substituting
	# zeros at the end

	xdelta=xdelta+xdelta

	i=0
	while (( xdelta > 9 ))
	do
		i=i+1
		xdelta=xdelta/10
	done

	# Make top digit of xdelta a "nice" number: 1, 2, 5, or 10

	case "$xdelta" in
	0|1|2)		xdelta=1 ;;
	3|4|5)		xdelta=5 ;;
	6|7|8|9)	xdelta=10 ;;
	esac

	while (( i ))
	do
		xdelta=xdelta*10
		i=i-1
	done

	if (( xdelta <= 0 ))
	then xdelta=1
	fi

	integer xscale='xdelta*scale/100' bottom

	i=0
	y=zero_y
	while (( y > margin ))
	do
		XDraw -string -fn $GRAPH_FONT "$w" 2 $((y)) "$((i))"
		y=y-xscale
		i=i+xdelta
	done

	i=0
	y=zero_y
	while (( y < height - margin ))
	do
		XDraw -string -fn $GRAPH_FONT "$w" 2 $((y)) "$((i))"
		y=y+xscale
		i=i-xdelta
	done

	XDraw -line "$w" $margin $zero_y $((width-margin+shadow)) $zero_y
}

#
# Here are fake trig functions which do approximations using precalculated
# arrays.  Array element n is 100 times the SIN (COS) of 10*n degrees.
#
# Of course, when KSH-92 comes along with floating point and math(3)
# functions, these will become unnecessary, we can do it for real then.
#

set -A GRAPH_SINE 0 17 34 49 64 76 86 93 98 100 98 93 86 76 64 50 34 17 0 -17 -34 -49 -64 -76 -86 -93 -98 -100 -98 -93 -86 -76 -64 -50 -34 -17

set -A GRAPH_COSINE 100 98 93 86 76 64 50 34 17 0 -17 -34 -49 -64 -76 -86 -93 -98 -100 -98 -93 -86 -76 -64 -50 -34 -17 0 17 34 49 64 76 86 93 98

Cosine() {	# $1=6400 times degree of angle
	echo "${GRAPH_COSINE[$(($1/64000))]}"
}

Sine() {
	echo "${GRAPH_SINE[$(($1/64000))]}"
}

DrawPieChart() { # $1=parent [color label data ...]
	typeset w=$1 C L DATA
	integer width height scale
	integer i a size radius
	integer totslice numdata margin=$((2*GRAPH_TEXTWIDTH))
	integer shadow=8
	integer total totangles
	integer label_x label_y center_x center_y
	integer tradius xoffset yoffset

	gv $1 width:width height:height

	if (( width > height ))
	then 
		size=height-4*margin
		y=2*margin
		x="2*margin+(width-height)/2"
	else 
		size=width-4*margin
		x=2*margin
		y="2*margin+(height-width)/2"
	fi
	center_x=width/2
	center_y=height/2

	radius=size/2
	shift

	# The rest of the args are of the form: color label data

	i=0
	total=0
	while (( $# > 0 ))
	do
		if (( $3 >= 0 ))
		then
			C[i]="$1"
			L[i]="$2"
			DATA[i]="$3"
			total=total+$3
		fi
		shift 3
		i=i+1
	done

	totslices=${#DATA[@]}
	scale="$(( (36000*64)/total ))"

	# Draw the background

	XDraw -fillrect -fg ${GRAPH_BACKGROUND:-grey} $w \
		$((margin)) \
		$((margin)) \
		$((width-2*margin+shadow)) \
		$((height-2*margin+shadow))

	# Draw the border

	XDraw -rect $w \
		$((margin)) \
		$((margin)) \
		$((width-2*margin+shadow)) \
		$((height-2*margin+shadow))

	# Draw the shadow

	XDraw -fillarc -fg black "$w" \
		$((x+shadow)) \
		$((y+shadow)) \
		$((size)) \
		$((size)) \
		0 $((360*64))

	# Draw the slices

	i=0
	totangles=0
	while (( i < ${#DATA[@]} ))
	do
		d='DATA[i]*scale'

		# Draw the slice

		XDraw -fillarc -fg "${C[i]}" "$w" \
			$((x)) \
			$((y)) \
			$((size)) \
			$((size)) \
			$((totangles/100)) \
			$((d/100))

		# Draw the outline of the slice

		XDraw -arc "$w" \
			$((x)) \
			$((y)) \
			$((size)) \
			$((size)) \
			$((totangles/100)) \
			$((d/100))

		# Draw the label, this is the hard part

		# Right justify text if in quadrants II or III,
		# left justify if in quadrants I or IV.
		#
		# Center if near one of the poles.
		#
		# Adjust a bit for the shadow if in quadrants I or IV.
		#
		# Also, if in quadrants III or IV you have to move
		# the text down by $GRAPH_TEXTHEIGHT.
		#
		# Aren't you glad you paid attention in trig
		# class?

		a="(totangles+d/2)/6400"	# convert to degrees

		if (( a > 80 && a < 100 ))
		then quadrant=northpole
		elif (( a > 260 && a < 280 ))
		then quadrant=southpole
		elif (( a >= 0 && a <= 90 )) 
		then quadrant=1
		elif (( a > 90 && a <= 180 ))
		then quadrant=2
		elif (( a > 180 && a <= 270 ))
		then quadrant=3
		else quadrant=4
		fi

		# set defaults
		tradius=radius+3
		xoffset=0
		yoffset=0

		case "$quadrant" in
		northpole)	# center
			xoffset="0-(${#L[$i]}*GRAPH_TEXTWIDTH)/2"
			;;
		southpole)	# center, adjust for height and some shadow
			tradius=tradius+shadow
			xoffset="0-(${#L[$i]}*GRAPH_TEXTWIDTH)/2"
			yoffset=GRAPH_TEXTHEIGHT
			;;
		1)		# adjust for a bit of shadow
			tradius=tradius+shadow
			;;
		2)		# right justify
			xoffset="0-(${#L[i]}*GRAPH_TEXTWIDTH)"
			;;
		3)		# right justify, adjust for text height
			xoffset="0-(${#L[i]}*GRAPH_TEXTWIDTH)"
			yoffset=GRAPH_TEXTHEIGHT
			;;
		4)		# adjust for shadow, text height
			tradius=tradius+shadow
			yoffset=GRAPH_TEXTHEIGHT
			;;
		esac

		label_x="center_x+xoffset+`Cosine $((totangles+d/2))`*tradius/100"
		label_y="center_y+yoffset-`Sine $((totangles+d/2))`*tradius/100"
		XDraw -string -fn $GRAPH_FONT $w $label_x $label_y "${L[i]}"

		totangles='totangles+d'
		i=i+1
	done
}

DrawLegend() {	# widget width background [color text ...]
	integer offset=10 shadow=6

	typeset w=$1
	integer legend_width=$2
	typeset bg=$3
	integer y=$((GRAPH_TEXTHEIGHT))
	integer orig_y=y
	integer x
	integer width height

	shift 3

	integer numpoints=$(( $#/2 ))

	gv $w width:width height:height

	x=width-legend_width-offset

	XDraw -fillrect -fg $bg $w \
		$((x-2)) \
		$((orig_y-offset)) \
		$((width-x-2)) \
		$(( numpoints * (GRAPH_TEXTHEIGHT+offset) + offset * 2 ))

	while (( $# > 1 ))
	do
		XDraw -fillrect -fg black "$w" \
			$((x+shadow)) \
			$((y+shadow)) \
			$((GRAPH_TEXTHEIGHT)) \
			$((GRAPH_TEXTHEIGHT))
		XDraw -fillrect -fg "$1" "$w" \
			$((x)) \
			$((y)) \
			$((GRAPH_TEXTHEIGHT)) \
			$((GRAPH_TEXTHEIGHT))
		XDraw -string -fn $GRAPH_FONT $w $((x+GRAPH_TEXTHEIGHT+offset)) $((y+GRAPH_TEXTHEIGHT)) "$2"
		y=y+GRAPH_TEXTHEIGHT+offset
		shift 2
	done

	XDraw -rect $w \
		$((x-2)) \
		$((orig_y-offset)) \
		$((width-x-2)) \
		$(( numpoints * (GRAPH_TEXTHEIGHT+offset) + offset * 2 ))
}
