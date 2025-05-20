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
#include <math.h>
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
	Window win = app->win->window;

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
		mlx_allow_resize_win(app->mlx->display, app->win->window);
	XSendEvent(dpy, app->mlx->root, False,
			   SubstructureNotifyMask | SubstructureRedirectMask,
			   &xev);
	if (!app->fullscreen)
		mlx_int_anti_resize_win(app->mlx, app->win->window,
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
	pix_copy_safe(app->fish.avatar, app->canvas, app->fish.coord);
	pix_copy_alpha(app->canvas, app->player.src2, app->player.coord);
}

void apply_transform(t_info *const app, t_img *img, t_ivec new_dir,
					 t_tr_type type)
{
	if (type == TR_FLIP_H || type == TR_FLIP_V)
		flip(app->mlx, img, type);
	else
		rotate90(app->mlx, img, type);
	app->fish.direct = new_dir;
}

void transform(t_info *const app, t_entity *entity, KeySym key)
{
	int				new_x;
	int				new_y;
	t_ivec			new_dir;
	t_img *const	avatar = entity->avatar;
	t_img *const	canvas = app->canvas;

	new_x = entity->coord.x;
	new_y = entity->coord.y;
	if (key == XK_a) new_x -= avatar->width;
	else if (key == XK_d) new_x += avatar->width;
	else if (key == XK_w) new_y -= avatar->height;
	else if (key == XK_s) new_y += avatar->height;
	new_dir = norm_ivec(sub_ivec(ivec(new_x, new_y), entity->coord));
	apply_transform(app, avatar, new_dir,
					get_texture_transform(entity->direct, new_dir));
	new_x = (new_x + canvas->width) % canvas->width;
	new_y = (new_y + canvas->height) % canvas->height;
	entity->coord = ivec(new_x, new_y);
}

void rotate_vect_inplace(t_vect *vect, double angle)
{
	double temp_x;
	double temp_y;

	temp_x = (vect->x * cos(angle)) - (vect->y * sin(angle));
	temp_y = (vect->x * sin(angle)) + (vect->y * cos(angle));
	vect->x = temp_x;
	vect->y = temp_y;
}

void	rotate_player(t_info *app, t_entity *player, int direction, double sensitivity)
{
	const double angle = (double []){-M_PI_4, M_PI_4}[direction == 0];
	double new_angle;

	rotate_vect_inplace(&player->dir,  angle);
	new_angle = atan2(player->dir.y, player->dir.x);
	rotate_img(app->player.src, app->player.avatar, app->player.angle_rad - new_angle);
	app->player.angle_rad = new_angle;
}

int mouse_move(int x, int y, void *param)
{
	t_info *const app = param;

	int dx = x - WIN_WIDTH / 2;

	if (dx != 0)
	{
//		rotate_player(app, &app->player, dx > 0, app->sensitivity);
//		mlx_mouse_move(app->mlx, app->win, WIN_WIDTH / 2, WIN_HEIGHT / 2); 		// Reset pointer to center

//		XFlush(app->mlx->display);

		double angle_rad = app->player.angle_rad;
		double diff = abs(dx / 2) * M_PI / 360;
		t_point center = (t_point){.x = app->player.src->width / 2, .y = app->player.src->height / 2 };

		if (dx > 0) angle_rad += diff;
		else angle_rad -= diff;
		angle_rad = angle_rad - 2 * M_PI * floor(angle_rad / (2 * M_PI));
//		transform(app, &app->fish, key);
//		rotate_img(app->player.src, app->player.avatar, angle_rad);
		pix_dup(app->player.src, app->player.src2);

		double angle_start = angle_rad - M_1_PI;
		double angle_end = angle_rad + M_1_PI;

		// Normalize angles to [0, 2π)
		if (angle_start < 0) angle_start += 2 * M_PI;
		if (angle_end   < 0) angle_end   += 2 * M_PI;
		if (angle_start >= 2 * M_PI) angle_start = fmod(angle_start, 2 * M_PI);
		if (angle_end   >= 2 * M_PI) angle_end   = fmod(angle_end, 2 * M_PI);

//		draw_ring_segment2(app->player.src2, center, R_OUTER, R_INNER, angle_start, angle_end, app->default_color);
		draw_ring_segment3(app->player.src2, center.x, center.y, R_OUTER, R_INNER, angle_start, angle_end, app->default_color);


		app->player.angle_rad = angle_rad;
		mlx_mouse_move(app->mlx, app->win, WIN_WIDTH / 2, WIN_HEIGHT / 2); 		// Reset pointer to center
		XFlush(app->mlx->display);
		//		redraw_img(app);
	}

	return (0);
	int dy = y - WIN_HEIGHT / 2;
	(void) dy;
}


int mouse_press(unsigned int button, int x, int y, void *param)
{
	t_info *const app = param;


	if (button == 1)
		;
	else if (button == 3)
		;
	else if (button == 5 || button == 4)
	{
		float angle_rad = app->player.angle_rad;

		if (button == 4) angle_rad -= M_1_PI;
		else if (button == 5) angle_rad += M_1_PI;
		rotate_img(app->player.src, app->player.avatar, angle_rad);
		app->player.angle_rad = angle_rad - 2 * M_PI * floor(angle_rad / (2 * M_PI));
		redraw_img(app);
	}
	return (0);
	((void) x, (void) y);
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
	else if (key == XK_Left || key == XK_Right || key == XK_Up || key == XK_Down)
	{
		t_ivec		new_coord;
		new_coord = app->player.coord;

		if (key == XK_Left) new_coord.x -= app->player.avatar->width;
		else if (key == XK_Right) new_coord.x += app->player.avatar->width;
		else if (key == XK_Up) new_coord.y -= app->player.avatar->height;
		else new_coord.y += app->player.avatar->height;
		new_coord.x = (new_coord.x + app->canvas->width) % app->canvas->width;
		new_coord.y = (new_coord.y + app->canvas->height) % app->canvas->height;
		app->player.coord = new_coord;
//		redraw_img(app);
	}
	else if (key == XK_a || key == XK_s || key == XK_d || key == XK_w)
	{
		float angle_rad = app->player.angle_rad;

		if (key == XK_a) angle_rad -= M_PI_4;
		else if (key == XK_d) angle_rad += M_PI_4;
//		transform(app, &app->fish, key);
		rotate_img(app->player.src, app->player.avatar, angle_rad);
		app->player.angle_rad = angle_rad - 2 * M_PI * floor(angle_rad / (2 * M_PI));
//		redraw_img(app);
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
	mlx_put_image_to_window(app->mlx, app->win,
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
	redraw_img(app);
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
	t_info	*const	app = &(t_info){.title = (char *)"mlx-test", .sensitivity = 7, };
	t_img	dummy;

	app->mlx = mlx_init();
	app->canvas = mlx_new_image(app->mlx, WIN_WIDTH, WIN_HEIGHT);
	app->framerate = 100;
	app->fr_delay = 1000000 / app->framerate;
	app->fr_scale = (double)app->framerate / 50.0;
	app->fish.avatar = mlx_xpm_file_to_image(app->mlx, (char *)"lib/minilibx-linux/test/open.xpm", &dummy.width, &dummy.height);
	app->default_color = MLX_DTURQUOISE;
	int i = -1;
	while (mlx_col_name[++i].name)
		if (!strcasecmp(mlx_col_name[i].name, "light slate"))
			app->default_color = mlx_col_name[i].color;


	t_img *dst = mlx_new_image(app->mlx, 400, 400);
	fill_with_colour(dst, XPM_TRANSPARENT, XPM_TRANSPARENT);


	t_point center = (t_point){.x = dst->width / 2, .y = dst->height / 2 };
//	draw_circle_stroke(dst, center, 42, 5, app->default_color);
//	draw_circle_filled(dst, center, 3, app->default_color);

	i = 5;
	while (--i)
		draw_circle_bresenham(dst, center, i, app->default_color);

//	draw_ring(dst, center, 42, 35, app->default_color);
//	draw_circle_filled(dst, center, 42, app->default_color);
	app->player.src = dst;
	app->player.angle_rad = 0;

	t_img *dst2 = mlx_new_image(app->mlx, 400, 400);
	pix_dup(dst, dst2);
	double angle_start = app->player.angle_rad - M_1_PI / 2 +  M_PI;
	double angle_end = app->player.angle_rad + M_1_PI / 2 +  M_PI;


	// Normalize angles to [0, 2π)
	if (angle_start < 0) angle_start += 2 * M_PI;
	if (angle_end   < 0) angle_end   += 2 * M_PI;
	if (angle_start >= 2 * M_PI) angle_start = fmod(angle_start, 2 * M_PI);
	if (angle_end   >= 2 * M_PI) angle_end   = fmod(angle_end, 2 * M_PI);

//	draw_ring_segment2(dst2, center, R_OUTER, R_INNER, angle_start, angle_end, app->default_color);
	draw_ring_segment3(dst2, center.x, center.y, R_OUTER, R_INNER, angle_start, angle_end, app->default_color);

	app->player.src2 = dst2;
	app->player.avatar = img_dup(app, app->player.src);

	app->fish.direct = (t_ivec){.x = -1, .y = 0};
	app->fish.coord = (t_ivec){.x = (WIN_WIDTH / 2) - (app->fish.avatar->width / 2), .y = 100};
	app->player.coord = (t_ivec){.x = (WIN_WIDTH / 2) - (app->player.avatar->width / 2), .y = (WIN_HEIGHT / 2) - (app->player.avatar->height / 2)};
//	redraw_img(app);

	app->win = mlx_new_window(app->mlx, WIN_WIDTH, WIN_HEIGHT, app->title);

	mlx_mouse_hide(app->mlx, app->win);

	mlx_hook(app->win, DestroyNotify, 0, (void *)&exit_win, app);
	mlx_hook(app->win, KeyPress, KeyPressMask, (void *) &key_press, app);
	mlx_hook(app->win, ButtonPress, ButtonPressMask, (void *) &mouse_press, app);
	mlx_hook(app->win, MotionNotify, PointerMotionMask, (void *) &mouse_move, app);
	mlx_loop_hook(app->mlx, &render, app);

	mlx_loop(app->mlx);
	mlx_mouse_show(app->mlx, app->win);
	cleanup(app);
	return (0);
}
