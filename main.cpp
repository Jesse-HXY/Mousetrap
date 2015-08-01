/*

Mousetrap v1.0
a small utility to trap the mouse inside the window it's currently hovering 
over

I wrote this because I like to play games in a window and some games when run 
with WINE don't properly lock the mouse to the window, like Starcraft II for 
instance.

Depends on libX11, notify-send and rw access to /tmp

Get Started:
	sudo apt-get install libx11-dev
	git clone https://github.com/gholken/Mousetrap.git
	cd Mousetrap
	make

Usage:
	put the mousetrap binary somewhere in your path
	setup a hotkey to run the mousetrap bin (I use xfce keyboard settings)
	running it once locks the mouse to whatever window is under the mouse
	running it a second time kills the previous one and unlocks the mouse

Options:
	mousetrap -h -n -t x -b x -l x -r x
 	[-h] display this message 
	[-n] turn off notify 
	[-t offset_top=34] set offset from top of window
	[-b offset_bot=12] set offset from bottom of window
	[-l offset_left=12] set offset from left side of window
	[-r offset_right=12] set offset from right side of window

tested on Xubuntu 14.04

Known issues:
-If you move the window, the locked area does not move with the window
-You can't specify negative values for the border offsets
-Makes Xorg use excessive cpu from XQueryPointer() calls

v1.0 - 07/31/2015 - greg@invisiblespaceship.com
Initial release.
	
*/

#include <X11/Xlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <errno.h>
#include <signal.h>
#include <getopt.h>

Display *display;
bool usenotify = true;

// Get the current window the mouse is over
Window GetMouseWindow( Display *d ) {
	Window unusedwin;
	Window win;
	int unuseddata;
	unsigned int mask;

    do {
    	XQueryPointer(d, DefaultRootWindow(d), &unusedwin, &win,
        &unuseddata, &unuseddata, &unuseddata, &unuseddata, &mask);
    } while(win <= 0);

    return(win);
}

// Check if there is already a running process using a pid file
bool ison() {
	int pid_file = open("/tmp/mousetrap.pid", O_CREAT | O_RDWR, 0666);
	int rc = flock(pid_file, LOCK_EX | LOCK_NB);
	if( rc ) {
		if( errno == EWOULDBLOCK  ) {
			//process is running
			return true;
		}
	} else {
		//process is not running, write the pid file
		pid_t pid = getpid();
		FILE *fp = fopen("/tmp/mousetrap.pid", "w");
		if( !fp ) { exit(1); }
		fprintf(fp, "%d\n", pid);
		fclose(fp);

		return false;
	}
}

int main( int argc, char* argv[] ) {

	int opt;
	int offset_top = 34; // offset the window decorations on top
	int offset_bot = 12;
	int offset_right = 12;
	int offset_left = 12;

	while( (opt = getopt( argc, argv, "nt:b:l:r:hv")) != -1 ) {
		switch(opt) {
			case 'n':
				usenotify = false;
				break;
			case 't':
				offset_top = atoi(optarg);
				break;
			case 'b':
				offset_bot = atoi(optarg);
				break;
			case 'l':
				offset_left = atoi(optarg);
				break;
			case 'r':
				offset_right = atoi(optarg);
				break;
			case 'v':
				printf("Mousetrap v1.0\nCopyright (C) 2015 Greg Holkenbrink\nMIT License see README for details\n");
				break;
			case 'h':
			default:
				fprintf(stderr, "Usage: %s [-h] display this message [-n] turn off notify [-t offset_top=34] [-b offset_bot=12] [-l offset_left=12] [-r offset_right=12]\n", argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	// We want mousetrap to act as a toggle. If it's already 
	// running kill the process, otherwise start up
	if( ison() ) { 
		// kill other process and exit
		pid_t pid = 0;
		FILE *fp = fopen("/tmp/mousetrap.pid", "r");
		if(!fp) { exit(1); }
		if( fscanf(fp, "%d\n", &pid) == 1) {
			kill(pid, SIGKILL);
		}
		unlink("/tmp/mousetrap.pid");
		if( usenotify ) { int ret = system("notify-send -t 2400 \"Mousetrap: OFF\""); }
		exit(0);
	}
	if( usenotify ) { int ret = system("notify-send -t 2400 \"Mousetrap: ON\""); }

	display = XOpenDisplay("");
	if( display == NULL ) {
		printf("Cannot connect to xserver\n");
		exit(-1);
	}

	Window root_window = DefaultRootWindow(display);
	Window current_window = GetMouseWindow(display);
	XWindowAttributes info_data;
	Status info = XGetWindowAttributes( display, current_window, &info_data);

	int max_x = info_data.x+info_data.width;
	int max_y = info_data.y+info_data.height;
	int min_x = info_data.x;
	int min_y = info_data.y;

	do {
		current_window = GetMouseWindow(display);

		Window unusedwin;
		Window win;
		int unuseddata;
		int cur_x;
		int cur_y;
		unsigned int mask;

		//NOTE: this could be refactored so that XQueryPointer is called once
		// and then the mouse position could be updated from reading /dev/input
		// because right now this uses lots of CPU
		// see: http://stackoverflow.com/questions/11519759/how-to-read-low-level-mouse-click-position-in-linux
    	XQueryPointer(display, current_window, &unusedwin, &win, &cur_x, &cur_y, &unuseddata, &unuseddata, &mask);

		bool warp = false;

		if( cur_x > max_x-offset_right ) { warp = true; cur_x = max_x-offset_right; }
		if( cur_y > max_y-offset_bot ) { warp = true; cur_y = max_y-offset_bot; }
		if( cur_x < min_x+offset_left ) { warp = true; cur_x = min_x+offset_left; }
		if( cur_y < min_y+offset_top ) { warp = true; cur_y = min_y+offset_top; }
		
		if( warp ) { XWarpPointer( display, None, root_window, 0, 0, 0, 0, cur_x, cur_y ); }
		//XFlush(display);

		usleep(300);
	} while(true);
	
	XCloseDisplay(display);
	return 0;
}