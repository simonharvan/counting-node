
#include <math.h> 
#include <string.h>
#include <stdlib.h>

#include <stdio.h>


struct Man
{
    int x;
    int y;
    int width;
    int height;
};



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

void printFloatMatrix(FILE *file, float *src, int width, int height) {
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

void printIntMatrix(FILE *file, int *src, int width, int height) {
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j) 
		{
			// printf("%04.5f, ", src[i * width + j]);
			fprintf(file, "%d, ", src[i * width + j]);
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

	// if (height < 0 || width < 0 || height >= heightOfImage || width >= widthOfImage) {
	// 	return ;
	// }

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

float* applyGaussian(float *src, int widthOfImage, int heightOfImage, const float gausian[][5]) {
	float *result = (float*) malloc(widthOfImage * heightOfImage * sizeof(float));
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
	               	result[i * widthOfImage + j] += gausian[h + 2][w + 2] * source;
	               	
				}
			}

		}
	}
	return result;
}

float findAvg(float *src, int size) {
	long double sum = 0;
	for (int i = 0; i < size; ++i)
	{
		sum += src[i];
	}
	return sum/size;
}

void divideByAvg(float *src, int size, float avg, float *background, float *foreground, int *bckSize, int *foreSize) { 
	*bckSize = 0;
	*foreSize = 0;
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

float findThreshold(float *src, int size, float minimumStep) {
	float background[768];
	float foreground[768];
	int foreSize;
	int bckSize;
	float step = 999999999999;
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
		newThreshold = (foreAvg + bckAvg) / 2;
		if (threshold > newThreshold) {
			step = fabs(threshold - newThreshold);
		}else {
			step = fabs(newThreshold - threshold);
		}
		
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

int findMax(int *array, int size, int *returnIndex) {
	int max = -99999;
	for (int i = 0; i < size; ++i)
	{
		if (array[i] > max) {
			max = array[i];
			*(returnIndex) = i;
		}
	}
	return max;
}


int* getUnvisitedNeighbours(int id, int width, int height, int *visited, int *size) {
	int x = id % width;
	int y = id / width;
	int *result = (int*) malloc(9 * sizeof(int));


	
	for (int j = -1; j <= 1; ++j)
	{
		for (int i = -1; i <= 1; ++i)
		{
			if (x + i < 0 || x + i >= width || y + j < 0 || y + j >= height) {
				continue;
			}

			int id = (y + j) * width + (x + i);
			if (visited[id] == 0){
				*(result + *(size)) = id;
				*size = *size + 1;
			}
		}
	}
	return result;
}

int* getNeighbours(int id, int width, int height, int *size) {
	int x = id % width;
	int y = id / width;
	int *result = (int*) malloc(9 * sizeof(int));
	
	for (int j = -1; j <= 1; ++j)
	{
		for (int i = -1; i <= 1; ++i)
		{
			if (x + i < 0 || x + i >= width || y + j < 0 || y + j >= height) {
				continue;
			}

			int thisId = (y + j) * width + (x + i);
			if (id != thisId){
				*(result + *(size)) = thisId;
				*size = *size + 1;
			}
		}
	}
	return result;
}

void findCentroid(int *src, int width, int height, int objectNum, int *x, int *y) {
	int volume = 0;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			if (src[i * width + j] == objectNum) {
				*x = *x + i;
				*y = *y + j;
				volume++;
			}
		}

	}

	*x = *x / volume;
	*y = *y / volume;
}

int* detectPeople(float *src, int width, int height, struct Man *people, int *peopleSize) {
	float queue[768];
	int numberOfItems = 1;
	int queueCount = 1;
	queue[0] = 0;
	int front = 0;
	int visited[768];
	int *objectNum = (int*) malloc (768 * sizeof(int));

	memset(visited, 0, 768 * sizeof(int));
	memset(objectNum, 0, 768 * sizeof(int));
	visited[0] = 1;

	int *volume = (int*) malloc (768 * sizeof(int));
	memset(volume, 0, 768 * sizeof(int));
	int counter = 1;

	while (numberOfItems > 0) {
		numberOfItems--;
		int id = queue[front++];
		
		int size = 0;
		int *neighbours = getUnvisitedNeighbours(id, width, height, visited, &size);
		
		for (int i = 0; i < size; ++i)
		{
			visited[neighbours[i]] = 1;
			queue[queueCount] = neighbours[i];
			queueCount++;
			numberOfItems++;
		}
		size = 0;
		neighbours = getNeighbours(id, width, height, &size);
		
		
		
		
		int neighboursObjectNum[10];
		memset(neighboursObjectNum, 0, 10 * sizeof(int));

		for (int i = 0; i < size; ++i)
		{
			float data = src[neighbours[i]];
			if (data == 1 && objectNum[neighbours[i]] != 0 && objectNum[neighbours[i]] < 10) {
				neighboursObjectNum[objectNum[neighbours[i]]] = neighboursObjectNum[objectNum[neighbours[i]]] + 1;
			}
		}

		float data = src[id];
		
		int index = 0;
		findMax(neighboursObjectNum, 10, &index);

		if (data == 1) {
			if (neighboursObjectNum[index] != 0) {
				objectNum[id] = index;
			} else {
				objectNum[id] = counter;
			}
			volume[objectNum[id]]++;
		}

		if (data == 0 && volume[counter] != 0) {
			counter++;
		}
	}

	// Starting from one because 0 is background
	for (int i = 1; i < counter; ++i)
	{
		// 1 pixel is approx. 2.5 cm^2. So this is 125 cm^2.
		if (volume[i] > 50) {
			int x = 0, y = 0;
			findCentroid(objectNum, width, height, i, &x, &y);
			people[*peopleSize].x = x;
			people[*peopleSize].y = y;
			*peopleSize = *peopleSize + 1;
		}
	}

	return objectNum;
}

int* calculateHistogram(float *src, int size, float *min, float *max) {
	int *result = (int*)malloc(50 * sizeof(int));
	memset(result, 0, 50 * sizeof(int));
	*min = 99999999999;
	*max = -99999999999;

	for (int i = 0; i < size; ++i)
	{
		if (src[i] < *min) {
			*min = src[i];
		}
		if (src[i] > *max) {
			*max = src[i];
		}
	}
	
	float boundaries[50];
	boundaries[0] = *min;
	for (int i = 1; i < 50; ++i)
	{
		boundaries[i] = *min + (((*max - *min) / 50) * i);
	}

	for (int i = 0; i < size; ++i)
	{
		for (int j = 0; j < 50 - 1; ++j)
		{
			if (src[i] > boundaries[j] && src[i] <= boundaries[j + 1]) {
				result[j]++;
			}
		}
	}

	return result; 
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
	FILE *outputfile = fopen ( outputFilename, "w+" );
	if ( outputfile == NULL ) {
		perror ( outputFilename ); /* why didn't the file open? */
		return 0;
	}

	static const char normalizedFilename[] = "normalize-output";
	FILE *normalizeFile = fopen ( normalizedFilename, "w+" );
	if ( normalizeFile == NULL ) {
		perror ( normalizedFilename ); /* why didn't the file open? */
		return 0;
	}

	char line [ 12000 ]; /* or other suitable maximum line size */
	
	float *numbers = (float* )malloc(768 * sizeof(float));
	memset(numbers, 0, 768);

	int *histogram;
	
	struct Man *people = malloc(10 * sizeof(struct Man));
	int peopleSize = 0;

	int lineNum = 0;
	float min, max;
	int AVG_TRAINING = 8;
	int trainingCycles = 0;

	float maxOfBackground = -99999999999;
	

	while ( fgets ( line, sizeof line, file ) != NULL ) /* read a line */
	{
		trainingCycles++;
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
		

		if (trainingCycles < AVG_TRAINING) {
			float avg = findAvg(numbers, 768);
			if (avg > maxOfBackground){
				maxOfBackground = avg;
			}
			continue;
		}
		
		// normalize(numbers, 768);
		
		
		// float avg = findAvg(numbers, 768);
		float threshold = findAvg(numbers, 768);
		
		if (threshold > maxOfBackground) {
			numbers = applyGaussian(numbers, 32, 24, gausian);
			// printFloatMatrix(normalizeFile, numbers, 32, 24);


			histogram = calculateHistogram(numbers, 768, &min, &max);
			threshold = findThreshold(numbers, 768, (max - min) / 2);
			
			setThreshold(numbers, 768, threshold);

			int *detected = detectPeople(numbers, 32, 24, people, &peopleSize);
			printIntMatrix(outputfile, detected, 32, 24);
			printf("Image n. %d - Threshold: %f\n", trainingCycles, threshold);
			for (int i = 0; i < peopleSize; ++i)
			{
				printf("Man detected at x - %d, y - %d\n", people[i].x, people[i].y);
			}
			
		}
		
		if (trainingCycles % 10) {
			peopleSize = 0;
		}
		
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
