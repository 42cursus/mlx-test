void draw_circle_stroke_v5(t_img *img, t_point c, int r, int thickness, int color)
{
	const double r_inner = r - thickness / 2.0;
	const double r_outer = r + thickness / 2.0;

	const double TWO_PI = 2.0 * M_PI;
	// Angular resolution
	const double dtheta = 1.0 / r;
	// Radial subpixel resolution
	const double dr = 0.25;

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