    [   0] = 7, /* syscall */
    [   1] = 1, /* exit */
    [   2] = 0, /* fork */
    [   3] = 3, /* read */
    [   4] = 3, /* write */
    [   5] = 3, /* open */
    [   6] = 1, /* close */
    [   7] = 3, /* waitpid */
    [   8] = 2, /* creat */
    [   9] = 2, /* link */
    [  10] = 1, /* unlink */
    [  11] = 3, /* execve */
    [  12] = 1, /* chdir */
    [  13] = 1, /* time */
    [  14] = 3, /* mknod */
    [  15] = 2, /* chmod */
    [  16] = 3, /* lchown */
    [  17] = 0, /* break */
    [  18] = 2, /* oldstat */
    [  19] = 3, /* lseek */
    [  20] = 0, /* getpid */
    [  21] = 5, /* mount */
    [  22] = 1, /* umount */
    [  23] = 1, /* setuid */
    [  24] = 0, /* getuid */
    [  25] = 1, /* stime */
    [  26] = 4, /* ptrace */
    [  27] = 1, /* alarm */
    [  28] = 2, /* oldfstat */
    [  29] = 0, /* pause */
    [  30] = 2, /* utime */
    [  31] = 0, /* stty */
    [  32] = 0, /* gtty */
    [  33] = 2, /* access */
    [  34] = 1, /* nice */
    [  35] = 1, /* ftime */
    [  36] = 0, /* sync */
    [  37] = 2, /* kill */
    [  38] = 2, /* rename */
    [  39] = 2, /* mkdir */
    [  40] = 1, /* rmdir */
    [  41] = 1, /* dup */
    [  42] = 0, /* pipe */
    [  43] = 1, /* times */
    [  44] = 0, /* prof */
    [  45] = 1, /* brk */
    [  46] = 1, /* setgid */
    [  47] = 0, /* getgid */
    [  48] = 2, /* signal */
    [  49] = 0, /* geteuid */
    [  50] = 0, /* getegid */
    [  51] = 1, /* acct */
    [  52] = 2, /* umount2 */
    [  53] = 0, /* lock */
    [  54] = 3, /* ioctl */
    [  55] = 3, /* fcntl */
    [  56] = 0, /* mpx */
    [  57] = 2, /* setpgid */
    [  58] = 0, /* ulimit */
    [  59] = 1, /* oldolduname */
    [  60] = 1, /* umask */
    [  61] = 1, /* chroot */
    [  62] = 2, /* ustat */
    [  63] = 2, /* dup2 */
    [  64] = 0, /* getppid */
    [  65] = 0, /* getpgrp */
    [  66] = 0, /* setsid */
    [  67] = 3, /* sigaction */
    [  68] = 0, /* sgetmask */
    [  69] = 1, /* ssetmask */
    [  70] = 2, /* setreuid */
    [  71] = 2, /* setregid */
    [  72] = 1, /* sigsuspend */
    [  73] = 1, /* sigpending */
    [  74] = 2, /* sethostname */
    [  75] = 2, /* setrlimit */
    [  76] = 2, /* getrlimit */
    [  77] = 2, /* getrusage */
    [  78] = 2, /* gettimeofday */
    [  79] = 2, /* settimeofday */
    [  80] = 2, /* getgroups */
    [  81] = 2, /* setgroups */
    [  82] = 0, /* reserved82 */
    [  83] = 2, /* symlink */
    [  84] = 2, /* oldlstat */
    [  85] = 3, /* readlink */
    [  86] = 1, /* uselib */
    [  87] = 2, /* swapon */
    [  88] = 4, /* reboot */
    [  89] = 3, /* readdir */
    [  90] = 6, /* mmap */
    [  91] = 2, /* munmap */
    [  92] = 2, /* truncate */
    [  93] = 2, /* ftruncate */
    [  94] = 2, /* fchmod */
    [  95] = 3, /* fchown */
    [  96] = 2, /* getpriority */
    [  97] = 3, /* setpriority */
    [  98] = 0, /* profil */
    [  99] = 2, /* statfs */
    [ 100] = 2, /* fstatfs */
    [ 101] = 3, /* ioperm */
    [ 102] = 2, /* socketcall */
    [ 103] = 3, /* syslog */
    [ 104] = 3, /* setitimer */
    [ 105] = 2, /* getitimer */
    [ 106] = 2, /* stat */
    [ 107] = 2, /* lstat */
    [ 108] = 2, /* fstat */
    [ 109] = 1, /* olduname */
    [ 110] = 1, /* iopl */
    [ 111] = 0, /* vhangup */
    [ 112] = 0, /* idle */
    [ 113] = 5, /* vm86 */
    [ 114] = 4, /* wait4 */
    [ 115] = 1, /* swapoff */
    [ 116] = 1, /* sysinfo */
    [ 117] = 6, /* ipc */
    [ 118] = 1, /* fsync */
    [ 119] = 0, /* sigreturn */
    [ 120] = 5, /* clone */
    [ 121] = 2, /* setdomainname */
    [ 122] = 1, /* uname */
    [ 123] = 0, /* modify_ldt */
    [ 124] = 1, /* adjtimex */
    [ 125] = 3, /* mprotect */
    [ 126] = 3, /* sigprocmask */
    [ 127] = 2, /* create_module */
    [ 128] = 3, /* init_module */
    [ 129] = 2, /* delete_module */
    [ 130] = 1, /* get_kernel_syms */
    [ 131] = 4, /* quotactl */
    [ 132] = 1, /* getpgid */
    [ 133] = 1, /* fchdir */
    [ 134] = 2, /* bdflush */
    [ 135] = 3, /* sysfs */
    [ 136] = 1, /* personality */
    [ 137] = 0, /* afs_syscall */
    [ 138] = 1, /* setfsuid */
    [ 139] = 1, /* setfsgid */
    [ 140] = 5, /* _llseek */
    [ 141] = 3, /* getdents */
    [ 142] = 5, /* _newselect */
    [ 143] = 2, /* flock */
    [ 144] = 3, /* msync */
    [ 145] = 3, /* readv */
    [ 146] = 3, /* writev */
    [ 147] = 3, /* cacheflush */
    [ 148] = 3, /* cachectl */
    [ 149] = 4, /* sysmips */
    [ 150] = 0, /* setup */
    [ 151] = 1, /* getsid */
    [ 152] = 1, /* fdatasync */
    [ 153] = 1, /* _sysctl */
    [ 154] = 2, /* mlock */
    [ 155] = 2, /* munlock */
    [ 156] = 1, /* mlockall */
    [ 157] = 0, /* munlockall */
    [ 158] = 2, /* sched_setparam */
    [ 159] = 2, /* sched_getparam */
    [ 160] = 3, /* sched_setscheduler */
    [ 161] = 1, /* sched_getscheduler */
    [ 162] = 0, /* sched_yield */
    [ 163] = 1, /* sched_get_priority_max */
    [ 164] = 1, /* sched_get_priority_min */
    [ 165] = 2, /* sched_rr_get_interval */
    [ 166] = 2, /* nanosleep */
    [ 167] = 5, /* mremap */
    [ 168] = 3, /* accept */
    [ 169] = 3, /* bind */
    [ 170] = 3, /* connect */
    [ 171] = 3, /* getpeername */
    [ 172] = 3, /* getsockname */
    [ 173] = 5, /* getsockopt */
    [ 174] = 2, /* listen */
    [ 175] = 4, /* recv */
    [ 176] = 6, /* recvfrom */
    [ 177] = 3, /* recvmsg */
    [ 178] = 4, /* send */
    [ 179] = 3, /* sendmsg */
    [ 180] = 6, /* sendto */
    [ 181] = 5, /* setsockopt */
    [ 182] = 2, /* shutdown */
    [ 183] = 3, /* socket */
    [ 184] = 4, /* socketpair */
    [ 185] = 3, /* setresuid */
    [ 186] = 3, /* getresuid */
    [ 187] = 5, /* query_module */
    [ 188] = 3, /* poll */
    [ 189] = 3, /* nfsservctl */
    [ 190] = 3, /* setresgid */
    [ 191] = 3, /* getresgid */
    [ 192] = 5, /* prctl */
    [ 193] = 0, /* rt_sigreturn */
    [ 194] = 4, /* rt_sigaction */
    [ 195] = 4, /* rt_sigprocmask */
    [ 196] = 2, /* rt_sigpending */
    [ 197] = 4, /* rt_sigtimedwait */
    [ 198] = 3, /* rt_sigqueueinfo */
    [ 199] = 2, /* rt_sigsuspend */
    [ 200] = 6, /* pread64 */
    [ 201] = 6, /* pwrite64 */
    [ 202] = 3, /* chown */
    [ 203] = 2, /* getcwd */
    [ 204] = 2, /* capget */
    [ 205] = 2, /* capset */
    [ 206] = 2, /* sigaltstack */
    [ 207] = 4, /* sendfile */
    [ 208] = 5, /* getpmsg */
    [ 209] = 5, /* putpmsg */
    [ 210] = 6, /* mmap2 */
    [ 211] = 4, /* truncate64 */
    [ 212] = 4, /* ftruncate64 */
    [ 213] = 2, /* stat64 */
    [ 214] = 2, /* lstat64 */
    [ 215] = 2, /* fstat64 */
    [ 216] = 2, /* pivot_root */
    [ 217] = 3, /* mincore */
    [ 218] = 3, /* madvise */
    [ 219] = 3, /* getdents64 */
    [ 220] = 3, /* fcntl64 */
    [ 221] = 0, /* reserved221 */
    [ 222] = 0, /* gettid */
    [ 223] = 5, /* readahead */
    [ 224] = 5, /* setxattr */
    [ 225] = 5, /* lsetxattr */
    [ 226] = 5, /* fsetxattr */
    [ 227] = 4, /* getxattr */
    [ 228] = 4, /* lgetxattr */
    [ 229] = 4, /* fgetxattr */
    [ 230] = 3, /* listxattr */
    [ 231] = 3, /* llistxattr */
    [ 232] = 3, /* flistxattr */
    [ 233] = 2, /* removexattr */
    [ 234] = 2, /* lremovexattr */
    [ 235] = 2, /* fremovexattr */
    [ 236] = 2, /* tkill */
    [ 237] = 4, /* sendfile64 */
    [ 238] = 6, /* futex */
    [ 239] = 3, /* sched_setaffinity */
    [ 240] = 3, /* sched_getaffinity */
    [ 241] = 2, /* io_setup */
    [ 242] = 1, /* io_destroy */
    [ 243] = 5, /* io_getevents */
    [ 244] = 3, /* io_submit */
    [ 245] = 3, /* io_cancel */
    [ 246] = 1, /* exit_group */
    [ 247] = 4, /* lookup_dcookie */
    [ 248] = 1, /* epoll_create */
    [ 249] = 4, /* epoll_ctl */
    [ 250] = 4, /* epoll_wait */
    [ 251] = 5, /* remap_file_pages */
    [ 252] = 1, /* set_tid_address */
    [ 253] = 0, /* restart_syscall */
    [ 254] = 7, /* fadvise64 */
    [ 255] = 3, /* statfs64 */
    [ 256] = 3, /* fstatfs64 */
    [ 257] = 3, /* timer_create */
    [ 258] = 4, /* timer_settime */
    [ 259] = 2, /* timer_gettime */
    [ 260] = 1, /* timer_getoverrun */
    [ 261] = 1, /* timer_delete */
    [ 262] = 2, /* clock_settime */
    [ 263] = 2, /* clock_gettime */
    [ 264] = 2, /* clock_getres */
    [ 265] = 4, /* clock_nanosleep */
    [ 266] = 3, /* tgkill */
    [ 267] = 2, /* utimes */
    [ 268] = 6, /* mbind */
    [ 269] = 5, /* get_mempolicy */
    [ 270] = 3, /* set_mempolicy */
    [ 271] = 4, /* mq_open */
    [ 272] = 1, /* mq_unlink */
    [ 273] = 5, /* mq_timedsend */
    [ 274] = 5, /* mq_timedreceive */
    [ 275] = 2, /* mq_notify */
    [ 276] = 3, /* mq_getsetattr */
    [ 277] = 5, /* vserver */
    [ 278] = 5, /* waitid */
    [ 279] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 280] = 5, /* add_key */
    [ 281] = 4, /* request_key */
    [ 282] = 5, /* keyctl */
    [ 283] = 1, /* set_thread_area */
    [ 284] = 0, /* inotify_init */
    [ 285] = 3, /* inotify_add_watch */
    [ 286] = 2, /* inotify_rm_watch */
    [ 287] = 4, /* migrate_pages */
    [ 288] = 4, /* openat */
    [ 289] = 3, /* mkdirat */
    [ 290] = 4, /* mknodat */
    [ 291] = 5, /* fchownat */
    [ 292] = 3, /* futimesat */
    [ 293] = 4, /* fstatat64 */
    [ 294] = 3, /* unlinkat */
    [ 295] = 4, /* renameat */
    [ 296] = 5, /* linkat */
    [ 297] = 3, /* symlinkat */
    [ 298] = 4, /* readlinkat */
    [ 299] = 3, /* fchmodat */
    [ 300] = 3, /* faccessat */
    [ 301] = 6, /* pselect6 */
    [ 302] = 5, /* ppoll */
    [ 303] = 1, /* unshare */
    [ 304] = 6, /* splice */
    [ 305] = 7, /* sync_file_range */
    [ 306] = 4, /* tee */
    [ 307] = 4, /* vmsplice */
    [ 308] = 6, /* move_pages */
    [ 309] = 2, /* set_robust_list */
    [ 310] = 3, /* get_robust_list */
    [ 311] = 4, /* kexec_load */
    [ 312] = 3, /* getcpu */
    [ 313] = 6, /* epoll_pwait */
    [ 314] = 3, /* ioprio_set */
    [ 315] = 2, /* ioprio_get */
    [ 316] = 4, /* utimensat */
    [ 317] = 3, /* signalfd */
    [ 318] = 4, /* timerfd */
    [ 319] = 1, /* eventfd */
    [ 320] = 6, /* fallocate */
    [ 321] = 2, /* timerfd_create */
    [ 322] = 2, /* timerfd_gettime */
    [ 323] = 4, /* timerfd_settime */
    [ 324] = 4, /* signalfd4 */
    [ 325] = 2, /* eventfd2 */
    [ 326] = 1, /* epoll_create1 */
    [ 327] = 3, /* dup3 */
    [ 328] = 2, /* pipe2 */
    [ 329] = 1, /* inotify_init1 */
    [ 330] = 5, /* preadv */
    [ 331] = 5, /* pwritev */
    [ 332] = 4, /* rt_tgsigqueueinfo */
    [ 333] = 5, /* perf_event_open */
    [ 334] = 4, /* accept4 */
    [ 335] = 5, /* recvmmsg */
    [ 336] = 2, /* fanotify_init */
    [ 337] = 6, /* fanotify_mark */
    [ 338] = 4, /* prlimit64 */
    [ 339] = 5, /* name_to_handle_at */
    [ 340] = 3, /* open_by_handle_at */
    [ 341] = 2, /* clock_adjtime */
    [ 342] = 1, /* syncfs */
    [ 343] = 4, /* sendmmsg */
    [ 344] = 2, /* setns */
    [ 345] = 6, /* process_vm_readv */
    [ 346] = 6, /* process_vm_writev */
    [ 347] = 5, /* kcmp */
    [ 348] = 3, /* finit_module */
    [ 349] = 3, /* sched_setattr */
    [ 350] = 4, /* sched_getattr */
    [ 351] = 5, /* renameat2 */
    [ 352] = 3, /* seccomp */
    [ 353] = 3, /* getrandom */
    [ 354] = 2, /* memfd_create */
    [ 355] = 3, /* bpf */
    [ 356] = 5, /* execveat */
    [ 357] = 1, /* userfaultfd */
    [ 358] = 3, /* membarrier */
    [ 359] = 3, /* mlock2 */
    [ 360] = 6, /* copy_file_range */
    [ 361] = 6, /* preadv2 */
    [ 362] = 6, /* pwritev2 */
    [ 363] = 4, /* pkey_mprotect */
    [ 364] = 2, /* pkey_alloc */
    [ 365] = 1, /* pkey_free */
    [ 366] = 5, /* statx */
    [ 367] = 4, /* rseq */
    [ 368] = 6, /* io_pgetevents */
    [ 369] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 370] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 371] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 372] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 373] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 374] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 375] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 376] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 377] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 378] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 379] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 380] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 381] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 382] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 383] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 384] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 385] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 386] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 387] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 388] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 389] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 390] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 391] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 392] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 393] = 3, /* semget */
    [ 394] = 4, /* semctl */
    [ 395] = 3, /* shmget */
    [ 396] = 3, /* shmctl */
    [ 397] = 3, /* shmat */
    [ 398] = 1, /* shmdt */
    [ 399] = 2, /* msgget */
    [ 400] = 4, /* msgsnd */
    [ 401] = 5, /* msgrcv */
    [ 402] = 3, /* msgctl */
    [ 403] = 2, /* clock_gettime64 */
    [ 404] = 2, /* clock_settime64 */
    [ 405] = 2, /* clock_adjtime64 */
    [ 406] = 2, /* clock_getres_time64 */
    [ 407] = 4, /* clock_nanosleep_time64 */
    [ 408] = 2, /* timer_gettime64 */
    [ 409] = 4, /* timer_settime64 */
    [ 410] = 2, /* timerfd_gettime64 */
    [ 411] = 4, /* timerfd_settime64 */
    [ 412] = 4, /* utimensat_time64 */
    [ 413] = 6, /* pselect6_time64 */
    [ 414] = 5, /* ppoll_time64 */
    [ 415] = MIPS_SYSCALL_NUMBER_UNUSED,
    [ 416] = 6, /* io_pgetevents_time64 */
    [ 417] = 5, /* recvmmsg_time64 */
    [ 418] = 5, /* mq_timedsend_time64 */
    [ 419] = 5, /* mq_timedreceive_time64 */
    [ 420] = 4, /* semtimedop_time64 */
    [ 421] = 4, /* rt_sigtimedwait_time64 */
    [ 422] = 6, /* futex_time64 */
    [ 423] = 2, /* sched_rr_get_interval_time64 */
    [ 424] = 4, /* pidfd_send_signal */
    [ 425] = 2, /* io_uring_setup */
    [ 426] = 6, /* io_uring_enter */
    [ 427] = 4, /* io_uring_register */
    [ 428] = 3, /* open_tree */
    [ 429] = 5, /* move_mount */
    [ 430] = 2, /* fsopen */
    [ 431] = 5, /* fsconfig */
    [ 432] = 3, /* fsmount */
    [ 433] = 3, /* fspick */
    [ 434] = 2, /* pidfd_open */
    [ 435] = 2, /* clone3 */
    [ 436] = 3, /* close_range */
    [ 437] = 4, /* openat2 */
    [ 438] = 3, /* pidfd_getfd */
    [ 439] = 4, /* faccessat2 */
    [ 440] = 5, /* process_madvise */
    [ 441] = 6, /* epoll_pwait2 */
    [ 442] = 5, /* mount_setattr */
    [ 443] = 4, /* quotactl_fd */
    [ 444] = 3, /* landlock_create_ruleset */
    [ 445] = 4, /* landlock_add_rule */
    [ 446] = 2, /* landlock_restrict_self */
    [ 447] = 1, /* memfd_secret */
    [ 448] = 2, /* process_mrelease */
    [ 449] = 5, /* futex_waitv */
    [ 450] = 4, /* set_mempolicy_home_node */
    [ 451] = 4, /* cachestat */
    [ 452] = 4, /* fchmodat2 */
    [ 453] = 3, /* map_shadow_stack */
    [ 454] = 4, /* futex_wake */
    [ 455] = 6, /* futex_wait */
    [ 456] = 4, /* futex_requeue */
    [ 457] = 4, /* statmount */
    [ 458] = 4, /* listmount */
    [ 459] = 4, /* lsm_get_self_attr */
    [ 460] = 4, /* lsm_set_self_attr */
    [ 461] = 3, /* lsm_list_modules */
    [ 462] = 3, /* mseal */
