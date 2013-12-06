#!/usr/bin/python

from gi.repository import Gtk, Gdk, GLib, Pango, PangoCairo
from cairo import Context, ImageSurface, RadialGradient, FORMAT_ARGB32

import signal
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop

import subprocess
from optparse import OptionParser

class DbusService(dbus.service.Object):
    def __init__( self, icon ) :
        self.m_icon = icon;
        bus_name = dbus.service.BusName('org.fbpager.control', bus=dbus.SessionBus())
        dbus.service.Object.__init__(self, bus_name, '/org/fbpager/control')

    @dbus.service.method('org.fbpager.control')
    def up(self):
        self.m_icon.move_char( 'u' ) ;

    @dbus.service.method('org.fbpager.control')
    def down(self):
        self.m_icon.move_char( 'd' ) ;

    @dbus.service.method('org.fbpager.control')
    def right(self):
        self.m_icon.move_char( 'r' ) ;

    @dbus.service.method('org.fbpager.control')
    def left(self):
        self.m_icon.move_char( 'l' ) ;

class TrayIcon :
    def __init__( self, width=2, height=2 ) :
        self.m_icon = Gtk.StatusIcon()
        self.m_icon.set_visible( True )
       # self.m_icon.connect( 'popup-menu', self.on_showpopup )

        self.m_active_color = (0.3,0.3,0.3)
        self.m_inactive_color = ( 0.6, 0.6, 0.6 )
        self.m_padding = 3

        self.m_rows = height
        self.m_cols = width

        nworkspaces = self.m_rows * self.m_cols
        if self.get_num_workspaces() != nworkspaces:
            self.set_num_workspaces( nworkspaces )

        self.repaint( self.get_current_workspace() )

    def draw_status_icon( self, selected_workspace  ) :
        pixels_per_square = int(64 / max(self.m_cols, self.m_rows))

        surface = ImageSurface( FORMAT_ARGB32, 64, 64 )
        ctx = Context( surface )

        yoff = (64 - (pixels_per_square * self.m_rows)) / 2
        xoff = (64 - (pixels_per_square * self.m_cols)) / 2

        counter = 0
        for y in range( self.m_rows ) :
            for x in range( self.m_cols ) :
#                print ('drawing rectangle')

                if counter == selected_workspace:
                    ctx.set_source_rgb( *self.m_active_color )
                else :
                    ctx.set_source_rgb( *self.m_inactive_color )

                ctx.rectangle( xoff + x * pixels_per_square, yoff + y * pixels_per_square,
                    pixels_per_square - self.m_padding,
                    pixels_per_square - self.m_padding )
                counter += 1

                ctx.fill()
        return Gdk.pixbuf_get_from_surface(
                surface, 0, 0,
                surface.get_width(),
                surface.get_height()
        )

    def repaint( self, workspace_num ):
        pixbuf = self.draw_status_icon( workspace_num )
        self.m_icon.set_from_pixbuf( pixbuf )

    def get_num_workspaces( self ):
        return int(subprocess.check_output( ["xdotool", "get_num_desktops" ] ))

    def get_current_workspacexy( self ):
        nworkspace = self.get_current_workspace()
        return ( nworkspace % self.m_cols, int(nworkspace / self.m_cols) )
    def get_current_workspace( self ):
        return int(subprocess.check_output( ["xdotool", "get_desktop" ] ))

    def set_num_workspaces( self, numdesktops ):
        return subprocess.call(["xdotool", "set_num_desktops", str(numdesktops)])

    def set_workspace( self, x, y ):
#        print ( "setting workspace to (%d,%d)\n" % (x,y) )
        if x < 0:
            self.set_workspace( self.m_cols + x, y )
        elif y < 0:
            self.set_workspace( x, self.m_rows + y )
        else:
#            print ( "(rows,cols) = (%d,%d)\n" % (self.m_rows,self.m_cols) )
            x = x % self.m_cols
            y = y % self.m_rows
            nextn = y * self.m_cols + x
            subprocess.call(["xdotool", "set_desktop", str(nextn)])
            self.repaint(nextn)
        # self.repaint( self.get_current_workspace() )

    def move_char( self, ch ):
        if ch == 'u':
            (x,y) = self.get_current_workspacexy()
            self.set_workspace( x, y - 1 )
        elif ch == 'd':
            (x,y) = self.get_current_workspacexy()
            self.set_workspace( x, y + 1 )
        elif ch == 'l':
            (x,y) = self.get_current_workspacexy()
            self.set_workspace( x - 1, y )
        elif ch == 'r':
            (x,y) = self.get_current_workspacexy()
            self.set_workspace( x + 1, y )

def parseArgs():
    parser = OptionParser()
    parser.add_option( "--width", dest="ncols", default=2,
        help="Set the number of workspaces in a row", metavar="N" )
    parser.add_option( "--height", dest="nrows", default=2,
        help="Set the number of workspaces in a column", metavar="N" )
    (opts,args) = parser.parse_args()
    return opts

if __name__ == '__main__' :
    opts = parseArgs();
    icon = TrayIcon( int(opts.ncols), int(opts.nrows) )

    signal.signal(signal.SIGINT, signal.SIG_DFL)
    DBusGMainLoop(set_as_default=True)
    myservice = DbusService( icon )
    Gtk.main()

