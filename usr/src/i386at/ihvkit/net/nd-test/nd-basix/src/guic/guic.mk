SUITE=nd-basix
include $(CMDRULES)
INCLUDES= -I ${SUITE_ROOT}/${SUITE}/inc
COPTS=-O
CFLAGS=${INCLUDES} ${COPTS}
LIBS=${SUITE_ROOT}/${SUITE}/lib/libgen.a

#CTARGETS=display_results sort_param_list clean_locks gethost get_params unc_read det_res encode Start_Server set_uid start_menu
CTARGETS=unc_read start_menu

#SHELLSCRIPTS=Disp_Notice Disp_Query FINAL KillProc Multi_Controller Result_Assign Results_End_Disp RunReport SERVERCONFIG X11PERFFUNCS XBENCHFUNCS XTESTFUNCS addrem arev_interface awk_symboltable common.profile configure disp.nawk felibs get_ctlr getres gui_notice.wksh gui_notice1.wksh gui_query.wksh gui_query1.wksh idfile inplace.nawk inplace_disp.nawk kill_X mawk.sh menu_handler plist procs.old.awk rpt rpt.new run_cmd status_window tcc vres su_kill get_arev_server
SHELLSCRIPTS=KillProc RunReport \
	libfuncs Configure \
	mainmenu rpt Run \
	vres su_kill kill_test \
	gui_notice.wksh gui_query.wksh \
	inplace.nawk procs.old.awk 

all build: ${CTARGETS} 

display_results: display_results.o
	$(CC) ${CFLAGS} $@.o ${LIBS} -o $@
	strip $@

sort_param_list: sort_param_list.o
	$(CC) ${CFLAGS} $@.o ${LIBS} -o $@
	strip $@

clean_locks: clean_locks.o
	$(CC) ${CFLAGS} $@.o ${LIBS} -o $@
	strip $@

gethost	   :
	$(CC) -o gethost gethost.c -lnsl -lsocket
	strip $@

get_params :
	$(CC) -o get_params get_params.c -lnsl -lsocket
	strip $@

unc_read :
	$(CC) -o unc_read unc_read.c
	strip $@
	cp unc_read ..

det_res :
	$(CC) -o det_res det_res.c
	strip $@

get_arev_server :
	$(CC) -o get_arev_server get_arev_server.c
	strip $@

encode : encode.c
	$(CC) -o encode encode.c ${SUITE_ROOT}/etet/src/posix_c/tcc/compress_write.o ../../lib/libuwcts_api.a
	strip $@

Start_Server : Start_Server.c
	$(CC) -o Start_Server Start_Server.c -lnsl -lsocket
	strip $@

start_menu : start_menu.c
	$(CC) -o start_menu start_menu.c 
	strip $@
	cp start_menu ..

clean :
	rm -f ${CTARGETS}  *.o

