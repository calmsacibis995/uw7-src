/*
 * @(#) @(#)iforpmapi.h	1.3 96/09/30
 *
 * Client side definitions
 *
 *============================================================================
 *                           iforpmapi.h
 *============================================================================
 */

#ifndef  ifor_pm_api_h
#define  ifor_pm_api_h

#include <ifortypes.h>

/* Mask for available bits for use as product id. */

#define IFOR_PM_PRODMASK   0x00FFFFFF /* a 1 indicates valid position to use. */

/* Flags available to pass to the IFOR/PMAPI request call. */

#define IFOR_PM_ASYNC      0x00000000  /* one of the value of 'flags'    */
#define IFOR_PM_SYNC       0x00000001  /* one of the value of 'flags'    */
#define IFOR_PM_NODELOCK   0x00000002  /* one of the value of 'flags'    */
#define IFOR_PM_SW         0x00000004  /* one of the value of 'flags'    */

/* Signal number indicating no signal is desired */

#define IFOR_PM_NO_SIGNAL  -1

/* Status codes returned from the IFOR/PMAPI calls. */

#define IFOR_PM_OK         0x00000000  /* Operation was successful. */
#define IFOR_PM_FATAL      0x00000001  /* A fatal error occured. */
#define IFOR_PM_REINIT     0x00000002  /* ifor_pm_init already called. */
#define IFOR_PM_BAD_PARAM  0x00000003  /* iFOR/pm function has bad parameter. */
#define IFOR_PM_NO_INIT    0x00000004  /* iforpm_init not yet called. */

/* Flags returned in the field of the policy handler. */

#define IFOR_PM_VND_MSG    0x00000001  /* The argument vmsg contains a vendor defined string */
#define IFOR_PM_MODE_0     0x00000002  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_1     0x00000004  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_2     0x00000008  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_3     0x00000010  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_4     0x00000020  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_5     0x00000040  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_6     0x00000080  /* Reserved for vendor defined mode.  */
#define IFOR_PM_MODE_7     0x00000100  /* Reserved for vendor defined mode.  */
#define IFOR_PM_WARN       0x00000200  /* Display the message from msg. */
#define IFOR_PM_CONTINUE   0x00000400  /* Application should continue. */
#define IFOR_PM_PAUSE_0    0x00000800  /* Reserved  */
#define IFOR_PM_PAUSE_1    0x00001000  /* Reserved  */
#define IFOR_PM_PAUSE_2    0x00002000  /* Reserved  */
#define IFOR_PM_PAUSE_3    0x00004000  /* Reserved  */
#define IFOR_PM_PAUSE_4    0x00008000  /* Reserved  */
#define IFOR_PM_PAUSE_5    0x00010000  /* Reserved  */
#define IFOR_PM_PAUSE_6    0x00020000  /* Reserved  */
#define IFOR_PM_PAUSE_7    0x00040000  /* Reserved  */
#define IFOR_PM_EXIT       0x00080000  /* Exit      */

#define SCO_REALID(vendor,bit) ((unsigned long)((((vendor)+1) << 16)|(bit)))

/* IFOR/PMAPI prototypes. */

extern char *pm_api_errmsg[];

#ifdef __cplusplus
extern "C" {
#endif

struct sco_license 
{
  int vendor;
  int id;
  char *version;
};

void ifor_pm_test_init(char *, int);
long ifor_pm_init_sco(ifor_pm_policy_handler_t, short);
long ifor_pm_init(ifor_pm_policy_handler_t, short, ifor_pm_vnd_t *);
long ifor_pm_request_sco(unsigned long, char *, unsigned long, ifor_pm_trns_hndl_t *);
long ifor_pm_request(unsigned long, char *, unsigned long, ifor_pm_trns_hndl_t *, ifor_pm_vnd_t *);
long ifor_pm_update(unsigned long, ifor_pm_trns_hndl_t);
long ifor_pm_release(unsigned long, ifor_pm_trns_hndl_t);

int check_any_license(struct sco_license *, int nlicenses, char *progname, char **msg_ptr);
int get_user_count(int lic_id, char *version);
int get_flag_count(int lic_id, char *version, unsigned char flag);
#ifdef __cplusplus
};
#endif

#endif
