/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   mlx_static_line.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: abelov <abelov@student.42london.com>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/05/27 12:26:16 by abelov            #+#    #+#             */
/*   Updated: 2025/05/27 12:26:16 by abelov           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <sys/types.h>

static size_t	ft_strlcpy(char *dest, const char *src, size_t size)
{
	const char	*save = src;

	if (size > 0)
	{
		while (size-- > 1 && *src)
			*dest++ = *src++;
		*dest = '\0';
	}
	while (*src)
		src++;
	return (src - save);
}

u_int	strlcpy_is_not_posix(char *dest, char *src, unsigned int size);

u_int	strlcpy_is_not_posix(char *dest, char *src, unsigned int size)
{
	return ((ft_strlcpy(dest, src, size + 1)));
}
