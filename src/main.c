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

//__attribute__ ((noreturn))
int	exit_win(t_info *const	app)
{
	app->mlx->end_loop = 1;
//	_exit(EX_OK);
	return (0);
}

void redraw_img(t_info *const app)
{
	fill_with_colour(app->canvas, MLX_TANG_YELLOW, MLX_DTURQUOISE);
	pix_copy(app->player.avatar, app->canvas, app->player.coord);
	pix_copy_safe(app->fish.avatar, app->canvas, app->fish.coord);
}

void apply_transform(t_info *const app, t_img *img, t_transform_type type)
{
	if (type == TRANSFORM_FLIP_H || type == TRANSFORM_FLIP_V)
		flip(app->mlx, img, type);
	else
		rotate90(app->mlx, img, type);
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
		t_ivec		new_coord;
		t_ivec		delta;

		if (key == XK_Left)
		{
			new_coord = app->player.coord;
			new_coord.x = (new_coord.x - app->player.avatar->width + app->canvas->width) %
				app->canvas->width;
			app->player.coord = new_coord;
			rotate90(app->mlx, app->fish.avatar, TRANSFORM_ROTATE_CCW_90);
		}
		else if (key == XK_Right)
		{
			app->player.coord.x =
				(app->player.coord.x + app->player.avatar->width + app->canvas->width) %
				app->canvas->width;
			rotate90(app->mlx, app->fish.avatar, TRANSFORM_ROTATE_CW_90);
		}
		else if (key == XK_Up)
			app->player.coord.y =
				(app->player.coord.y - app->player.avatar->height + app->canvas->height) %
				app->canvas->height;
		else if (key == XK_Down)
			app->player.coord.y =
				(app->player.coord.y + app->player.avatar->height + app->canvas->height) %
				app->canvas->height;
		else if (key == XK_a)
		{
			new_coord = app->fish.coord;
			new_coord.x = (new_coord.x - app->fish.avatar->width + app->canvas->width) % app->canvas->width;
			delta = norm_ivec(sub_ivec(new_coord, app->fish.coord));
			apply_transform(app, app->fish.avatar, get_texture_transform(app->fish.dir, delta));
			app->fish.coord = new_coord;
		}
		else if (key == XK_d)
		{
			new_coord = app->fish.coord;
			new_coord.x = (new_coord.x + app->fish.avatar->width + app->canvas->width) % app->canvas->width;
			delta = norm_ivec(sub_ivec(new_coord, app->fish.coord));
			apply_transform(app, app->fish.avatar, get_texture_transform(app->fish.dir, delta));
			app->fish.coord = new_coord;
		}
		else if (key == XK_w)
		{
			new_coord = app->fish.coord;
			new_coord.y = (new_coord.y - app->fish.avatar->height + app->canvas->height) % app->canvas->height;
			delta = norm_ivec(sub_ivec(new_coord, app->fish.coord));
			apply_transform(app, app->fish.avatar, get_texture_transform(app->fish.dir, delta));
			app->fish.coord = new_coord;
		}
		else if (key == XK_s)
		{
			new_coord = app->fish.coord;
			new_coord.y = (new_coord.y + app->fish.avatar->height + app->canvas->height) % app->canvas->height;
			delta = norm_ivec(sub_ivec(new_coord, app->fish.coord));
			apply_transform(app, app->fish.avatar, get_texture_transform(app->fish.dir, delta));
			app->fish.coord = new_coord;
		}
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
		usleep(500);
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

int main(void)
{
	t_info	*const	app = &(t_info){.title = (char *)"mlx-test"};
	t_img	dummy;

	app->mlx = mlx_init();
	app->canvas = mlx_new_image(app->mlx, WIN_WIDTH, WIN_HEIGHT);
	app->framerate = 100;
	app->fr_delay = 1000000 / app->framerate;
	app->fish.avatar = mlx_xpm_file_to_image(app->mlx, (char *)"lib/minilibx-linux/test/open.xpm", &dummy.width, &dummy.height);
	app->player.avatar = mlx_xpm_file_to_image(app->mlx, (char *)"textures/map_pointer.xpm", &dummy.width, &dummy.height);
	app->fish.dir = (t_ivec){.x = -1, .y = 0};
	app->fish.coord = (t_ivec){.x = (WIN_WIDTH / 2) - (app->fish.avatar->width / 2), .y = 0};
	app->player.coord = (t_ivec){.x = (WIN_WIDTH / 2) - (app->player.avatar->width / 2), .y = (WIN_HEIGHT / 2) - (app->player.avatar->height / 2)};
	redraw_img(app);

	app->root = mlx_new_window(app->mlx, WIN_WIDTH, WIN_HEIGHT, app->title);

	mlx_hook(app->root, DestroyNotify, 0, (void *)&exit_win, app);
	mlx_hook(app->root, KeyPress, KeyPressMask, (void *) &key_press, app);
	mlx_loop_hook(app->mlx, &render, app);

	mlx_loop(app->mlx);

	cleanup(app);
	return (0);
}
