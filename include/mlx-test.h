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
# include <mlx.h>

typedef struct s_info
{
	t_xvar		*mlx;
	t_win_list	*root;
	int			fullscreen;
}	t_info;

#endif //MLX_TEST_H
