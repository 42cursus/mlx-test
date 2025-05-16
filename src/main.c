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

#include <sysexits.h>
#include "mlx-test.h"


void mlx_allow_resize_win(Display *display, Window win)
{
	XSizeHints	hints;
	long		supplied;

	if (XGetWMNormalHints(display, win, &hints, &supplied))
	{
		hints.min_width = 1;
		hints.min_height = 1;
		hints.max_width = DisplayWidth(display, DefaultScreen(display));
		hints.max_height = DisplayHeight(display, DefaultScreen(display));
		hints.flags &= ~(PMinSize | PMaxSize);
		XSetWMNormalHints(display, win, &hints);
	}
}

static void toggle_fullscreen(t_info *const app)
{
	Display *dpy = app->mlx->display;
	Window win = app->root->window;

	Atom wm_fullscreen = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);

	XEvent xev = {0};
	xev.type = ClientMessage;
	xev.xclient.window = win;
	xev.xclient.message_type = XInternAtom(dpy, "_NET_WM_STATE", False);
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = (long[]){0, 1}[app->fullscreen];
	xev.xclient.data.l[1] = (long)wm_fullscreen;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 1;
	xev.xclient.data.l[4] = 0;
	if (app->fullscreen)
		mlx_allow_resize_win(app->mlx->display, app->root->window);
	XSendEvent(dpy, app->mlx->root, False,
			   SubstructureNotifyMask | SubstructureRedirectMask,
			   &xev);
	if (!app->fullscreen)
		mlx_int_anti_resize_win(app->mlx, app->root->window,
								WIN_WIDTH, WIN_HEIGHT);
}

__attribute__ ((noreturn))
int	exit_win(t_info *const	app)
{
	cleanup(app);
	_exit(EX_OK);
}

void redraw_img(t_info *const app)
{
	fill_with_colour(app->canvas, MLX_TANG_YELLOW, MLX_DTURQUOISE);
	pix_copy(app->cur, app->canvas, app->coord);
	pix_copy(app->fish, app->canvas, app->fish_coord);
}

int key_press(KeySym key, void *param)
{
	t_info *const app = param;

	if (key == XK_F11)
	{
		app->fullscreen = !app->fullscreen;
		toggle_fullscreen(app);
		mlx_do_sync(app->mlx);
	}
	else if (key == XK_Escape)
		app->mlx->end_loop = 1;
	else
	{
		if (key == XK_Left)
			app->coord.x =
				(app->coord.x - app->cur->width + app->canvas->width) %
				app->canvas->width;
		else if (key == XK_Right)
			app->coord.x =
				(app->coord.x + app->cur->width + app->canvas->width) %
				app->canvas->width;
		else if (key == XK_Up)
			app->coord.y =
				(app->coord.y - app->cur->height + app->canvas->height) %
				app->canvas->height;
		else if (key == XK_Down)
			app->coord.y =
				(app->coord.y + app->cur->height + app->canvas->height) %
				app->canvas->height;
		else if (key == XK_a)
			app->fish_coord.x =
				(app->fish_coord.x - app->cur->width + app->canvas->width) %
				app->canvas->width;
		else if (key == XK_d)
			app->fish_coord.x =
				(app->fish_coord.x + app->cur->width + app->canvas->width) %
				app->canvas->width;
		else if (key == XK_w)
			app->fish_coord.y =
				(app->coord.y - app->cur->height + app->canvas->height) %
				app->canvas->height;
		else if (key == XK_s)
			app->fish_coord.y =
				(app->fish_coord.y + app->cur->height + app->canvas->height) %
				app->canvas->height;
		redraw_img(app);
	}

	return (0);
}

size_t	get_time_us(void)
{
	struct timeval	current_time;

	gettimeofday(&current_time, NULL);
	return (current_time.tv_sec * 1000000 + current_time.tv_usec);
}

void on_expose(t_info *app)
{
	mlx_put_image_to_window(app->mlx, app->root,
							app->canvas, app->clip_x_origin,
							app->clip_y_origin);
}

int	render(void *param)
{
	size_t time;
	t_info *const app = param;

	time = get_time_us();
	app->last_frame = time;
	while (get_time_us() - app->last_frame < app->fr_delay)
		usleep(100);
	on_expose(app);
	return (0);
}

void fill_with_colour(t_img *img, int f_col, int c_col)
{
	const int	mid = img->height / 2;
	int			i;
	int			j;

	u_int (*pixels)[img->height][img->width] = (void *)img->data;
	i = -1;
	while (++i <= mid)
	{
		j = -1;
		while (++j < img->width)
			(*pixels)[i][j] = c_col;
	}
	i--;
	while (++i < img->height)
	{
		j = -1;
		while (++j < img->width)
			(*pixels)[i][j] = f_col;
	}
}

__attribute__ ((noreturn))
int main(void)
{
	t_info	*const	app = &(t_info){.title = (char *)"mlx-test"};

	app->mlx = mlx_init();
	app->root = mlx_new_window(app->mlx, WIN_WIDTH,
						WIN_HEIGHT, app->title);
	app->canvas = mlx_new_image(app->mlx, WIN_WIDTH, WIN_HEIGHT);

	mlx_hook(app->root, DestroyNotify, 0, (void *)&exit_win, app);
	mlx_hook(app->root, KeyPress, KeyPressMask, (void *) &key_press, app);


	t_img dummy;
	app->fish_coord = (t_ivec){.x = 300, .y = 40};
	app->fish = mlx_xpm_file_to_image(app->mlx, (char *) "lib/minilibx-linux/test/open.xpm", &dummy.width, &dummy.height);
	app->cur = mlx_xpm_file_to_image(app->mlx, (char *) "textures/map_pointer.xpm", &dummy.width, &dummy.height);
	redraw_img(app);

	mlx_loop_hook(app->mlx, &render, app);
	mlx_loop(app->mlx);

	exit_win(app);
}
