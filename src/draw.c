/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   draw.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/18 21:02:05 by abelov            #+#    #+#             */
/*   Updated: 2025/05/18 21:02:06 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <math.h>
#include "mlx-test.h"

u_int	interpolate_colour(t_colour *col1, t_colour *col2)
{
	int	r;
	int	g;
	int	b;

	if (col1->raw == col2->raw)
		return col2->raw;

	const double frac = col1->a / 255.0;
	r = ((col2->r - col1->r) * frac) + col1->r;
	g = ((col2->g - col1->g) * frac) + col1->g;
	b = ((col2->b - col1->b) * frac) + col1->b;
	return ((t_colour){.r = r, .g = g, .b = b}).raw;
}

int	interpolate_colour2(int col1, int col2)
{
	int	r;
	int	g;
	int	b;

	if (col1 == col2)
		return (col1);
	const double frac = (col1 & XPM_TRANSPARENT) / 255.0;
	r = ((col2 & MLX_RED) - (col1 & MLX_RED)) * frac + (col1 & MLX_RED);
	g = ((col2 & MLX_GREEN) - (col1 & MLX_GREEN)) * frac + (col1 & MLX_GREEN);
	b = ((col2 & MLX_BLUE) - (col1 & MLX_BLUE)) * frac + (col1 & MLX_BLUE);
	return ((r & MLX_RED) + (g & MLX_GREEN) + b);
}

void	put_pixel_alpha_alt(t_img *img, t_point p, int base_color, double alpha_frac);
void	put_pixel_alpha_alt(t_img *img, t_point p, int base_color, double alpha_frac)
{
	if (p.x < 0 || p.y < 0 || p.x >= img->width || p.y >= img->height)
		return;

	u_int32_t *dst = (u_int32_t *)img->data + p.y * img->width + p.x;

	// Clamp and convert to 0-255 range
	u_int alpha = (u_int)(alpha_frac * 255.0);
	if (alpha >= 255)
		alpha = 127;

	// Write RGB from base_color and new alpha
	*dst = (alpha << 24) | (base_color & MLX_WHITE);
}
void	put_pixel_alpha(t_img *img, t_point p, int base_color, double alpha_frac)
{
	if (p.x < 0 || p.y < 0 || p.x >= img->width || p.y >= img->height)
		return;

	u_int32_t *dst = (u_int32_t *)img->data + p.y * img->width + p.x;

	// Clamp and convert to 0-255 range
	u_int alpha = (u_int)(alpha_frac * 255.0);
	if (alpha > 255)
		alpha = 255;

	// Write RGB from base_color and new alpha
	*dst = (alpha << 24) | (base_color & MLX_WHITE);
}

void	put_pixel_alpha_add(t_img *img, t_point p, int base_color, double alpha_frac)
{
	int 				alpha;
	u_int				new_alpha;
	u_int32_t * const	dst = (u_int32_t *)img->data + p.y * img->width + p.x;

	if (p.x < 0 || p.y < 0 || p.x >= img->width || p.y >= img->height || alpha_frac <= 0.0)
		return;

	alpha = (int) (alpha_frac * 255.0);
	if (alpha < 0)
		alpha = 0;
	if (alpha > 255)
		alpha = 255;

	if (((t_colour *)dst)->a >= 255)
		return;
	new_alpha = ((t_colour *)dst)->a + alpha;
	if (new_alpha > 255)
		new_alpha = 255;
	*dst = (new_alpha << 24) | (base_color & MLX_WHITE);
}

/**
 * https://en.wikipedia.org/wiki/Xiaolin_Wu's_line_algorithm
 * @param img
 * @param c
 * @param r
 * @param color
 */
void draw_circle_wu(t_img *img, t_point c, int r, int color)
{
	double theta;
	t_vect p;
	t_vect f;
	t_point i;
	t_point cc;

	double step = 1.0 / (r * 8);  // finer steps for smoother edge
	theta = 0;
	while (theta < 2 * M_PI)
	{
		p = (t_vect){.x = cos(theta), .y = sin(theta)};
		i = (t_point){.x = (int)floor(p.x), .y = (int)floor(p.y)};
		f = (t_vect){.x = p.x - i.x, .y = p.y - i.y};

		cc = (t_point){.x = c.x + i.x, .y = c.y + i.y};
		put_pixel_alpha(img, cc, color, (1 - f.x) * (1 - f.y));
		cc = (t_point){.x = c.x + i.x + 1, .y = c.y + i.y};
		put_pixel_alpha(img, cc, color, (f.x) * (1 - f.y));
		cc = (t_point){.x = c.x + i.x, .y = c.y + i.y + 1};
		put_pixel_alpha(img, cc, color, (1 - f.x) * (f.y));
		cc = (t_point){.x =c.x + i.x + 1, .y = c.y + i.y + 1};
		put_pixel_alpha(img, cc, color, (f.x) * (f.y));
		theta += step;
	}
}

/**
 * https://en.wikipedia.org/wiki/Midpoint_circle_algorithm
 * @param img
 * @param c
 * @param r
 * @param color
 */
void	draw_circle_bresenham(t_img *img, t_point c, int r, int color)
{
	int x = 0;
	int y = r;
	int d = 1 - r;

	u_int32_t (*const dst)[img->height][img->width] = (void *)img->data;

	while (x <= y)
	{
		(*dst)[c.y + y][c.x + x] = color;
		(*dst)[c.y + x][c.x + y] = color;
		(*dst)[c.y + y][c.x - x] = color;
		(*dst)[c.y + x][c.x - y] = color;
		(*dst)[c.y - y][c.x + x] = color;
		(*dst)[c.y - x][c.x + y] = color;
		(*dst)[c.y - y][c.x - x] = color;
		(*dst)[c.y - x][c.x - y] = color;

		if (d < 0)
			d += 2 * x + 3;
		else
		{
			d += 2 * (x - y) + 5;
			y--;
		}
		x++;
	}
}

/**
 * Draw using the parametric form of a circle:
 * 	x = r * cos(θ), y = r * sin(θ)
 * @param img
 * @param c
 * @param r
 * @param color
 */
void	draw_circle_on_img(t_img *img, t_point c, int r, int color)
{
	double		theta;
	double		step = 1.0 / r;  // Smaller step for smoother circle
	u_int32_t	*dst_row;

	theta = 0;
	while (theta < 2 * M_PI)
	{
		int x = c.x + (int)(r * cos(theta));
		int y = c.y + (int)(r * sin(theta));
		dst_row = (u_int32_t *) img->data + (y * img->width);
		dst_row[x] = color;
		theta += step;
	}
}

void draw_circle_stroke(t_img *img, t_point c, int r, int thickness, int color)
{
	double step = 1.0 / (r * 8);
	double r_min = r - thickness / 2.0;
	double r_max = r + thickness / 2.0;
	double theta;
	double d;
	t_vect v;
	t_vect p;
	t_vect f;
	t_point i;
	t_point cc;

	theta = 0;
	while (theta < 2 * M_PI)
	{
		v = (t_vect){.x = cos(theta), .y = sin(theta)};
		d = r_min;
		while (d <= r_max)
		{
			p = (t_vect){.x = d * v.x, .y = d * v.y};
			i = (t_point){.x = (int)floor(p.x), .y = (int)floor(p.y)};
			f = (t_vect){.x = p.x - i.x, .y = p.y - i.y};

			cc = (t_point){.x = c.x + i.x, .y = c.y + i.y};
			put_pixel_alpha(img, cc, color, (1 - f.x) * (1 - f.y));
			cc = (t_point){.x = c.x + i.x + 1, .y = c.y + i.y};
			put_pixel_alpha(img, cc, color, (f.x) * (1 - f.y));
			cc = (t_point){.x = c.x + i.x, .y = c.y + i.y + 1};
			put_pixel_alpha(img, cc, color, (1 - f.x) * (f.y));
			cc = (t_point){.x =c.x + i.x + 1, .y = c.y + i.y + 1};
			put_pixel_alpha(img, cc, color, (f.x) * (f.y));
			d += 0.5;
		}
		theta += step;
	}
}

void	draw_circle_filled(t_img *img, t_point c, int r, int color)
{
	t_point cc;
	double dist;
	int y = -r;
	while (++y <= r)
	{
		int x = -r;
		while (++x <= r)
		{
			dist = sqrt(x * x + y * y);
			cc = (t_point){.x = c.x + x, .y = c.y + y};
			double frac = r - dist;
			if (dist <= r - 1.0)
				put_pixel_alpha(img, cc, color, 1.0); // full opacity
			else if (dist <= r)
			{

				put_pixel_alpha(img, cc, color, frac);
			} // fade out
		}
	}
}

void draw_ring(t_img *img, t_point c, int r_outer, int r_inner, int color)
{
	t_point	cc;
	double	dist;
	int		y;

	y = -r_outer;
	while (++y <= r_outer)
	{
		int x = -r_outer;
		while (++x <= r_outer)
		{
			dist = sqrt(x * x + y * y);
			cc = (t_point){.x = c.x + x, .y = c.y + y};
			if (dist < r_inner - 1.0 || dist > r_outer)
				continue;
			else if (dist <= r_inner)
				put_pixel_alpha(img, cc, color, dist - (r_inner - 1));
			else if (dist <= r_outer - 1.0)
				put_pixel_alpha(img, cc, color, 0);
			else
				put_pixel_alpha(img, cc, color, r_outer - dist);
		}
	}
}

static int angle_in_range(double angle, double start, double end)
{
	if (start <= end)
		return angle >= start && angle <= end;
	else
		return angle >= start || angle <= end; // wraps around 2π
}

void	draw_ring_segment(t_img *img, t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color)
{
	int		x;
	int		y;
	double	dist;
	double	angle;
	double	alpha;

	y = -r_outer - 1;
	while (++y <= r_outer)
	{
		x = -r_outer - 1;
		while(++x <= r_outer)
		{
			dist = sqrt(x * x + y * y);
			if (dist < r_inner - 1.0 || dist > r_outer)
				continue;
			angle = atan2(y, x); // from -π to π
			if (angle < 0)
				angle += 2 * M_PI;
			if (angle_in_range(angle, angle_start, angle_end))
			{
				alpha = 1.0;
				if (dist < r_inner)
					alpha = dist - (r_inner - 1.0);
				else if (dist > r_outer - 1.0)
					alpha = r_outer - dist;
				t_point cc = (t_point){.x =  c.x + x, .y = c.y + y};
				put_pixel_alpha(img,cc, color, 1 - alpha);
			}
		}
	}
}



void draw_ring_segment2(t_img *img, t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color)
{
	int		x;
	int		y;
	double	dist;
	double	angle;
	double	alpha;

	y = -r_outer - 1;
	while (++y <= r_outer)
	{
		x = -r_outer - 1;
		while(++x <= r_outer)
		{
			dist = sqrt(x * x + y * y);
			if (dist < r_inner - 1.0 || dist > r_outer)
				continue;
			angle = atan2(y, x); // from -π to π
			if (angle < 0)
				angle += 2 * M_PI;
			if (angle_in_range(angle, angle_start, angle_end))
			{
				alpha = 1.0;
				if (dist < r_inner)
					alpha = dist - (r_inner - 1.0);
				else if (dist > r_outer - 1.0)
					alpha = r_outer - dist;

				t_point cc = (t_point){.x =  c.x + x, .y = c.y + y};
				u_int32_t *dst = (u_int32_t *) img->data + cc.y * img->width + cc.x;

				// Clamp and convert to 0-255 range
				u_int alpha_1 = (u_int) (alpha * 255.0);
				if (alpha_1 >= 255)
					alpha_1 = 127;

				// Write RGB from base_color and new alpha
				*dst = (alpha_1 << 24) | (color & MLX_WHITE);
			}
		}
	}
}

static inline double smoothstep(double edge0, double edge1, double x)
{
	x = (x - edge0) / (edge1 - edge0);
	if (x < 0.0) x = 0.0;
	if (x > 1.0) x = 1.0;
	return x * x * (3 - 2 * x); // classic smoothstep
}

void draw_ring_segment3(t_img *img, t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color)
{
	normalize_angles(&angle_start, &angle_end); // Normalize angles to [0, 2π)
	for (int y = -r_outer - 1; y <= r_outer + 1; ++y)
	{
		for (int x = -r_outer - 1; x <= r_outer + 1; ++x)
		{
			double fx = x + 0.5;
			double fy = y + 0.5;
			double dist = sqrt(fx * fx + fy * fy);

			if (dist > r_outer + 1.0 || dist < r_inner - 1.0)
				continue;

			double angle = atan2(fy, fx);
			if (angle < 0)
				angle += 2 * M_PI;

			if (!angle_in_range(angle, angle_start, angle_end))
				continue;

			// Signed distance to ring band (inner and outer)
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;

			double alpha_outer = smoothstep(1.0, 0.0, d_outer);
			double alpha_inner = smoothstep(1.0, 0.0, d_inner);

			double alpha = 1 - fmin(alpha_outer, alpha_inner);
			put_pixel_alpha(img, (t_point) {.x = c.x + x, .y = c.y + y},
							color, alpha);
		}
	}
}

void draw_ring_segment4(t_img *img,  t_point c, int r_outer, int r_inner,
					   double angle_start, double angle_end, int color)
{
	normalize_angles(&angle_start, &angle_end); // Normalize angles to [0, 2π)

	for (int y = -r_outer - 1; y <= r_outer + 1; ++y)
	{
		for (int x = -r_outer - 1; x <= r_outer + 1; ++x)
		{
			double fx = x + 0.5;
			double fy = y + 0.5;
			double dist = sqrt(fx * fx + fy * fy);
			if (dist > r_outer + 1.0 || dist < r_inner - 1.0)
				continue;

			double angle = atan2(fy, fx);
			if (angle < 0) angle += 2 * M_PI;

			if (!angle_in_range(angle, angle_start, angle_end))
				continue;

			// Radial edge AA
			double da_start = angle - angle_start;
			double da_end   = angle_end - angle;

			if (da_start < 0) da_start += 2 * M_PI;
			if (da_end   < 0) da_end   += 2 * M_PI;

			double a_edge_start = smoothstep(0.0, ANGLE_EPSILON, da_start);
			double a_edge_end   = smoothstep(0.0, ANGLE_EPSILON, da_end);

			// Radius edge AA
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;
			double a_outer = smoothstep(1.0, 0.0, d_outer);
			double a_inner = smoothstep(1.0, 0.0, d_inner);

			double alpha = 1 - fmin(fmin(a_outer, a_inner), fmin(a_edge_start, a_edge_end));

			put_pixel_alpha(img, (t_point) {.x = c.x + x, .y = c.y + y},
							color, alpha);
		}
	}
}