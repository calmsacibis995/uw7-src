#define TC_NIOC \
"TRACE:NAME: IOCTLs(Normal-User)\n"
#define TC_PIOC \
"TRACE:NAME: IOCTLs(Previleged-User)\n"
#define NotAllowed(name) \
"INFO:" "-" #name " ioctl is allowed to previleged process\n"
#define EinvalPass(name) \
"INFO:"#name " ioctl fails with EINVAL if an invalid user buffer given\n"
#define EinvalFail(name) \
"INFO:" "-" #name " ioctl fails with 0x%x [not EINVAL] if an invalid user buffer given\n"
#define EinvalFail0(name) \
"INFO:" "-" #name " ioctl passes even with an invalid user buffer\n"
#define EpermPass(name) \
"INFO:"#name " ioctl fails with EPERM for a normal user\n"
#define EpermFail(name) \
"INFO:" "-" #name " ioctl fails with 0x%x [not EPERM] for a normal user\n"
#define EpermFail0(name) \
"INFO:" "-" #name " ioctl passes even for a normal user\n"
#define IocFail(name) \
"INFO:" "-" #name " ioctl fails with errorno 0x%x\n"
#define SMIB_PASS \
"INFO:DLIOCSMIB ioctl allows previleged process to set MIB successfully\n"
#define SMIB_PASS0 \
"INFO:DLIOCSMIB ioctl fails with EPERM for a normal user\n"
#define SMIB_FAIL \
"INFO: - DLIOCSMIB ioctl fails for previleged user\n"
#define GMIB_PASS \
"INFO:DLIOCGMIB ioctl gets MIB successfully\n"
#define CCSMACDMODE_INVAL_PASS \
"INFO:DLIOCCSMACDMODE ioctl fails with EINVAL for stream not in DL_IDLE\n"
#define CCSMACDMODE_INVAL_FAIL0 \
"INFO: - DLIOCCSMACDMODE ioctl fails with %x [expected EINVAL] for stream not in DL_IDLE\n"
#define CCSMACDMODE_INVAL_FAIL \
"INFO: - DLIOCCSMACDMODE ioctl passes even for stream not in DL_IDLE state\n"
#define CCSMACDMODE_INVAL1_PASS \
"INFO:DLIOCCSMACDMODE ioctl fails with EINVAL for DL_ETHER sap\n"
#define CCSMACDMODE_INVAL1_FAIL0 \
"INFO: - DLIOCCSMACDMODE ioctl fails with %x [expected EINVAL] for DL_ETHER sap\n"
#define CCSMACDMODE_INVAL1_FAIL \
"INFO: - DLIOCCSMACDMODE ioctl passes even for DL_ETHER sap\n"
#define CCSMACDMODE_INVAL2_PASS \
"INFO:DLIOCCSMAMODE ioctl fails with EINVAL for SNAP sap\n"
#define CCSMACDMODE_INVAL2_FAIL0 \
"INFO: - DLIOCCSMACDMODE ioctl fails with %x [expected EINVAL] for SNAP sap\n"
#define CCSMACDMODE_INVAL2_FAIL \
"INFO: - DLIOCCSMACDMODE ioctl passes even for SNAP sap\n"
#define CCSMACDMODE_PASS \
"INFO:DLIOCCSMACDMODE ioctl allows previleged process to switch SAP type successfully\n"
#define CCSMACDMODE_PASS0 \
"INFO:DLIOCCSMACDMODE ioctl fails with EPERM for a normal user\n"
#define CCSMACDMODE_FAIL \
"INFO: - DLIOCCSMACDMODE ioctl fails for a previlege process\n"
#define GENADDR_PASS \
"INFO:DLIOCGENADDR gets ethernet address successfully\n"
#define GENADDR_FAIL \
"INFO: - DLIOCGENADDR does not get ethernet address successfully\n"
#define SENADDR_PASS \
"INFO:DLIOCSENADDR sets ethernet address successfully\n"
#define SENADDR_PASS0 \
"INFO:DLIOCSENADDR fails with EPERM for a normal user\n"
#define SENADDR_FAIL \
"INFO: - DLIOCSENADDR does not set ethernet address successfully\n"
#define SENADDR_FAIL0 \
"INFO: - DLIOCSENADDR does not set ethernet address correctly\n"
#define ADDMULTI_PASS \
"INFO:DLIOCADDMULTI adds multicast address successfully\n"
#define ADDMULTI_PASS0 \
"INFO:DLIOCADDMULTI ioctl fails with EPERM for a normal user\n"
#define ADDMULTI_FAIL \
"INFO: - DLIOCADDMULTI does not add multicast address successfully\n"
#define DELMULTI_PASS \
"INFO:DLIOCDELMULTI deletes multicast address successfully\n"
#define DELMULTI_PASS0 \
"INFO:DLIOCDELMULTI ioctl fails with EPERM for a normal user\n"
#define DELMULTI_FAIL \
"INFO: - DLIOCDELMULTI does not delete multicast address successfully\n"
#define SPROMISC_PASS \
"INFO:DLIOCSPROMISC sets the board in promiscuous mode\n"
#define SPROMISC_PASS0 \
"INFO:DLIOCSPROMISC ioctl fails with EPERM for a normal user\n"
#define SPROMISC_FAIL \
"INFO: - DLIOCSPROMISC does not set the board in promiscuous mode\n" 
#define GETMULTI_PASS \
"INFO:DLIOCGETMULTI retrieves multicast address successfully\n"
#define GETMULTI_FAIL \
"INFO: - DLIOCGETMULTI does not retrive multicast address successfully\n"
#define GETMULTI_FAIL0 \
"INFO: - DLIOCGETMULTI retrives wrong multicast address\n"
#define INVALMULTI_PASS \
"INFO:The ioctl fails with EINVAL if an invalid multicast address given\n"
#define INVALMULTI_FAIL \
"INFO: - The ioctl fails with 0x%x [not EINVAL] if an invalid multicast address given\n"
#define INVALMULTI_FAIL0 \
"INFO: - The ioctl passes even if an invalid multicast address is given\n"
#define RESET_PASS \
"INFO:DLIOCRESET resets the board successfully\n"
#define RESET_PASS0 \
"INFO:DLIOCRESET ioctl fails with EPERM for a normal user\n"
#define RESET_FAIL \
"INFO: - DLIOCRESET ioctl is not successfull\n"
#define ENABLE_PASS \
"INFO:DLIOCENABLE enables the board successfully\n"
#define ENABLE_PASS0 \
"INFO:DLIOCENABLE ioctl fails with EPERM for a normal user\n"
#define ENABLE_FAIL \
"INFO: - DLIOCENABLE ioctl is not successfull\n"
#define DISABLE_PASS \
"INFO:DLIOCDISABLE enables the board successfully\n"
#define DISABLE_PASS0 \
"INFO:DLIOCDISABLE ioctl fails with EPERM for a normal user\n"
#define DISABLE_FAIL \
"INFO: - DLIOCDISABLE ioctl is not successfull\n"
#define EPERM_ADDMULTI_PASS \
"INFO:Only Previlege user can add multicast address\n"
#define ADDMULTI_LIMIT \
"INFO:DLIOCADDMULTI fails with EINVAL after a finite number of successful attempts\n"
#define EPERM_DELMULTI_PASS \
"INFO:Only Previlege user can delete multicast address\n"
#define SLPCFLG_PASS \
"INFO:DLIOCSLPCFLG sets the local packet copy flag successfully\n"
#define SLPCFLG_FAIL \
"INFO: - DLIOCSLPCFLG does not set the local packet copy flag successfully!!\n"
#define SLPCFLG_PASS0 \
"INFO:DLIOCSLPCFLG fails with EPERM for normal user\n"
#define GLPCFLG_PASS \
"INFO:DLIOCGLPCFLG gets the local packet copy flag successfully\n"
#define GLPCFLG_FAIL0 \
"INFO: - DLIOCGLPCFLG fails with %x [not expected]\n"
#define GPROMISC_PASS \
"INFO:DLIOCGPROMISC get promiscuous flag successfully\n"
#define GPROMISC_FAIL0 \
"INFO: - DLIOCGPROMISC fails with %x [not expected]\n"
#define PROMISC_MODE_FAIL \
"INFO: - Listener couldn't be set(reset) to promiscuous mode!!\n"
#define LINK_OK \
"INFO: - Link is fine after set local packet copy flag ioctl\n"
#define LINK_NOTOK_DISABLED \
"INFO: - Could not talk to neighbour - board may be disabled\n"
