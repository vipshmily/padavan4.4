/*-
 * Copyright (c) 2011 - 2020 Rozhuk Ivan <rozhuk.im@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Author: Rozhuk Ivan <rozhuk.im@gmail.com>
 *
 */

 
#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <sys/param.h>
#include <sys/types.h>
#include <inttypes.h>
#include <time.h>
#ifdef THREAD_POOL_SETTINGS_INI
#	include "utils/ini.h"
#endif


typedef struct thread_pool_s		*tp_p;		/* Thread pool. */
typedef struct thread_pool_thread_s	*tpt_p;		/* Thread pool thread. */
typedef struct thread_pool_udata_s	*tp_udata_p;	/* Thread pool user data. */


typedef struct thread_pool_event_s { /* Thread pool event. */
	uint16_t	event;	/* Filter for event. */
	uint16_t	flags;	/* Action flags. */
	uint32_t	fflags;	/* Filter flag value. */
	uint64_t	data;	/* Filter data value: Read: ioctl(FIONREAD), write: ioctl(FIONSPACE) FIONWRITE, SIOCGIFBUFS, (SIOCOUTQ/SIOCINQ TIOCOUTQ/TIOCINQ + getsockopt(s, SOL_SOCKET, SO_SNDBUF, ...))? */
} tp_event_t, *tp_event_p;

/* Events		val	FreeBSD		__linux__ */
#define TP_EV_READ	0 /* EVFILT_READ	EPOLLET | EPOLLIN | EPOLLRDHUP | EPOLLERR */
#define TP_EV_WRITE	1 /* EVFILT_WRITE	EPOLLET | EPOLLOUT | EPOLLERR */
#define TP_EV_TIMER	2 /* EVFILT_TIMER	TP_EV_READ + timerfd_create */
#define TP_EV_LAST	TP_EV_TIMER
#define TP_EV_MASK	0x0003u /* For internal use: event set mask. */

/* Event flags. */
/* Only for set.	val			FreeBSD			__linux__ */
#define TP_F_ONESHOT	(((uint16_t)1) << 0) /* Set: EV_ONESHOT		EPOLLONESHOT */ /* Delete event after recv. */
#define TP_F_DISPATCH	(((uint16_t)1) << 1) /* Set: EV_DISPATCH	EPOLLONESHOT */ /* DISABLE event after recv. */
#define TP_F_EDGE	(((uint16_t)1) << 2) /* Set: EV_CLEAR		EPOLLET */ /* Report only if avaible data changed.*/
 									/* If not set will report if data/space avaible untill disable/delete event. */
#define TP_F_S_MASK	0x0007u /* For internal use: flags set mask. */
/* Return only. */
#define TP_F_EOF	(((uint16_t)1) << 3) /* Ret: EV_EOF		EPOLLRDHUP */
#define TP_F_ERROR	(((uint16_t)1) << 4) /* Ret: EV_EOF+fflags	EPOLLERR +  getsockopt(SO_ERROR) */ /* fflags contain error code. */

/* Event fflags. */
/* TP_EV_TIMER specific: if not set - the default is milliseconds. */
#define TP_FF_T_SEC	(((uint32_t)1) << 0) /* data is seconds. */
#define TP_FF_T_MSEC	(((uint32_t)1) << 1) /* data is milliseconds. */
#define TP_FF_T_USEC	(((uint32_t)1) << 2) /* data is microseconds. */
#define TP_FF_T_NSEC	(((uint32_t)1) << 3) /* data is nanoseconds. */
#define TP_FF_T_ABSTIME	(((uint32_t)1) << 4) /* timeout is absolute. */
#define TP_FF_T_TM_MASK	0x0000000fu /* For internal use: fflags set mask for time. */
#define TP_FF_T_MASK	0x0000001fu /* For internal use: fflags set mask. */


typedef void (*tp_cb)(tp_event_p ev, tp_udata_p tp_udata);

typedef struct thread_pool_udata_s { /* Thread pool ident and opaque user data. */
	tp_cb		cb_func;/* Function to handle IO complete/err. */
	uintptr_t	ident;	/* Identifier for this event: socket, file, etc.
				 * For timer ident can be ponter to mem or any
				 * unique number. */
	/* User defined. */
	void		*ptr;	/* Some pointer. */
	size_t		size;	/* Some size. */
	/* Internal data, do not use!!! */
	tpt_p		tpt;	/* Pointer to thread data. */
	uint64_t	tpdata;	/* Linux: timer - timer file handle;
				 * read/write/timer - event: TP_EV_*;
				 * TP_F_* flags. */
	/* Opaque user data ... */
} tp_udata_t;


int	tp_signal_handler_add_tp(tp_p tp);
void	tp_signal_handler(int sig);


typedef struct thread_pool_settings_s { /* Settings */
	uint32_t	flags;	/* TP_S_F_* */
	size_t		threads_max;
} tp_settings_t, *tp_settings_p;

#define TP_S_F_BIND2CPU		(((uint32_t)1) << 0)	/* Bind threads to CPUs */
//--#define TP_S_F_SHARE_EVENTS	(((uint32_t)1) << 1)	/* Not affected if threads_max = 1 */

/* Default values. */
#define TP_S_DEF_FLAGS		(TP_S_F_BIND2CPU)
#define TP_S_DEF_THREADS_MAX	(0)

void	tp_settings_def(tp_settings_p s_ret);

#ifdef THREAD_POOL_SETTINGS_XML
int	tp_settings_load_xml(const uint8_t *buf, size_t buf_size,
	    tp_settings_p s);
#endif
#ifdef THREAD_POOL_SETTINGS_INI
int	tp_settings_load_ini(const ini_p ini, const uint8_t *sect_name,
	    const size_t sect_name_size, tp_settings_p s);
#endif



int	tp_init(void);
int	tp_create(tp_settings_p s, tp_p *ptp);

void	tp_shutdown(tp_p tp);
void	tp_shutdown_wait(tp_p tp);
void	tp_destroy(tp_p tp);

int	tp_threads_create(tp_p tp, int skip_first);
int	tp_thread_attach_first(tp_p tp);
int	tp_thread_dettach(tpt_p tpt);
size_t	tp_thread_count_max_get(tp_p tp);
size_t	tp_thread_count_get(tp_p tp);

tpt_p	tp_thread_get_current(void);
tpt_p	tp_thread_get(tp_p tp, size_t thread_num);
tpt_p	tp_thread_get_rr(tp_p tp);
tpt_p	tp_thread_get_pvt(tp_p tp); /* Shared virtual thread. */
int	tp_thread_get_cpu_id(tpt_p tpt);
size_t	tp_thread_get_num(tpt_p tpt);

tp_p	tpt_get_tp(tpt_p tpt);
size_t	tpt_is_running(tpt_p tpt);
void	*tpt_get_msg_queue(tpt_p tpt);



int	tpt_ev_add(tpt_p tpt, tp_event_p ev, tp_udata_p tp_udata);
int	tpt_ev_add_args(tpt_p tpt, uint16_t event, uint16_t flags,
	    uint32_t fflags, uint64_t data, tp_udata_p tp_udata);
int	tpt_ev_add_args2(tpt_p tpt, uint16_t event, uint16_t flags,
	    tp_udata_p tp_udata);

/* flags - allowed: TP_F_ONESHOT, TP_F_DISPATCH, TP_F_EDGE */
int	tpt_ev_del(tp_event_p ev, tp_udata_p tp_udata);
int	tpt_ev_del_args1(uint16_t event, tp_udata_p tp_udata);
int	tpt_ev_enable(int enable, tp_event_p ev, tp_udata_p tp_udata);
int	tpt_ev_enable_args(int enable, uint16_t event, uint16_t flags,
	    uint32_t fflags, uint64_t data, tp_udata_p tp_udata);
int	tpt_ev_enable_args1(int enable, uint16_t event, tp_udata_p tp_udata);

/* fflags: TP_FF_T_* */
int	tpt_timer_add(tpt_p tpt, int enable, uintptr_t ident,
	    uint64_t timeout, uint16_t flags, uint32_t fflags,
	    tp_cb cb_func, tp_udata_p tp_udata);


#ifdef NOT_YET__FreeBSD__ /* Per thread queue functions. Only for kqueue! */
int	tpt_ev_q_add(tpt_p tpt, uint16_t event, uint16_t flags,
	    tp_udata_p tp_udata);
int	tpt_ev_q_del(uint16_t event, tp_udata_p tp_udata);
int	tpt_ev_q_enable(int enable, uint16_t event, tp_udata_p tp_udata);
int	tpt_ev_q_enable_ex(int enable, uint16_t event, uint16_t flags,
	    uint32_t fflags, uint64_t data, tp_udata_p tp_udata);
int	tpt_ev_q_flush(tpt_p tpt);
#else
#define	tpt_ev_q_add		tpt_ev_add
#define	tpt_ev_q_del		tpt_ev_del
#define	tpt_ev_q_enable		tpt_ev_enable
#define	tpt_ev_q_enable_args	tpt_ev_enable_args
#define	tpt_ev_q_enable_args1	tpt_ev_enable_args1
#define	tpt_ev_q_flush
#endif


#endif /* __THREAD_POOL_H__ */
