/*
 * File hw_util.h
 * Misc utilities
 *
 * @(#) hw_util.h 65.1 97/07/11 
 * Copyright (C) The Santa Cruz Operation, 1993-1997
 * This Module contains Proprietary Information of
 * The Santa Cruz Operation, and should be treated as Confidential.
 */

#ifndef _hw_util_h
#define _hw_util_h

#define MsrLowVal(vp)	(vp)->msr_val[0]
#define MsrHiVal(vp)	(vp)->msr_val[1]

extern FILE		*errorFd;

extern const char	Yes[];
extern const char	No[];

extern const char	Enabled[];
extern const char	Disabled[];

extern int		verbose;
extern int		debug;

extern const char	*KernelName;

extern const char	dflt_lib_dir[];
extern const char	lib_dir_env[];
extern const char	*lib_dir;

void		debug_print(const char *fmt, ...);
void		error_print(const char *fmt, ...);
void		debug_dump(const void *buf, u_long len,
					u_long rel_adr, const char *fmt, ...);

u_char		io_inb(u_long addr);
u_short		io_inw(u_long addr);
u_long		io_ind(u_long addr);

void		io_outb(u_long addr, u_char value);
void		io_outw(u_long addr, u_short value);
void		io_outd(u_long addr, u_long value);

int		open_cdev(int maj_dev, int min_dev, mode_t mode);

#ifdef _SYS_MSR_H
    int		read_msr(int cpu, u_long addr, msr_t *value);
#endif

int		read_mem(u_long paddr, void *buf, size_t len);
int		read_kmem(u_long kaddr, void *buf, size_t len);

const char	*GetKernelName(void);

int		get_kaddr(const char *name, u_long *kaddr);
const char	*get_ksym(void *kaddr);

int		read_kvar(const char *name, void *value, size_t len);
int		read_kvar_b(const char *name, u_char *value);
int		read_kvar_w(const char *name, u_short *value);
int		read_kvar_d(const char *name, u_long *value);

const char	*read_k_string(u_long kaddr);

int		is_mp_kernel(void);
int		get_num_cpu(void);
int		read_cpu_kvar(int cpu, const char *name, void *value,
								size_t len);
int		read_cpu_kvar_b(int cpu, const char *name, u_char *value);
int		read_cpu_kvar_w(int cpu, const char *name, u_short *value);
int		read_cpu_kvar_d(int cpu, const char *name, u_long *value);

const char	*get_lib_file_name(const char *file);
void		report_when(FILE *out, const char *what);
void		*Fmalloc(size_t len);
const char	*spaces(size_t len);

#endif	/* _hw_util_h */

