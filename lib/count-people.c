
#include <math.h> 
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

void normalize(float *src, int size) {
	float min = 99999999999;
	float max = -99999999999;
	for (int i = 0; i < size; ++i)
	{
		if (src[i] < min) {
			min = src[i];
		}
		if (src[i] > max) {
			max = src[i];
		}
	}

	for (int i = 0; i < size; ++i)
	{
		src[i] = (src[i] - min)/(max - min);
	}
}

void printMatrix(float *src, int width, int height) {
	static const char filename[] = "output";
	FILE *file = fopen ( filename, "w+" );
	if ( file == NULL ) {
		perror ( filename ); /* why didn't the file open? */
		return;
	}

	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j) 
		{
			// printf("%04.5f, ", src[i * width + j]);
			fprintf(file, "%f, ", src[i * width + j]);
		}
		// printf("\n");
		fprintf(file, "\n");
	}
}

//1 2 3
//4   6
//7 8 9
float getSourceGaussian(float *src, int height,int width,int heightOfImage,int widthOfImage) {
	float source = src[height * widthOfImage + width];

	// 1
	if (height < 0 && width < 0) {
		source = src[(heightOfImage + height) * widthOfImage + (widthOfImage + width)];
	// 2
	} else if (height < 0 && width >= 0 && width < widthOfImage) {
		source = src[(heightOfImage + height) * widthOfImage + width];
 	// 3
	} else if (height < 0 && width >= widthOfImage) {
		source = src[(heightOfImage + height) * widthOfImage + (width - widthOfImage)];
	// 4 
	} else if (height >= 0 && height < heightOfImage && width < 0) {
		source = src[height * widthOfImage + (widthOfImage + width)];
	// 6
	} else if(height >= 0 && height < heightOfImage && width >= widthOfImage) {
		source = src[height * widthOfImage + (width - widthOfImage)];

	// 7
	} else if(height >= heightOfImage && width < 0) {
		source = src[(height - heightOfImage) * widthOfImage + (widthOfImage + width)];
	
	// 8
	} else if(height >= heightOfImage && width >= 0 && width < widthOfImage) {
		source = src[(height - heightOfImage) * widthOfImage + width];

	// 9
	} else if(height >= heightOfImage && width >= widthOfImage) {
		source = src[(height - heightOfImage) * widthOfImage + (width - widthOfImage)];
	}
	return source;
}

void applyGaussian(float *src, int widthOfImage, int heightOfImage, float *result, const float gausian[][5]) {
	int gausianDim = 5;
	for (int i = 0; i < heightOfImage; ++i)
	{
		for (int j = 0; j < widthOfImage; ++j)
		{
			printf("\n--------%d, %d------------\n\n", i, j);
			for (int h=-2; h < gausianDim - 2 ; h++) {
                for (int w=-2; w < gausianDim - 2 ; w++) {
                	
                	int height = h + i; 
                	int width = w + j;
					float source = getSourceGaussian(src, height, width, heightOfImage, widthOfImage);
               		printf("%d, %d\n", height, width);
                	result[i * width + j] += gausian[h + 2][w + 2] * source;
				}
			}
		}
	}
}
	

int main ( void )
{
	static const char filename[] = "dataset";
	static const float gausian[5][5] = {
										{0.002969,0.013306,0.021938,0.013306,0.002969},
										{0.013306,0.059634,0.098320,0.059634,0.013306},
										{0.021938,0.098320,0.162103,0.098320,0.021938},
										{0.013306,0.059634,0.098320,0.059634,0.013306},
										{0.002969,0.013306,0.021938,0.013306,0.002969}
									};
	FILE *file = fopen ( filename, "r" );
	if ( file == NULL ) {
		perror ( filename ); /* why didn't the file open? */
		return 0;
	}


	char line [ 12000 ]; /* or other suitable maximum line size */
	char *numberStrings[768];
	float numbers[768];
	float afterGausian[768];
	int i = 0;
	// while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
	// {
		fgets ( line, sizeof line, file );
		char *p = strtok( line, ",");
		while (p != NULL) {
			numberStrings[i++] = p;
			p = strtok(NULL, ",");
		}
		for (int i = 0; i < 768; ++i)
		{
			numbers[i] = strtof(numberStrings[i], NULL);
		}
		// free(numberStrings);
		// free(line);
		normalize(numbers, 768);
		// printMatrix(normalized, 32, 24);
		applyGaussian(numbers, 32, 24, afterGausian, gausian);
		printMatrix(afterGausian, 32, 24);
	// }
	fclose ( file );
	
	return 0;
}

// void createFilter(double gKernel[][5])
// {
//     // set standard deviation to 1.0
//     double sigma = 1.0;
//     double r, s = 2.0 * sigma * sigma;
 
//     // sum is for normalization
//     double sum = 0.0;
 
//     // generate 5x5 kernel
//     for (int x = -2; x <= 2; x++)
//     {
//         for(int y = -2; y <= 2; y++)
//         {
//             r = sqrt(x*x + y*y);
//             gKernel[x + 2][y + 2] = (exp(-(r*r)/s))/(M_PI * s);
//             sum += gKernel[x + 2][y + 2];
//         }
//     }
 
//     // normalize the Kernel
//     for(int i = 0; i < 5; ++i)
//         for(int j = 0; j < 5; ++j)
//             gKernel[i][j] /= sum;
 
// }
