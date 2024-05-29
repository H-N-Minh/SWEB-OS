#pragma once

#define fd_stdin 0
#define fd_stdout 1
#define fd_stderr 2

#define sc_exit 1
#define sc_fork 2
#define sc_read 3
#define sc_write 4
#define sc_open 5
#define sc_close 6

#define sc_wait_pid 7

#define sc_lseek 19
#define sc_pseudols 43
#define sc_outline 105
#define sc_sched_yield 158
#define sc_createprocess 191
#define sc_trace 252

#define sc_threadcount 260

#define sc_pthread_create 300
#define sc_pthread_exit 301
#define sc_pthread_join 302
#define sc_pthread_cancel 303
#define sc_pthread_setcanceltype 304
#define sc_pthread_setcancelstate 305
#define sc_pthread_testcancel 306
#define sc_pthread_detach 307
#define sc_pthread_self 308

#define sc_execv 400

#define sc_sleep 500
#define sc_clock 501

// malloc
#define sc_sbrk 600
#define sc_brk 601


// Needed for test system Tortillas
#define sc_tortillas_bootup 1337
#define sc_tortillas_finished 1338

#define sc_pipe 111
#define sc_dup 112

#define sc_getIPTInfos 401
#define sc_assertIPT 402

#define sc_setPRA 403
#define sc_getPRAstats 404
