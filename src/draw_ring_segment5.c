void draw_circle_stroke_v6(t_img *img, t_point c, int r, int thickness, int color)
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
			double fx = dist * cos_t;
			double fy = dist * sin_t;

			// Anti-aliasing weights
			double d_outer = dist - r_outer;
			double d_inner = r_inner - dist;
			double a_outer = smoothstep(1.0, 0.0, d_outer);
			double a_inner = smoothstep(1.0, 0.0, d_inner);
			double coverage = 1 - fmin(a_outer, a_inner);  // 0=opaque, 1=transparent

			// Bilinear subpixel interpolation
			int base_x = (int)floor(fx);
			int base_y = (int)floor(fy);
			double sub_x = fx - base_x;
			double sub_y = fy - base_y;

			double w00 = (1 - sub_x) * (1 - sub_y); // top-left
			double w10 = sub_x * (1 - sub_y);       // top-right
			double w01 = (1 - sub_x) * sub_y;       // bottom-left
			double w11 = sub_x * sub_y;             // bottom-right

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