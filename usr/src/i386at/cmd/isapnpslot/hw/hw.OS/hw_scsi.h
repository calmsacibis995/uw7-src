/*
 * File hw_scsi.h
 * Information handler for scsi
 *
 * @(#) hw_scsi.h 58.1 96/10/16 
 * Copyright (C) The Santa Cruz Operation, 1993-1996
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_scsi_h
#define _hw_scsi_h

extern const char	* const callme_scsi[];
extern const char	short_help_scsi[];

int	have_scsi(void);
void	report_scsi(FILE *out);

#endif	/* _hw_scsi_h */

