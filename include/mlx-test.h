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
# define MLX_RED 0x00ff0000
# define MLX_GREEN 0x0000ff00
# define MLX_BLUE 0x000000ff
# define MLX_WHITE 0x00ffffff
# define XPM_TRANSPARENT 0xff000000
# define MLX_TRANSPARENT 0x00000042
# define MLX_DTURQUOISE 0x0000ddcc
# define MLX_TANG_YELLOW 0x00ffcc00

typedef struct s_colour
{
	union
	{
		u_int32_t raw;
#if __BYTE_ORDER == __LITTLE_ENDIAN
		struct
		{
			u_char	b;
			u_char	g;
			u_char	r;
			u_char	a;
		};
#elif __BYTE_ORDER == __BIG_ENDIAN
		struct
		{
			u_char	a;
			u_char	r;
			u_char	g;
			u_char	b;
		};
#else
# error "Unsupported byte order"
#endif
	};
}	t_colour;

typedef struct s_ivec
{
	int	x;
	int	y;
}	t_ivec;

typedef struct s_ivec	t_point;

typedef struct s_fvec
{
	float	x;
	float	y;
}	t_fvec;

typedef struct s_vect
{
	double	x;
	double	y;
}	t_vect;

typedef enum e_transform_type
{
	TR_NONE,
	TR_ROTATE_CW_90,
	TR_ROTATE_CCW_90,
	TR_FLIP_H,  // Flip horizontally (mirror over Y)
	TR_FLIP_V,  // Flip vertically (mirror over X)
	TR_INVALID
}	t_tr_type;

typedef enum e_dir
{
	UP = 0,
	DOWN,
	LEFT,
	RIGHT,
	NO_DIRS
}	t_dir;

# define ANGLE_EPSILON 0.02 // angle blend width (radians)
# define ANGULAR_FEATHER (M_PI / 180.0) // 1 degree
# define RADIAL_FEATHER 1.0
# define R_OUTER 150
# define R_INNER 50
//# define R_OUTER 28
//# define R_INNER 10

typedef struct s_entity
{
	t_ivec	coord;
	t_ivec	direct;
	t_vect	dir;
	double	angle_rad;
	t_img	*src;
	t_img	*src2;
	t_img	*avatar;
}	t_entity;

typedef struct s_info
{
	t_xvar		*mlx;
	t_win_list	*win;
	int			fullscreen;
	char 		*title;
	t_img		*canvas;
	t_entity	player;
	t_entity	fish;
	int			default_color;
	int			clip_x_origin;
	int			clip_y_origin;
	size_t		last_frame;
	size_t		fr_delay;
	size_t		framerate;
	double		fr_scale;
	int			sensitivity;
}	t_info;

typedef struct s_col_name t_col_name;
extern t_col_name mlx_col_name[];

int		cleanup(t_info *app);
int		exit_win(t_info *app);
void	mlx_allow_resize_win(Display *display, Window win);
int		render(void *param);
size_t	get_time_us(void);
void	on_expose(t_info *app);
int		key_press(KeySym key, void *param);

int		mouse_move(int x, int y, void *param);
int		mouse_press(unsigned int button, int x, int y, void *param);
void	rotate_vect_inplace(t_vect *vect, double angle);
void	rotate_player(t_info *app, t_entity *player, int direction, double sensitivity);
void	normalize_angles(double *angle_start, double *angle_end);
void	fill_with_colour(t_img *img, int f_col, int c_col);

t_tr_type get_texture_transform(t_ivec base_dir, t_ivec new_dir);

t_ivec	ivec(int x, int y);
t_ivec	norm_ivec(t_ivec v);
t_ivec	add_ivec(t_ivec v1, t_ivec v2);
t_ivec	sub_ivec(t_ivec v1, t_ivec v2);

void	memcpy_avx2_nt(void *dst, const void *src, size_t count);
void	memcpy_sse2(void *dst_void, const void *src_void, size_t size);
void	pixcpy_sse2(const t_img *src, const t_img *dst);
t_img	*img_dup(t_info *app, t_img *src);
void	pix_dup(t_img *src, t_img *dst);
void	pix_copy(t_img *src, t_img *dst, t_point coord);
void	pix_copy_alpha(t_img *image, t_img *tile, t_point p);
void	pix_copy_safe(const t_img *src, const t_img *dst, t_point coord);
void	copy_row(const u_int32_t *src_row, u_int32_t *dst_row, int width);
void	redraw_img(t_info *const app);
void	rotate90(t_xvar *mlx, t_img *src, t_tr_type transform);
void	flip(t_xvar *mlx, t_img *src, t_tr_type type);
void	rotate90_blit(t_img *dst, t_img *src, t_tr_type transform);
void	flip_blit(t_img *dst, t_img *src, t_tr_type transform);
void	transform(t_info *const app, t_entity *entity, KeySym key);
void	apply_transform(t_info *const app, t_img *img, t_ivec new_dir,
					 t_tr_type type);
void	rotate_img(t_img *src, t_img *dst, double angle_rad);
void	rotate_arbitrary_blit(t_img *dst, t_img *src, double angle_rad);
t_ivec	get_rotated_bounds(int w, int h, float angle);

u_int	interpolate_colour(t_colour *col1, t_colour *col2);
int		interpolate_colour2(int col1, int col2);
void	put_pixel_alpha(t_img *img, t_point p, int base_color, double alpha_frac);
void	put_pixel_alpha2(t_img *img, t_point p, int base_color, double alpha_frac);
void	put_pixel_alpha_blend(t_img *img, t_point p, int base_color, double alpha_frac);
void	put_pixel_alpha_add(t_img *img, t_point p, int base_color, double alpha_frac);
void	draw_circle_wu(t_img *img, t_point c, int r, int color);
void	draw_circle_bresenham(t_img *img, t_point c, int r, int color);
void	draw_circle_on_img(t_img *img, t_point c, int r, int color);

void	draw_circle_stroke(t_img *img, t_point center, int r_outer, int r_inner, int color);
void	draw_circle_stroke_soft(t_img *img, t_point c, int r, int thickness, int color);
void	draw_circle_stroke_v2(t_img *img, t_point center, int r_outer, int r_inner, int color);
void	draw_circle_stroke_v3(t_img *img, t_point c, int r, int thickness, int color);
void	draw_circle_stroke_v4(t_img *img, t_point c, int r, int thickness, int color);
void	draw_circle_stroke_v5(t_img *img, t_point c, int r, int thickness, int color);
void	draw_circle_stroke_v6(t_img *img, t_point c, int r, int thickness, int color);
void	draw_circle_filled(t_img *img, t_point c, int r, int color);
void	draw_ring(t_img *img, t_point c, int r_outer, int r_inner, int color);
void	draw_ring_segment(t_img *img, t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color);
void	draw_ring_segment2(t_img *img, t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color);
void	draw_ring_segment3(t_img *img, t_point c, int r_outer, int r_inner,
						double angle_start, double angle_end, int color);
void	draw_ring_segment4(t_img *img,  t_point c, int r_outer, int r_inner,
						double angle_start, double angle_end, int color);
void	draw_ring_segment5(t_img *img,  t_point c, int r_outer, int r_inner,
						double angle_start, double angle_end, int color);
#endif //MLX_TEST_H
