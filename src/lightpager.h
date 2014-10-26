#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo/cairo.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>

#define true 1
#define false 0

/* Simple color struct */
typedef struct {
    float a;
    float r;
    float g;
    float b;
} color_t;

/* Main options used when displaying the icon.
 * these are configuarble in the configuration flie */
typedef struct {
    
    /* colors of each component */
    color_t active_color;
    color_t passive_color;
    color_t background_color;

    /* The sizes of each component */
    float outer_padding;
    float padding;
    float relative_size;
    float active_size;

    uint interp_speed;
} lightpager_icon_options_t;

/* Fill an options struct with the defaults */
void fill_defaults( lightpager_icon_options_t* opts ) ;

struct MAIN_APP ;

typedef struct {
    GtkStatusIcon* super;

    GdkPixbuf* (*make_pixbuf)(void*);
    double (*interp_function)(double dest, double current);

    uint nrows;
    uint ncols;

    uint current_row;
    uint current_col;

    lightpager_icon_options_t options;

    float current_offset_x;
    float current_offset_y;

    struct MAIN_APP* owner;
    int timer_tag;
} lightpager_icon_t;

typedef struct MAIN_APP {
    lightpager_icon_t* icon;
    GtkWidget *window;

    pthread_t fifo_thread;
    int fifo_fd;
    void (*fifo_read_callback)(struct MAIN_APP* self, const char* line, size_t len);

    pthread_mutex_t mutex;
} main_app_t;

main_app_t* new_main_app() ;
GdkPixbuf* _lightpager_icon_make_pixbuf( void* );
void _main_app_fifo_callback( main_app_t* self, const char* line, size_t len );
void _main_app_update_icon( main_app_t* app );
void _main_app_read_config( main_app_t* );
void start_fifo_thread( main_app_t* app );

color_t mk_color( int a, int r, int g, int b );
color_t mk_color_float( float a, float r, float g, float b );
/* Format: argb */
color_t mk_color_int( int color );
