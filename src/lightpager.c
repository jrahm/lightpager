/* light, 2d pager imlemented with gtk 3.0 */

#include "lightpager.h"
#include "config.h"
#include "timers.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/wait.h>

#include <errno.h>

#define max(a,b) ((a) > (b) ? (a) : (b))

/* Different implementations of the interpolation
 * functions */
double avg_interp ( double dest, double cur ) {
    /* Take the average of the destination and the
     * current */
    return (dest + cur) / 2.0;
}

double linear_interp ( double dest, double cur ) {
    /* Just move in the right direction */
    if( dest > cur ) {
        cur += 2.0;
    } else {
        cur -= 2.0;
    }
    return cur;
}

double no_interp ( double dest, double cur ) {
    (void)cur;
    /* the new position is just the destination */
    return dest;
}

/* Create the new main app. This is the struct that conains
 * pretty much all of our data. To display the application. */
main_app_t* new_main_app() {
    main_app_t* ret = (main_app_t*)malloc(sizeof(main_app_t));

    /* Contruct the icon */
    ret->icon = calloc(1,sizeof(lightpager_icon_t));
    ret->icon->interp_function = linear_interp;
    ret->icon->owner = ret;
    ret->icon->super = gtk_status_icon_new();
    ret->icon->make_pixbuf = _lightpager_icon_make_pixbuf;

    /* Fill the icon options with the defaults */
    fill_defaults(&ret->icon->options);

    /* The default number of rows and columns */
    ret->icon->nrows = 3;
    ret->icon->ncols = 3;

    /* the window for GTK */
    ret->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    /* Set the callback for the fifo for async io */
    ret->fifo_read_callback = _main_app_fifo_callback;

    pthread_mutex_init( &ret->mutex, NULL );

    g_signal_connect(G_OBJECT(ret->window), "delete-event", G_CALLBACK(gtk_main_quit), NULL);
    gtk_status_icon_set_visible( ret->icon->super, true );

    return ret;
}

void _with_xdo_tool( const char* cmd, const char* arg ) {
    if( fork() == 0 ) {
        execl("/usr/bin/xdotool", "/usr/bin/xdotool", cmd, arg, NULL);
        perror("execl");
    }

    int status;
    wait( &status );

    if( status ) {
        perror("switch desktop failed");
    }
}

void _set_desktop( main_app_t* self, uint desktop ) {
    (void) self;
    char num[16];
    snprintf(num, 16, "%d", desktop);
    _with_xdo_tool( "set_desktop", num );
}

void _set_num_desktops( main_app_t* self, uint n ) {
    (void) self;
    char num[16];
    snprintf(num, 16, "%d", n);
    _with_xdo_tool( "set_num_desktops", num );
}

void _main_app_set_desktop( main_app_t* self ) {
    uint desktop = self->icon->current_row * self->icon->ncols +
                   self->icon->current_col;
    _set_desktop( self, desktop );
}

int _go_right( main_app_t* self ) {
    if ( self->icon->current_col <
         self->icon->ncols - 1 ) {
        self->icon->current_col += 1;
        return 1;
    }
    return 0;
}

int _go_left( main_app_t* self ) {
    if ( self->icon->current_col > 0) {
        self->icon->current_col -= 1;
        return 1;
    }
    return 0;
}

int _go_up( main_app_t* self ) {
    if ( self->icon->current_row > 0) {
        self->icon->current_row -= 1;
        return 1;
    }
    return 0;
}

int _go_down( main_app_t* self ) {
    if ( self->icon->current_row <
         self->icon->nrows - 1) {
        self->icon->current_row += 1;
        return 1;
    }
    return 0;
}

void _main_app_fifo_callback( main_app_t* self, const char* line, size_t len ) {
    int rc = 0;

    if( strncmp(line,"right\n",len) == 0 ) {
        rc = _go_right( self );
    } else if ( strncmp(line,"left\n",len) == 0 ) {
        rc = _go_left( self );
    } else if ( strncmp(line,"up\n",len) == 0 ) {
        rc = _go_up( self );
    } else if ( strncmp(line,"down\n",len) == 0 ) {
        rc = _go_down( self );
    } else if ( strncmp(line,"reload\n",len) == 0) {
        printf("Reloading config per user request\n");
        _main_app_read_config( self );
        rc = 1;
    }

    if( rc ) {
        _main_app_update_icon( self );
        _main_app_set_desktop( self );
    }
}

gboolean _main_app_update_icon_idle( gpointer _app ){
    main_app_t* app = _app;
    pthread_mutex_lock( &app->mutex );

    GdkPixbuf* image = app->icon->make_pixbuf( app->icon );
    gtk_status_icon_set_from_pixbuf(app->icon->super, image);
    g_object_unref(image);

    pthread_mutex_unlock( &app->mutex );
    return false;
}
void _main_app_update_icon( main_app_t* app ) {
    gdk_threads_add_idle( _main_app_update_icon_idle, app );
}

void _cairo_set_color( cairo_t* ctx, color_t color ) {
    cairo_set_source_rgba(ctx,
        color.r, color.g, color.b, 1.0 - color.a );
}

color_t mk_color( int a, int r, int g, int b) {
    return mk_color_float( a / 256.0, r / 256.0, g / 256.0, b / 256.0 );
}

color_t mk_color_float( float a, float r, float g, float b ) {
    return (color_t){ a, r, g, b };
}

color_t mk_color_int( int color ) {
    return mk_color( (color >> 24) & 0xFF,
                     (color >> 16) & 0xFF,
                     (color >> 8 ) & 0xFF,
                      color & 0xFF );
}

GdkPixbuf* _lightpager_icon_make_pixbuf( void* _self ) {
    uint x, y;
    lightpager_icon_t* self = _self;

    /* Bring the options onto the stack */
    color_t active_color = self->options.active_color; 
    color_t passive_color = self->options.passive_color; 
    color_t bg_color = self->options.background_color;

    float padding = self->options.padding;
    float outer_padding = self->options.outer_padding;
    float relative_size = self->options.relative_size;
    float active_size = self->options.active_size;

    uint interp_speed = 1000 - self->options.interp_speed ;
    /* End options */

    
    uint pixels_per_square = (uint)(64*relative_size / max(self->nrows, self->ncols));

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 64, 64);
    cairo_t* ctx = cairo_create( surface );

    cairo_set_antialias( ctx, CAIRO_ANTIALIAS_SUBPIXEL );
    cairo_set_antialias( ctx, CAIRO_ANTIALIAS_BEST );

    _cairo_set_color( ctx, bg_color );
    cairo_rectangle(ctx, outer_padding, outer_padding, 64 - 2*outer_padding, 64-2*outer_padding);
    cairo_fill(ctx);
    
    double yoff = (64 - (pixels_per_square * self->nrows)) / 2.0 + 0.5;
    double xoff;
    double cyoff = 0;
    double cxoff = 0;

    double width = pixels_per_square - padding; /* padding */

    for(y = 0; y < self->nrows; ++ y ) {
        xoff = (64 - (pixels_per_square * self->ncols)) / 2.0 + 0.5;
        for( x = 0; x < self->ncols; ++ x ) {


            if( x == self->current_col &&
                y == self->current_row ) {
                    cxoff = xoff;
                    cyoff = yoff;
            } else {
            }

            _cairo_set_color(ctx, passive_color);
            cairo_rectangle( ctx, xoff, yoff, width, width );
            cairo_fill(ctx);
            xoff += pixels_per_square;
        }
        yoff += pixels_per_square;
    }

    if( self->current_offset_x == 0 &&
        self->current_offset_y == 0 ) {
        self->current_offset_x = cxoff;
        self->current_offset_y = cyoff;
    }

    int not_stop = 0 ;
    if( abs(self->current_offset_x - cxoff) < 2.0 ) {
        /* The animation is in the threashold of completion for
         * the x direction */
        self->current_offset_x = cxoff;
    } else {
        not_stop = 1;
        cxoff = self->current_offset_x =
            self->interp_function(cxoff, self->current_offset_x);
    }

    if( abs(self->current_offset_y - cyoff) < 2.0 ) {
        /* The animaiton is in the threashold of completion for the
         * y direction */
        self->current_offset_y = cyoff;
    } else {
        not_stop = 1;
        cyoff = self->current_offset_y =
	        self->interp_function(cyoff, self->current_offset_y);
    }

    /* The animation is complete, we can stop the timer */
    if( not_stop ) {
        /* if the animation is not yet complete, we set another
         * timeout */
        self->timer_tag = g_timeout_add( interp_speed,
					        _main_app_update_icon_idle,
                            self->owner);
    }


    /* If the width of the selected workspace in enlarged */
    double diff = width * (active_size - 1.0) / 2.0 ;
    width *= active_size;

    /* Draw the selected thing */
    _cairo_set_color(ctx, active_color);
    cairo_rectangle( ctx, cxoff-diff, cyoff-diff, width, width );
    cairo_fill(ctx);

    cairo_destroy(ctx);
    GdkPixbuf* ret = gdk_pixbuf_get_from_surface( surface, 0, 0,
			            cairo_image_surface_get_width(surface),
			            cairo_image_surface_get_height(surface));
    cairo_surface_destroy(surface);
    return ret;
}

int _config_parse_color(const char* color, color_t* into) {
    int col;
    if( color[0] == '#' ) {
        color ++;
        col = strtol(color, NULL, 16);
        *into = mk_color_int( col );
        return 0;
    }
    fprintf(stderr, "Error: Bad color %s\n", color);
    return 1;
}

int _config_read_lambda(main_app_t* closure, const char* key, const char* val) {
    if( !strcmp(key, "active_color") )
        _config_parse_color( val, &closure->icon->options.active_color );
    else if( !strcmp(key, "passive_color") )
        _config_parse_color( val, &closure->icon->options.passive_color );
    else if( !strcmp(key, "background_color") )
        _config_parse_color( val, &closure->icon->options.background_color );
    else if( !strcmp(key, "padding") )
        closure->icon->options.padding = atof(val);
    else if( !strcmp(key, "relative_size") )
        closure->icon->options.relative_size = atof(val);
    else if( !strcmp(key, "outer_padding") )
        closure->icon->options.outer_padding = atof(val);
    else if( !strcmp(key, "active_size") )
        closure->icon->options.active_size = atof(val);
    else if( !strcmp(key, "width") )
        closure->icon->ncols = atoi(val);
    else if( !strcmp(key, "height") )
        closure->icon->nrows = atoi(val);
    else if( !strcmp(key, "interpolation_speed") )
        closure->icon->options.interp_speed = atoi(val);
    else if( !strcmp(key, "interpolation") ) {
        if( !strcmp(val, "none\n") ) 
	        closure->icon->interp_function = no_interp;
        if( !strcmp(val, "linear\n") ) 
	        closure->icon->interp_function = linear_interp;
        if( !strcmp(val, "average\n") ) 
	        closure->icon->interp_function = avg_interp;
        else fprintf(stderr, "Error: Interpolation strategy not recognized \"%s\"", val);
    }
    else {
        fprintf(stderr, "Error: Key not recognized \"%s\"", key);
    }
    return 0;
}

void _main_app_read_config( main_app_t* mainapp ) {
    char* home = getenv("HOME");
    char buf[1024];
    snprintf(buf, 1024, "%s/.config/lightpager.conf", home);

    if( read_config(buf, (conf_lambda_t)_config_read_lambda, mainapp )) {
        fprintf(stderr, "There was an error reading the config file: %s\n", strerror(errno));
    } else {
        _set_num_desktops(mainapp, mainapp->icon->ncols * mainapp->icon->nrows);
    }
}
/* Main function */
int main ( int argc, char** argv ) {
    gtk_init(&argc, &argv);
    main_app_t* mainapp = new_main_app();
    _main_app_read_config( mainapp );


    _main_app_update_icon( mainapp );
    start_fifo_thread( mainapp );

    gtk_main();
    return 0;
}

void fill_defaults( lightpager_icon_options_t* opts ) {
    opts->active_color = mk_color_float(0,1,1,1);
    opts->passive_color = mk_color_float(0,0.5,0.5,0.5);
    opts->background_color = mk_color_float(0,0,0,0);
    opts->relative_size = 1.0;
    opts->active_size = 1.0;
    opts->interp_speed = 900;
}
