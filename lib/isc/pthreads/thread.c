/*
 * Copyright (C) 2000  Internet Software Consortium.
 * 
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL INTERNET SOFTWARE
 * CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include <config.h>

#include <isc/thread.h>

#ifndef THREAD_MINSTACKSIZE
#define THREAD_MINSTACKSIZE		(64 * 1024)
#endif

isc_result_t
isc_thread_create(isc_threadfunc_t func, isc_threadarg_t arg,
		  isc_thread_t *thread)
{
	pthread_attr_t attr;
	size_t stacksize;
	int ret;

	pthread_attr_init(&attr);

	ret = pthread_attr_getstacksize(&attr, &stacksize);
	if (ret != 0)
		return (ISC_R_UNEXPECTED);

	if (stacksize < THREAD_MINSTACKSIZE) {
		ret = pthread_attr_setstacksize(&attr, THREAD_MINSTACKSIZE);
		if (ret != 0)
			return (ISC_R_UNEXPECTED);
	}

	ret = pthread_create(thread, &attr, func, arg);
	if (ret != 0)
		return (ISC_R_UNEXPECTED);

	pthread_attr_destroy(&attr);

	return (ISC_R_SUCCESS);
}
