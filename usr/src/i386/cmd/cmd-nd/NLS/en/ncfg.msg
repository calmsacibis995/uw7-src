$ ident "@(#)ncfg.msg	29.1"
$ ident "$Header$"
$
$  Based on OpenServer ncfg.msg version 13.1
$
$	Copyright (C) The Santa Cruz Operation, 1994-1996
$	This Module contains Proprietary Information of
$	The Santa Cruz Operation, and should be treated as Confidential.

$ WARNING - Place all new messages at the end of the appropriate moduleset
$ to not interfere with localization efforts

$moduleset SCO_NETCONFIG_UI TCL=ncfgUI
SCO_NETCONFIG_UI_ERR_CODE "netcfg(ERRORCB) Error code:"
SCO_NETCONFIG_UI_ERR_INFO "netcfg(ERRORCB) Error info:"
SCO_NETCONFIG_UI_ERR_BE_DIED "netcfg(SendRequest) Back-End script error"
SCO_NETCONFIG_UI_ERR_CANT_FORK "netcfg(main): Unable to fork back-end-script"
SCO_NETCONFIG_UI_ERR_CANT_FORK2 "netcfg(CallPOP): Unable to fork to add/remove chain"
$
$ Titles of windows that popup while chains are being configured,
$ deconfigured, reconfigured and examined (respectively).
SCO_NETCONFIG_UI_MSG_CONFIGURE_TITLE "Configure networking product"
SCO_NETCONFIG_UI_MSG_DECONFIGURE_TITLE "Remove networking product"
SCO_NETCONFIG_UI_MSG_RECONFIGURE_TITLE "Modify networking product"
SCO_NETCONFIG_UI_MSG_EXAMINE_TITLE "View networking product configuration"
$
$ Confirmation messages that chains were successfully, configured, deconfigured
$ reconfigured (respectively) - no confirmation for chain examination.
SCO_NETCONFIG_UI_MSG_CONFIGURE_OK "The following products were successfully configured into the system"
SCO_NETCONFIG_UI_MSG_DECONFIGURE_OK "The following products were successfully removed from the system"
SCO_NETCONFIG_UI_MSG_RECONFIGURE_OK "The following product was successfully modified"
$
$ Warning messages displayed when chains were not successfully configured,
$ deconfigured and reconfigured (respectively) - no error for chain examination
SCO_NETCONFIG_UI_ERR_CONFIGURE "A problem occurred while the following products were\nbeing added to the system:"
SCO_NETCONFIG_UI_ERR_DECONFIGURE "A problem occurred while the following products were\nbeing removed from the system:"
SCO_NETCONFIG_UI_ERR_RECONFIGURE "A problem occurred while the following product was being\nmodified:"
SCO_NETCONFIG_UI_MSG_REVIEW_LOG "Review the error log for more information"
$
$ Single characters to be displayed in the CHARM version of netcfg to
$ replace the green card icon in the X version.  Currently set to 'HW'
$ to represent HardWare
SCO_NETCONFIG_UI_MSG_HWICON_L "H"
SCO_NETCONFIG_UI_MSG_HWICON_R "W"
$
$ Short words used on buttons to indicate an action of some kind
SCO_NETCONFIG_UI_MSG_YES "Yes"
SCO_NETCONFIG_UI_MSG_NO "No"
SCO_NETCONFIG_UI_MSG_ADD "Add"
SCO_NETCONFIG_UI_MSG_CONTINUE "Continue"
$
$ Messages displayed during a kernel relink
SCO_NETCONFIG_UI_MSG_RELINK_TITLE "Unix kernel relink"
SCO_NETCONFIG_UI_MSG_RELINK "A Unix kernel relink has been scheuled.\nThe system must be rebooted before the changes\nmade here will have an effect.\n"
$
$ Messages displayed during a chain deconfiguration
SCO_NETCONFIG_UI_MSG_DECONFIGURE_CONFIRM "Do you wish to remove the networking products highlighted above?"
$
$ Messages displayed during a chain configuration
SCO_NETCONFIG_UI_MSG_SELECT_LAN "Select network adapter to be added:"
SCO_NETCONFIG_UI_MSG_CONFIGURE_OTHER_HW "Configure hardware not listed above..."
SCO_NETCONFIG_UI_MSG_SELECT_WAN "Select WAN device to be added:"
SCO_NETCONFIG_UI_MSG_NO_PROTOCOLS "No protocol stacks available on the system\nare supported on this device."
SCO_NETCONFIG_UI_MSG_NO_LANS "No network adapters are installed on the system."
SCO_NETCONFIG_UI_MSG_NO_WANS "No WAN devices are installed on the system."
SCO_NETCONFIG_UI_MSG_SELECT_ONE "Select one networking product to be added to\n%s:"
SCO_NETCONFIG_UI_MSG_NO_ADAPTER_DETECTED "No %s found in the system"
SCO_NETCONFIG_UI_MSG_DETECT_TITLE "Network Adapters Detected"
SCO_NETCONFIG_UI_MSG_ADAPTER_DETECTED "The following adapters have been found:"
SCO_NETCONFIG_UI_MSG_USING_GENERIC "Using generic entry for %s."
SCO_NETCONFIG_UI_MSG_SEARCH_TITLE "Search for network adapter"
SCO_NETCONFIG_UI_MSG_SEARCH_QUESTION "  Do you wish to search for a %s?"
$
$ Pull down menu options and message dialog titles for those menu options
SCO_NETCONFIG_UI_MSG_HW_PULLDOWN "Hardware"
SCO_NETCONFIG_UI_MSG_HW_PULLDOWN_HELP "Add, Remove, or Modify a network adapter or serial link"
SCO_NETCONFIG_UI_MSG_SW_PULLDOWN "Protocol"
SCO_NETCONFIG_UI_MSG_SW_PULLDOWN_HELP "Add, Remove, or Modify a network protocol"
$
SCO_NETCONFIG_UI_MSG_ADD_LAN "Add new LAN adapter"
SCO_NETCONFIG_UI_MSG_ADD_LAN_HELP "Configure a network adapter just installed in the machine"
SCO_NETCONFIG_UI_MSG_ADD_WAN "Add new WAN device"
SCO_NETCONFIG_UI_MSG_ADD_WAN_HELP "Configure a new ISDN device"
SCO_NETCONFIG_UI_MSG_RMV_HW "Remove network device"
SCO_NETCONFIG_UI_MSG_RMV_HW_HELP "Remove the selected network device"
SCO_NETCONFIG_UI_MSG_RCNF_HW "Modify hardware configuration"
SCO_NETCONFIG_UI_MSG_RCNF_HW_HELP "Modify the current configuration of the selected network device"
SCO_NETCONFIG_UI_MSG_RCNF_SW "Modify protocol configuration"
SCO_NETCONFIG_UI_MSG_RCNF_SW_HELP "Modify the current configuration of the selected network protocol"
SCO_NETCONFIG_UI_MSG_EXAMINE_HW "View hardware configuration"
SCO_NETCONFIG_UI_MSG_EXAMINE_HW_HELP "View the current configuration of the selected network device"
SCO_NETCONFIG_UI_MSG_EXAMINE_SW "View protocol configuration"
SCO_NETCONFIG_UI_MSG_EXAMINE_SW_HELP "View the current configuration of the selected network protocol"
SCO_NETCONFIG_UI_MSG_EXIT "Exit"
SCO_NETCONFIG_UI_MSG_EXIT_HELP "Exit this network configuration utility"
$
SCO_NETCONFIG_UI_MSG_ADD_PROTOCOL "Add protocol"
SCO_NETCONFIG_UI_MSG_ADD_PROTOCOL_HELP "Configure a network protocol to use a device already in the system"
SCO_NETCONFIG_UI_MSG_RMV_PROTOCOL "Remove protocol"
SCO_NETCONFIG_UI_MSG_RMV_PROTOCOL_HELP "Remove the selected network protocol"
$
$ Messages displayed on the main screen
SCO_NETCONFIG_UI_MSG_MAIN_FORM_TITLE "Network Configuration Manager"
SCO_NETCONFIG_UI_MSG_MAIN_FORM_LABEL "Current networking configuration:"
SCO_NETCONFIG_UI_MSG_MAIN_FORM_HELP "Current configuration of the LAN software in this system"
$
SCO_NETCONFIG_UI_MSG_STARTUP  "Network Configuration Manager starting..."
SCO_NETCONFIG_UI_ERR_NOT_ROOT "Must be root to run netcfg"
SCO_NETCONFIG_UI_ERR_NOT_EXCL "netcfg already running"
$
$ error messages returned from ncfgBE
SCO_NETCONFIG_UI_ERR_BE_INFO_MISSING "corrupt netcfg database: info file missing"
SCO_NETCONFIG_UI_ERR_FORK_FAIL "netcfg: Unable to fork back-end script"
SCO_NETCONFIG_UI_ERR_BE_SCRIPT_DIED "ncfgBE(SendNDRequest): Network Driver Utility (ndcfg) died"
SCO_NETCONFIG_UI_ERR_BE_UNKNOWN_REQ "Unknown Request passed to ncfgBE"
$
$ Mnemonics for SCO_NETCONFIG_UI_MSG_EXAMINE_HW
$           and SCO_NETCONFIG_UI_MSG_EXAMINE_SW
SCO_NETCONFIG_UI_MSG_EXAMINE_MNEMONIC "V"
$ 
$ Mnemonic for SCO_NETCONFIG_UI_MSG_HW_PULLDOWN
SCO_NETCONFIG_UI_MSG_HARDWARE_MNEMONIC "a"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_SW_PULLDOWN
SCO_NETCONFIG_UI_MSG_PROTOCOL_MNEMONIC "P"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_ADD_LAN
SCO_NETCONFIG_UI_MSG_ADD_LAN_MNEMONIC "L"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_ADD_WAN
SCO_NETCONFIG_UI_MSG_ADD_WAN_MNEMONIC "W"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_ADD_PROTOCOL 
SCO_NETCONFIG_UI_MSG_ADD_PROTOCOL_MNEMONIC "A"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_RMV_HW and SCO_NETCONFIG_UI_MSG_RMV_PROTOCOL
SCO_NETCONFIG_UI_MSG_REMOVE_MNEMONIC "R"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_RCNF_HW and SCO_NETCONFIG_UI_MSG_RCNF_SW
SCO_NETCONFIG_UI_MSG_MODIFY_MNEMONIC "M"
$
$ Mnemonic for SCO_NETCONFIG_UI_MSG_EXIT
SCO_NETCONFIG_UI_MSG_EXIT_MNEMONIC "x"
$
$ Modified messages for AHS 5.2
SCO_NETCONFIG_UI_MSG_NO_ADAPTER_DETECTED_REV_AHS52 "No %s found in the system\nthat have not been configured previously"
$
$ New message for AHS 5.2
SCO_NETCONFIG_UI_ERR_RELINK_FAILED "Unix kernel relink failed"
$
$ New messages added for Gemini netcfg
SCO_NETCONFIG_UI_MSG_ADD_TOPO "Select network topology:"
SCO_NETCONFIG_UI_MSG_VIEW_PULLDOWN "View"
SCO_NETCONFIG_UI_MSG_VIEW_PULLDOWN_HELP "LAN or WAN Configuration View"
SCO_NETCONFIG_UI_MSG_VIEW_MNEMONIC "i"
SCO_NETCONFIG_UI_MSG_VIEW_WAN "WAN"
SCO_NETCONFIG_UI_MSG_VIEW_WAN_MNEMONIC "W"
SCO_NETCONFIG_UI_MSG_VIEW_WAN_HELP "WAN Configuration View"
SCO_NETCONFIG_UI_MSG_VIEW_LAN "LAN"
SCO_NETCONFIG_UI_MSG_VIEW_LAN_MNEMONIC "L"
SCO_NETCONFIG_UI_MSG_VIEW_LAN_HELP "LAN Configuration View"
SCO_NETCONFIG_UI_MSG_WAN_SW_FORM_LABEL "WAN Software Configuration:"
SCO_NETCONFIG_UI_MSG_WAN_SW_FORM_HELP "Current configuration of the WAN software in this system"
SCO_NETCONFIG_UI_MSG_WAN_HW_FORM_LABEL "WAN Hardware Configuration:"
SCO_NETCONFIG_UI_MSG_WAN_HW_FORM_HELP "Current configuration of the WAN hardware in this system"
SCO_NETCONFIG_UI_MSG_WAN_CALL_LABEL "Call Services"
SCO_NETCONFIG_UI_MSG_OUTCALL_BUTTON "Outgoing"
SCO_NETCONFIG_UI_MSG_OUTCALL_HELP "Configure Outgoing Call Services"
SCO_NETCONFIG_UI_MSG_INCALL_BUTTON "Incoming"
SCO_NETCONFIG_UI_MSG_INCALL_HELP "Configure Incoming Call Services"
SCO_NETCONFIG_UI_ERR_BE_SCRIPT_ERROR "ncfgBE(SendNDRequest): Network Driver Utility ndcfg error"
SCO_NETCONFIG_UI_ERR_BE_PROMPTER_FAILED "ncfgprompter Failed"
SCO_NETCONFIG_UI_MSG_NO_ADAPTER_DETECTED_DANGER "Would you like to search again using all addresses,\n including those that may conflict with other devices\n already in the system?"
SCO_NETCONFIG_UI_MSG_HW_ADD "Please wait while network drivers are added..."
SCO_NETCONFIG_UI_MSG_HW_RECONF "Please wait while network drivers are modified..."
SCO_NETCONFIG_UI_MSG_DELETE "Please wait while network products are removed..."
SCO_NETCONFIG_UI_MSG_WANSW_PULLDOWN "Software"
$ Mnemonic for SCO_NETCONFIG_UI_MSG_WANSW_PULLDOWN
SCO_NETCONFIG_UI_MSG_SOFTWARE_MNEMONIC "S"
$ The following were added as per ul97-24007
$ was SCO_NETCONFIG_UI_MSG_EXAMINE_MNEMONIC "V"
SCO_NETCONFIG_UI_MSG_EXAMINE_HW_MNEMONIC "V"
SCO_NETCONFIG_UI_MSG_EXAMINE_SW_MNEMONIC "V"
$ was SCO_NETCONFIG_UI_MSG_REMOVE_MNEMONIC "R"
SCO_NETCONFIG_UI_MSG_RMV_HW_MNEMONIC "R"
SCO_NETCONFIG_UI_MSG_RMV_PROTOCOL_MNEMONIC "R"
$ was SCO_NETCONFIG_UI_MSG_MODIFY_MNEMONIC "M"
SCO_NETCONFIG_UI_MSG_RCNF_HW_MNEMONIC "M"
SCO_NETCONFIG_UI_MSG_RCNF_SW_MNEMONIC "M"
SCO_NETCONFIG_UI_MSG_SERIAL "Serial Port"
SCO_NETCONFIG_UI_MSG_SERIAL_MNEMONIC "S"
SCO_NETCONFIG_UI_MSG_SERIAL_HELP "Add New Serial Port"
SCO_NETCONFIG_UI_MSG_MODEM "Modem"
SCO_NETCONFIG_UI_MSG_MODEM_MNEMONIC "M"
SCO_NETCONFIG_UI_MSG_MODEM_HELP "Add New Modem"
SCO_NETCONFIG_UI_MSG_WAN_CALL_MNEMONIC "C"
SCO_NETCONFIG_UI_ERR_NOT_AUTH "You are not authorized to run netcfg"
SCO_NETCONFIG_UI_MSG_TEST_HW_LABEL "Test Network Connectivity"
SCO_NETCONFIG_UI_MSG_TEST_HW_MNEMONIC "T"
SCO_NETCONFIG_UI_MSG_TEST_HW_HELP "Test Network Connectivity Using LLC TEST Frames"
SCO_NETCONFIG_UI_MSG_TEST_HW_RESULT "Test result for %s"
SCO_NETCONFIG_UI_MSG_REBOOT "The system must be rebooted before the changes\nmade here will have an effect.\n"


$moduleset SCO_NCFGPROMPTER TCL=ncfgprompter
SCO_NCFGPROMPTER_MSG_ERR_TITLE "Error"
SCO_NCFGPROMPTER_MSG_DISMISS "Dismiss"
SCO_NCFGPROMPTER_MSG_ADVANCED_BUTTON "Advanced\nOptions..."
SCO_NCFGPROMPTER_MSG_ADVANCED_TITLE "Advanced %s"
SCO_NCFGPROMPTER_MSG_NDTYPE_READ_ONLY "Please verify adapter is set to the configuration just entered"
SCO_NCFGPROMPTER_MSG_VIEW "View"
SCO_NCFGPROMPTER_ERR_BE_SCRIPT_DIED "prompter(SendRequest): Back-End script died"
SCO_NCFGPROMPTER_ERR_USAGE "Usage\n\t%s back-end-script"
SCO_NCFGPROMPTER_ERR_FORK_FAIL "%s: Unable to fork back-end-script"


$moduleset SCO_NCFGINSTALL TCL=ncfginstall
SCO_NCFGINSTALL_MSG_USAGE "Usage:"
SCO_NCFGINSTALL_MSG_COMPONENT "component"
SCO_NCFGINSTALL_MSG_OLD_COMPONENT_NAME "old_component_name"
SCO_NCFGINSTALL_MSG_FORK "ncfginstall: Unable to fork back-end-script"
SCO_NCFGINSTALL_MSG_COMP_EXISTS "ncfginstall: Chain component already exists"
SCO_NCFGINSTALL_MSG_NO_COMP_EXISTS "ncfginstall: Chain component does not exist"
SCO_NCFGINSTALL_MSG_NO_FILE "ncfginstall: Missing required file"
SCO_NCFGINSTALL_MSG_EMPTY_INFO_DIR "ncfginstall: Empty Info directory"
SCO_NCFGINSTALL_ERR_NCFGBE_SCRIPT_DIED "ncfginstall(SendBERequest): script %s died"


$moduleset SCO_NCFG TCL=netcfg
SCO_NCFG_ERR_NOT_PART_OF_CHAIN "netcfg: %1$s is not part of %2$s"
SCO_NCFG_ERR_ARGLIST_NOT_EXIST "netcfg: %s does not exist"
SCO_NCFG_ERR_UNABLE_TO_EXEC "netcfg: Unable to execute %s"
SCO_NCFG_ERR_BE_DIED "netcfg: BE died: %s"
SCO_NCFG_ERR_CANNOT_FORK_BE "netcfg: can't fork BE"
SCO_NCFG_ERR_CHAIN_CMPNT_NOT_EXIST "netcfg: Chain component %1$s (%2$s) does not exist"
SCO_NCFG_ERR_USAGE "Usage:"
SCO_NCFG_ERR_USAGE_UI "\tnetcfg\t\t\t\t- User Interface"
SCO_NCFG_ERR_USAGE_ADD_CHAIN "\tnetcfg [options] -a chain\t\t- Add a chain"
SCO_NCFG_ERR_USAGE_DEFER_ADD "\tnetcfg [options] -da chain\t\t- Defer-Add a chain"
SCO_NCFG_ERR_USAGE_REMOVE_CHAIN "\tnetcfg [options] -r chain\t\t- Remove a chain"
SCO_NCFG_ERR_USAGE_REMOVE_CHAIN_ELEMENT "\tnetcfg [options] -re element\t\t- Remove a chain element"
SCO_NCFG_ERR_USAGE_RECONFIG_CHAIN_ELEMENT "\tnetcfg [options] -c chain element\t- Reconfigure a chain element"
SCO_NCFG_ERR_USAGE_EXAMINE_CHAIN "\tnetcfg [options] -e chain element\t- Examine a chain element configuration"
SCO_NCFG_ERR_USAGE_LIST_CHAINS "\tnetcfg [options] -s\t\t\t- List all configured chains"
SCO_NCFG_ERR_USAGE_DEFER_ADD_NOW "\tnetcfg [options] -add_deferred_now\t- Add all '-da' chains now"
SCO_NCFG_ERR_USAGE_OPTIONS "\n\t\toptions:"
SCO_NCFG_ERR_USAGE_RELINK "\t\t-l\t\t\t\t- Always relink the kernel"
SCO_NCFG_ERR_USAGE_NO_RELINK "\t\t-n\t\t\t\t- Never relink the kernel"
SCO_NCFG_ERR_CHAIN_EXISTS "netcfg: Chain %s already exists"
SCO_NCFG_ERR_DEFERRED_CHAIN_EXISTS "netcfg: Deferred chain %s already exists"
SCO_NCFG_ERR_CHAIN_DOESNT_EXIST "netcfg: Chain %s does not exist"
SCO_NCFG_ERR_DRIVER_NOT_INSTALLED "\nDriver for %s is not installed."
SCO_NCFG_ERR_INSTALL_DRIVER "If you wish to use this adapter, please install the driver.\n"
SCO_NCFG_ERR_REMOVING_PRODUCTS "Removing all networking products configured with this adapter..."


$moduleset SCO_NETCONFIG_BE TCL=ncfgBE
SCO_NETCONFIG_BE_MSG_IOADDR "I/O Address"
SCO_NETCONFIG_BE_MSG_DMAC "DMA Channel"
SCO_NETCONFIG_BE_MSG_IRQ "Interrupt Vector"
SCO_NETCONFIG_BE_MSG_CABLE_TYPE "Media/Cable Type"
SCO_NETCONFIG_BE_MSG_MEMADDR "Shared RAM"
SCO_NETCONFIG_BE_MSG_BOOT_ROM "Boot ROM"
SCO_NETCONFIG_BE_MSG_SLOT_NUM "Slot #"
SCO_NETCONFIG_BE_MSG_TRANSCEIVER_TYPE "Transceiver Type"
SCO_NETCONFIG_BE_MSG_MAC_ADDRESS "MAC Address"
SCO_NETCONFIG_BE_MSG_PROMPTER_TITLE "Network Driver Configuration"
SCO_NETCONFIG_BE_MSG_PROMPTER_UPPER "Enter the configuration of %s"
SCO_NETCONFIG_BE_MSG_PROMPTER_VIEW_UPPER "Current configuration of %s"
SCO_NETCONFIG_BE_MSG_SERIAL_OSA "Serial Manager failed (OSA call)"
SCO_NETCONFIG_BE_MSG_ADV_OPTIONS "Advanced Options"
SCO_NETCONFIG_BE_MSG_ADV_TITLE "Advanced Network Driver Configuration"
SCO_NETCONFIG_BE_MSG_ISDN_SHELP "Enable adapter for incoming or outgoing calls"


$moduleset SCO_LIBSCO TCL=libSCO
SCO_LIBSCO_MSG_PROMPT_IO_ADDRESS "I/O Address"
SCO_LIBSCO_MSG_PROMPT_DMA_CHANNEL "DMA Channel"
SCO_LIBSCO_MSG_PROMPT_INT_VECTOR "Interrupt Vector"
SCO_LIBSCO_MSG_PROMPT_MEDIA_TYPE "Media/Cable Type"
SCO_LIBSCO_MSG_PROMPT_SHARED_RAM "Shared RAM"
SCO_LIBSCO_MSG_PROMPT_BOOT_ROM "Boot ROM"
SCO_LIBSCO_MSG_PROMPT_SLOT_NUM "Slot #"
SCO_LIBSCO_MSG_PROMPT_TRANSCEIVER_TYPE "Transceiver Type"
SCO_LIBSCO_MSG_PROMPT_MAC_ADDRESS "MAC Address"

$moduleset SCO_DLPID TCL=dlpid
SCO_DLPID_ERR_DLPI_ALREADY_RUNNING "Error:\t%s already running"
SCO_DLPID_ERR_USE_RESTART "Use:\t\"%s restart [ interface ]\""
SCO_DLPID_ERR_DLPID_NOT_TERMINATING "dlpid process id=%s not terminating properly"
SCO_DLPID_ERR_USAGE "Usage:"
SCO_DLPID_ERR_USAGE_START "\t%s start \[ interface \]"
SCO_DLPID_ERR_USAGE_STOP "\t%s stop \[ interface \]"
SCO_DLPID_ERR_USAGE_RESTART "\t%s restart \[ interface \]"
SCO_DLPID_ERR_ILLEGAL_OPTION "%s: Illegal command or option specified"
