#ifndef ifor_types_h
#define ifor_types_h

#ifndef IFOR_VENDOR_NAME_LEN
#define IFOR_VENDOR_NAME_LEN      32  /* these are copied from iforp.h */
#define IFOR_VID_STR_LEN          37  /* it defines too much other stuff */
#endif /* IFOR_VENDOR_NAME_LEN */

typedef	struct ifor_pm_vnd_s {
	char		vendor_name[IFOR_VENDOR_NAME_LEN];
	char		vendor_id[IFOR_VID_STR_LEN];
	unsigned int	vendor_key_cksum;
} ifor_pm_vnd_t;

typedef	ifor_pm_vnd_t *ifor_pm_vnd_hndl_t;
typedef	void *ifor_pm_trns_hndl_t;
typedef	void (*ifor_pm_policy_handler_t)(char *, unsigned int, ifor_pm_trns_hndl_t, char *);

#endif
