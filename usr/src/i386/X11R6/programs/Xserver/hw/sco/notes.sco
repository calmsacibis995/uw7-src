
	@(#)notes.sco	6.1	12/8/95	14:45:01

Directories in the hw/sco port:

./Xserver/hw/sco/mit - was R5 ./server/sys/intel - satisify calls
	from the core server
./Xserver/hw/sco/grafinfo - R5 ./server/sys/grafinfo - handle grafinfo files
./Xserver/hw/sco/dyddx - R5 ./server/sys/common - does the dynamic loading
./Xserver/hw/sco/nfb - R5 ./server/ddx/nfb - nfb layer interface
./Xserver/hw/sco/ports - R5 ./server/ddx/ports - all SCO drivers
./Xserver/hw/sco/include - I have been placing includes here required
	by more than one of the sco directories.  May be good to
	place includes:: targets in the Imakefiles of directories
	that would like to export their include files here.

Linking a server without any actual specific server linked
in produces the following undefines:

	cc -o Xsco -O -b elf  -b elf   -L../../usrlib  dix/libdix.a os/libos.a ../../lib/Xau/libXau.a ../../lib/Xdmcp/libXdmcp.a ../../usrlib/libfont.a  mfb/libmfb.a cfb/libcfb.a cfb16/libcfb.a cfb32/libcfb.a mi/libmi.a Xext/libext.a   XIE/dixie/libdixie.a XIE/mixie/libmixie.a    PEX5/dipex/dispatch/libdidipex.a  PEX5/dipex/swap/libdiswapex.a  PEX5/dipex/objects/libdiobpex.a  PEX5/dipex/dispatch/libdidipex.a  PEX5/ddpex/mi/level4/libddpex4.a  PEX5/ddpex/mi/level3/libddpex3.a  PEX5/ddpex/mi/shared/libddpexs.a  PEX5/ddpex/mi/level2/libddpex2.a  PEX5/ddpex/mi/level1/libddpex1.a  PEX5/ospex/libospex.a    -lsocket -lm  -ldbm  
Undefined			first referenced
 symbol  			    in file
OsVendorInit                        os/libos.a(osinit.o)
XTestJumpPointer                    Xext/libext.a(xtest1dd.o)
InitOutput                          dix/libdix.a(main.o)
ddxProcessArgument                  os/libos.a(utils.o)
InitInput                           dix/libdix.a(main.o)
LegalModifier                       dix/libdix.a(devices.o)
XTestGenerateEvent                  Xext/libext.a(xtest1dd.o)
AbortDDX                            os/libos.a(utils.o)
ddxUseMsg                           os/libos.a(utils.o)
XTestGetPointerPos                  Xext/libext.a(xtest1dd.o)
GetTimeInMillis                     dix/libdix.a(dispatch.o)
ProcessInputEvents                  dix/libdix.a(dispatch.o)
ddxGiveUp                           dix/libdix.a(main.o)
Xsco: fatal error: Symbol referencing errors. No output written to Xsco
*** Error code 1 (bu21)
