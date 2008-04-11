#include <stdio.h>

#define DEBUG 0

void buildColourMatrix(float *matrix, const float Kr, const float Kg,
                       const float Kb, bool Yscale, bool Cscale)
{
	//R row
	matrix[0] = 1.0; //Y    column
	matrix[1] = 0.0; //Cb
	matrix[2] = 2.0 * (1-Kr); //Cr

	//G row
	matrix[3] = 1.0;
	matrix[4] = -1.0 * ( (2.0*(1.0-Kb)*Kb)/Kg );
	matrix[5] = -1.0 * ( (2.0*(1.0-Kr)*Kr)/Kg );

	//B row
	matrix[6] = 1.0;
	matrix[7] = 2.0 * (1.0-Kb);
	matrix[8] = 0.0;

	float Ys = Yscale ? (255.0/219.0) : 1.0;
	float Cs = Cscale ? (255.0/224.0) : 1.0;

	//apply any necessary scaling
	matrix[0] *= Ys;
	matrix[1] *= Cs;
	matrix[2] *= Cs;
	matrix[3] *= Ys;
	matrix[4] *= Cs;
	matrix[5] *= Cs;
	matrix[6] *= Ys;
	matrix[7] *= Cs;
	matrix[8] *= Cs;

	if (DEBUG) {
		printf("Building YUV->RGB matrix with Kr=%f Kg=%f, Kb=%f\n", Kr, Kg, Kb);
		printf("%f %f %f\n", matrix[0], matrix[1], matrix[2]);
		printf("%f %f %f\n", matrix[3], matrix[4], matrix[5]);
		printf("%f %f %f\n", matrix[6], matrix[7], matrix[8]);
		printf("\n");
	}
}
