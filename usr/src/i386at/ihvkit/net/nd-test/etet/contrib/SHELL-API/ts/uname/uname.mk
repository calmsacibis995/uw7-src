include $(CMDRULES)
TET_EXECUTE = ../../ts_exec
INSTALL_DIR = $(TET_EXECUTE)/ts/uname

$(INSTALL_DIR)/uname-tc: uname-tc.sh clean
	cp uname-tc.sh $@
	chmod 755 $@

clean:
	rm -f $(INSTALL_DIR)/uname-tc
