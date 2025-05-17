/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mlx-test.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/15 19:32:01 by abelov            #+#    #+#             */
/*   Updated: 2025/05/15 19:32:08 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MLX_TEST_H
# define MLX_TEST_H

# include <X11/Xlib.h>
# include <X11/keysym.h>
# include <sys/time.h>
# include <mlx.h>

# define WIN_HEIGHT 480
# define WIN_WIDTH 720
# define XPM_TRANSPARENT 0xff000000
# define MLX_TRANSPARENT 0x00000042
# define MLX_DTURQUOISE 0x0000ddcc
# define MLX_TANG_YELLOW 0x00ffcc00

typedef struct s_ivec
{
	int	x;
	int	y;
}	t_ivec;

typedef enum e_transform_type
{
	TRANSFORM_NONE,
	TRANSFORM_ROTATE_CW_90,
	TRANSFORM_ROTATE_CCW_90,
	TRANSFORM_FLIP_H,  // Flip horizontally (mirror over Y)
	TRANSFORM_FLIP_V,  // Flip vertically (mirror over X)
	TRANSFORM_INVALID
}	t_transform_type;

typedef enum e_dir
{
	UP = 0,
	DOWN,
	LEFT,
	RIGHT,
	NO_DIRS
}	t_dir;

typedef struct s_entity
{
	t_ivec	coord;
	t_ivec	dir;
	double	angle;
	t_img	*avatar;
}	t_entity;

typedef struct s_info
{
	t_xvar		*mlx;
	t_win_list	*root;
	int			fullscreen;
	char 		*title;
	t_img		*canvas;
	t_entity	player;
	t_entity	fish;
	int			clip_x_origin;
	int			clip_y_origin;
	size_t		last_frame;
	size_t		fr_delay;
	size_t		framerate;
}	t_info;

int		cleanup(t_info *app);
int		exit_win(t_info *app);
void	mlx_allow_resize_win(Display *display, Window win);
int		render(void *param);
size_t	get_time_us(void);
void	on_expose(t_info *app);
int		key_press(KeySym key, void *param);
void	fill_with_colour(t_img *img, int f_col, int c_col);

t_transform_type get_texture_transform(t_ivec base_dir, t_ivec new_dir);

t_ivec	norm_ivec(t_ivec v);
t_ivec	add_ivec(t_ivec v1, t_ivec v2);
t_ivec	sub_ivec(t_ivec v1, t_ivec v2);

void	memcpy_avx2_nt(void *dst, const void *src, size_t count);
void	memcpy_sse2(void *dst_void, const void *src_void, size_t size);
void	pixcpy_sse2(const t_img *src, const t_img *dst);
void	pix_dup(const t_img *src, const t_img *dst);
void	pix_copy(const t_img *src, const t_img *dst, t_ivec coord);
void	pix_copy_safe(const t_img *src, const t_img *dst, t_ivec coord);
void	copy_row(const u_int32_t *src_row, u_int32_t *dst_row, int width);
void	redraw_img(t_info *const app);
void	rotate90(t_xvar *mlx, t_img *src, t_transform_type transform);
void	flip(t_xvar *mlx, t_img *src, t_transform_type type);
void	rotate90_blit(t_img *dst, t_img *src, t_transform_type transform);
void	flip_blit(t_img *dst, t_img *src, t_transform_type transform);
void	apply_transform(t_info *const app, t_img *img, t_transform_type type);
#endif //MLX_TEST_H
