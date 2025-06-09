/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   linear_filter.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/07 17:23:22 by abelov            #+#    #+#             */
/*   Updated: 2025/06/07 17:23:22 by abelov           ###   ########.fr       */
/*                                                                            */
#include <sys/param.h>
#include <malloc.h>

/* ************************************************************************** */



static inline __attribute__((always_inline, unused))
int	anisotropic_sample(double tex_x, double tex_y, double step_y, const t_texture *tex)
{
	int		samples = (int)fmin(8, fmax(1, fabs(step_y) * tex->y));

	int		r_sum = 0, g_sum = 0, b_sum = 0;
	for (int i = 0; i < samples; ++i)
	{
		double sample_y = tex_y + i * step_y / samples;
		int color = bilinear_filter_old(tex_x, sample_y, tex);

		r_sum += (color & MLX_RED);
		g_sum += (color & MLX_GREEN);
		b_sum += (color & MLX_BLUE);
	}
	return (r_sum / samples & MLX_RED) + (g_sum / samples & MLX_GREEN) + (b_sum / samples);
}

#define ALPHA_BLEND_THRESHOLD 15

// Horizontal step smooth blend function
static inline
t_colour smooth_alpha_step(t_colour A, t_colour B, double frac, int threshold)
{
	int diff = abs(A.a - B.a);
	t_colour res;

	double alpha_interp = (A.a > B.a) ?
						  (A.a - B.a) * (1.0 - frac) + B.a :
						  (B.a - A.a) * frac + A.a;

	if (diff < threshold)
	{
		res.a = (unsigned char)alpha_interp;
		res.r = (unsigned char)(A.r * (1 - frac) + B.r * frac);
		res.g = (unsigned char)(A.g * (1 - frac) + B.g * frac);
		res.b = (unsigned char)(A.b * (1 - frac) + B.b * frac);
	}
	else
	{
		if (A.a > B.a) res = B;
		else res = A;
		res.a = (unsigned char)alpha_interp;
	}

	return res;
}

static inline __attribute__((always_inline, unused))
int anisotropic_sample_smooth(t_vect idx, const t_texture *tex)
{
	const int x = (int)idx.x;
	const int y = (int)idx.y;

	const double frac_x = fmod(idx.x, 1.0);
	const double frac_y = fmod(idx.y, 1.0);

	const int max_x = tex->x - 1;
	const int max_y = tex->y - 1;

	// Clamp x2, y2 to texture size
	const int x1 = (x < max_x) ? x : max_x;
	const int y1 = (y < max_y) ? y : max_y;
	const int x2 = (x + 1 < tex->x) ? x + 1 : x1;
	const int y2 = (y + 1 < tex->y) ? y + 1 : y1;

	const int row1 = y1 * tex->x;
	const int row2 = y2 * tex->x;

	t_colour A = *(t_colour *)&tex->data[row1 + x1];
	t_colour B = *(t_colour *)&tex->data[row1 + x2];
	t_colour C = *(t_colour *)&tex->data[row2 + x1];
	t_colour D = *(t_colour *)&tex->data[row2 + x2];



	t_colour top = smooth_alpha_step(A, B, frac_x, ALPHA_BLEND_THRESHOLD);
	t_colour bottom = smooth_alpha_step(C, D, frac_x, ALPHA_BLEND_THRESHOLD);

	// Vertical smooth step
	int diff = abs(top.a - bottom.a);
	double alpha_interp = (top.a > bottom.a) ?
						  (top.a - bottom.a) * (1.0 - frac_y) + bottom.a :
						  (bottom.a - top.a) * frac_y + top.a;

	t_colour out;
	if (diff < ALPHA_BLEND_THRESHOLD)
	{
		out.a = (unsigned char)alpha_interp;
		out.r = (unsigned char)(top.r * (1 - frac_y) + bottom.r * frac_y);
		out.g = (unsigned char)(top.g * (1 - frac_y) + bottom.g * frac_y);
		out.b = (unsigned char)(top.b * (1 - frac_y) + bottom.b * frac_y);
	}
	else
	{
		if (top.a > bottom.a) out = bottom;
		else out = top;
		out.a = (unsigned char)alpha_interp;
	}

	return out.raw;
}

static inline __attribute__((always_inline, unused))
int anisotropic_sampling(t_vect idx, const t_texture *tex)
{
	const int x = (int)idx.x;
	const int y = (int)idx.y;

	const double frac_x = fmod(idx.x, 1.0);
	const double frac_y = fmod(idx.y, 1.0);

	const int max_x = tex->x - 1;
	const int max_y = tex->y - 1;

	// Clamp x2, y2 to texture size
	const int x1 = (x < max_x) ? x : max_x;
	const int y1 = (y < max_y) ? y : max_y;
	const int x2 = (x + 1 < tex->x) ? x + 1 : x1;
	const int y2 = (y + 1 < tex->y) ? y + 1 : y1;

	const int row1 = y1 * tex->x;
	const int row2 = y2 * tex->x;

	t_colour A = *(t_colour *)&tex->data[row1 + x1];
	t_colour B = *(t_colour *)&tex->data[row1 + x2];
	t_colour C = *(t_colour *)&tex->data[row2 + x1];
	t_colour D = *(t_colour *)&tex->data[row2 + x2];

	t_colour top, bottom, out;

	// Horizontal alpha interpolation with direction and color pick
	if (A.a > B.a)
	{
		top.a = (unsigned char)((A.a - B.a) * (1.0 - frac_x) + B.a);
		top.r = B.r; top.g = B.g; top.b = B.b;
	}
	else
	{
		top.a = (unsigned char)((B.a - A.a) * frac_x + A.a);
		top.r = A.r; top.g = A.g; top.b = A.b;
	}

	if (C.a > D.a)
	{
		bottom.a = (unsigned char)((C.a - D.a) * (1.0 - frac_x) + D.a);
		bottom.r = D.r; bottom.g = D.g; bottom.b = D.b;
	}
	else
	{
		bottom.a = (unsigned char)((D.a - C.a) * frac_x + C.a);
		bottom.r = C.r; bottom.g = C.g; bottom.b = C.b;
	}

	// Vertical alpha interpolation with direction and color pick
	if (top.a > bottom.a)
	{
		out.a = (unsigned char)((top.a - bottom.a) * (1.0 - frac_y) + bottom.a);
		out.r = bottom.r; out.g = bottom.g; out.b = bottom.b;
	}
	else
	{
		out.a = (unsigned char)((bottom.a - top.a) * frac_y + top.a);
		out.r = top.r; out.g = top.g; out.b = top.b;
	}

	return out.raw;
}

void	draw_credits_row(t_info *app, t_vect l_pos, t_vect r_pos, int row)
{
	const t_texture		*tex = &app->shtex->credits;
	int					i;
	double				step_x;
	double				curr_x;
	t_vect				idx;
	t_vect				lim = {-0.48,  0.48}; // Relative to 1 block on the map
	u_int *const		p_row = (u_int *) app->overlay->data + app->overlay->width * row;

	step_x = (r_pos.x - l_pos.x) / WIN_WIDTH;
	curr_x = l_pos.x;
	idx.y = (-l_pos.y) * tex->x;
	if (l_pos.y > 0 || idx.y > tex->y)
		return ;
	i = -1;
	while (++i < WIN_WIDTH)
	{
		if (curr_x > lim.x && curr_x < lim.y)
		{
			idx.x = (0.5 + curr_x) * tex->x;
//			t_colour *colour = (void *)&(int [1]){bilinear_credits(idx, tex)};
//			t_colour *colour = (void *)&(int [1]){linear_filter_credits(idx, tex)};
			t_colour *colour = (void *)&(int [1]){bilinear_credits(idx, tex)};
//			t_colour *colour = (void *)&(int [1]){linear_filter_credits(idx, tex)};
//			t_colour *colour;

			if (fabs(step_x) > 1.0)
				colour = (void *)&(int [1]){anisotropic_sample(idx.x, idx.y, step_x, tex)};
			else
				colour = (void *)&(int [1]){bilinear_credits(idx, tex)};
//			double dist = app->dummy->credits_offsets[row - 1];
//			p_row[i] = dim_colour_alpha(*colour, (dist - 1.5) * 6).raw;
			p_row[i] = colour->raw;
		}
		curr_x += step_x;
	}
}


int	get_tile_idx(char **map, int i, int j)
{
	int	index;

	index = 0;
	/* Direct neighbors */
	index += (map[i - 0][j - 1] - '0' != 0) << 0;
	index += (map[i - 1][j + 0] - '0' != 0) << 1;
	index += (map[i - 0][j + 1] - '0' != 0) << 2;
	index += (map[i + 1][j + 0] - '0' != 0) << 3;
	/* Diagonal neighbors */
	index += (map[i - 1][j - 1] - '0' != 0) << 4;
	index += (map[i - 1][j + 1] - '0' != 0) << 5;
	index += (map[i + 1][j - 1] - '0' != 0) << 6;
	index += (map[i + 1][j + 1] - '0' != 0) << 7;
	return (index);
}


#define TILE_W 8
#define TILE_H 8

#define MAP_LEFT		0b00000001
#define MAP_TOP			0b00000010
#define MAP_RIGHT		0b00000100
#define MAP_BOTTOM		0b00001000
#define MAP_TOP_LEFT	0b00010000
#define MAP_TOP_RIGHT	0b00100000
#define MAP_BOT_LEFT	0b01000000
#define MAP_BOT_RIGHT	0b10000000

# define MLX_PINK 0x00d6428e
# define MLX_PALE_GRAY 0xf8f8f8

static inline __attribute__((always_inline, unused))
u_int	get_tile_pix(int x, int y, int idx)
{
	int	is_edge = 0;

	/* Direct neighbors */
	if (idx & MAP_LEFT && x == 0)
		is_edge = 1;
	if (idx & MAP_RIGHT && x == TILE_W - 1)
		is_edge = 1;
	if (idx & MAP_BOTTOM && y == 0)
		is_edge = 1;
	if (idx & MAP_TOP && y == TILE_H - 1)
		is_edge = 1;
	/* Diagonal neighbors */
	if ((idx & MAP_BOT_LEFT) && x == 0 && y == 0)
		is_edge = 1;
	if ((idx & MAP_BOT_RIGHT) && x == TILE_W - 1 && y == 0)
		is_edge = 1;
	if ((idx & MAP_TOP_LEFT) && x == 0 && y == TILE_H - 1)
		is_edge = 1;
	if ((idx & MAP_TOP_RIGHT) && x == TILE_W - 1 && y == TILE_H - 1)
		is_edge = 1;
	return (-(is_edge) & MLX_PALE_GRAY) | (MLX_PINK & ~(-(is_edge)));
}

typedef struct s_texture
{
	u_int	*data;
	int		w;
	int		h;
	int		sl;
}	t_texture;

typedef struct s_ivect
{
	int	x;
	int	y;
}	t_ivect;

static t_texture	get_tile(int idx)
{
	t_ivect		it;
	t_texture	tex;
	u_int32_t	*row;

	tex = (t_texture){.w = TILE_W, .h = TILE_H, .sl = TILE_W * sizeof(int)};
	tex.data = malloc(sizeof(u_int32_t) * TILE_W * TILE_H);
	if (tex.data != NULL)
	{
		it.y = -1;
		while (++it.y < TILE_H)
		{
			row = tex.data + it.y * TILE_W;
			it.x = -1;
			while (++it.x < TILE_W)
				row[it.x] = get_tile_pix(it.x, it.y, idx);
		}
	}
	return (tex);
}
