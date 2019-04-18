#include "MLX_count_people.h"
#include <math.h> 
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

#define TRUE 1
#define FALSE 0

float* applyGaussian(float *src, int widthOfImage, int heightOfImage);
float findAvg(float *src, int size);
float findThreshold(float *src, int size, float minimumStep);
float* setThreshold(float *src, int size, float threshold);
int* detectPeople(float *src, int width, int height, Man people[], int *peopleSize);
float getSourceGaussian(float *src, int width, int height, int widthOfImage, int heightOfImage);
void divideByAvg(float *src, int size, float avg, float *background, float *foreground, int *bckSize, int *foreSize);
void findMax(int *array, int size, int *returnIndex);
int* getUnvisitedNeighbours(int id, int width, int height, int *visited, int *size);
int* getNeighbours(int id, int width, int height, int *size);
void findCentroid(int *src, int width, int height, int objectNum, int *x, int *y);
void findMinMax(float *src, int size, float *min, float *max);
float getStdDev(float *src, int size);


typedef struct node{

	int data;
	struct node* previous;

}*node_ptr;


node_ptr front = NULL;
node_ptr rear = NULL;

int isEmpty(){

	if (front == NULL)
		return TRUE;
	return FALSE;
}

void skipTheQueue(int value) {
	node_ptr item = (node_ptr) malloc(sizeof(struct node));

	if (item == NULL)
		return;

	if(rear == NULL) {
		item->data = value;
		item->previous = NULL;
		front = rear = item;
	}else {
		item->data = value;
		item->previous = front;
		front = item;
	}
}

void enqueue(int value){

	node_ptr item = (node_ptr) malloc(sizeof(struct node));

	if (item == NULL)
		return;

	item->data = value;
	item->previous = NULL;

	if(rear == NULL)
		front = rear = item;
	else{
		rear->previous = item;
		rear = item;
	}
}

int dequeue(){

	int value = front->data;

	node_ptr temp = front;
	if (rear == front) {
		rear = front->previous;
	}
	front = front->previous;

	free(temp);

	return value;
}

void clear(){

	if(isEmpty()){
		return;
	}

	node_ptr current = front;
	node_ptr previous = NULL;

	while(current != NULL){

		previous = current->previous;
		free(current);
		current = previous;
	}

	front = NULL;
	rear = NULL;
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

void findMinMax(float *src, int size, float *min, float *max) {
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
}

float* applyGaussian(float *src, int widthOfImage, int heightOfImage) {
	float *result = (float*) malloc(widthOfImage * heightOfImage * sizeof(float));
	int gausianDim = 5;
	for (int i = 0; i < heightOfImage; ++i)
	{
		for (int j = 0; j < widthOfImage; ++j)
		{
			float sum = 0;
			for (int h=-2; h < gausianDim - 2 ; h++) {
                for (int w=-2; w < gausianDim - 2 ; w++) {
					int height = h + i; 
                	int width = w + j;
                	
					float source = getSourceGaussian(src, width, height, widthOfImage, heightOfImage);
	               	sum += gausian[h + 2][w + 2] * source;
				}

			}
			result[i * widthOfImage + j] = sum;

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

float* setThreshold(float *src, int size, float threshold) {
	for (int i = 0; i < size; ++i)
	{
		if (src[i] <= threshold) {
			src[i] = 0;
		}else {
			src[i] = 1;
		}
	}
	return src;
}

void findMax(int *array, int size, int *returnIndex) {
	int max = -99999;
	for (int i = 0; i < size; ++i)
	{
		if (array[i] > max) {
			max = array[i];
			*(returnIndex) = i;
		}
	}
}


int* getUnvisitedNeighbours(int id, int width, int height, int *visited, int *size) {
	int x = id % width;
	int y = id / width;
	int *result = (int*) malloc (9 * sizeof(int));
	memset(result, 0, 9 * sizeof(int));

	
	for (int j = -1; j <= 1; ++j)
	{
		for (int i = -1; i <= 1; ++i)
		{
			if (x + i < 0 || x + i >= width || y + j < 0 || y + j >= height) {
				continue;
			}

			int id = (y + j) * width + (x + i);
			if (visited[id] == 0){
				result[*(size)] = id;
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
	memset(result, 0, 9 * sizeof(int));

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
	int volumeVal = 0;
	*x = 0;
	*y = 0;
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			if (src[i * width + j] == objectNum) {
				*x = *x + i;
				*y = *y + j;
				volumeVal++;
			}
		}

	}

	*x = *x / volumeVal;
	*y = *y / volumeVal;
}

int visited[768];
int objectNum[768];
int volume[768];
int counter;

int* detectPeople(float *src, int width, int height, Man *people, int *peopleSize) {
	
	enqueue(0);

	memset(visited, 0, 768 * sizeof(int));
	memset(objectNum, 0, 768 * sizeof(int));
	memset(volume, 0, 768 * sizeof(int));
	visited[0] = 1;
	counter = 1;

	while (isEmpty()) {
		int id = dequeue();
		
		int size = 0;
		int *neighbours = getUnvisitedNeighbours(id, width, height, visited, &size);
		
		for (int i = 0; i < size; ++i)
		{
			visited[neighbours[i]] = 1;
			if (src[neighbours[i]] == 1) {
				skipTheQueue(neighbours[i]);
			}else {
				enqueue(neighbours[i]);
			}
		}

		size = 0;
		
		free(neighbours);
		
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

		free(neighbours);

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
	
	clear();
	
	
	int x, y;
	// Starting from one because 0 is background
	for (int i = 1; i < counter; ++i)
	{
		// 1 pixel is approx. 2.5 cm^2. So this is 125 cm^2.
		if (volume[i] > 50) {
			findCentroid(objectNum, width, height, i, &x, &y);
			people[*peopleSize].x = x;
			people[*peopleSize].y = y;
			*peopleSize = *peopleSize + 1;
		}
	}
	return objectNum;
}

float getStdDev(float *src, int size) {
	float avg = findAvg(src, size); 

	float standardDeviation = 0.0;

	for (int i = 0; i < size; ++i)
	{
		standardDeviation += pow(src[i] - avg, 2);
	}
	
	return sqrt(standardDeviation/size);
}





