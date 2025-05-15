/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cleanup.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/15 20:31:39 by abelov            #+#    #+#             */
/*   Updated: 2025/05/15 20:31:39 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx-test.h"

int	cleanup(t_info *app)
{
	if (app->canvas != NULL)
		mlx_destroy_image(app->mlx, app->canvas);
	mlx_destroy_window(app->mlx, app->root);
	mlx_destroy_display(app->mlx);
	free(app->mlx);
	return (0);
}
