#include "timers.h"

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

void* _timer_main( void* data ) {
	struct timer* t = data;
	usleep( t->n * 1000 );
	t->callback( t->data );
	free(t);
	pthread_exit(NULL);
}

void set_timer( void (*cb)(void*), void* arg, long t ) {
	struct timer* timer_str = malloc( sizeof (struct timer) );
	timer_str->data = arg;
	timer_str->n = t;
	timer_str->callback = cb;
	pthread_t thread;
	pthread_create( &thread, NULL, _timer_main, timer_str );
}
