include $(CMDRULES)
TET_EXECUTE = ../../ts_exec
INSTALL_DIR = $(TET_EXECUTE)/ts/chmod

$(INSTALL_DIR)/chmod-tc: chmod-tc.sh clean
	cp chmod-tc.sh $@
	chmod 755 $@

clean:
	rm -f $(INSTALL_DIR)/chmod-tc
