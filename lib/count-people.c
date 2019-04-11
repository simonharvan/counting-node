
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

void printMatrix(FILE *file, float *src, int width, int height) {
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j) 
		{
			// printf("%04.5f, ", src[i * width + j]);
			fprintf(file, "%f, ", src[i * width + j]);
		}
		// printf("\n");
	}
	fprintf(file, "\n");
}

//1 2 3
//4   6
//7 8 9
float getSourceGaussian(float *src, int width, int height, int widthOfImage, int heightOfImage) {
	float source = src[height * widthOfImage + width];

	// 1
	if (height < 0 && width < 0) {
		// source = src[(heightOfImage + height) * widthOfImage + (widthOfImage + width)];
		source = 0.5;
	// 2
	} else if (height < 0 && width >= 0 && width < widthOfImage) {
		// source = src[(heightOfImage + height) * widthOfImage + width];
		source = 0.5;
 	// 3
	} else if (height < 0 && width >= widthOfImage) {
		// source = src[(heightOfImage + height) * widthOfImage + (width - widthOfImage)];
		source = 0.5;
	// 4 
	} else if (height >= 0 && height < heightOfImage && width < 0) {
		// source = src[height * widthOfImage + (widthOfImage + width)];
		source = 0.5;
	// 6
	} else if(height >= 0 && height < heightOfImage && width >= widthOfImage) {
		// source = src[height * widthOfImage + (width - widthOfImage)];
		source = 0.5;

	// 7
	} else if(height >= heightOfImage && width < 0) {
		// source = src[(height - heightOfImage) * widthOfImage + (widthOfImage + width)];
		source = 0.5;
	
	// 8
	} else if(height >= heightOfImage && width >= 0 && width < widthOfImage) {
		// source = src[(height - heightOfImage) * widthOfImage + width];
		source = 0.5;

	// 9
	} else if(height >= heightOfImage && width >= widthOfImage) {
		// source = src[(height - heightOfImage) * widthOfImage + (width - widthOfImage)];
		source = 0.5;
	}
	return source;
}

void applyGaussian(float *src, int widthOfImage, int heightOfImage, float *result, const float gausian[][5]) {
	int gausianDim = 5;
	for (int i = 0; i < heightOfImage; ++i)
	{
		for (int j = 0; j < widthOfImage; ++j)
		{
			for (int h=-2; h < gausianDim - 2 ; h++) {
                for (int w=-2; w < gausianDim - 2 ; w++) {

					int height = h + i; 
                	int width = w + j;
                	
					float source = getSourceGaussian(src, width, height, widthOfImage, heightOfImage);
	               	result[i * widthOfImage + j] = gausian[h + 2][w + 2] * source;
	               	
				}
			}

		}
	}
}

float findAvg(float *src, int size) {
	float sum = 0;
	for (int i = 0; i < size; ++i)
	{
		sum += src[i];
	}
	return sum/size;
}

void divideByAvg(float *src, int size, float avg, float *background, float *foreground, int *bckSize, int *foreSize) { 
	for (int i = 0; i < size; ++i)
	{
		if (src[i] <= avg) {
			background[*bckSize] = src[i];
			*bckSize = *bckSize + 1; 
		}else {
			foreground[*foreSize] = src[i];
			*foreSize = *foreSize + 1;
		}
	}
}

float findThreshold(float *src, int size) {
	float background[768];
	float foreground[768];
	int foreSize;
	int bckSize;
	float step = 1;
	float minimumStep = 0.1;
	float bckAvg;
	float foreAvg;
	float threshold = findAvg(src, size);
	float newThreshold;
	while (step > minimumStep){
		foreSize = 0.0f;
		bckSize = 0.0f;
		divideByAvg(src, size, threshold, background, foreground, &bckSize, &foreSize);
		bckAvg = findAvg(background, bckSize);
		foreAvg = findAvg(foreground, foreSize);
		newThreshold = foreAvg - bckAvg;
		step = fabs(threshold - newThreshold);
		threshold = newThreshold;
	}
	return threshold;
}

void setThreshold(float *src, int size, float threshold) {
	for (int i = 0; i < size; ++i)
	{
		if (src[i] <= threshold) {
			src[i] = 0;
		}else {
			src[i] = 1;
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

	static const char outputFilename[] = "output";
	FILE *outputfile = fopen ( outputFilename, "a" );
	if ( outputfile == NULL ) {
		perror ( outputFilename ); /* why didn't the file open? */
		return 0;
	}

	static const char normalizedFilename[] = "normalize-output";
	FILE *normalizeFile = fopen ( normalizedFilename, "a" );
	if ( normalizeFile == NULL ) {
		perror ( normalizedFilename ); /* why didn't the file open? */
		return 0;
	}

	char line [ 12000 ]; /* or other suitable maximum line size */
	
	float numbers[768];
	float afterGausian[768];
	memset(afterGausian, 0, 768);
	
	while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
	{
		// for (int j = 0; j < 2; ++j)
		fgets ( line, sizeof line, file );
		char *numberStrings[768];
		memset(numberStrings, 0, 768);
		int i = 0;
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
		printMatrix(normalizeFile, numbers, 32, 24);
		applyGaussian(numbers, 32, 24, afterGausian, gausian);
		float threshold = findThreshold(afterGausian, 768);
		// float threshold = findAvg(afterGausian, 768);
		
		setThreshold(afterGausian, 768, threshold);
		printMatrix(outputfile, afterGausian, 32, 24);
	}
	fclose ( outputfile );
	fclose ( normalizeFile );
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
