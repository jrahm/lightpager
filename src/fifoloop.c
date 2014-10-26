#include "lightpager.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define FIFO_PATH "/tmp/lightpager_intf.fifo"

void* _fifo_thread_main( void* _self ) {

    main_app_t* self = _self;
    size_t bytes_read;

    mkfifo(FIFO_PATH, 00600);

    char buf[1024];
    while(1) {
        self->fifo_fd = open( FIFO_PATH, O_RDONLY );

        bytes_read = read( self->fifo_fd, buf, 1024 );
        while( bytes_read > 0 ) {
            self->fifo_read_callback(self, buf, bytes_read);
            bytes_read = read( self->fifo_fd, buf, 1024 );
        }
    }
}

void start_fifo_thread( main_app_t* app ) {
    pthread_create( &app->fifo_thread, NULL, _fifo_thread_main, app );
}
