#==============================================================================
# Audio Configuration Manager
#------------------------------------------------------------------------------
# uiexamine.tcl
#------------------------------------------------------------------------------
# @(#)uiexamine.tcl	5.2	97/07/14
#------------------------------------------------------------------------------
# User interface, Examine Dialog
#------------------------------------------------------------------------------
# Revision History:
# 1997-Oct-15, georgec, add modname to settings, check if settings are changed
#                       before Modify
# 1996-Dec-06, shawnm, created
#==============================================================================

#====================================================================== XXX ===
# UiExamineCB
#   
#------------------------------------------------------------------------------

load /usr/lib/libtclrm.so.1
source /usr/lib/PnP.tlib

proc UiExamineCB {cbs} {
	global appvals

	SaStatusBarSet [VxGetVar $appvals(vtMain) statusBar] [IntlMsg BUILDING_EXAMINE_DIALOG]

	set appvals(examineInProgress) 1

	set parent $appvals(vtMain) 

	# if not adding a new card then get the current values out of the list
	if {!$appvals(addInProgress)} {
		set selected [UiMainListGetSelected]
		set appvals(pendingUnit) \
			[MainListEntryGetUnit $selected]
		set appvals(pendingManufacturer) \
			[MainListEntryGetManufacturer $selected]
		set appvals(pendingModel) \
			[MainListEntryGetModel $selected]
		set appvals(pendingAudioIO) \
			[addModName [MainListEntryGetAudioIO $selected]]
		#echo "notAddAIO" $appvals(pendingAudioIO)
		set appvals(pendingMidiIO) \
			[addModName [MainListEntryGetMidiIO $selected]]
		#echo "notAddMIO" $appvals(pendingMidiIO)
		set appvals(pendingSynthIO) \
			[addModName [MainListEntryGetSynthIO $selected]]
		set appvals(pendingPrimaryIRQ) \
			[addModName [MainListEntryGetPrimaryIRQ $selected]]
		set appvals(pendingSecondaryIRQ) \
			[addModName [MainListEntryGetSecondaryIRQ $selected]]
		set appvals(pendingPrimaryDMA) \
			[addModName [MainListEntryGetPrimaryDMA $selected]]
		set appvals(pendingSecondaryDMA) \
			[addModName [MainListEntryGetSecondaryDMA $selected]]
		set appvals(pendingDrivers) \
			[MainListEntryGetEnabledDrivers $selected]
	}

	# temp variables to reduce array references and typing
	set mf $appvals(pendingManufacturer)
	set mo $appvals(pendingModel)

	# if adding a new card use default config values
	# manufacturer and model were set in add dialog
	if {$appvals(addInProgress)} {
		set appvals(pendingAudioIO) \
			[GetAudinfo $mf $mo audio_io_default]
		set appvals(pendingMidiIO) \
			[GetAudinfo $mf $mo midi_io_default]
		set appvals(pendingSynthIO) \
			[GetAudinfo $mf $mo synth_io_default]
		set appvals(pendingPrimaryIRQ) \
			[GetAudinfo $mf $mo primary_irq_default]
		set appvals(pendingSecondaryIRQ) \
			[GetAudinfo $mf $mo secondary_irq_default]
		set appvals(pendingPrimaryDMA) \
			[GetAudinfo $mf $mo primary_dma_default]
		set appvals(pendingSecondaryDMA) \
			[GetAudinfo $mf $mo secondary_dma_default]
		set appvals(pendingDrivers) \
			[GetAudinfo $mf $mo drivers]
	}

	# build the examine dialog
	set examine [VtFormDialog $parent.examineDialog \
		-ok -okCallback UiExamineOkCB \
		-cancel -cancelCallback UiExamineCancelCB \
		-cancelButton CANCEL \
		-title [IntlMsg EXAMINE_DIALOG_TITLE] \
		]
	set appvals(examineDialog) $examine
	VxSetVar $appvals(examineDialog) examineDialog $examine

	set label [VtLabel $examine.label \
		-label [IntlMsg EXAMINE_DIALOG_LABEL \
		[list "$mf $mo"]]]

	set sep [VtSeparator $examine.sep -horizontal -rightSide FORM]

	set optionLabels {}

	if {[GetAudinfo $mf $mo has_audio_io]} {
		set audioIOOptionLabel [VtLabel $examine.audioIOLabel \
			-topSide $sep \
			-MOTIF_width 150 \
			-CHARM_width 30 \
			-leftSide FORM \
			-labelRight  \
			-label [IntlMsg EXAMINE_DIALOG_AUDIO_IO]]

		#set takenIO [CheckRes {IOADDR}]
		#for {set index 0} {$index < [llength $optionList]} {incr index} {
			#set op [lindex $optionList $index]
			#foreach taken $takenIO {
				#if {$op==[lindex [lindex $taken 0] 0]} {
					#set newop [format "%s (%s)" $op [lindex $taken 1]]
					#set optionList [lreplace $optionList $index $index $newop]
					#if {$appvals(pendingAudioIO)==$op} {
						#set appvals(pendingAudioIO) $newop
					#}
					#break
				#}
			#}
		#}
		set audioIOOptionMenu [VxOptionMenu $examine.audioIO \
			{} \
			[GetAudinfo $mf $mo audio_io_options] \
			AudioIOCB \
			$appvals(pendingAudioIO)]
		VtSetValues $audioIOOptionMenu \
			-topSide $sep \
			-leftSide $audioIOOptionLabel \
			-rightSide FORM
#		VtSetValues $audioIOOptionLabel -alignBottom $audioIOOptionMenu
		set appvals(pendingAudioIO) \
			[VxOptionMenuGetSelected $audioIOOptionMenu]
		set previousLabel $audioIOOptionLabel
		lappend optionLabels $previousLabel
		set previousMenu $audioIOOptionMenu
	} else {
		set appvals(pendingAudioIO) {}
	}

	if {[GetAudinfo $mf $mo has_midi_io]} {
		set midiIOOptionLabel [VtLabel $examine.midiIOLabel \
			-topSide $previousMenu \
			-leftSide FORM \
			-alignRight $previousLabel \
			-labelRight  \
			-label [IntlMsg EXAMINE_DIALOG_MIDI_IO]]
		#echo [GetAudinfo $mf $mo midi_io_options]
		set midiIOOptionMenu [VxOptionMenu $examine.midiIO \
			{} \
			[GetAudinfo $mf $mo midi_io_options] \
			MidiIOCB \
			$appvals(pendingMidiIO)]
		VtSetValues $midiIOOptionMenu \
			-topSide $previousMenu \
			-leftSide $midiIOOptionLabel \
			-rightSide FORM
		#VtSetValues $midiIOOptionLabel \
			#-alignBottom $midiIOOptionMenu
		set appvals(pendingMidiIO) \
			[VxOptionMenuGetSelected $midiIOOptionMenu]
		set previousLabel $midiIOOptionLabel
		lappend optionLabels $previousLabel
		set previousMenu $midiIOOptionMenu
	} else {
		set appvals(pendingMidiIO) {}
	}

	if {[GetAudinfo $mf $mo has_synth_io]} {
		set synthIOOptionLabel [VtLabel $examine.synthIOLabel \
			-topSide $previousMenu \
			-leftSide FORM \
			-alignRight $previousLabel \
			-labelRight  \
			-label [IntlMsg EXAMINE_DIALOG_SYNTH_IO]]
		set synthIOOptionMenu [VxOptionMenu $examine.synthIO \
			{} \
			[GetAudinfo $mf $mo synth_io_options] \
			SynthIOCB \
			$appvals(pendingSynthIO)]
		VtSetValues $synthIOOptionMenu \
			-topSide $previousMenu \
			-leftSide $synthIOOptionLabel \
			-rightSide FORM
		#VtSetValues $synthIOOptionLabel \
			#-alignBottom $synthIOOptionMenu
		set appvals(pendingSynthIO) \
			[VxOptionMenuGetSelected $synthIOOptionMenu]
		set previousLabel $synthIOOptionLabel
		lappend optionLabels $previousLabel
		set previousMenu $synthIOOptionMenu
	} else {
		set appvals(pendingSynthIO) {}
	}
	if {$appvals(pendingPrimaryIRQ)>0} {	# has_irq
		set primaryIRQOptionLabel [VtLabel $examine.primaryIRQLabel \
			-topSide $previousMenu \
			-leftSide FORM \
			-alignRight $previousLabel \
			-labelRight  \
			-label [IntlMsg EXAMINE_DIALOG_PRIMARY_IRQ]]
		set primaryIRQOptionMenu [VxOptionMenu $examine.primaryIRQ \
			{} \
			[GetAudinfo $mf $mo primary_irq_options] \
			PrimaryIRQCB \
			$appvals(pendingPrimaryIRQ)]
		VtSetValues $primaryIRQOptionMenu \
			-topSide $previousMenu \
			-leftSide $primaryIRQOptionLabel \
			-rightSide FORM
		#VtSetValues $primaryIRQOptionLabel -alignBottom $primaryIRQOptionMenu
		set appvals(pendingPrimaryIRQ) \
			[VxOptionMenuGetSelected $primaryIRQOptionMenu]
		set previousLabel $primaryIRQOptionLabel
		lappend optionLabels $previousLabel
		set previousMenu $primaryIRQOptionMenu
	 
		if {[GetAudinfo $mf $mo has_secondary_irq]} {
			set secondaryIRQOptionLabel \
				[VtLabel $examine.secondaryIRQLabel \
				-topSide $previousMenu \
				-leftSide FORM \
				-alignRight $previousLabel \
				-labelRight  \
				-label [IntlMsg EXAMINE_DIALOG_SECONDARY_IRQ]]
			set secondaryIRQOptionMenu \
				[VxOptionMenu $examine.secondaryIRQ \
				{} \
				[GetAudinfo $mf $mo secondary_irq_options] \
				SecondaryIRQCB \
				$appvals(pendingSecondaryIRQ)]
			VtSetValues $secondaryIRQOptionMenu \
				-topSide $previousMenu \
				-leftSide $secondaryIRQOptionLabel \
				-rightSide FORM
			#VtSetValues $secondaryIRQOptionLabel \
				#-alignBottom $secondaryIRQOptionMenu
			set appvals(pendingSecondaryIRQ) \
				[VxOptionMenuGetSelected $secondaryIRQOptionMenu]
			set previousLabel $secondaryIRQOptionLabel
			lappend optionLabels $previousLabel
			set previousMenu $secondaryIRQOptionMenu
		} else {
			set appvals(pendingSecondaryIRQ) {}
		}
	}
	if {$appvals(pendingPrimaryDMA)>0} {	# has_dma
		set primaryDMAOptionLabel [VtLabel $examine.primaryDMALabel \
			-topSide $previousMenu \
			-leftSide FORM \
			-alignRight $previousLabel \
			-labelRight  \
			-label [IntlMsg EXAMINE_DIALOG_PRIMARY_DMA]]
		set primaryDMAOptionMenu [VxOptionMenu $examine.primaryDMA \
			{} \
			[GetAudinfo $mf $mo primary_dma_options] \
			PrimaryDMACB \
			$appvals(pendingPrimaryDMA)]
		VtSetValues $primaryDMAOptionMenu \
			-topSide $previousMenu \
			-leftSide $primaryDMAOptionLabel \
			-rightSide FORM
		#VtSetValues $primaryDMAOptionLabel -alignBottom $primaryDMAOptionMenu
		set appvals(pendingPrimaryDMA) \
			[VxOptionMenuGetSelected $primaryDMAOptionMenu]
		set previousLabel $primaryDMAOptionLabel
		lappend optionLabels $previousLabel
		set previousMenu $primaryDMAOptionMenu
	
		if {[GetAudinfo $mf $mo has_secondary_dma]} {
			set secondaryDMAOptionLabel \
				[VtLabel $examine.secondaryDMALabel \
				-topSide $previousMenu \
				-leftSide FORM \
				-alignRight $previousLabel \
				-labelRight  \
				-label [IntlMsg EXAMINE_DIALOG_SECONDARY_DMA]]
			set secondaryDMAOptionMenu \
				[VxOptionMenu $examine.secondaryDMA \
				{} \
				[GetAudinfo $mf $mo secondary_dma_options] \
				SecondaryDMACB \
				$appvals(pendingSecondaryDMA)]
			VtSetValues $secondaryDMAOptionMenu \
				-topSide $previousMenu \
				-leftSide $secondaryDMAOptionLabel \
				-rightSide FORM
			#VtSetValues $secondaryDMAOptionLabel \
				#-alignBottom $secondaryDMAOptionMenu
			set appvals(pendingSecondaryDMA) \
				[VxOptionMenuGetSelected $secondaryDMAOptionMenu]
			set previousLabel $secondaryDMAOptionLabel
			lappend optionLabels $previousLabel
			set previousMenu $secondaryDMAOptionMenu
		} else {
			set appvals(pendingSecondaryDMA) {}
		}
	}
	# right align the option menu labels
	# VxSetLeftOffsets $optionLabels

	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]
	VtShow $examine
	VtUnLock
}


proc AudioIOCB {cbs} {
	global appvals
	set appvals(pendingAudioIO) [VtGetValues [keylget cbs value] -label]
	#echo "AudioIOCB" $appvals(pendingAudioIO)
}


proc MidiIOCB {cbs} {
	global appvals

	set appvals(pendingMidiIO) [VtGetValues [keylget cbs value] -label]
}


proc SynthIOCB {cbs} {
	global appvals

	set appvals(pendingSynthIO) [VtGetValues [keylget cbs value] -label]
}


proc PrimaryIRQCB {cbs} {
	global appvals

	set appvals(pendingPrimaryIRQ) [VtGetValues [keylget cbs value] -label]
}


proc SecondaryIRQCB {cbs} {
	global appvals

	set appvals(pendingSecondaryIRQ) \
		[VtGetValues [keylget cbs value] -label]
}


proc PrimaryDMACB {cbs} {
	global appvals

	set appvals(pendingPrimaryDMA) [VtGetValues [keylget cbs value] -label]
}


proc SecondaryDMACB {cbs} {
	global appvals

	set appvals(pendingSecondaryDMA) \
		[VtGetValues [keylget cbs value] -label]
}


proc UiExamineOkCB {cbs} {
	global appvals

	VtLock
	set w [keylget cbs widget]
	VtHideDialog $w

	SaStatusBarSet [VxGetVar $appvals(vtMain) statusBar] [IntlMsg UPDATING_DRIVERS]

	if {$appvals(addInProgress)} {
		## Add "(oss)" to the following entries
		## GC 10/10
		#set appvals(pendingAudioIO) [addModName $appvals(pendingAudioIO)]
		#set appvals(pendingMidiIO) [addModName $appvals(pendingMidiIO)]
		#set appvals(pendingSynthIO) [addModName $appvals(pendingSynthIO)]
		#set appvals(pendingPrimaryIRQ) [addModName $appvals(pendingPrimaryIRQ)]
		#set appvals(pendingSecondaryIRQ) [addModName $appvals(pendingSecondaryIRQ)]
		#set appvals(pendingPrimaryDMA) [addModName $appvals(pendingPrimaryDMA)]
		#set appvals(pendingSecondaryDMA) [addModName $appvals(pendingSecondaryDMA)]
		#echo $appvals(pendingUnit) $appvals(pendingManufacturer) $appvals(pendingModel) $appvals(pendingAudioIO) $appvals(pendingMidiIO)
		# do the actual add of the driver
		AddSoundcard \
			$appvals(pendingUnit) \
			$appvals(pendingManufacturer) \
			$appvals(pendingModel) \
			[lindex $appvals(pendingAudioIO) 0] \
			[lindex $appvals(pendingMidiIO) 0] \
			[lindex $appvals(pendingSynthIO) 0] \
			[lindex $appvals(pendingPrimaryIRQ) 0] \
			[lindex $appvals(pendingSecondaryIRQ) 0] \
			[lindex $appvals(pendingPrimaryDMA) 0] \
			[lindex $appvals(pendingSecondaryDMA) 0] \
			$appvals(pendingDrivers)

		# add the new entry to the list
		UiMainListAddItem [list $appvals(pendingUnit) \
			"$appvals(pendingManufacturer)" \
			"$appvals(pendingModel)" \
			[lindex $appvals(pendingAudioIO) 0] \
			[lindex $appvals(pendingMidiIO) 0] \
			[lindex $appvals(pendingSynthIO) 0] \
			[lindex $appvals(pendingPrimaryIRQ) 0] \
			[lindex $appvals(pendingSecondaryIRQ) 0] \
			[lindex $appvals(pendingPrimaryDMA) 0] \
			[lindex $appvals(pendingSecondaryDMA) 0] \
			$appvals(pendingDrivers)]

		# add includes examine so destroy both dialogs
		VtDestroyDialog $appvals(examineDialog)
		set appvals(examineInProgress) 0
		VtDestroyDialog $appvals(addDialog)
		set appvals(addInProgress) 0
	} else {
		# check if anything actually changed

		set changed 0
		foreach attr {AudioIO MidiIO SynthIO PrimaryIRQ SecondaryIRQ PrimaryDMA SecondaryDMA} {
			set changed [expr {![lempty $appvals(pending$attr)]} && {[lindex $appvals(pending$attr) 1]!="(oss)"} || $changed]
		}
		if {$changed} {
		# modify config
		ModifySoundcard \
			$appvals(pendingUnit) \
			$appvals(pendingManufacturer) \
			$appvals(pendingModel) \
			[lindex $appvals(pendingAudioIO) 0] \
			[lindex $appvals(pendingMidiIO) 0] \
			[lindex $appvals(pendingSynthIO) 0] \
			[lindex $appvals(pendingPrimaryIRQ) 0] \
			[lindex $appvals(pendingSecondaryIRQ) 0] \
			[lindex $appvals(pendingPrimaryDMA) 0] \
			[lindex $appvals(pendingSecondaryDMA) 0] \
			$appvals(pendingDrivers)

		# update the config data in the list
		UiMainListReplaceItem [list $appvals(pendingUnit) \
			"$appvals(pendingManufacturer)" \
			"$appvals(pendingModel)" \
			[lindex $appvals(pendingAudioIO) 0] \
			[lindex $appvals(pendingMidiIO) 0] \
			[lindex $appvals(pendingSynthIO) 0] \
			[lindex $appvals(pendingPrimaryIRQ) 0] \
			[lindex $appvals(pendingSecondaryIRQ) 0] \
			[lindex $appvals(pendingPrimaryDMA) 0] \
			[lindex $appvals(pendingSecondaryDMA) 0] \
			$appvals(pendingDrivers) \
			]
#		} else {
#			echo "No change."
		}

		# destroy the examine dialog
		VtDestroyDialog $appvals(examineDialog)
		set appvals(examineInProgress) 0
	}

	SaStatusBarClear [VxGetVar $appvals(vtMain) statusBar]
	VtUnLock
}


proc UiExamineCancelCB {cbs} {
	global appvals

	VtLock
	set w [keylget cbs widget]
	VtDestroyDialog $w
	set appvals(examineInProgress) 0

	if {$appvals(addInProgress)} {
		VtShowDialog $appvals(addDialog)
	}
	VtUnLock
}

# Add a string (default "(oss)") to value of resources

proc addModName {res {mod (oss)}} {
	if {$res!=""} {
		return  [list $res $mod]
	} else {
		return $res
	}
}
