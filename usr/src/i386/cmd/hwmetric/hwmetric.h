/* copyright "%c%" */
#ident	"@(#)hwmetric:hwmetric.h	1.4"
#ident	"$Header$"


#ifndef HWMETRIC_H
#define HWMETRIC_H

#ifndef TRUE
# define FALSE 0
# define TRUE  1
#endif /* !TRUE */
typedef int flag_type;

#define streq(a,b) (strcmp(a,b)==0)

extern int Num_cpus;
extern int Num_cgs;

#define METER_TYPE_NAME_LENGTH 6

typedef struct parse_meterlist {
  const char *metername;
  const char *next_metername;
  char meter_type_name[METER_TYPE_NAME_LENGTH + 1];

  int point_type;
  int point;
  int meter_type;
  int instance;

  int all_point_type;
  int all_point;
  int all_meter_type;
  int all_instance;
} pml_type;

#define POINT_TYPE_CPU 0
#define POINT_TYPE_CG 1

typedef union {
  cpumtr_meter_setup_t cpu;
  cgmtr_meter_setup_t cg;
} meter_setup_type;

/* do.c - major options */
void do_info (const char *);
void do_activate (flag_type, const char *, const char *);
void do_deactivate (const char *);
void do_deactivate_metric (const char *, const char *);
void do_cpu_trace (const char *, const char *, const char *);
void do_cg_trace (const char *, const char *, const char *);

/* hooks.c - kernel hooks */
void swtch_hooks (flag_type);
void syscall_hooks (flag_type);

/* pfmt.c - display diagnostic messages */
void pfmtact (const char *, ...);
void pfmtinf (const char *, ...);
void pfmterr (const char *, ...);

/* pml.c - parse meterlists */
const char *name_point (int);
const char *name_meter (int, int);
int pml_read (pml_type *, int *, int *, int *, int *);
void pml_init (pml_type *, const char *);
flag_type pml_at_end (const pml_type *);
void pml_point_types (const char *, flag_type *, flag_type *);

/* rc.c - info from initialization file */
int rc_metric_point_type (int);
const char *rc_meterlist (int);
const char *rc_metric_name (int);
unsigned long rc_metric_interval (int);
const meter_setup_type *rc_metric_setup (int);
int rc_find_metric (const char *);
void rc_read (void);

/* cpu.c - CPU-specific functions */
extern flag_type *Cpu_reachable;
extern cpumtr_cputype_t Cpu_type;
const char * cpu_name_meter (int);
int cpu_last_instance (int);
int cpu_first_meter_type (void);
int cpu_last_meter_type (void);
int cpu_init (void);
void cpu_parse_setups (const char *, int, cpumtr_meter_setup_t *);

/* cpumtr.c - driver interfaces */
int cpumtr_open (void);
cpumtr_rdstats_t *cpumtr_rdstats (void);
int cpumtr_reserve (int, int, int);
void cpumtr_setup_init (cpumtr_setup_t *);
void cpumtr_setup_add (cpumtr_setup_t *, int, int, int, int);
void cpumtr_setup (cpumtr_setup_t *);
void cpumtr_trace (unsigned long, unsigned long, cpumtr_rdsamples_t *);

/* cg.c - CG-specific functions */
extern cgmtr_cgtype_t Cg_type;
const char * cg_name_meter (int);
int cg_meter_index (int, int);
int cg_last_instance (int);
int cg_first_meter_type (void);
int cg_last_meter_type (void);
int cg_init (void);
void cg_parse_setups (const char *, int, cgmtr_meter_setup_t *);

/* cgmtr.c - driver interfaces */
int cgmtr_open (void);
cgmtr_rdstats_t *cgmtr_rdstats (void);
int cgmtr_reserve (int, int, int);
void cgmtr_setup_init (cgmtr_setup_t *);
void cgmtr_setup_add (cgmtr_setup_t *, int, int, int, int);
void cgmtr_setup (cgmtr_setup_t *);
void cgmtr_trace (unsigned long, unsigned long, cgmtr_rdsamples_t *);

#endif /* !HWMETRIC_H */
