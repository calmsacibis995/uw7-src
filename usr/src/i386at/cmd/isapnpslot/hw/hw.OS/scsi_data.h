/*
 * File scsi_data.h
 *
 * @(#) scsi_data.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _scsi_data_h
#define _scsi_data_h

typedef struct scsi_asc_s scsi_asc_t;
struct scsi_asc_s
{
    u_char	asc;
    u_char	ascq;
    u_long	flags;
    const char	*descr;
    scsi_asc_t	*next;
};

typedef enum
{
    scsi_key_none,
    scsi_key_mandatory,
    scsi_key_optional,
    scsi_key_vendor_spec,
    scsi_key_reserved,
    scsi_key_obsolete
} scsi_dev_key_t;

typedef struct scsi_op_s scsi_op_t;
struct scsi_op_s
{
    u_char		op;
    scsi_dev_key_t	*flags;
    const char		*descr;
    scsi_op_t		*next;
};

typedef struct scsi_vend_s scsi_vend_t;
struct scsi_vend_s
{
    const char	*id;
    const char	*name;
    scsi_vend_t	*next;
};

extern scsi_asc_t	*scsi_asc;
extern scsi_op_t	*scsi_op;
extern scsi_vend_t	*scsi_vendors;

int load_scsi_data(void);

#endif	/* _scsi_data_h */

