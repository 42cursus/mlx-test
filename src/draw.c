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
#include <emmintrin.h>
#include <sys/param.h>
#include "mlx-test.h"

__attribute__((unused)) static inline t_colour blend_colour(t_colour src, t_colour dst)
{
	t_colour out;

	u_int src_a = src.a;            // 0–255
	u_int inv_a = 255 - src_a;      // inverse alpha

	// Blend each channel
	out.r = (u_char)((src.r * src_a + dst.r * inv_a) / 255);
	out.g = (u_char)((src.g * src_a + dst.g * inv_a) / 255);
	out.b = (u_char)((src.b * src_a + dst.b * inv_a) / 255);

	// Optional: blend alpha channel too
	out.a = (u_char)(src_a + (dst.a * inv_a) / 255);

	return out;
}

u_int	interpolate_colour(t_colour *col1, t_colour *col2)
{
	t_colour		out;
	const double	frac = col1->a / 255.0;

	if (col1->raw == col2->raw)
		return col1->raw;
	out.r = ((col2->r - col1->r) * frac) + col1->r + 0.5;
	out.g = ((col2->g - col1->g) * frac) + col1->g + 0.5;
	out.b = ((col2->b - col1->b) * frac) + col1->b + 0.5;
	return (out.raw);
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


void	put_pixel_alpha(t_img *img, t_point p, int base_color, double alpha_frac);
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

static inline __m128i pack_color(__m128 r, __m128 g, __m128 b, __m128 a)
{
	__m128i ri = _mm_cvtps_epi32(r);
	__m128i gi = _mm_cvtps_epi32(g);
	__m128i bi = _mm_cvtps_epi32(b);
	__m128i ai = _mm_cvtps_epi32(a);

	__m128i rb = _mm_or_si128(ri, _mm_slli_epi32(gi, 8));
	__m128i ga = _mm_or_si128(_mm_slli_epi32(bi, 16), _mm_slli_epi32(ai, 24));
	return _mm_or_si128(rb, ga);
}

__attribute__((optnone, used))
static void place_tile_on_image32_alpha(t_img *image, t_img *tile, t_point p)
{
	t_point		it;
	t_point		offset;
	t_point		boundaries;

	offset.x = (int[]){0, -p.x}[p.x < 0];
	offset.y = (int[]){0, -p.y}[p.y < 0];

	boundaries.x = MIN(tile->width, image->width - p.x);
	boundaries.y = MIN(tile->height, image->height - p.y);
	it.y = offset.y - 1;

#pragma clang loop unroll(disable)
	while (++it.y < boundaries.y)
	{
		u_int32_t *src_row = (u_int32_t *)tile->data + (it.y * tile->width);
		u_int32_t *dst_row = (u_int32_t *)image->data + ((it.y + p.y) * image->width) + p.x;
		it.x = offset.x - 1;

		while (++it.x < boundaries.x)
		{
			t_colour src = *(t_colour *)&src_row[it.x];
			t_colour dst = *(t_colour *)&dst_row[it.x];

//			if (src.a == 255 || src.raw == dst.raw)
//				continue;

			// Load into SIMD vectors
			__m128 src_v = _mm_set_ps(src.a, src.r, src.g, src.b);
			__m128 dst_v = _mm_set_ps(dst.a, dst.r, dst.g, dst.b);

			// Normalize alpha
			__m128 alpha = _mm_set1_ps(src.a / 255.0f);

			// Blend each channel
			__m128 out = _mm_add_ps(src_v, _mm_mul_ps(_mm_sub_ps(dst_v, src_v), alpha));

			// Store result
			dst_row[it.x] = _mm_cvtsi128_si32(pack_color(
				_mm_shuffle_ps(out, out, _MM_SHUFFLE(0, 0, 0, 0)),	// A
				_mm_shuffle_ps(out, out, _MM_SHUFFLE(1, 1, 1, 1)),	// R
				_mm_shuffle_ps(out, out, _MM_SHUFFLE(2, 2, 2, 2)),	// G
				_mm_shuffle_ps(out, out, _MM_SHUFFLE(3, 3, 3, 3))	// B
			));
		}
	}
}
__attribute__((optnone, used))
static inline __m128i blend_4pixels2(__m128i src, __m128i dst)
{
	__m128i zero = _mm_setzero_si128();

	// Unpack 8-bit to 16-bit
	__m128i src_lo = _mm_unpacklo_epi8(src, zero); // [B0 G0 R0 A0][B1 G1 R1 A1]
	__m128i src_hi = _mm_unpackhi_epi8(src, zero); // [B2 G2 R2 A2][B3 G3 R3 A3]
	__m128i dst_lo = _mm_unpacklo_epi8(dst, zero);
	__m128i dst_hi = _mm_unpackhi_epi8(dst, zero);

	// Interleave src channels into 4 parallel float vectors
	__m128i src_bi = _mm_setr_epi32(
		_mm_extract_epi16(src_lo, 0),
		_mm_extract_epi16(src_lo, 4),
		_mm_extract_epi16(src_hi, 0),
		_mm_extract_epi16(src_hi, 4)
	);
	__m128i src_gi = _mm_setr_epi32(
		_mm_extract_epi16(src_lo, 1),
		_mm_extract_epi16(src_lo, 5),
		_mm_extract_epi16(src_hi, 1),
		_mm_extract_epi16(src_hi, 5)
	);
	__m128i src_ri = _mm_setr_epi32(
		_mm_extract_epi16(src_lo, 2),
		_mm_extract_epi16(src_lo, 6),
		_mm_extract_epi16(src_hi, 2),
		_mm_extract_epi16(src_hi, 6)
	);
	__m128i src_ai = _mm_setr_epi32(
		_mm_extract_epi16(src_lo, 3),
		_mm_extract_epi16(src_lo, 7),
		_mm_extract_epi16(src_hi, 3),
		_mm_extract_epi16(src_hi, 7)
	);

	__m128 src_b = _mm_cvtepi32_ps(src_bi);
	__m128 src_g = _mm_cvtepi32_ps(src_gi);
	__m128 src_r = _mm_cvtepi32_ps(src_ri);
	__m128 src_a = _mm_cvtepi32_ps(src_ai);

	// Repeat for dst
	__m128i dst_bi = _mm_setr_epi32(
		_mm_extract_epi16(dst_lo, 0),
		_mm_extract_epi16(dst_lo, 4),
		_mm_extract_epi16(dst_hi, 0),
		_mm_extract_epi16(dst_hi, 4)
	);
	__m128i dst_gi = _mm_setr_epi32(
		_mm_extract_epi16(dst_lo, 1),
		_mm_extract_epi16(dst_lo, 5),
		_mm_extract_epi16(dst_hi, 1),
		_mm_extract_epi16(dst_hi, 5)
	);
	__m128i dst_ri = _mm_setr_epi32(
		_mm_extract_epi16(dst_lo, 2),
		_mm_extract_epi16(dst_lo, 6),
		_mm_extract_epi16(dst_hi, 2),
		_mm_extract_epi16(dst_hi, 6)
	);

	__m128 dst_b = _mm_cvtepi32_ps(dst_bi);
	__m128 dst_g = _mm_cvtepi32_ps(dst_gi);
	__m128 dst_r = _mm_cvtepi32_ps(dst_ri);

	// Normalize alpha
	__m128 alpha = _mm_div_ps(src_a, _mm_set1_ps(255.0f));
	__m128 opacity = _mm_sub_ps(_mm_set1_ps(1.0f), alpha);

	// Blend
	__m128 out_b = _mm_add_ps(src_b, _mm_mul_ps(_mm_sub_ps(dst_b, src_b), opacity));
	__m128 out_g = _mm_add_ps(src_g, _mm_mul_ps(_mm_sub_ps(dst_g, src_g), opacity));
	__m128 out_r = _mm_add_ps(src_r, _mm_mul_ps(_mm_sub_ps(dst_r, src_r), opacity));

	// Convert to integers
	__m128i out_bi = _mm_cvtps_epi32(out_b);
	__m128i out_gi = _mm_cvtps_epi32(out_g);
	__m128i out_ri = _mm_cvtps_epi32(out_r);
	__m128i out_ai = _mm_cvtps_epi32(src_a); // keep original alpha

	// Pack into 8-bit BGRA
	__m128i bg = _mm_or_si128(out_bi, _mm_slli_epi32(out_gi, 8));
	__m128i ra = _mm_or_si128(out_ri, _mm_slli_epi32(out_ai, 8));
	__m128i bgralo = _mm_or_si128(bg, _mm_slli_epi32(ra, 16));

	return bgralo;
}

// Blend src and dst using per-channel alpha
__attribute__((optnone, used))
static inline __m128i blend_4pixels(__m128i src, __m128i dst)
{
	__m128i zero = _mm_setzero_si128();

	// Unpack bytes to 16-bit integers: [B G R A] × 4
	__m128i src_lo = _mm_unpacklo_epi8(src, zero); // pixels 0 and 1
	__m128i src_hi = _mm_unpackhi_epi8(src, zero); // pixels 2 and 3
	__m128i dst_lo = _mm_unpacklo_epi8(dst, zero);
	__m128i dst_hi = _mm_unpackhi_epi8(dst, zero);

	// Unpack 16-bit to 32-bit per channel
	__m128i s0 = _mm_unpacklo_epi16(src_lo, zero); // pixel 0: B0 G0 R0 A0
	__m128i s1 = _mm_unpackhi_epi16(src_lo, zero); // pixel 1: B1 G1 R1 A1
	__m128i s2 = _mm_unpacklo_epi16(src_hi, zero); // pixel 2: B2 G2 R2 A2
	__m128i s3 = _mm_unpackhi_epi16(src_hi, zero); // pixel 3: B3 G3 R3 A3

	__m128i d0 = _mm_unpacklo_epi16(dst_lo, zero);
	__m128i d1 = _mm_unpackhi_epi16(dst_lo, zero);
	__m128i d2 = _mm_unpacklo_epi16(dst_hi, zero);
	__m128i d3 = _mm_unpackhi_epi16(dst_hi, zero);

	// Convert to float
	__m128 fs0 = _mm_cvtepi32_ps(s0);
	__m128 fs1 = _mm_cvtepi32_ps(s1);
	__m128 fs2 = _mm_cvtepi32_ps(s2);
	__m128 fs3 = _mm_cvtepi32_ps(s3);

	__m128 fd0 = _mm_cvtepi32_ps(d0);
	__m128 fd1 = _mm_cvtepi32_ps(d1);
	__m128 fd2 = _mm_cvtepi32_ps(d2);
	__m128 fd3 = _mm_cvtepi32_ps(d3);

	// Extract alpha from source
	__m128 a0 = _mm_shuffle_ps(fs0, fs0, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 a1 = _mm_shuffle_ps(fs1, fs1, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 a2 = _mm_shuffle_ps(fs2, fs2, _MM_SHUFFLE(3, 3, 3, 3));
	__m128 a3 = _mm_shuffle_ps(fs3, fs3, _MM_SHUFFLE(3, 3, 3, 3));

	__m128 alpha0 = _mm_div_ps(a0, _mm_set1_ps(255.0f));
	__m128 opacity0 = _mm_sub_ps(_mm_set1_ps(1.0f), alpha0);
	__m128 alpha1 = _mm_div_ps(a1, _mm_set1_ps(255.0f));
	__m128 opacity1 = _mm_sub_ps(_mm_set1_ps(1.0f), alpha1);
	__m128 alpha2 = _mm_div_ps(a2, _mm_set1_ps(255.0f));
	__m128 opacity2 = _mm_sub_ps(_mm_set1_ps(1.0f), alpha2);
	__m128 alpha3 = _mm_div_ps(a3, _mm_set1_ps(255.0f));
	__m128 opacity3 = _mm_sub_ps(_mm_set1_ps(1.0f), alpha3);

	// out = src + (dst - src) * (1 - alpha)
	fs0 = _mm_add_ps(fs0, _mm_mul_ps(_mm_sub_ps(fd0, fs0), _mm_sub_ps(_mm_set1_ps(1.0f), opacity0)));
	fs1 = _mm_add_ps(fs1, _mm_mul_ps(_mm_sub_ps(fd1, fs1), _mm_sub_ps(_mm_set1_ps(1.0f), opacity1)));
	fs2 = _mm_add_ps(fs2, _mm_mul_ps(_mm_sub_ps(fd2, fs2), _mm_sub_ps(_mm_set1_ps(1.0f), opacity2)));
	fs3 = _mm_add_ps(fs3, _mm_mul_ps(_mm_sub_ps(fd3, fs3), _mm_sub_ps(_mm_set1_ps(1.0f), opacity3)));

	// Convert back to int
	__m128i i0 = _mm_cvtps_epi32(fs0);
	__m128i i1 = _mm_cvtps_epi32(fs1);
	__m128i i2 = _mm_cvtps_epi32(fs2);
	__m128i i3 = _mm_cvtps_epi32(fs3);

	// Pack 32-bit -> 16-bit
	__m128i p01 = _mm_packs_epi32(i0, i1); // 2 pixels
	__m128i p23 = _mm_packs_epi32(i2, i3); // 2 pixels

	// Pack 16-bit -> 8-bit
	__m128i result = _mm_packus_epi16(p01, p23); // 4 pixels packed in 16 bytes

	return result;
}
/**
 * Stage 1: Unpack 4 RGBA pixels into 4 __m128 float vectors
 * @param pixels
 * @return
 */
static inline __attribute__((always_inline, used))
t_vec4	unpack_rgba_bytes_to_floats(__m128i pixels)
{
	t_vec4			out;
	const __m128i	zero = _mm_setzero_si128();
	const __m128i	lo = _mm_unpacklo_epi8(pixels, zero);
	const __m128i	hi = _mm_unpackhi_epi8(pixels, zero);

	out.r0 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(lo, zero));
	out.r1 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(lo, zero));
	out.r2 = _mm_cvtepi32_ps(_mm_unpacklo_epi16(hi, zero));
	out.r3 = _mm_cvtepi32_ps(_mm_unpackhi_epi16(hi, zero));
	return (out);
}

/**
 * Stage 2: Extract normalized alpha (alpha / 255)
 * @param s
 * @return
 */
static inline __attribute__((always_inline, used))
t_vec4	extract_normalized_alpha(t_vec4 s)
{
	t_vec4			alpha;
	t_vec4			opacity;
	const __m128	byte = _mm_set1_ps(255.0f);
	const __m128	one = _mm_set1_ps(1.0f);

	alpha.r0 = _mm_shuffle_ps(s.r0, s.r0, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r1 = _mm_shuffle_ps(s.r1, s.r1, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r2 = _mm_shuffle_ps(s.r2, s.r2, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r3 = _mm_shuffle_ps(s.r3, s.r3, _MM_SHUFFLE(3, 3, 3, 3));

	opacity.r0 = _mm_sub_ps(one, _mm_div_ps(alpha.r0, byte));
	opacity.r1 = _mm_sub_ps(one, _mm_div_ps(alpha.r1, byte));
	opacity.r2 = _mm_sub_ps(one, _mm_div_ps(alpha.r2, byte));
	opacity.r3 = _mm_sub_ps(one, _mm_div_ps(alpha.r3, byte));

	return opacity;
}

/**
 * Stage 2.1: Extract opacity
 * the alpha already represents opacity
 * opacity = (255.0 - alpha_byte) / 255.0
 * @param s
 * @return
 */
static inline __attribute__((optnone, used))
t_vec4	extract_opacity_from_inverted_alpha_old(t_vec4 s)
{
	t_vec4			opacity;
	t_vec4			alpha;
	const __m128	byte = _mm_set1_ps(255.0f);

	alpha.r0 = _mm_shuffle_ps(s.r0, s.r0, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r1 = _mm_shuffle_ps(s.r1, s.r1, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r2 = _mm_shuffle_ps(s.r2, s.r2, _MM_SHUFFLE(3, 3, 3, 3));
	alpha.r3 = _mm_shuffle_ps(s.r3, s.r3, _MM_SHUFFLE(3, 3, 3, 3));

	opacity.r0 = _mm_div_ps(_mm_sub_ps(byte , alpha.r0), byte);
	opacity.r1 = _mm_div_ps(_mm_sub_ps(byte, alpha.r1), byte);
	opacity.r2 = _mm_div_ps(_mm_sub_ps(byte, alpha.r2), byte);
	opacity.r3 = _mm_div_ps(_mm_sub_ps(byte, alpha.r3), byte);

	return (opacity);
}

static inline __attribute__((always_inline, used))
t_vec4	extract_opacity_from_inverted_alpha(t_vec4 s)
{
	t_vec4		out;
	const float	inv255 = 255.0f;

	__asm__ __volatile__ (
		// Broadcast 255.0f into xmm0
		"vbroadcastss %[inv], %%xmm0\n\t"

		// Broadcast alpha components and compute: (255 - alpha) / 255
		"vbroadcastss 12(%[src]), %%xmm1\n\t"   // s.r0[3] → xmm1
		"vsubps %%xmm1, %%xmm0, %%xmm2\n\t"
		"vdivps %%xmm0, %%xmm2, %%xmm2\n\t"
		"vmovaps %%xmm2, 0(%[dst])\n\t"

		"vbroadcastss 28(%[src]), %%xmm1\n\t"   // s.r1[3] → xmm1
		"vsubps %%xmm1, %%xmm0, %%xmm2\n\t"
		"vdivps %%xmm0, %%xmm2, %%xmm2\n\t"
		"vmovaps %%xmm2, 16(%[dst])\n\t"

		"vbroadcastss 44(%[src]), %%xmm1\n\t"   // s.r2[3] → xmm1
		"vsubps %%xmm1, %%xmm0, %%xmm2\n\t"
		"vdivps %%xmm0, %%xmm2, %%xmm2\n\t"
		"vmovaps %%xmm2, 32(%[dst])\n\t"

		"vbroadcastss 60(%[src]), %%xmm1\n\t"   // s.r3[3] → xmm1
		"vsubps %%xmm1, %%xmm0, %%xmm2\n\t"
		"vdivps %%xmm0, %%xmm2, %%xmm2\n\t"
		"vmovaps %%xmm2, 48(%[dst])\n\t"
		:
		: [dst] "r" (&out), [src] "r" (&s), [inv] "m" (inv255)
	: "xmm0", "xmm1", "xmm2", "memory"
	);
	return (out);
}

/**
 * Stage 3: Blend pixel = src + (dst - src) * (1 - alpha)
 * @param src
 * @param dst
 * @param alpha
 * @return
 */
static inline __attribute__((always_inline, used))
t_vec4	blend_pixels(t_vec4 src, t_vec4 dst, t_vec4 alpha) {
	t_vec4	out;
	__m128	one = _mm_set1_ps(1.0f);

	out.r0 = _mm_add_ps(src.r0, _mm_mul_ps(_mm_sub_ps(dst.r0, src.r0), _mm_sub_ps(one, alpha.r0)));
	out.r1 = _mm_add_ps(src.r1, _mm_mul_ps(_mm_sub_ps(dst.r1, src.r1), _mm_sub_ps(one, alpha.r1)));
	out.r2 = _mm_add_ps(src.r2, _mm_mul_ps(_mm_sub_ps(dst.r2, src.r2), _mm_sub_ps(one, alpha.r2)));
	out.r3 = _mm_add_ps(src.r3, _mm_mul_ps(_mm_sub_ps(dst.r3, src.r3), _mm_sub_ps(one, alpha.r3)));
	return out;
}

/**
 * Stage 4: Convert 4 float vectors to a packed 4-pixel __m128i
 * @param blended
 * @return
 */
static inline __attribute__((always_inline))
__m128i	repack_floats_to_bytes(t_vec4 blended)
{
	__m128i i0 = _mm_cvtps_epi32(blended.r0);
	__m128i i1 = _mm_cvtps_epi32(blended.r1);
	__m128i i2 = _mm_cvtps_epi32(blended.r2);
	__m128i i3 = _mm_cvtps_epi32(blended.r3);

	__m128i p01 = _mm_packs_epi32(i0, i1);
	__m128i p23 = _mm_packs_epi32(i2, i3);
	return _mm_packus_epi16(p01, p23);
}

static inline __attribute__((always_inline, used))
void	blend_4pixels3(u_int32_t *src, u_int32_t *dst)
{
	__m128i _src = _mm_loadu_si128((__m128i *) src);
	__m128i _dst = _mm_loadu_si128((__m128i *) dst);

	t_vec4 fs = unpack_rgba_bytes_to_floats(_src);
	t_vec4 fd = unpack_rgba_bytes_to_floats(_dst);
	t_vec4 opacity = extract_opacity_from_inverted_alpha(fs);
	t_vec4 blended = blend_pixels(fs, fd, opacity);
	_mm_storeu_si128((__m128i *) dst, repack_floats_to_bytes(blended));
}

void	place_tile_on_image32_alpha1(t_img *image, t_img *tile, t_point p);
void	place_tile_on_image32_alpha1(t_img *image, t_img *tile, t_point p)
{
	t_point	it;
	t_point	offset;
	t_point	limit;
	u_int	*src_row;
	u_int	*dst_row;

	offset.x = (int[]){0, -p.x}[p.x < 0];
	offset.y = (int[]){0, -p.y}[p.y < 0];
	limit.x = MIN(tile->width, image->width - p.x);
	limit.y = MIN(tile->height, image->height - p.y);
	it.y = offset.y - 1;
	while (++it.y < limit.y)
	{
		src_row = (u_int32_t *) tile->data + it.y * tile->width;
		dst_row = (u_int32_t *) image->data + (it.y + p.y) * image->width + p.x;
		it.x = offset.x;
		while (it.x + 3 < limit.x)
		{
			blend_4pixels3(src_row + it.x, dst_row + it.x);
			it.x += 4;
		}
	}
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

static void	put_pixel_alpha_alt(t_img *img, t_point p, int base_color, double alpha_frac)
{
	put_pixel_alpha(img, p, base_color, alpha_frac);
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
				put_pixel_alpha(img, cc, color, 1 - frac); // fade out

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
	angle_start = normalize_angle(angle_start);
	angle_end = normalize_angle(angle_end);

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
	angle_start = normalize_angle(angle_start);
	angle_end = normalize_angle(angle_end);
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
