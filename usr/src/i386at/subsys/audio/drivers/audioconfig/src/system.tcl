#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# system.tcl
#------------------------------------------------------------------------------
# @(#)system.tcl	7.3	12/17/97	12:16:00
#------------------------------------------------------------------------------
# System interface, binding to OSA
#------------------------------------------------------------------------------
# Revision History:
# 1997-Sep-16, stevegi, same as below for: RemoveSoundcard, ModifySoundcard
# 1997-Sep-03, stevegi, added oss-specific opts to attrvals in AddSoundcard
# 1997-Aug-29, stevegi, various mods to work with PnP and new OSS driver
# 1997-Jan-24, shawnm, add GetAudinfo
# 1997-Jan-23, shawnm, add AuthQuery
# 1997-Jan-02, shawnm, change primary/secondary io to audio/midi/synth io
# 1996-Dec-06, shawnm, created
#==============================================================================


# FullInstance returns the OSA instance expanded to include the host

proc FullInstance {host instance} {
    return [list [list [list systemId $host] $instance]]
}


# AuthQuery returns whether the user is authorized to use the audio config
# manager on the host being managed

proc AuthQuery {} {
	global appvals

	set authorized 0
	set fullinstance [FullInstance $appvals(managedhost) NULL]

	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance query_auth {}]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	} else {
		set authorized [BmipResponseActionInfo bmipResponse]
	}
	return $authorized
}


# GetInstalledCardList returns all of the config data for all currently
# installed soundcards derived from the audioconfig.cfg file

proc GetInstalledCardList {} {
	global appvals

	set cardlist {}
	set fullinstance [FullInstance $appvals(managedhost) NULL]
	set attrlist [list \
			unit \
			manufacturer \
			model \
			audio_io \
			midi_io \
			synth_io \
			primary_irq \
			secondary_irq \
			primary_dma \
			secondary_dma \
			enabled_drivers \
			]

	set objcall [list ObjectGet \
		-scope 1 [list sco audioconfig] $fullinstance $attrlist]

	set bmipResponse [SaMakeObjectCall $objcall]
	set firstBmip [lindex $bmipResponse 0]
	set errorStack [BmipResponseErrorStack firstBmip]
	if {![lempty $errorStack]} {
		ErrorThrow errorStack
	} else {
		foreach item $bmipResponse {
			set attrval [BmipResponseAttrValList item]
			lappend cardlist $attrval
		}
	}

	return $cardlist
}


# GetNewCardList returns a list of any new PnP cards as {$manufacturer $model}

proc GetNewCardList {} {
	global appvals

	set newcardlist {}
	set fullinstance [FullInstance $appvals(managedhost) NULL]

	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance list_new_cards {}]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorThrow errorStack
	} else {
		set newcardlist [BmipResponseActionInfo bmipResponse]
	}
	return $newcardlist
}


# GetManufacturerList returns a list of all the supported manufacturers
# derived from the audinfo directories

proc GetManufacturerList {} {
	global appvals

	set manufacturerlist {}
	set fullinstance [FullInstance $appvals(managedhost) NULL]

	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance list_manufacturers {}]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	} else {
		set manufacturerlist [BmipResponseActionInfo bmipResponse]
	}
	return $manufacturerlist
}


# GetModelList returns a list of all the supported models for the given
# manufacturer derived from the audinfo directories

proc GetModelList {manufacturer} {
	global appvals

	set modellist {}
	set fullinstance [FullInstance $appvals(managedhost) NULL]

	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance list_models $manufacturer]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	} else {
		set modellist [BmipResponseActionInfo bmipResponse]
	}
	return $modellist
}


# Note that LoadAudinfo does not return the audinfo file.
# It stores the audinfo file values in the global keyed list
# audinfo(manufacturer/model) and returns whether it was successfully loaded

proc LoadAudinfo {manufacturer model} {
	global appvals
	global audinfo

	set mf [translit { } {} $manufacturer]
	set mo [translit { } {} $model]

	if {![catch "keylget audinfo($mf/$mo) loaded" loaded]} {
		if {$loaded} {return 1}
	}
	
	set fullinstance [FullInstance $appvals(managedhost) NULL]

	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance \
		get_audinfo [list $manufacturer $model]]

	#cmdtrace on [open /tmp/audinfo  w]
	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]

	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set loaded 0
		keylset audinfo($mf/$mo) loaded 0
		set errorStack [BmipResponseErrorStack bmipResponse]
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorThrow errorStack
	} else {
		set loaded 1
		set audinfo($mf/$mo) [BmipResponseActionInfo bmipResponse]
		keylset audinfo($mf/$mo) loaded 1
	}
	return $loaded
}


# GetAudinfo returns the value of attr in the audinfo file manufacturer/model

proc GetAudinfo {manufacturer model attr} {
	global audinfo

	set mf [translit { } {} $manufacturer]
	set mo [translit { } {} $model]

	LoadAudinfo $manufacturer $model
	if {[catch "keylget audinfo($mf/$mo) $attr" val]} {
		set errorstack {}
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorPush errorstack 1 \
			SCO_AUDIOCONFIG_ERR_CANT_GET_AUDIO_IO_OPTIONS $mf/$mo
	}

	return $val
}


# TestAudioStart initiates the digital audio test on the host being managed

proc TestAudioStart {} {
	global appvals

	set fullinstance [FullInstance $appvals(managedhost) NULL]

	# Note this should pass a real unit value to support multiple cards
	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance test_audio_start 0]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorThrow errorStack
	}
	return
}


# TestAudioStop terminates the digital audio test on the host being managed

proc TestAudioStop {} {
	global appvals

	set fullinstance [FullInstance $appvals(managedhost) NULL]

	# Note this should pass a real unit value to support multiple cards
	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance test_audio_stop 0]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	}
	return
}


proc AddSoundcard {unit manufacturer model aio mio sio pirq sirq pdma sdma drivers} {
	global appvals
	global audinfo

	set mf [translit { } {} $manufacturer]
	set mo [translit { } {} $model]

	set attrvals [list \
		[list unit $unit] \
		[list manufacturer $manufacturer] \
		[list model $model] \
		[list audio_io $aio] \
		[list midi_io $mio] \
		[list synth_io $sio] \
		[list primary_irq $pirq] \
		[list secondary_irq $sirq] \
		[list primary_dma $pdma] \
		[list secondary_dma $sdma] \
		[list enabled_drivers $drivers] \
		[list ossid [keylget audinfo($mf/$mo) ossid] ] \
		[list ossdevs [keylget audinfo($mf/$mo) ossdevs] ] \
		[list bustype [keylget audinfo($mf/$mo) bus] ] \
		]

	set fullinstance [FullInstance $appvals(managedhost) $unit]

	set objcall [list ObjectCreate \
		{sco audiocard} $fullinstance $attrvals]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorThrow errorStack
	}

	# Force to add "(oss)" to IO/DMA/IRQ entries on next loading
	# echo [keylget audinfo($mf/$mo)]
	keylset audinfo($mf/$mo) loaded 0
}


proc ModifySoundcard {unit manufacturer model aio mio sio pirq sirq pdma sdma drivers} {
	global appvals
	global audinfo

	#cmdtrace on [open /tmp/modify w]
	set mf [translit { } {} $manufacturer]
	set mo [translit { } {} $model]

	set attrvals [list \
		[list unit $unit] \
		[list manufacturer $manufacturer] \
		[list model $model] \
		[list audio_io $aio] \
		[list midi_io $mio] \
		[list synth_io $sio] \
		[list primary_irq $pirq] \
		[list secondary_irq $sirq] \
		[list primary_dma $pdma] \
		[list secondary_dma $sdma] \
		[list enabled_drivers $drivers] \
		[list ossid [keylget audinfo($mf/$mo) ossid] ] \
		[list ossdevs [keylget audinfo($mf/$mo) ossdevs] ] \
		[list bustype [keylget audinfo($mf/$mo) bus] ] \
		]

	set fullinstance [FullInstance $appvals(managedhost) $unit]

	set objcall [list ObjectReplace \
		{sco audiocard} $fullinstance $attrvals]
	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		# DEBUG INFO *** GC ***
		#echo [list $errorStack 0]
		ErrorThrow errorStack
	}
	# Force to add "(oss)" to IO/DMA/IRQ entries on next loading
	keylset audinfo($mf/$mo) loaded 0
}


proc RemoveSoundcard {unit manufacturer model} {
	global appvals
	global audinfo

	# Make sure we have the audinfo data.
	# (Since we didn't go through an examine dialog,
	# the data may never actually have been loaded!)
	LoadAudinfo $manufacturer $model

	set mf [translit { } {} $manufacturer]
	set mo [translit { } {} $model]

	set attrvals [list \
		[list unit $unit] \
		[list manufacturer $manufacturer] \
		[list model $model] \
		[list bustype [keylget audinfo($mf/$mo) bus] ] \
		]

	set fullinstance [FullInstance $appvals(managedhost) $unit]

	set objcall [list ObjectDelete \
		{sco audiocard} $fullinstance $attrvals]

	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	}
	# Force to add "(oss)" to IO/DMA/IRQ entries on next loading
	keylset audinfo($mf/$mo) loaded 0
}

proc CheckRes {res} {
	global appvals

	set fullinstance [FullInstance $appvals(managedhost) NULL]
	set objcall [list ObjectAction \
		{sco audioconfig} $fullinstance \
		check_res $res]
	#cmdtrace on [open /tmp/check w]
	set bmipResponse [lindex [SaMakeObjectCall $objcall] 0]
	if {[BmipResponseErrorIsPresent bmipResponse]} {
		set errorStack [BmipResponseErrorStack bmipResponse]
		ErrorThrow errorStack
	} else {
		set list [BmipResponseActionInfo bmipResponse]
	}
	return $list
}	
