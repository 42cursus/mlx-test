/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   memcpy.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/16 00:17:21 by abelov            #+#    #+#             */
/*   Updated: 2025/05/16 00:17:21 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <immintrin.h>
#include <stdint.h>
#include <emmintrin.h>  // SSE2
#include <sys/param.h>
#include "mlx-test.h"

void	pix_dup(t_img *const src, t_img *const dst)
{
	if (src->height != dst->height || src->size_line != dst->size_line)
		return ;
	memcpy(dst->data, src->data, src->height * src->size_line);
}

t_img	*img_dup(t_info *app, t_img *const src)
{
	t_img *const new = mlx_new_image(app->mlx, src->width, src->height);
	if (!new)
		return (NULL);
	pix_dup(src, new);
	return (new);
}

void	pix_copy(t_img *const src, t_img *const dst, t_ivec coord)
{
	int i = -1;
	while(++i < src->height)
	{
		u_int32_t *src_row = (u_int32_t *)src->data + (i * src->width);
		u_int32_t *dst_row = (u_int32_t *)dst->data + ((i + coord.y) * dst->width) + coord.x;
		int j = -1;
		while (++j < src->width)
		{
			u_int32_t src_pixel = src_row[j];
			u_int32_t mask = -(src_pixel != XPM_TRANSPARENT);
			dst_row[j] = (src_pixel & mask) | (dst_row[j] & ~mask);
		}
	}
}

void	copy_row(const u_int32_t *src_row, u_int32_t *dst_row, int width)
{
	int j = -1;
	while (++j < width)
	{
		u_int32_t src_pixel = src_row[j];
		u_int32_t mask = -(src_pixel != XPM_TRANSPARENT);
		dst_row[j] = (src_pixel & mask) | (dst_row[j] & ~mask);
	}
}

void	pix_copy_safe(const t_img *src, const t_img *dst, t_ivec coord)
{
	int i = -1;
	int	copy1_height = MIN((dst->height - coord.y), src->height);
	while(++i < copy1_height)
	{
		u_int32_t *src_row = (u_int32_t *)src->data + (i * src->width);
		u_int32_t *dst_row = (u_int32_t *)dst->data + ((i + coord.y) * dst->width) + coord.x;

		int	copy1_width = MIN((dst->width - coord.x), src->width);
		copy_row(src_row, dst_row, copy1_width);
		copy_row(src_row + copy1_width - 1, dst_row + copy1_width - dst->width, src->width - copy1_width);
	}
	i--;
	while(++i < src->height)
	{
		u_int32_t *src_row = (u_int32_t *)src->data + (i * src->width);
		u_int32_t *dst_row = (u_int32_t *)dst->data + ((i - copy1_height) * dst->width) + coord.x;

		int	copy1_width = MIN((dst->width - coord.x), src->width);
		copy_row(src_row, dst_row, copy1_width);
		copy_row(src_row + copy1_width - 1, dst_row + copy1_width - dst->width, src->width - copy1_width);
	}
}

void	pixcpy_sse2(const t_img *src, const t_img *dst)
{
	int num_pixels = src->width * src->height;
	uint32_t *src_data = (uint32_t *)src->data;
	uint32_t *dst_data = (uint32_t *)dst->data;

	__m128i transparent = _mm_set1_epi32(XPM_TRANSPARENT);

	int i = 0;
	while(i < num_pixels)
	{
		__m128i src_vec = _mm_loadu_si128((__m128i *)&src_data[i]);
		__m128i dst_vec = _mm_loadu_si128((__m128i *)&dst_data[i]);

		__m128i mask = _mm_cmpeq_epi32(src_vec, transparent);    // Transparent pixels -> 0xFFFFFFFF
		mask = _mm_xor_si128(mask, _mm_set1_epi32(-1));          // Invert mask

		__m128i result = _mm_or_si128(_mm_and_si128(mask, src_vec),
									  _mm_andnot_si128(mask, dst_vec));

		_mm_storeu_si128((__m128i *)&dst_data[i], result);
		i += 4;
	}
}

void	memcpy_sse2(void *dst_void, const void *src_void, size_t size)
{
	size_t			i;
	uint8_t			*dst = (uint8_t *)dst_void;
	const uint8_t	*src = (const uint8_t *)src_void;
	const size_t	stride = 16;

	i = 0;
	while (i + stride - 1 < size)
	{
		__m128i chunk = _mm_loadu_si128((const __m128i *)(src + i));
		_mm_storeu_si128((__m128i *)(dst + i), chunk);
		i += stride;
	}
	i--;
	while(++i < size)
		((uint8_t *)dst)[i] = ((const uint8_t *)src)[i];
}

void	memcpy_avx2_nt(void *dst, const void *src, size_t count)
{
	size_t i = 0;
	const size_t stride = 32; // 256-bit = 32 bytes
	const size_t prefetch_distance = 256; // ahead by 256 bytes

	// Process 8 integers (256 bits) at a time
	if (((uintptr_t) src % 32 == 0) && ((uintptr_t) dst % 32 == 0))
	{
		while (i + stride - 1 < count) // Use non-temporal store
		{
//			if (i + prefetch_distance < count)
			_mm_prefetch((const char *) (src + i + prefetch_distance), _MM_HINT_T0);
			__m256i chunk = _mm256_load_si256((const __m256i *) (src + i));
			_mm256_stream_si256((__m256i *) (dst + i), chunk);
			i += stride;
		}
		_mm_sfence();        // Ensure the stores are globally visible
	}
	else  // Fallback to unaligned AVX2
	{
		while (i + stride - 1 < count)
		{
			__m256i chunk = _mm256_loadu_si256((const __m256i *) (src + i));
			_mm256_storeu_si256((__m256i *) (dst + i), chunk);
			i += stride;
		}
	}

	i--;
	while (++i < count)
		((uint8_t *)dst)[i] = ((const uint8_t *)src)[i];
}
