/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/09 16:20:01 by abelov            #+#    #+#             */
/*   Updated: 2025/05/09 16:20:02 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx-test.h"

static void set_fullscreen(Display *dpy, Window win, int fullscreen)
{
	Atom wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
	Atom wm_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent xev = {0};
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = wm_state;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = fullscreen ? 1
									   : 0; // 1 = add fullscreen, 0 = remove fullscreen
	xev.xclient.data.l[1] = wm_fullscreen;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 1;
	xev.xclient.data.l[4] = 0;

	XSendEvent(dpy, DefaultRootWindow(dpy), False,
			   SubstructureNotifyMask | SubstructureRedirectMask,
			   &xev);
}

int main(void)
{
	t_info *const	app = &(t_info){.title = (char *)"mlx-test"};

	app->mlx = mlx_init();
	Display	*dpy = app->mlx->display;
	int		screen = app->mlx->screen;
	Window	root = app->mlx->root;
	Window	win = XCreateSimpleWindow(dpy, root, 10, 10, 640, 480, 1,
									 BlackPixel(dpy, screen),
									 WhitePixel(dpy, screen));
	XSelectInput(dpy, win, KeyPressMask | StructureNotifyMask);
	XMapWindow(dpy, win);

	while (1)
	{
		XEvent ev;
		XNextEvent(dpy, &ev);

		if (ev.type == KeyPress)
		{
			KeySym keysym = XLookupKeysym(&ev.xkey, 0);
			if (keysym == XK_F11)
			{
				app->fullscreen = !app->fullscreen;
				set_fullscreen(dpy, win, app->fullscreen);
			}
			if (keysym == XK_Escape)
				break; // exit on ESC
		}
	}

	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
	free(app->mlx);
	return 0;
}
