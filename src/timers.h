#ifndef TIMERS_H_
#define TIMERS_H_

/*
 * Author: jrahm
 * created: 2014/10/24
 * timers.h: <description>
 */

struct timer {
	void* data;
	long n;
	void (*callback)(void *data);
};

/* Set timer of n milliseconds in the future */
void set_timer( void (*)(void*), void*, long );

#endif /* TIMERS_H_ */
