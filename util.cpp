#include <stdio.h>
#include <stdlib.h>

#include <GLvideo_params.h>

#define DEBUG 0

static const float identity4[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};

#if 0
/* Prettyprint a 4x4 matrix @m@ */
static void
matrix_dump4(float m[4][4])
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			printf("%f", m[i][j]);
			if (j < 3)
				printf(", ");
		}
		printf("\n");
	}
}
#endif

/* Perform 4x4 matrix multiplication:
 *  - @dst@ = @a@ * @b@
 *  - @dst@ may be a pointer to @a@ andor @b@
 */
static void
matrix_mul4(float dst[4][4], const float a[4][4], const float b[4][4])
{
	float tmp[4][4] = {{0},{0},{0},{0}};

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			for (int e = 0; e < 4; e++) {
				tmp[i][j] += a[i][e] * b[e][j];
			}
		}
	}

	memcpy(dst, tmp, 16*sizeof(float));
}

void
buildColourMatrix(float dst[4][4], const GLvideo_params &p)
{
	const int bitdepth = 8;
	const int bitdepth_max = (1<<bitdepth) -1;

	/*
	 * At this point, everything is in YCbCr
	 * All components are in the range [0,1.0]
	 */
	memcpy(dst, identity4, 16*sizeof(float));

	/* offset required to get input video black to (0.,0.,0.) */
	const float input_blacklevel[4][4] = {
		{1., 0., 0., -p.input_luma_blacklevel/(float)bitdepth_max},
		{0., 1., 0., -p.input_chroma_blacklevel/(float)bitdepth_max},
		{0., 0., 1., -p.input_chroma_blacklevel/(float)bitdepth_max},
		{0., 0., 0.,  1.},
	};
	matrix_mul4(dst, input_blacklevel, dst);

	/* user supplied operations */
	const float u_lm = p.luminance_mul;
	const float u_cm = p.chrominance_mul;
	const float u_lo = p.luminance_offset2;
	const float u_co = p.chrominance_offset2;
	const float user_matrix[4][4] = {
		{u_lm, 0.,   0.,   u_lo},
		{0.,   u_cm, 0.,   u_co},
		{0.,   0.,   u_cm, u_co},
		{0.,   0.,   0.,   1.},
	};
	matrix_mul4(dst, user_matrix, dst);

	/* colour matrix, YCbCr -> RGB */
	/* Requires Y in [0,1.0], Cb&Cr in [-0.5,0.5] */
	float Kr = p.matrix_Kr;
	float Kg = p.matrix_Kg;
	float Kb = p.matrix_Kb;
	const float colour[4][4] = {
		{1.,  0.,              2*(1-Kr),       0.},
		{1., -2*Kb*(1-Kb)/Kg, -2*Kr*(1-Kr)/Kg, 0.},
		{1.,  2*(1-Kb),        0.,             0.},
		{0.,  0.,              0.,             1.},
	};
	matrix_mul4(dst, colour, dst);

	/*
	 * We are now in RGB space
	 */

	/* scale to output range. */
	/* NB output_max = p.output_blacklevel + p.output_range -1; */
	const float s = (p.output_range-1) / (float)(p.input_luma_range-1);
	const float output_range[4][4] = {
		{s,  0., 0., 0.},
		{0., s,  0., 0.},
		{0., 0., s,  0.},
		{0., 0., 0., 1.},
	};
	matrix_mul4(dst, output_range, dst);

	/* move (0.,0.,0.)rgb to required blacklevel */
	const float output_blacklevel[4][4] = {
		{1., 0., 0., p.output_blacklevel/(float)bitdepth_max},
		{0., 1., 0., p.output_blacklevel/(float)bitdepth_max},
		{0., 0., 1., p.output_blacklevel/(float)bitdepth_max},
		{0., 0., 0., 1.},
	};
	matrix_mul4(dst, output_blacklevel, dst);

	/* printf("---\n");matrix_dump4(dst); */
}
