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

void	put_pixel_alpha_blend(t_img *img, t_point p, int base_color, double alpha_frac)
{
	if (p.x < 0 || p.y < 0 || p.x >= img->width || p.y >= img->height)
		return;

	u_int32_t *dst = (u_int32_t *)img->data + p.y * img->width + p.x;
	t_colour src = {.raw = base_color};
	t_colour dst_col = {.raw = *dst};

	double inv_alpha = 1.0 - alpha_frac;

	t_colour result = {
		.r = src.r * alpha_frac + dst_col.r * inv_alpha,
		.g = src.g * alpha_frac + dst_col.g * inv_alpha,
		.b = src.b * alpha_frac + dst_col.b * inv_alpha,
		.a = 255
	};

	*dst = result.raw;
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

static void	put_pixel_alpha_alt(t_img *img, t_point p, int base_color, double alpha_frac)
{
	put_pixel_alpha(img, p, base_color, alpha_frac);
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

			// Bilinear blend over 4 surrounding pixels
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

static inline double smoothstep(double edge0, double edge1, double x)
{
	x = (x - edge0) / (edge1 - edge0);
	if (x < 0.0) x = 0.0;
	if (x > 1.0) x = 1.0;
	return x * x * (3 - 2 * x); // classic smoothstep
}


void	draw_circle_stroke_soft(t_img *img, t_point c, int r, int thickness, int color)
{
	double step = 1.0 / (r * 8);
	double r_min = r - thickness / 2.0;
	double r_max = r + thickness / 2.0;
	double theta;
	double d;
	t_vect v, p, f;
	t_point i, cc;
	double dist, a_inner, a_outer, radial_alpha, alpha;

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

			// Radial AA: smooth edge falloff on r_min and r_max
			dist = sqrt(p.x * p.x + p.y * p.y);
			a_inner = smoothstep(r_min - 1.0, r_min, dist);
			a_outer = 1.0 - smoothstep(r_max, r_max + 1.0, dist);
			radial_alpha = fmin(a_inner, a_outer); // combined edge AA

			// Bilinear blend over 4 surrounding pixels
			alpha = radial_alpha * (1 - f.x) * (1 - f.y);
			cc = (t_point){.x = c.x + i.x, .y = c.y + i.y};
			put_pixel_alpha(img, cc, color, alpha);

			alpha = radial_alpha * (f.x) * (1 - f.y);
			cc = (t_point){.x = c.x + i.x + 1, .y = c.y + i.y};
			put_pixel_alpha(img, cc, color, alpha);

			alpha = radial_alpha * (1 - f.x) * (f.y);
			cc = (t_point){.x = c.x + i.x, .y = c.y + i.y + 1};
			put_pixel_alpha(img, cc, color, alpha);

			alpha = radial_alpha * (f.x) * (f.y);
			cc = (t_point){.x = c.x + i.x + 1, .y = c.y + i.y + 1};
			put_pixel_alpha(img, cc, color, alpha);

			d += 0.5;
		}
		theta += step;
	}
}


void draw_circle_stroke_v2(t_img *img, t_point center, int r_outer, int r_inner, int color)
{
	const double TWO_PI = 2.0 * M_PI;
	const double dtheta = 1.0 / r_outer; // Step size based on radius (smooth sweep)
	const double sample_step = 0.5; // Subpixel radial step

	for (double theta = 0; theta < TWO_PI; theta += dtheta)
	{
		double cos_t = cos(theta);
		double sin_t = sin(theta);

		for (double r = r_inner - 1.0; r <= r_outer + 1.0; r += sample_step)
		{
			double fx = r * cos_t;
			double fy = r * sin_t;
			double dist = sqrt(fx * fx + fy * fy);

			// Radial AA with smoothstep
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;
			double a_outer = smoothstep(1.0, 0.0, d_outer);
			double a_inner = smoothstep(1.0, 0.0, d_inner);
			double alpha = 1 - fmin(a_outer, a_inner);

			// Convert to integer pixel coordinate
			int px = (int)round(center.x + fx);
			int py = (int)round(center.y + fy);

			t_point p = {.x = px, .y = py};
			put_pixel_alpha(img, p, color, alpha);
		}
	}
}


// Draw a stroked circle centered at c with radius r and thickness.
// Edges are anti-aliased with smoothstep and bilinear blending.
void draw_circle_stroke_v3(t_img *img, t_point c, int r, int thickness, int color)
{
	const double TWO_PI = 2.0 * M_PI;

	const double r_inner = r - thickness / 2.0;
	const double r_outer = r + thickness / 2.0;

	// Angular step to cover circumference with roughly 1 pixel per step
	const double dtheta = 1.0 / r;

	// Radial step for smooth AA inside stroke thickness
	const double dr = 0.25;

	for (double theta = 0; theta < TWO_PI; theta += dtheta)
	{
		// Cartesian coords relative to center
		double cos_t = cos(theta);
		double sin_t = sin(theta);

		for (double dist = r_inner - 1.0; dist <= r_outer + 1.0; dist += dr)
		{
			// Cartesian coords relative to center
			double fx = dist * cos_t;
			double fy = dist * sin_t;

			// Distance to inner and outer edges (for alpha)
			double d_inner = r_inner - dist;
			double d_outer = dist - r_outer;

			// Alpha from smoothstep for inner and outer edge blending
			double a_inner = smoothstep(1.0, 0.0, d_inner); // fades in near inner edge
			double a_outer = smoothstep(1.0, 0.0, d_outer); // fades out near outer edge

			double alpha = 1 - fmin(a_inner, a_outer);
//			if (alpha >= 1.0) continue;  // Skip fully transparent

			// Pixel integer coordinates (top-left pixel of bilinear cell)
			int ix = (int)floor(c.x + fx);
			int iy = (int)floor(c.y + fy);

			// Fractional part inside pixel cell
			double fx_frac = (c.x + fx) - ix;
			double fy_frac = (c.y + fy) - iy;

			// Bilinear blending to 4 pixels with weights scaled by alpha
			t_point p;

			p = (t_point){ix, iy};
			put_pixel_alpha(img, p, color, alpha * fx_frac * fy_frac);

			p = (t_point){ix + 1, iy};
			put_pixel_alpha(img, p, color, alpha * (1.0 - fx_frac) * fy_frac);

			p = (t_point){ix, iy + 1};
			put_pixel_alpha(img, p, color, alpha * fx_frac * (1.0 - fy_frac));

			p = (t_point){ix + 1, iy + 1};
			put_pixel_alpha(img, p, color, alpha * (1.0 - fx_frac) * (1.0 - fy_frac));
		}
	}
}

void draw_circle_stroke_v4(t_img *img, t_point c, int r, int thickness, int color)
{
	const double TWO_PI = 2.0 * M_PI;
	const double r_inner = r - thickness / 2.0;
	const double r_outer = r + thickness / 2.0;
	const double dtheta = 1.0 / r;
	const double dr = 0.25;

	for (double theta = 0; theta < TWO_PI; theta += dtheta)
	{
		double cos_t = cos(theta);
		double sin_t = sin(theta);

		for (double dist = r_inner - 1.0; dist <= r_outer + 1.0; dist += dr)
		{
			double fx = dist * cos_t;
			double fy = dist * sin_t;

			double d_inner = dist - r_inner;
			double d_outer = r_outer - dist;

			double a_inner = smoothstep(1.0, 0.0, d_inner);
			double a_outer = smoothstep(1.0, 0.0, d_outer);

			double alpha = 1 - fmin(a_inner, a_outer);
//			if (alpha <= 0.0) continue;

			int ix = (int)floor(c.x + fx);
			int iy = (int)floor(c.y + fy);

			double fx_frac = (c.x + fx) - ix;
			double fy_frac = (c.y + fy) - iy;

			int alpha_tl = (int)((alpha * (1.0 - fx_frac) * (1.0 - fy_frac)));
			int alpha_tr = (int)((alpha * fx_frac * (1.0 - fy_frac)));
			int alpha_bl = (int)((alpha * (1.0 - fx_frac) * fy_frac));
			int alpha_br = (int)((alpha * fx_frac * fy_frac));

			t_point p;

			p = (t_point){ix, iy};
			put_pixel_alpha(img, p, color, alpha_tl);

			p = (t_point){ix + 1, iy};
			put_pixel_alpha(img, p, color, alpha_tr);

			p = (t_point){ix, iy + 1};
			put_pixel_alpha(img, p, color, alpha_bl);

			p = (t_point){ix + 1, iy + 1};
			put_pixel_alpha(img, p, color, alpha_br);
		}
	}
}




void draw_circle_stroke_v5(t_img *img, t_point c, int r, int thickness, int color)
{
	const double r_inner = r - thickness / 2.0;
	const double r_outer = r + thickness / 2.0;

	const double TWO_PI = 2.0 * M_PI;

	const double dtheta = 1.0 / r;    // Angular resolution

	const double dr = 0.25;                 // Radial subpixel resolution

	for (double theta = 0; theta < TWO_PI; theta += dtheta)
	{
		double cos_t = cos(theta);
		double sin_t = sin(theta);

		for (double dist = r_inner - 1.0; dist <= r_outer + 1.0; dist += dr)
		{
			// Cartesian coords relative to center
			double fx = dist * cos_t;
			double fy = dist * sin_t;

			// Anti-aliasing for radial edges
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;
			double a_outer = smoothstep(1.0, 0.0, d_outer); // closer to r_outer: 0
			double a_inner = smoothstep(1.0, 0.0, d_inner); // closer to r_inner: 0
			double alpha = 1 - fmin(a_outer, a_inner); // 0 is opaque, 1 is transparent

			// Convert to pixel coordinates
			int px = (int)floor(c.x + fx);
			int py = (int)floor(c.y + fy);
			t_point p = {.x = px, .y = py};

			put_pixel_alpha(img, p, color, alpha);
		}
	}
}
/**
 * draw_circle_stroke is meant to produce something that looks like tick marks
 * 	or stroke rings you'd find on the dial of a wristwatch — smooth,
 * 	thin, curved lines forming a perfect ring (like the boundary of the dial,
 * 	or minute/second tick marks drawn as arcs or partial rings):
 *
 * @param img
 * @param c
 * @param r
 * @param thickness
 * @param color
 */
void draw_circle_stroke_v6(t_img *img, t_point c, int r, int thickness, int color)
{
	const double r_inner = r - thickness / 2.0;
	const double r_outer = r + thickness / 2.0;

	const double TWO_PI = 2.0 * M_PI;
	const double dtheta = 1.0 / r * 32;
	const double dr = 0.25;

	for (double theta = 0; theta < TWO_PI; theta += dtheta)
	{
		double cos_t = cos(theta);
		double sin_t = sin(theta);

		for (double dist = r_inner - 1.0; dist <= r_outer + 1.0; dist += dr)
		{
			double fx = dist * cos_t;
			double fy = dist * sin_t;

			// Anti-aliasing weights
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;
			double a_outer = smoothstep(1.0, 0.0, d_outer);
			double a_inner = smoothstep(1.0, 0.0, d_inner);
			double coverage = 1 -  fmin(a_outer, a_inner);  // 0=opaque, 1=transparent

			// Bilinear subpixel interpolation
			int base_x = (int)floor(fx);
			int base_y = (int)floor(fy);
			double sub_x = fx - base_x;
			double sub_y = fy - base_y;

//			double w00 = (1 - sub_x) * (1 - sub_y); // top-left
//			double w10 = sub_x * (1 - sub_y);       // top-right
//			double w01 = (1 - sub_x) * sub_y;       // bottom-left
//			double w11 = sub_x * sub_y;             // bottom-right

			double w11 = 1 - (1 - sub_x) * (1 - sub_y); // top-left
			double w01 = 1 - sub_x * (1 - sub_y);       // top-right
			double w10 = 1 - (1 - sub_x) * sub_y;       // bottom-left
			double w00 = 1 - sub_x * sub_y;             // bottom-right

			// Apply coverage (remember: 1 = transparent, 0 = opaque)
			t_point p;

			p = (t_point){.x = c.x + base_x,     .y = c.y + base_y};
			put_pixel_alpha_alt(img, p, color, 1 - w00 * (1 - coverage));

			p = (t_point){.x = c.x + base_x + 1, .y = c.y + base_y};
			put_pixel_alpha_alt(img, p, color, 1 - w10 * (1 - coverage));

			p = (t_point){.x = c.x + base_x,     .y = c.y + base_y + 1};
			put_pixel_alpha_alt(img, p, color, 1 - w01 * (1 - coverage));

			p = (t_point){.x = c.x + base_x + 1, .y = c.y + base_y + 1};
			put_pixel_alpha_alt(img, p, color, 1 - w11 * (1 - coverage));
		}
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
				put_pixel_alpha(img, cc, color, 0); // full opacity
			else if (dist <= r)
				put_pixel_alpha(img, cc, color, 1 - frac);
			// fade out
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

/**
 * Render a ring segment (annulus section).
 * @param img
 * @param c
 * @param r_outer
 * @param r_inner
 * @param angle_start
 * @param angle_end
 * @param color
 */
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

				// Angular blur (soft arc boundary)
				double da_start = angle - angle_start;
				double da_end   = angle_end - angle;
				if (da_start < 0) da_start += 2 * M_PI;
				if (da_end   < 0) da_end   += 2 * M_PI;

				double a_edge_start = smoothstep(0.0, ANGLE_EPSILON, da_start);
				double a_edge_end   = smoothstep(0.0, ANGLE_EPSILON, da_end);


				alpha = 1.0;
				if (dist < r_inner)
					alpha = dist - (r_inner - 1.0);
				else if (dist > r_outer - 1.0)
					alpha = r_outer - dist;

				alpha = 1 - fmin(alpha, fmin(a_edge_start, a_edge_end));
				t_point cc = (t_point){.x =  c.x + x, .y = c.y + y};
				put_pixel_alpha(img,cc, color, alpha);
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

/**
 * Precision sampling:
 * 	Using (x + 0.5, y + 0.5) avoids corner bias and aligns with true pixel centers.
 * Radial smoothing:
 * 	smoothstep yields a smoother falloff than linear.
 * Edge consistency:
 *  Merging angular and radial AA more evenly.
 *
 * @param img
 * @param c
 * @param r_outer
 * @param r_inner
 * @param angle_start
 * @param angle_end
 * @param color
 */
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

void draw_ring_segment5(t_img *img, t_point c, int r_outer, int r_inner,
						  double angle_start, double angle_end, int color)
{
	int		x, y;
	double	fx, fy, dist, angle, alpha;

	y = -r_outer - 1;
	while (++y <= r_outer)
	{
		x = -r_outer - 1;
		while (++x <= r_outer)
		{
			fx = x + 0.5;
			fy = y + 0.5;
			dist = sqrt(fx * fx + fy * fy);
			if (dist < r_inner - 1.0 || dist > r_outer)
				continue;

			angle = atan2(fy, fx); // range: -π to π
			if (angle < 0)
				angle += 2 * M_PI;

			if (angle_in_range(angle, angle_start, angle_end))
			{
				// Angular blur (soft arc boundary)
				double da_start = angle - angle_start;
				double da_end   = angle_end - angle;
				if (da_start < 0) da_start += 2 * M_PI;
				if (da_end   < 0) da_end   += 2 * M_PI;

				double a_edge_start = smoothstep(0.0, ANGLE_EPSILON, da_start);
				double a_edge_end   = smoothstep(0.0, ANGLE_EPSILON, da_end);

				// Linear radial edge AA
				alpha = 1.0;
				if (dist < r_inner)
					alpha = dist - (r_inner - 1.0);
				else if (dist > r_outer - 1.0)
					alpha = r_outer - dist;

				alpha = 1 - fmin(alpha, fmin(a_edge_start, a_edge_end));

				t_point cc = { .x = c.x + x, .y = c.y + y };
				put_pixel_alpha(img, cc, color, alpha);
			}
		}
	}
}
