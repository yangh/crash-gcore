/* gcore_defs.h -- core analysis suite
 *
 * Copyright (C) 2010 FUJITSU LIMITED
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef GCORE_DEFS_H_
#define GCORE_DEFS_H_

#define PN_XNUM 0xffff

#define ELF_CORE_EFLAGS 0

#ifdef X86_64
#define ELF_EXEC_PAGESIZE 4096

#define ELF_MACHINE EM_X86_64
#define ELF_OSABI ELFOSABI_NONE

#define ELF_CLASS ELFCLASS64
#define ELF_DATA ELFDATA2LSB
#define ELF_ARCH EM_X86_64

#define Elf_Half Elf64_Half
#define Elf_Word Elf64_Word
#define Elf_Off Elf64_Off

#define Elf_Ehdr Elf64_Ehdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Nhdr Elf64_Nhdr
#elif X86
#define ELF_EXEC_PAGESIZE 4096

#define ELF_MACHINE EM_386
#define ELF_OSABI ELFOSABI_NONE

#define ELF_CLASS ELFCLASS32
#define ELF_DATA ELFDATA2LSB
#define ELF_ARCH EM_386

#define Elf_Half Elf32_Half
#define Elf_Word Elf32_Word
#define Elf_Off Elf32_Off

#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Nhdr Elf32_Nhdr
#endif

/*
 * gcore_regset.c
 *
 * The regset interface is fully borrowed from the library with the
 * same name in kernel used in the implementation of collecting note
 * information. See include/regset.h in detail.
 */
struct user_regset;
struct task_context;
struct elf_thread_core_info;

/**
 * user_regset_active_fn - type of @active function in &struct user_regset
 * @target:	thread being examined
 * @regset:	task context being examined
 *
 * Return TRUE if there is an interesting resource.
 * Return FALSE otherwise.
 */
typedef int user_regset_active_fn(struct task_context *target,
				  const struct user_regset *regset);

/**
 * user_regset_get_fn - type of @get function in &struct user_regset
 * @target:	task context being examined
 * @regset:	regset being examined
 * @size:	amount of data to copy, in bytes
 * @buf:	if a user-space pointer to copy into
 *
 * Fetch register values. Return TRUE on success and FALSE otherwise.
 * The @size is in bytes.
 */
typedef int user_regset_get_fn(struct task_context *target,
			       const struct user_regset *regset,
			       unsigned int size,
			       void *buf);

/**
 * user_regset_writeback_fn - type of @writeback function in &struct user_regset
 * @target:	thread being examined
 * @regset:	regset being examined
 * @immediate:	zero if writeback at completion of next context switch is OK
 *
 * This call is optional; usually the pointer is %NULL.
 *
 * Return TRUE on success or FALSE otherwise.
 */
typedef int user_regset_writeback_fn(struct task_context *target,
				     const struct user_regset *regset,
				     int immediate);

/**
 * user_regset_callback_fn - type of @callback function in &struct user_regset
 * @t:          thread core information being gathered
 * @regset:	regset being examined
 *
 * Edit another piece of information contained in @t in terms of @regset.
 * This call is optional; the pointer is %NULL if there is no requirement to
 * edit.
 */
typedef void user_regset_callback_fn(struct elf_thread_core_info *t,
				     const struct user_regset *regset);

/**
 * struct user_regset - accessible thread CPU state
 * @size:		Size in bytes of a slot (register).
 * @core_note_type:	ELF note @n_type value used in core dumps.
 * @get:		Function to fetch values.
 * @active:		Function to report if regset is active, or %NULL.
 *
 * @name:               Note section name.
 * @callback:           Function to edit thread core information, or %NULL.
 *
 * This data structure describes machine resource to be retrieved as
 * process core dump. Each member of this structure characterizes the
 * resource and the operations necessary in core dump process.
 *
 * @get provides a means of retrieving the corresponding resource;
 * @active provides a means of checking if the resource exists;
 * @writeback performs some architecture-specific operation to make it
 * reflect the current actual state; @size means a size of the machine
 * resource in bytes; @core_note_type is a type of note information;
 * @name is a note section name representing the owner originator that
 * handles this kind of the machine resource; @callback is an extra
 * operation to edit another note information of the same thread,
 * required when the machine resource is collected.
 */
struct user_regset {
	user_regset_get_fn		*get;
	user_regset_active_fn		*active;
	user_regset_writeback_fn	*writeback;
	unsigned int 			size;
	unsigned int 			core_note_type;
	char                            *name;
	user_regset_callback_fn         *callback;
};

/**
 * struct user_regset_view - available regsets
 * @name:	Identifier, e.g. UTS_MACHINE string.
 * @regsets:	Array of @n regsets available in this view.
 * @n:		Number of elements in @regsets.
 * @e_machine:	ELF header @e_machine %EM_* value written in core dumps.
 * @e_flags:	ELF header @e_flags value written in core dumps.
 * @ei_osabi:	ELF header @e_ident[%EI_OSABI] value written in core dumps.
 *
 * A regset view is a collection of regsets (&struct user_regset,
 * above).  This describes all the state of a thread that are
 * collected as note information of process core dump.
 */
struct user_regset_view {
	const char *name;
	const struct user_regset *regsets;
	unsigned int n;
	uint32_t e_flags;
	uint16_t e_machine;
	uint8_t ei_osabi;
};

/**
 * task_user_regset_view - Return the process's regset view.
 *
 * Return the &struct user_regset_view. By default, it returns
 * &gcore_default_regset_view.
 *
 * This is defined as a weak symbol. If there's another
 * task_user_regset_view at linking time, it is used instead, useful
 * to support different kernel version or architecture.
 */
extern const struct user_regset_view *task_user_regset_view(void);
extern void gcore_default_regsets_init(void);

#if X86
#define REGSET_VIEW_NAME "i386"
#define REGSET_VIEW_MACHINE EM_386
#elif X86_64
#define REGSET_VIEW_NAME "x86_64"
#define REGSET_VIEW_MACHINE EM_X86_64
#elif IA64
#define REGSET_VIEW_NAME "ia64"
#define REGSET_VIEW_MACHINE EM_IA_64
#endif

/*
 * gcore_dumpfilter.c
 */
extern int gcore_dumpfilter_set(ulong filter);
extern void gcore_dumpfilter_set_default(void);
extern ulong gcore_dumpfilter_vma_dump_size(ulong vma);

/*
 * gcore_verbose.c
 */
#define VERBOSE_PROGRESS  0x1
#define VERBOSE_NONQUIET  0x2
#define VERBOSE_PAGEFAULT 0x4
#define VERBOSE_DEFAULT_LEVEL VERBOSE_PAGEFAULT
#define VERBOSE_MAX_LEVEL (VERBOSE_PROGRESS + VERBOSE_NONQUIET + \
			   VERBOSE_PAGEFAULT)

#define VERBOSE_DEFAULT_ERROR_HANDLE (FAULT_ON_ERROR | QUIET)

/*
 * Verbose flag is set each time gcore is executed. The same verbose
 * flag value is used for all the tasks given together in the command
 * line.
 */
extern void gcore_verbose_set_default(void);

/**
 * gcore_verbose_set() - set verbose level
 *
 * @level verbose level intended to be assigend: might be minus and
 *        larger than VERBOSE_DEFAULT_LEVEL.
 *
 * If @level is a minus value or strictly larger than VERBOSE_MAX_LEVEL,
 * return FALSE. Otherwise, update a global date, gvd, to @level, and returns
 * TRUE.
 */
extern int gcore_verbose_set(ulong level);

/**
 * gcore_verbose_get() - get verbose level
 *
 * Return the current verbose level contained in the global data.
 */
extern ulong gcore_verbose_get(void);

/**
 * gcore_verbose_error_handle() - get error handle
 *
 * Return the current error_handle contained in the global data.
 */
extern ulong gcore_verbose_error_handle(void);

/*
 * Helper printing functions for respective verbose flags
 */

/**
 * verbosef() - print verbose information if flag is set currently.
 *
 * @flag   verbose flag that is currently concerned about.
 * @format printf style format that is printed into standard output.
 *
 * Always returns FALSE.
 */
#define verbosef(vflag, eflag, ...)					\
	({								\
		if (gcore_verbose_get() & (vflag)) {			\
			(void) error((eflag), __VA_ARGS__);		\
		}							\
		FALSE;							\
	})

/**
 * progressf() - print progress verbose information
 *
 * @format printf style format that is printed into standard output.
 *
 * Print progress verbose informaiton if VERBOSE_PROGRESS is set currently.
 */
#define progressf(...) verbosef(VERBOSE_PROGRESS, INFO, __VA_ARGS__)

/**
 * pagefaultf() - print page fault verbose information
 *
 * @format printf style format that is printed into standard output.
 *
 * print pagefault verbose informaiton if VERBOSE_PAGEFAULT is set currently.
 */
#define pagefaultf(...) verbosef(VERBOSE_PAGEFAULT, WARNING, __VA_ARGS__)

/*
 * gcore_x86.c
 */
extern struct gcore_x86_table *gxt;

extern void gcore_x86_table_init(void);

#ifdef X86_64
struct user_regs_struct {
	unsigned long	r15;
	unsigned long	r14;
	unsigned long	r13;
	unsigned long	r12;
	unsigned long	bp;
	unsigned long	bx;
	unsigned long	r11;
	unsigned long	r10;
	unsigned long	r9;
	unsigned long	r8;
	unsigned long	ax;
	unsigned long	cx;
	unsigned long	dx;
	unsigned long	si;
	unsigned long	di;
	unsigned long	orig_ax;
	unsigned long	ip;
	unsigned long	cs;
	unsigned long	flags;
	unsigned long	sp;
	unsigned long	ss;
	unsigned long	fs_base;
	unsigned long	gs_base;
	unsigned long	ds;
	unsigned long	es;
	unsigned long	fs;
	unsigned long	gs;
};
#endif

#ifdef X86
struct user_regs_struct {
	unsigned long	bx;
	unsigned long	cx;
	unsigned long	dx;
	unsigned long	si;
	unsigned long	di;
	unsigned long	bp;
	unsigned long	ax;
	unsigned long	ds;
	unsigned long	es;
	unsigned long	fs;
	unsigned long	gs;
	unsigned long	orig_ax;
	unsigned long	ip;
	unsigned long	cs;
	unsigned long	flags;
	unsigned long	sp;
	unsigned long	ss;
};
#endif

typedef ulong elf_greg_t;
#define ELF_NGREG (sizeof(struct user_regs_struct) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];

#ifdef X86
#define PAGE_SIZE 4096
#endif

/*
 * gcore_coredump_table.c
 */
extern void gcore_coredump_table_init(void);

/*
 * gcore_coredump.c
 */
extern void gcore_coredump(void);

/*
 * gcore_global_data.c
 */
extern struct gcore_data *gcore;
extern struct gcore_coredump_table *ggt;
extern struct gcore_offset_table gcore_offset_table;
extern struct gcore_size_table gcore_size_table;

/*
 * Misc
 */
enum pid_type
{
        PIDTYPE_PID,
        PIDTYPE_PGID,
        PIDTYPE_SID,
        PIDTYPE_MAX
};

struct elf_siginfo
{
        int     si_signo;                       /* signal number */
	int     si_code;                        /* extra code */
        int     si_errno;                       /* errno */
};

/* Parameters used to convert the timespec values: */
#define NSEC_PER_USEC   1000L
#define NSEC_PER_SEC    1000000000L

/* The clock frequency of the i8253/i8254 PIT */
#define PIT_TICK_RATE 1193182ul

/* Assume we use the PIT time source for the clock tick */
#define CLOCK_TICK_RATE         PIT_TICK_RATE

/* LATCH is used in the interval timer and ftape setup. */
#define LATCH  ((CLOCK_TICK_RATE + HZ/2) / HZ)  /* For divider */

/* Suppose we want to devide two numbers NOM and DEN: NOM/DEN, then we can
 * improve accuracy by shifting LSH bits, hence calculating:
 *     (NOM << LSH) / DEN
 * This however means trouble for large NOM, because (NOM << LSH) may no
 * longer fit in 32 bits. The following way of calculating this gives us
 * some slack, under the following conditions:
 *   - (NOM / DEN) fits in (32 - LSH) bits.
 *   - (NOM % DEN) fits in (32 - LSH) bits.
 */
#define SH_DIV(NOM,DEN,LSH) (   (((NOM) / (DEN)) << (LSH))              \
				+ ((((NOM) % (DEN)) << (LSH)) + (DEN) / 2) / (DEN))

/* HZ is the requested value. ACTHZ is actual HZ ("<< 8" is for accuracy) */
#define ACTHZ (SH_DIV (CLOCK_TICK_RATE, LATCH, 8))

/* TICK_NSEC is the time between ticks in nsec assuming real ACTHZ */
#define TICK_NSEC (SH_DIV (1000000UL * 1000, ACTHZ, 8))

#define cputime_add(__a, __b)           ((__a) +  (__b))
#define cputime_sub(__a, __b)           ((__a) -  (__b))

typedef unsigned long cputime_t;

#define cputime_zero                    (0UL)

struct task_cputime {
        cputime_t utime;
        cputime_t stime;
        unsigned long long sum_exec_runtime;
};

#define INIT_CPUTIME						\
        (struct task_cputime) {                                 \
                .utime = cputime_zero,                          \
			.stime = cputime_zero,                          \
			.sum_exec_runtime = 0,                          \
			}

static inline uint64_t div_u64_rem(uint64_t dividend, uint32_t divisor,
				   uint32_t *remainder)
{
        *remainder = dividend % divisor;
        return dividend / divisor;
}

static inline void
jiffies_to_timeval(const unsigned long jiffies, struct timeval *value)
{
        /*
         * Convert jiffies to nanoseconds and separate with
         * one divide.
         */
        uint32_t rem;

        value->tv_sec = div_u64_rem((uint64_t)jiffies * TICK_NSEC,
                                    NSEC_PER_SEC, &rem);
        value->tv_usec = rem / NSEC_PER_USEC;
}

#define cputime_to_timeval(__ct,__val)  jiffies_to_timeval(__ct,__val)

struct elf_prstatus
{
	struct elf_siginfo pr_info;	/* Info associated with signal */
	short	pr_cursig;		/* Current signal */
	unsigned long pr_sigpend;	/* Set of pending signals */
	unsigned long pr_sighold;	/* Set of held signals */
	int	pr_pid;
	int	pr_ppid;
	int	pr_pgrp;
	int	pr_sid;
	struct timeval pr_utime;	/* User time */
	struct timeval pr_stime;	/* System time */
	struct timeval pr_cutime;	/* Cumulative user time */
	struct timeval pr_cstime;	/* Cumulative system time */
	elf_gregset_t pr_reg;	/* GP registers */
	int pr_fpvalid;		/* True if math co-processor being used.  */
};

typedef unsigned short __kernel_old_uid_t;
typedef unsigned short __kernel_old_gid_t;

typedef __kernel_old_uid_t      old_uid_t;
typedef __kernel_old_gid_t      old_gid_t;

#ifdef X86_64
typedef unsigned int __kernel_uid_t;
typedef unsigned int __kernel_gid_t;
#elif X86
typedef unsigned short __kernel_uid_t;
typedef unsigned short __kernel_gid_t;
#endif

#define overflowuid (symbol_exists("overflowuid"))
#define overflowgid (symbol_exists("overflowgid"))

#define high2lowuid(uid) ((uid) & ~0xFFFF ? (old_uid_t)overflowuid : (old_uid_t)(uid))
#define high2lowgid(gid) ((gid) & ~0xFFFF ? (old_gid_t)overflowgid : (old_gid_t)(gid))

#define __convert_uid(size, uid) \
        (size >= sizeof(uid) ? (uid) : high2lowuid(uid))
#define __convert_gid(size, gid) \
        (size >= sizeof(gid) ? (gid) : high2lowgid(gid))

#define SET_UID(var, uid) do { (var) = __convert_uid(sizeof(var), (uid)); } while (0)
#define SET_GID(var, gid) do { (var) = __convert_gid(sizeof(var), (gid)); } while (0)

#define MAX_USER_RT_PRIO        100
#define MAX_RT_PRIO             MAX_USER_RT_PRIO

#define PRIO_TO_NICE(prio)      ((prio) - MAX_RT_PRIO - 20)
#define TASK_NICE(p)            PRIO_TO_NICE((p)->static_prio)

static inline ulong ffz(ulong word)
{
        int num = 0;

#if defined(X86_64) || defined(IA64)
        if ((word & 0xffffffff) == 0) {
                num += 32;
                word >>= 32;
        }
#endif
        if ((word & 0xffff) == 0) {
                num += 16;
                word >>= 16;
        }
        if ((word & 0xff) == 0) {
                num += 8;
                word >>= 8;
        }
        if ((word & 0xf) == 0) {
                num += 4;
                word >>= 4;
        }
        if ((word & 0x3) == 0) {
                num += 2;
                word >>= 2;
        }
        if ((word & 0x1) == 0)
                num += 1;
        return num;
}

#define ELF_PRARGSZ     (80)    /* Number of chars for args */

struct elf_prpsinfo
{
        char    pr_state;       /* numeric process state */
        char    pr_sname;       /* char for pr_state */
        char    pr_zomb;        /* zombie */
        char    pr_nice;        /* nice val */
        unsigned long pr_flag;  /* flags */
        __kernel_uid_t  pr_uid;
        __kernel_gid_t  pr_gid;
        pid_t   pr_pid, pr_ppid, pr_pgrp, pr_sid;
        /* Lots missing */
        char    pr_fname[16];   /* filename of executable */
        char    pr_psargs[ELF_PRARGSZ]; /* initial part of arg list */
};

#define TASK_COMM_LEN 16

#define	CORENAME_MAX_SIZE 128

struct memelfnote
{
	const char *name;
	int type;
	unsigned int datasz;
	void *data;
};

struct thread_group_list {
	struct thread_group_list *next;
	ulong task;
};

struct elf_thread_core_info {
	struct elf_thread_core_info *next;
	ulong task;
	struct elf_prstatus prstatus;
	struct memelfnote notes[0];
};

struct elf_note_info {
	struct elf_thread_core_info *thread;
	struct memelfnote psinfo;
	struct memelfnote auxv;
	size_t size;
	int thread_notes;
};

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 */
#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008
#define VM_IO           0x00004000      /* Memory mapped I/O or similar */
#define VM_RESERVED     0x00080000      /* Count as reserved_vm like IO */
#define VM_HUGETLB      0x00400000      /* Huge TLB Page VM */
#define VM_ALWAYSDUMP   0x04000000      /* Always include in core dumps */

#define FOR_EACH_VMA_OBJECT(vma, index, mmap)		\
	for (index = 0, vma = mmap; vma; ++index, vma = next_vma(vma))

extern int _init(void);
extern int _fini(void);
extern char *help_gcore[];
extern void cmd_gcore(void);

struct gcore_coredump_table {

	unsigned int (*get_inode_i_nlink)(ulong file);

	pid_t (*task_pid)(ulong task);
	pid_t (*task_pgrp)(ulong task);
	pid_t (*task_session)(ulong task);

	void (*thread_group_cputime)(ulong task,
				     const struct thread_group_list *threads,
				     struct task_cputime *cputime);

	__kernel_uid_t (*task_uid)(ulong task);
	__kernel_gid_t (*task_gid)(ulong task);
};

struct gcore_offset_table
{
	long cpuinfo_x86_hard_math;
	long cpuinfo_x86_x86_capability;
	long cred_gid;
	long cred_uid;
	long desc_struct_base0;
	long desc_struct_base1;
	long desc_struct_base2;
	long fpu_state;
	long inode_i_nlink;
	long nsproxy_pid_ns;
	long mm_struct_arg_start;
	long mm_struct_arg_end;
	long mm_struct_map_count;
	long mm_struct_saved_auxv;
	long pid_level;
	long pid_namespace_level;
	long pt_regs_ax;
	long pt_regs_bp;
	long pt_regs_bx;
	long pt_regs_cs;
	long pt_regs_cx;
	long pt_regs_di;
	long pt_regs_ds;
	long pt_regs_dx;
	long pt_regs_es;
	long pt_regs_flags;
	long pt_regs_fs;
	long pt_regs_gs;
	long pt_regs_ip;
	long pt_regs_orig_ax;
	long pt_regs_si;
	long pt_regs_sp;
	long pt_regs_ss;
	long pt_regs_xfs;
	long pt_regs_xgs;
	long sched_entity_sum_exec_runtime;
	long signal_struct_cutime;
	long signal_struct_pgrp;
	long signal_struct_session;
	long signal_struct_stime;
	long signal_struct_sum_sched_runtime;
	long signal_struct_utime;
	long task_struct_cred;
	long task_struct_gid;
	long task_struct_group_leader;
	long task_struct_real_cred;
	long task_struct_real_parent;
	long task_struct_se;
	long task_struct_static_prio;
	long task_struct_uid;
	long task_struct_used_math;
	long thread_info_status;
	long thread_struct_ds;
	long thread_struct_es;
	long thread_struct_fs;
	long thread_struct_fsindex;
	long thread_struct_fpu;
	long thread_struct_gs;
	long thread_struct_gsindex;
	long thread_struct_i387;
	long thread_struct_tls_array;
	long thread_struct_usersp;
	long thread_struct_xstate;
	long thread_struct_io_bitmap_max;
	long thread_struct_io_bitmap_ptr;
	long user_regset_n;
	long vm_area_struct_anon_vma;
	long x8664_pda_oldrsp;
};

struct gcore_size_table
{
	long mm_struct_saved_auxv;
	long thread_struct_fs;
	long thread_struct_fsindex;
	long thread_struct_gs;
	long thread_struct_gsindex;
	long thread_struct_tls_array;
	long vm_area_struct_anon_vma;
	long thread_xstate;
	long i387_union;
};

#define GCORE_OFFSET(X) (OFFSET_verify(gcore_offset_table.X, (char *)__FUNCTION__, __FILE__, __LINE__, #X))
#define GCORE_SIZE(X) (SIZE_verify(gcore_size_table.X, (char *)__FUNCTION__, __FILE__, __LINE__, #X))
#define GCORE_VALID_MEMBER(X) (gcore_offset_table.X >= 0)
#define GCORE_ASSIGN_OFFSET(X) (gcore_offset_table.X)
#define GCORE_MEMBER_OFFSET_INIT(X, Y, Z) (GCORE_ASSIGN_OFFSET(X) = MEMBER_OFFSET(Y, Z))
#define GCORE_ASSIGN_SIZE(X) (gcore_size_table.X)
#define GCORE_SIZE_INIT(X, Y, Z) (GCORE_ASSIGN_SIZE(X) = MEMBER_SIZE(Y, Z))
#define GCORE_MEMBER_SIZE_INIT(X, Y, Z) (GCORE_ASSIGN_SIZE(X) = MEMBER_SIZE(Y, Z))
#define GCORE_STRUCT_SIZE_INIT(X, Y) (GCORE_ASSIGN_SIZE(X) = STRUCT_SIZE(Y))

extern struct gcore_offset_table gcore_offset_table;
extern struct gcore_size_table gcore_size_table;

/*
 * gcore flags
 */
#define GCF_SUCCESS     0x1
#define GCF_UNDER_COREDUMP 0x2

struct gcore_data
{
	ulong flags;
	int fd;
	struct task_context *orig;
	char corename[CORENAME_MAX_SIZE + 1];
};

static inline void gcore_arch_table_init(void)
{
#if defined (X86_64) || defined (X86)
	gcore_x86_table_init();
#endif
}

static inline void gcore_arch_regsets_init(void)
{
#if X86_64
	extern void gcore_x86_64_regsets_init(void);
	gcore_x86_64_regsets_init();
#elif X86
	extern void gcore_x86_32_regsets_init(void);
	gcore_x86_32_regsets_init();
#else
	extern void gcore_default_regsets_init(void);
	gcore_default_regsets_init();
#endif
}

#ifdef GCORE_TEST

static inline int gcore_proc_version_contains(const char *s)
{
	return strstr(kt->proc_version, s) ? TRUE : FALSE;
}

static inline int gcore_is_rhel4(void)
{
	return THIS_KERNEL_VERSION == LINUX(2,6,9)
		&& gcore_proc_version_contains(".EL");
}

static inline int gcore_is_rhel5(void)
{
	return THIS_KERNEL_VERSION == LINUX(2,6,18)
		&& gcore_proc_version_contains(".el5");
}

static inline int gcore_is_rhel6(void)
{
	return THIS_KERNEL_VERSION == LINUX(2,6,32)
		&& gcore_proc_version_contains(".el6");
}

extern char *help_gcore_test[];
extern void cmd_gcore_test(void);

#define mu_assert(message, test) do { if (!(test)) return message; } while (0)
#define mu_run_test(test) do { char *message = test(); tests_run++; \
		　　　　if (message) return message; } while (0)
extern int tests_run;

extern char *gcore_x86_test(void);
extern char *gcore_coredump_table_test(void);
extern char *gcore_dumpfilter_test(void);

#endif

#endif /* GCORE_DEFS_H_ */