/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   player.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/16 18:17:33 by abelov            #+#    #+#             */
/*   Updated: 2025/05/16 18:17:33 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx-test.h"
#include <math.h>
#include <stdint.h>

t_ivec	get_rotated_bounds(int w, int h, float angle)
{
	const float c = fabsf(cosf(angle));
	const float s = fabsf(sinf(angle));
	const int x = (int)ceilf(w * c + h * s);
	const int y = (int)ceilf(w * s + h * c);

	return (t_ivec){.x = x, .y = y};
}

void rotate_img(t_img *src, t_img *dst, double angle_rad)
{
	t_ivec	b = get_rotated_bounds(src->width, src->height, M_PI_4);
	t_img	*tmp = &(t_img){.width = b.x, .height = b.y};

	tmp->data = malloc(b.x * b.y * sizeof(int));
	fill_with_colour(tmp, XPM_TRANSPARENT, XPM_TRANSPARENT);
	rotate_arbitrary_blit(tmp, src, angle_rad);
	int start_col = b.x / 2 - src->width / 2;
	int start_row = b.y / 2 - src->height / 2;

	u_int32_t	*src_row;
	u_int32_t	*dst_row;

	int y = -1;
	while(++y < src->height)
	{
		src_row = (u_int32_t *) tmp->data + (y + start_row) * tmp->width + start_col;
		dst_row = (u_int32_t *) dst->data + (y * dst->width);
		int x = -1;
		while (++x < src->width)
			dst_row[x] = src_row[x];
	}
	free(tmp->data);
}

void rotate_arbitrary_blit(t_img *dst, t_img *src, double angle_rad)
{
	uint32_t *const	src_data = (uint32_t *)src->data;
	uint32_t *const	dst_data = (uint32_t *)dst->data;

	int sw = src->width;
	int sh = src->height;
	int dw = dst->width;
	int dh = dst->height;



	double cx_src = sw / 2.0;
	double cy_src = sh / 2.0;
	double cx_dst = dw / 2.0;
	double cy_dst = dh / 2.0;

	double cos_a = cos(angle_rad);
	double sin_a = sin(angle_rad);

	int y = -1;
	while (++y < dh)
	{
		int x = -1;
		while (++x < dw)
		{
			// Compute source coords by inverse rotation
			double tx = x - cx_dst;
			double ty = y - cy_dst;

			double sx = tx * cos_a + ty * sin_a + cx_src;
			double sy = -tx * sin_a + ty * cos_a + cy_src;

			int isx = (int)(sx + 0.5f);
			int isy = (int)(sy + 0.5f);

			if (isx >= 0 && isx < sw && isy >= 0 && isy < sh)
			{
				uint32_t pixel = src_data[isy * sw + isx];
				uint32_t mask = -(pixel != XPM_TRANSPARENT);
				dst_data[y * dw + x] = (pixel & mask) | (dst_data[y * dw + x] & ~mask);
			}
		}
	}
}

/**
 * blit is short for bit block transfer.
 *  often written as bitblt (from bit + block transfer).
 *  https://en.wikipedia.org/wiki/Bit_blit
 * @param dst
 * @param src
 * @param angle
 */
void	rotate90_blit(t_img *dst, t_img *src, t_tr_type type)
{
	u_int32_t *src_data = (u_int32_t *)src->data;
	u_int32_t *dst_data = (u_int32_t *)dst->data;
	int sw = src->width;
	int sh = src->height;

	int y = -1;
	while(++y < sh)
	{
		int x = -1;
		while(++x < sw)
		{
			u_int32_t pixel = src_data[y * sw + x];
			u_int32_t mask = -(pixel != XPM_TRANSPARENT);

			int dx = x;
			int dy = y;

			if (type == TR_ROTATE_CCW_90) // (x, y) → (h - 1 - y, x)
			{
				dx = sh - 1 - y;
				dy = x;
			}
			else if (type == TR_ROTATE_CW_90) // (x, y) → (y, w - 1 - x)
			{
				dx = y;
				dy = sw - 1 - x;
			}
			// Destination coordinates after 90° rotation
			int dst_index = dy * dst->width + dx;
			dst_data[dst_index] = (pixel & mask) | (dst_data[dst_index] & ~mask);
		}
	}
}

void flip_blit(t_img *dst, t_img *src, t_tr_type transform)
{
	u_int32_t *src_data = (u_int32_t *)src->data;
	u_int32_t *dst_data = (u_int32_t *)dst->data;
	const t_ivec	dim = (t_ivec){.x = src->width, .y = src->height};

	int y = -1;
	while (++y < dim.y)
	{
		int x = -1;
		while(++x < dim.x)
		{
			u_int32_t pixel = src_data[y * dim.x + x];
			u_int32_t mask = -(pixel != XPM_TRANSPARENT);
			int dx = x;
			int dy = y;
			if (transform == TR_FLIP_H)
				dx = dim.x - 1 - x;
			else if (transform == TR_FLIP_V)
				dy = dim.y - 1 - y;
			int dst_index = dy * dim.x + dx;
			dst_data[dst_index] = (pixel & mask) | (dst_data[dst_index] & ~mask);
		}
	}
}

void	rotate90(t_xvar *mlx, t_img *src, t_tr_type type)
{
	t_img *const tmp =  mlx_new_image(mlx, src->width, src->height);

	fill_with_colour(tmp, XPM_TRANSPARENT, XPM_TRANSPARENT);
	rotate90_blit(tmp, src, type);
	pix_dup(tmp, src);
	mlx_destroy_image(mlx, tmp);
}

void flip(t_xvar *mlx, t_img *src, t_tr_type type)
{
	t_img *const tmp =  mlx_new_image(mlx, src->width, src->height);

	fill_with_colour(tmp, XPM_TRANSPARENT, XPM_TRANSPARENT);
	flip_blit(tmp, src, type);
	pix_dup(tmp, src);
	mlx_destroy_image(mlx, tmp);
}

t_tr_type get_texture_transform(t_ivec base_dir, t_ivec new_dir)
{
	t_tr_type type;

	type = TR_INVALID;

	if (new_dir.x == base_dir.x && new_dir.y == base_dir.y) 	// No change
		type = TR_NONE;
	else if (new_dir.x == -base_dir.x && new_dir.y == -base_dir.y)	// 180° rotation (flip)
		type = ((t_tr_type[]){TR_FLIP_V, TR_FLIP_H}[base_dir.x != 0]);
	else if (new_dir.x == -base_dir.y && new_dir.y == base_dir.x) 	// 90° CCW: (x, y) becomes (-y, x)
		type = TR_ROTATE_CCW_90;
	else if (new_dir.x == base_dir.y && new_dir.y == -base_dir.x)	// 90° CW: (x, y) becomes (y, -x)
		type = TR_ROTATE_CW_90;
	return (type);
}