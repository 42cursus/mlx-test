/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   vector.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/17 03:07:38 by abelov            #+#    #+#             */
/*   Updated: 2025/05/17 03:07:39 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx-test.h"


t_ivec	norm_ivec(t_ivec v)
{
	if (v.x)
		v.x /= v.x;
	if (v.y)
		v.y /= v.y;
	return v;
}

t_ivec	sub_ivec(t_ivec v1, t_ivec v2)
{
	return (t_ivec){.x = v1.x - v2.x, .y = v1.y - v2.y};
}

t_ivec	add_ivec(t_ivec v1, t_ivec v2)
{
	return (t_ivec){.x = v1.x + v2.x, .y = v1.y + v2.y};
}