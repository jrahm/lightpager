#!/usr/bin/python

from gi.repository import Gtk, Gdk, GLib, Pango, PangoCairo
from cairo import Context, ImageSurface, RadialGradient, FORMAT_ARGB32

import signal
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop

import subprocess
from optparse import OptionParser

# The service that is used to communicate
# with the application via dbus
class DbusService(dbus.service.Object):
    
    # Initializes the DBus service with the 
    # pager to be used
    def __init__( self, a_pager ) :
        self.m_pager = a_pager;
        bus_name = dbus.service.BusName('org.fbpager.control', bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, '/org/fbpager/control')

    # Move one workspace up in the grid
    @dbus.service.method('org.fbpager.control')
    def up(self):
        self.m_pager.move_char( 'u' ) ;

    # Move one workspace down
    @dbus.service.method('org.fbpager.control')
    def down(self):
        self.m_pager.move_char( 'd' ) ;

    # Move one workspace right
    @dbus.service.method('org.fbpager.control')
    def right(self):
        self.m_pager.move_char( 'r' ) ;

    # Move one workspace left
    @dbus.service.method('org.fbpager.control')
    def left(self):
        self.m_pager.move_char( 'l' ) ;

# Class that encapsulates the most of the environment
# for the pager application
class Pager() :
    
    # initialize the Pager with some options
    # for the pager
    def __init__( self, width=2, height=2 ) :
        
        # Create the status icon
        self.m_pager = Gtk.StatusIcon()
        self.m_pager.set_visible( True )

        # Colors for active and inactive
        # windows
        self.m_active_color = (0.3, 0.3, 0.3)
        self.m_inactive_color = ( 0.6, 0.6, 0.6 )
        self.m_padding = 3

        # set the height and width of the pager
        self.m_rows = height
        self.m_cols = width

        # set the number of workspaces if we
        # need to
        nworkspaces = self.m_rows * self.m_cols
        if get_num_workspaces() != nworkspaces:
            set_num_workspaces( nworkspaces )

        # repaint the icon
        self.repaint( get_current_workspace() )

    # main function for drawing the
    # pager icon
    def draw_status_pager( self, selected_workspace  ) :
        
        # the number of pixels on for each one of the
        # squares
        pixels_per_square = int(64 / max(self.m_cols, self.m_rows))

        # general GTK drawing boiler
        surface = ImageSurface( FORMAT_ARGB32, 64, 64 )
        ctx = Context( surface )

        # used for centering
        yoff = (64 - (pixels_per_square * self.m_rows)) / 2
        xoff = (64 - (pixels_per_square * self.m_cols)) / 2

        # what workspace we are on
        counter = 0
        for y in range( self.m_rows ) :
            for x in range( self.m_cols ) :

                # set the color, which is different
                # based on whether or not the workspace
                # is selected
                if counter == selected_workspace:
                    ctx.set_source_rgb( *self.m_active_color )
                else :
                    ctx.set_source_rgb( *self.m_inactive_color )

                # draw a rectangle for the rectangle
                ctx.rectangle( xoff + x * pixels_per_square, yoff + y * pixels_per_square,
                    pixels_per_square - self.m_padding,
                    pixels_per_square - self.m_padding )

                counter += 1
                # fill the rectangle
                ctx.fill()

        # convert the icon to 
        return Gdk.pixbuf_get_from_surface(
                surface, 0, 0,
                surface.get_width(),
                surface.get_height()
        )

    def repaint( self, workspace_num ):
        pixbuf = self.draw_status_pager( workspace_num )
        self.m_pager.set_from_pixbuf( pixbuf )

    # sets the workspace in x,y coordinates
    # in the context of this pager
    def set_workspace( self, x, y ):
        # wrap if the workspace if the
        # coordinates are less than 0
        if x < 0:
            self.set_workspace( self.m_cols + x, y )
        elif y < 0:
            self.set_workspace( x, self.m_rows + y )

        # Calculate the workspace and
        # set it
        else:
            x = x % self.m_cols
            y = y % self.m_rows
            nextn = y * self.m_cols + x

            # utilize xdotool
            subprocess.call(["xdotool", "set_desktop", str(nextn)])
            self.repaint(nextn)

    def move_char( self, ch ):
        # bad code, but still useful
        if ch == 'u':
            (x, y) = self.get_current_workspacexy()
            self.set_workspace( x, y - 1 )
        elif ch == 'd':
            (x, y) = self.get_current_workspacexy()
            self.set_workspace( x, y + 1 )
        elif ch == 'l':
            (x, y) = self.get_current_workspacexy()
            self.set_workspace( x - 1, y )
        elif ch == 'r':
            (x, y) = self.get_current_workspacexy()
            self.set_workspace( x + 1, y )

    # return the current workspace as x,y
    # coordinates in the context of this
    # Pager
    def get_current_workspacexy( self ):
        nworkspace = get_current_workspace()
        return ( nworkspace % self.m_cols, int(nworkspace / self.m_cols) )


# returns the number of workspaces
def get_num_workspaces(  ):
    return int(subprocess.check_output( ["xdotool", "get_num_desktops" ] ))

# returns the number of the current workspaces
def get_current_workspace(  ):
    return int(subprocess.check_output( ["xdotool", "get_desktop" ] ))

# set the number of workspaces
def set_num_workspaces(  numdesktops ):
    return subprocess.call(["xdotool", "set_num_desktops", str(numdesktops)])

# parse the command line arguments
def parseArgs():
    parser = OptionParser()
    parser.add_option( "--width", dest="ncols", default=2,
        help="Set the number of workspaces in a row", metavar="N" )
    parser.add_option( "--height", dest="nrows", default=2,
        help="Set the number of workspaces in a column", metavar="N" )
    (opt, _) = parser.parse_args()
    return opt

if __name__ == '__main__' :
    opts = parseArgs();
    pager = Pager( int(opts.ncols), int(opts.nrows) )

    signal.signal(signal.SIGINT, signal.SIG_DFL)
    DBusGMainLoop(set_as_default=True)
    myservice = DbusService( pager )
    Gtk.main()

