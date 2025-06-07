/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   bilinear_gamma.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/05 01:03:31 by abelov            #+#    #+#             */
/*   Updated: 2025/06/05 01:03:31 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "mlx-test.h"

static inline double srgb_to_linear(unsigned char c)
{
	double x = c / 255.0;
	return pow(x, 2.2); // approximate gamma
}

static inline unsigned char linear_to_srgb(double x)
{
	if (x <= 0.0)
		return 0;
	if (x >= 1.0)
		return 255;
	return (unsigned char)(pow(x, 1.0 / 2.2) * 255.0 + 0.5);
}

typedef struct s_texture
{
	u_int32_t	*data;
	int			x;
	int			y;
}	t_texture;

static inline __attribute__((always_inline, unused))
int linear_filter_credits_centered(t_vect idx, const t_texture *tex)
{
	const double fx = idx.x - 0.5;  // Shift to sample *between* texels
	const int x0 = (int)fx;
	const int x1 = (x0 + 1 < tex->x) ? x0 + 1 : x0;
	const int y = (int)idx.y;

	const double frac = fx - x0;

	const t_colour left  = *(t_colour *)&tex->data[y * tex->x + x0];
	const t_colour right = *(t_colour *)&tex->data[y * tex->x + x1];

	t_colour out;

	out = left;
	out.a = (unsigned char)((right.a - left.a) * frac + left.a);

	return out.raw;
}

static inline __attribute__((always_inline, unused))
int bilinear_gamma(t_vect idx, const t_texture *tex)
{
	const int x = (int)idx.x;
	const int y = (int)idx.y;
	const double frac_x = fmod(idx.x, 1.0);
	const double frac_y = fmod(idx.y, 1.0);

	const int row1 = y * tex->x;
	const int row2 = (y + 1 < tex->y) ? (y + 1) * tex->x : row1;

	const int x1 = x;
	const int x2 = (x + 1 < tex->x) ? x + 1 : x;

	t_colour A = *(t_colour *)&tex->data[row1 + x1];
	t_colour B = *(t_colour *)&tex->data[row1 + x2];
	t_colour C = *(t_colour *)&tex->data[row2 + x1];
	t_colour D = *(t_colour *)&tex->data[row2 + x2];

	// --- Interpolate alpha directionally ---
	t_colour top, bottom, out;

	if (A.a > B.a)
		top.a = (unsigned char)((A.a - B.a) * (1.0 - frac_x) + B.a);
	else
		top.a = (unsigned char)((B.a - A.a) * frac_x + A.a);

	if (C.a > D.a)
		bottom.a = (unsigned char)((C.a - D.a) * (1.0 - frac_x) + D.a);
	else
		bottom.a = (unsigned char)((D.a - C.a) * frac_x + C.a);

	if (top.a > bottom.a)
		out.a = (unsigned char)((top.a - bottom.a) * (1.0 - frac_y) + bottom.a);
	else
		out.a = (unsigned char)((bottom.a - top.a) * frac_y + top.a);

	// --- Interpolate RGB in gamma-correct space ---
	double rA = srgb_to_linear(A.r);
	double rB = srgb_to_linear(B.r);
	double rC = srgb_to_linear(C.r);
	double rD = srgb_to_linear(D.r);

	double gA = srgb_to_linear(A.g);
	double gB = srgb_to_linear(B.g);
	double gC = srgb_to_linear(C.g);
	double gD = srgb_to_linear(D.g);

	double bA = srgb_to_linear(A.b);
	double bB = srgb_to_linear(B.b);
	double bC = srgb_to_linear(C.b);
	double bD = srgb_to_linear(D.b);

	// Horizontal blend top and bottom
	double r_top = rA * (1.0 - frac_x) + rB * frac_x;
	double r_bot = rC * (1.0 - frac_x) + rD * frac_x;
	double r = r_top * (1.0 - frac_y) + r_bot * frac_y;

	double g_top = gA * (1.0 - frac_x) + gB * frac_x;
	double g_bot = gC * (1.0 - frac_x) + gD * frac_x;
	double g = g_top * (1.0 - frac_y) + g_bot * frac_y;

	double b_top = bA * (1.0 - frac_x) + bB * frac_x;
	double b_bot = bC * (1.0 - frac_x) + bD * frac_x;
	double b = b_top * (1.0 - frac_y) + b_bot * frac_y;

	// Convert back to sRGB
	out.r = linear_to_srgb(r);
	out.g = linear_to_srgb(g);
	out.b = linear_to_srgb(b);

	return out.raw;
}

