/* @(#)src/common/cmd/dmapi/libdmi/dmapi.h	3.3 07/22/97 15:39:39 -  */
/* #ident	"@(#)vxfs:src/common/cmd/dmapi/libdmi/dmapi.h	3.3" */
#ident	"@(#)vxfs.cmds:common/cmd/fs.d/dmapi/libdmi/dmapi.h	1.1"
/*
 * Copyright (c) 1996 VERITAS Software Corporation.  ALL RIGHTS RESERVED.
 * UNPUBLISHED -- RIGHTS RESERVED UNDER THE COPYRIGHT
 * LAWS OF THE UNITED STATES.  USE OF A COPYRIGHT NOTICE
 * IS PRECAUTIONARY ONLY AND DOES NOT IMPLY PUBLICATION
 * OR DISCLOSURE.
 *
 * THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
 * TRADE SECRETS OF VERITAS SOFTWARE.  USE, DISCLOSURE,
 * OR REPRODUCTION IS PROHIBITED WITHOUT THE PRIOR
 * EXPRESS WRITTEN PERMISSION OF VERITAS SOFTWARE.
 *
 *	       RESTRICTED RIGHTS LEGEND
 * USE, DUPLICATION, OR DISCLOSURE BY THE GOVERNMENT IS
 * SUBJECT TO RESTRICTIONS AS SET FORTH IN SUBPARAGRAPH
 * (C) (1) (ii) OF THE RIGHTS IN TECHNICAL DATA AND
 * COMPUTER SOFTWARE CLAUSE AT DFARS 252.227-7013.
 *	       VERITAS SOFTWARE
 * 1600 PLYMOUTH STREET, MOUNTAIN VIEW, CA 94043
 */

#ifndef _DMAPI_H
#define _DMAPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/fs/dmapi.h>

#ifdef	__STDC__

extern int	dm_clear_inherit(dm_sessid_t, void *, size_t,
				 dm_token_t, dm_attrname_t *);
extern int	dm_create_by_handle(dm_sessid_t, void *, size_t, dm_token_t,
				    void *, size_t, char *);
extern int	dm_create_session(dm_sessid_t, char *, dm_sessid_t *);
extern int	dm_create_userevent(dm_sessid_t, size_t, void *, dm_token_t *);
extern int	dm_destroy_session(dm_sessid_t);
extern int	dm_event_query(dm_sessid_t, dm_token_t, dm_eventquery_t,
			       dm_size_t *);
extern int	dm_find_eventmsg(dm_sessid_t, dm_token_t, size_t,
				 void *, size_t *);
extern int	dm_get_allocinfo(dm_sessid_t, void *, size_t, dm_token_t,
				 dm_off_t *, u_int, dm_extent_t *, u_int *);
extern int	dm_get_bulkall(dm_sessid_t, void *, size_t, dm_token_t,
			       u_int, dm_attrname_t *, dm_attrloc_t *, size_t,
			       void *, size_t *);
extern int	dm_get_bulkattr(dm_sessid_t, void *, size_t, dm_token_t,
				u_int, dm_attrloc_t *, size_t,
				void *, size_t *);
extern int	dm_get_dirattrs(dm_sessid_t, void *, size_t, dm_token_t,
				u_int, dm_attrloc_t *, size_t, void *,
				size_t *);
extern int	dm_init_attrloc(dm_sessid_t, void *, size_t, dm_token_t,
				dm_attrloc_t *);
extern int	dm_get_config(void *, size_t, dm_config_t, dm_size_t *);
extern int	dm_get_config_events(void *, size_t, u_int, dm_eventset_t *,
				     u_int *);
extern int	dm_get_dmattr(dm_sessid_t, void *, size_t, dm_token_t,
			      dm_attrname_t *, size_t, void *, size_t *);
extern int	dm_get_eventlist(dm_sessid_t, void *, size_t, dm_token_t,
				 u_int, dm_eventset_t *, u_int	*);
extern int	dm_get_events(dm_sessid_t, u_int, u_int, size_t, void *,
			      size_t *);
extern int	dm_get_fileattr(dm_sessid_t, void *, size_t, dm_token_t,
				u_int, dm_stat_t *);
extern int	dm_get_mountinfo(dm_sessid_t, void *, size_t, dm_token_t,
				 size_t, void *, size_t *);
extern int	dm_get_region(dm_sessid_t, void *, size_t, dm_token_t,
			      u_int, dm_region_t *, u_int *);
extern int	dm_getall_disp(dm_sessid_t, size_t, void *, size_t *);
extern int	dm_getall_dmattr(dm_sessid_t, void *, size_t, dm_token_t,
				 size_t, void *, size_t *);
extern int	dm_getall_inherit(dm_sessid_t, void *, size_t, dm_token_t,
				  u_int, dm_inherit_t *, u_int *);
extern int	dm_getall_sessions(u_int, dm_sessid_t *, u_int *);
extern int	dm_getall_tokens(dm_sessid_t, u_int, dm_token_t *, u_int *);
extern int	dm_handle_cmp(void *, size_t, void *, size_t);
extern dm_boolean_t dm_handle_is_valid(void *, size_t);
extern u_int	dm_handle_hash(void *, size_t);
extern int	dm_handle_to_fshandle(void *, size_t, void **, size_t *);
extern int	dm_handle_to_path(void *, size_t, void *, size_t,
				  size_t, char *, size_t *);
extern int	dm_init_service(char **);
extern int	dm_mkdir_by_handle(dm_sessid_t, void *, size_t, dm_token_t,
				   void *, size_t, char *);
extern int	dm_move_event(dm_sessid_t, dm_token_t, dm_sessid_t,
			      dm_token_t *);
extern int	dm_fd_to_handle(int, void **, size_t *);
extern int	dm_path_to_fshandle(char *, void **, size_t *);
extern int	dm_path_to_handle(char *, void **, size_t *);
extern void	dm_handle_free(void *, size_t);
extern int	dm_punch_hole(dm_sessid_t, void *, size_t, dm_token_t,
			      dm_off_t, dm_size_t);
extern int	dm_probe_hole(dm_sessid_t, void *, size_t, dm_token_t,
			      dm_off_t, dm_size_t, dm_off_t *, dm_size_t *);
extern int	dm_query_right(dm_sessid_t, void *, size_t, dm_token_t,
			       dm_right_t *);
extern int	dm_query_session(dm_sessid_t, size_t, void *, size_t *);
extern dm_ssize_t dm_read_invis(dm_sessid_t, void *, size_t, dm_token_t,
				dm_off_t, dm_size_t, void *);
extern dm_ssize_t dm_write_invis(dm_sessid_t, void *, size_t, dm_token_t,
				 int, dm_off_t, dm_size_t, void *);
extern int	dm_release_right(dm_sessid_t, void *, size_t, dm_token_t);
extern int	dm_remove_dmattr(dm_sessid_t, void *, size_t, dm_token_t,
				 int, dm_attrname_t *);
extern int	dm_request_right(dm_sessid_t, void *, size_t, dm_token_t,
				 u_int, dm_right_t);
extern int	dm_respond_event(dm_sessid_t, dm_token_t, dm_response_t,
				 int, size_t, void *);
extern int	dm_send_msg(dm_sessid_t, dm_msgtype_t, size_t, void *);
extern int	dm_set_disp(dm_sessid_t, void *, size_t, dm_token_t,
			    dm_eventset_t *, u_int);
extern int	dm_set_dmattr(dm_sessid_t, void *, size_t, dm_token_t,
			      dm_attrname_t *, int, size_t, void *);
extern int	dm_set_eventlist(dm_sessid_t, void *, size_t, dm_token_t,
				 dm_eventset_t *, u_int);
extern int	dm_set_fileattr(dm_sessid_t, void *, size_t, dm_token_t,
				u_int, dm_fileattr_t *);
extern int	dm_set_inherit(dm_sessid_t, void *, size_t, dm_token_t,
			       dm_attrname_t *, u_int);
extern int	dm_set_region(dm_sessid_t, void *, size_t, dm_token_t,
			      u_int, dm_region_t *, dm_boolean_t *);
extern int	dm_set_return_on_destroy(dm_sessid_t, void *, size_t,
					 dm_token_t,  dm_attrname_t *,
					 dm_boolean_t);
extern int	dm_symlink_by_handle(dm_sessid_t, void *, size_t, dm_token_t,
				     void *, size_t, char *, char *);
extern int	dm_sync_by_handle(dm_sessid_t, void *, size_t, dm_token_t);
extern int	dm_handle_to_fsid(void *, size_t, dm_fsid_t *);
extern int	dm_handle_to_igen(void *, size_t, dm_igen_t *);
extern int	dm_handle_to_ino(void *, size_t, dm_ino_t *);
extern int	dm_make_handle(dm_fsid_t, dm_ino_t, dm_igen_t, void **,
			       size_t *);
extern int	dm_make_fshandle(dm_fsid_t, void **, size_t *);

/*
 * Stubs for non-supported optional interfaces
 */

extern int	dm_upgrade_right(dm_sessid_t, void *, size_t, dm_token_t);
extern int	dm_downgrade_right(dm_sessid_t, void *, size_t, dm_token_t);
extern int	dm_pending(dm_sessid_t, dm_token_t, timestruc_t *);
extern int	dm_obj_ref_hold(dm_sessid_t, dm_token_t, void *, size_t);
extern int	dm_obj_ref_rele(dm_sessid_t, dm_token_t, void *, size_t);
extern int	dm_obj_ref_query(dm_sessid_t, dm_token_t, void *, size_t);

#else	/* __STDC__ */

extern int	dm_clear_inherit();
extern int	dm_create_by_handle();
extern int	dm_create_session();
extern int	dm_create_userevent();
extern int	dm_destroy_session();
extern int	dm_event_query();
extern int	dm_find_eventmsg();
extern int	dm_get_allocinfo();
extern int	dm_get_bulkall();
extern int	dm_get_bulkattr();
extern int	dm_get_dirattrs();
extern int	dm_init_attrloc();
extern int	dm_get_config();
extern int	dm_get_config_events();
extern int	dm_get_dmattr();
extern int	dm_get_eventlist();
extern int	dm_get_events();
extern int	dm_get_fileattr();
extern int	dm_get_mountinfo();
extern int	dm_get_region();
extern int	dm_getall_disp();
extern int	dm_getall_dmattr();
extern int	dm_getall_inherit();
extern int	dm_getall_sessions();
extern int	dm_getall_tokens();
extern int	dm_handle_cmp();
extern dm_boolean_t dm_handle_is_valid();
extern u_int	dm_handle_hash();
extern int	dm_handle_to_fshandle();
extern int	dm_handle_to_path();
extern int	dm_init_service();
extern int	dm_mkdir_by_handle();
extern int	dm_move_event();
extern int	dm_fd_to_handle();
extern int	dm_path_to_fshandle();
extern int	dm_path_to_handle();
extern void	dm_handle_free();
extern int	dm_punch_hole();
extern int	dm_probe_hole();
extern int	dm_query_right();
extern int	dm_query_session();
extern dm_ssize_t dm_read_invis();
extern dm_ssize_t dm_write_invis();
extern int	dm_release_right();
extern int	dm_remove_dmattr();
extern int	dm_request_right();
extern int	dm_respond_event();
extern int	dm_send_msg();
extern int	dm_set_disp();
extern int	dm_set_dmattr();
extern int	dm_set_eventlist();
extern int	dm_set_fileattr();
extern int	dm_set_inherit();
extern int	dm_set_region();
extern int	dm_set_return_on_destroy();
extern int	dm_symlink_by_handle();
extern int	dm_sync_by_handle();
extern int	dm_handle_to_fsid();
extern int	dm_handle_to_igen();
extern int	dm_handle_to_ino();
extern int	dm_make_handle();
extern int	dm_make_fshandle();

/*
 * Stubs for non-supported optional interfaces
 */

extern int	dm_upgrade_right();
extern int	dm_downgrade_right();
extern int	dm_pending();
extern int	dm_obj_ref_hold();
extern int	dm_obj_ref_rele();
extern int	dm_obj_ref_query();

#endif	/* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif	/* _DMAPI_H */
