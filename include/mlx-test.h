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

typedef struct s_info
{
	t_xvar		*mlx;
	t_win_list	*root;
	int			fullscreen;
	char 		*title;
	t_img		*canvas;
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

#endif //MLX_TEST_H
