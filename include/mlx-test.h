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

typedef struct s_info
{
	t_xvar		*mlx;
	t_win_list	*root;
	int			fullscreen;
	char 		*title;
	t_img		*canvas;
	t_img		*cur;
	t_img		*fish;
	t_ivec		coord;
	t_ivec		fish_coord;
	int			clip_x_origin;
	int			clip_y_origin;
	size_t		last_frame;
	size_t		fr_delay;
}	t_info;

int		cleanup(t_info *app);
int		exit_win(t_info *app);
void	mlx_allow_resize_win(Display *display, Window win);
int		render(void *param);
size_t	get_time_us(void);
void	on_expose(t_info *app);
int		key_press(KeySym key, void *param);
void	fill_with_colour(t_img *img, int f_col, int c_col);


void	memcpy_avx2_nt(void *dst, const void *src, size_t count);
void	memcpy_sse2(void *dst_void, const void *src_void, size_t size);
void	pixcpy_sse2(const t_img *src, const t_img *dst);
void	pix_copy(const t_img *src, const t_img *dst, t_ivec coord);
void	redraw_img(t_info *const app);
#endif //MLX_TEST_H
