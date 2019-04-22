
#include <math.h> 
#include <string.h>
#include <stdlib.h>

#include <stdio.h>

#define TRUE 1
#define FALSE 0

#define IMAGE_NUM 10
#define MAX_PEOPLE 5 /// Maximum possible people in one frame
#define MAX_PEOPLE_2 25 /// Maximum possible people in two frame
#define MAX_VERTICES 100
#define INF (1<<29)
#define NIL -1
#define verbose 1

struct Man
{
	int x;
    int y;
    int width;
    int height;
    int space;
    float intensity;
    short alreadyCounted;
};

struct Image {
	int size;
	struct Man people[MAX_PEOPLE];
	long time;
};

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

int skipTheQueue(int value) {
	node_ptr item = (node_ptr) malloc(sizeof(struct node));

	if (item == NULL)
		return FALSE;
	
	if(rear == NULL) {
		item->data = value;
		item->previous = NULL;
		front = rear = item;
	}else {
		item->data = value;
		item->previous = front;
		front = item;
	}
	return TRUE;
}

int enqueue(int value){

	node_ptr item = (node_ptr) malloc(sizeof(struct node));

	if (item == NULL)
		return FALSE;

	item->data = value;
	item->previous = NULL;

	if(rear == NULL)
		front = rear = item;
	else{
		rear->previous = item;
		rear = item;
	}

	return TRUE;
}

int dequeue(){

	if(isEmpty()){
		return FALSE;
	}
	
	int value = front->data;

	node_ptr temp = front;
	if (rear == front) {
		rear = front->previous;
	}
	front = front->previous;

	free(temp);

	return value;
}
void display(){


	if(isEmpty()){
		return;
	}

	node_ptr temp = front;

	printf("\n[front -> ");

	while(temp != NULL){
		printf("[%d]", temp->data);
		temp = temp->previous;
	}

	printf(" <- rear]\n");

}

int clear(){

	if(isEmpty()){
		return FALSE;
	}

	node_ptr current = front;
	node_ptr previous = NULL;

	while(current != NULL){

		previous = current->previous;
		free(current);
		current = previous;
	}

	front = NULL;

	return isEmpty();
}



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

float findMaxFloat(float *array, int size, int *returnIndex) {
	float max = -99999;
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


	
	for (int i = -1; i <= 1; ++i)
	{
		for (int j = -1; j <= 1; ++j)
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

void getObjectsProperties(int *src, float *intensities, int width, int height, int objectNum, int *x, int *y, int *widthOfObject, int *heightOfObject, float* intensity) {
	int volume = 0;
	int minX = width, minY = height;
	int maxX = 0, maxY = 0;

	
	for (int i = 0; i < height; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			if (src[i * width + j] == objectNum) {
				if (i <= minY) {
					minY = i;
				}
				if (j <= minX) {
					minX = j;
				}
				if (j >= maxX) {
					maxX = j;
				}
				if (i >= maxY) {
					maxY = i;
				}
				
				*x = *x + i;
				*y = *y + j;
				*intensity = *intensity + intensities[i * width + j];
				volume++;
			}
		}

	}

	*widthOfObject = maxX - minX;
	*heightOfObject = maxY - minY;
	*x = *x / volume;
	*y = *y / volume;
	*intensity = *intensity / volume;
}

int* detectPeople(float *src, float *intensities, int width, int height, struct Man *people, int *peopleSize) {
	
	enqueue(0);

	int visited[768];
	int *objectNum = (int*) malloc (768 * sizeof(int));

	memset(visited, 0, 768 * sizeof(int));
	memset(objectNum, 0, 768 * sizeof(int));
	visited[0] = 1;

	int *volume = (int*) malloc (768 * sizeof(int));
	memset(volume, 0, 768 * sizeof(int));
	int counter = 1;
	
	while (!isEmpty()) {
		
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
	clear();
	

	// Starting from one because 0 is background
	for (int i = 1; i < counter; ++i)
	{
		// 1 pixel is approx. 2.5 cm^2. So this is 125 cm^2.
		if (volume[i] > 50) {

			int x = 0, y = 0;
			int w = 0, h = 0;
			float intensity = 0;
			// Centroid x, y and width, height of object and average intensity in object
			getObjectsProperties(objectNum, intensities, width, height, i, &x, &y, &w, &h, &intensity);
			
			// Because of noise everything that is not wider than 12.5 cm is noise.
			if (w > 5) {
				people[*peopleSize].x = x;
				people[*peopleSize].y = y;
				people[*peopleSize].width = w;
				people[*peopleSize].height = h;
				people[*peopleSize].space = volume[i];
				people[*peopleSize].intensity = intensity;
				people[*peopleSize].alreadyCounted = 0;
				*peopleSize = *peopleSize + 1;
			}
		}
	}

	return objectNum;
}

int* calculateHistogram(float *src, int size, float *min, float *max) {
	int *result = (int*)malloc(50 * sizeof(int));
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

	return result; 
}

// detect
short wasFull = 0;

int getIndexForImages(int index) {
	if (index < 0) {
		if (!wasFull) {
			return -1;
		}
		index = IMAGE_NUM - index;
	}
	if (index >= IMAGE_NUM) {
		index = index % IMAGE_NUM;
		wasFull = 1;
	}
	return index;
}

struct Vertex {
	int idLeft;
	int idRight;
	struct Man *human;
	struct Edge *edges;
	int edgesSize;
	int frameIndex;
};

struct Edge {
	float weight;
	struct Vertex *to;
	short correctionEdge;
};

struct Frame {
	struct Vertex vertices[MAX_PEOPLE];
	int verticesSize;
};

struct Graph {
	struct Frame frames[IMAGE_NUM];
	int frameSize;
};

int dotProduct(int *array1, int *array2, int size) {
	int result = 0;
	for (int i = 0; i < size; ++i)
	{
		result += array1[i] * array2[i];
	} 
	return result;
}

float norm(int *array, int size) {
	int result = 0;
	for (int i = 0; i < size; ++i)
	{
		result += pow(array[i],2);
	}
	return sqrt(result);
}
int* subtract2dVector(int *array1, int *array2, int size) {
	int *result = (int*) malloc (size * sizeof(int));
	for (int i = 0; i < size; ++i){
		result[i] = array1[i] - array2[i];
	}
	return result;
}


char** hungarian(int **array, int width, int height)
{
    char **results = (char **)
    malloc(height * sizeof(char *));
    for (int i = 0; i < height; ++i){
        results[i] = (char *)
        malloc(width * sizeof(char));
    }
    int i, j;
    unsigned int m = height, n = width;
    int k;
    int l;
    int s;
    int *col_mate = (int *)
    malloc(height * sizeof(int));
    col_mate[0] = 0;
    int *row_mate = (int *)
    malloc(width * sizeof(int));
    row_mate[0] = 0;
    int *parent_row = (int *)
    malloc(width * sizeof(int));
    parent_row[0] = 0;
    int *unchosen_row = (int *)
    malloc(height * sizeof(int));
    unchosen_row[0] = 0;
    int t;
    int q;
    int *row_dec = (int *)
    malloc(height * sizeof(int));
    row_dec[0] = 0;
    int *col_inc = (int *)
    malloc(width * sizeof(int));
    col_inc[0] = 0;
    int *slack = (int *)
    malloc(width * sizeof(int));
    slack[0] = 0;
    int *slack_row = (int *)
    malloc(width * sizeof(int));
    slack_row[0] = 0;
    int unmatched;
    int cost = 0;

    for (i = 0; i < height; ++i)
        for (j = 0; j < width; ++j)
            results[i][j] = FALSE;

    // Begin subtract column minima in order to start with lots of zeroes 12
    for (l = 0; l < n; l++) {
        s = array[0][l];
        for (k = 1; k < n; k++)
            if (array[k][l] < s)
                s = array[k][l];
        cost += s;
        if (s != 0)
            for (k = 0; k < n; k++)
                array[k][l] -= s;
    }
    // End subtract column minima in order to start with lots of zeroes

    // Begin initial state
    t = 0;
    for (l = 0; l < n; l++) {
        row_mate[l] = -1;
        parent_row[l] = -1;
        col_inc[l] = 0;
        slack[l] = INF;
    }
    for (k = 0; k < m; k++) {
        s = array[k][0];
        for (l = 1; l < n; l++){
            if (array[k][l] < s){
                s = array[k][l];
            }
        }
        row_dec[k] = s;
        for (l = 0; l < n; l++){
            if (s == array[k][l] && row_mate[l] < 0) {
                col_mate[k] = l;
                row_mate[l] = k;

                goto
                row_done;
            }
        }
        col_mate[k] = -1;

        unchosen_row[t++] = k;
        row_done:;
    }
    // End initial state

    // Begin Hungarian algorithm
    if (t == 0)
        goto
    done;
    unmatched = t;
    while (1) {

        q = 0;
        while (1) {
            while (q < t) {
                // Begin explore node q of the forest
                {
                    k = unchosen_row[q];
                    s = row_dec[k];
                    for (l = 0; l < n; l++){
                        if (slack[l]) {
                            int
                            del;
                            del = array[k][l] - s + col_inc[l];
                            if (del < slack[l]) {
                                if (del == 0) {
                                    if (row_mate[l] < 0)
                                        goto
                                    breakthru;
                                    slack[l] = 0;
                                    parent_row[l] = k;

                                    unchosen_row[t++] = row_mate[l];
                                } else {
                                    slack[l] = del;
                                    slack_row[l] = k;
                                }
                            }
                        }
                    }
                }
                // End explore node q of the forest
                q++;
            }

            // Begin introduce a new zero into the matrix
            s = INF;
            for (l = 0; l < n; l++){
                if (slack[l] && slack[l] < s){
                    s = slack[l];
                }
            }
            for (q = 0; q < t; q++){
                row_dec[unchosen_row[q]] += s;
            }
            for (l = 0; l < n; l++){
                if (slack[l]) {
                    slack[l] -= s;
                    if (slack[l] == 0) {
                        // Begin look at a new zero
                        k = slack_row[l];

                        if (row_mate[l] < 0) {
                            for (j = l + 1; j < n; j++)
                                if (slack[j] == 0)
                                    col_inc[j] += s;
                            goto
                            breakthru;
                        } else {
                            parent_row[l] = k;

                            unchosen_row[t++] = row_mate[l];
                        }
                        // End look at a new zero
                    }
                } else{
                    col_inc[l] += s;
                }
            }
            // End introduce a new zero into the matrix
        }
        breakthru:
            // Begin update the matching

            while (1) {
                j = col_mate[k];
                col_mate[k] = l;
                row_mate[l] = k;

                if (j < 0){
                    break;
                }
                k = parent_row[j];
                l = j;
            }
        // End update the matching
        if (--unmatched == 0){
            goto done;
        }
        
        // Begin get ready for another stage 
        t = 0;
        for (l = 0; l < n; l++) {
            parent_row[l] = -1;
            slack[l] = INF;
        }
        for (k = 0; k < m; k++){
            if (col_mate[k] < 0) {
                unchosen_row[t++] = k;
            }
        }
        // End get ready for another stage 
    }
    done:

    // Begin doublecheck the solution
    for (k = 0; k < m; k++) {
        for (l = 0; l < n; l++) {
            if (array[k][l] < row_dec[k] - col_inc[l]) {
                exit(0);
            }
        }
    }

    for (k = 0; k < m; k++) {
        l = col_mate[k];
        if (l < 0 || array[k][l] != row_dec[k] - col_inc[l]) {
            exit(0);
        }
    }
    k = 0;
    for (l = 0; l < n; l++) {
        if (col_inc[l]) {
            k++;
        }
    }
    if (k > m) {
        exit(0);
    }
    // End doublecheck the solution 
    // End Hungarian algorithm 

    for (i = 0; i < m; ++i) {
        results[i][col_mate[i]] = TRUE;
    }
    for (k = 0; k < m; ++k) {
        for (l = 0; l < n; ++l) {
            
            array[k][l] = array[k][l] - row_dec[k] + col_inc[l];
        }
        
    }
    for (i = 0; i < m; i++) {
        cost += row_dec[i];
    }

    for (i = 0; i < n; i++) {
        cost -= col_inc[i];
    }

    return results;
}

int findVertexById(struct Vertex *vertices, int verticesSize, int vertexId, struct Vertex **vertex) {
 	for (int i = 0; i < verticesSize; ++i)
 	{
 		if (vertices[i].idLeft == vertexId || vertices[i].idRight == vertexId) {
			*vertex = &vertices[i];
			return 0;

 		}	
 	}	
	return -1;
}


int findVertexByIdInGraph(struct Graph *graph, int vertexId, struct Vertex **vertex) {
	int status;
	
	for (int i = 0; i < graph->frameSize; ++i)
	{
		status = findVertexById(graph->frames[i].vertices, graph->frames[i].verticesSize, vertexId, &*vertex);
		if (status != -1){
			return status;
		}
 	}
	return -1;

	
}

void removeEdgesFromVertexExceptToId(struct Vertex *vertex,int idLeft) {
	for (int i = 0; i < vertex->edgesSize; ++i)
	{
		if (vertex->edges[i].to->idLeft == idLeft) {
			vertex->edges[0] = vertex->edges[i];
			vertex->edgesSize = 1;
		
		}
	}
}


void hungarianMatch(struct Graph *graph) {

	int counter = 0;
	for (int i = 0; i <= graph->frameSize; ++i)
	{
		counter += graph->frames[i].verticesSize;
	}

	int size = counter;
	int **array = (int**) malloc(size *  sizeof(int*));
	for (int i = 0; i < size; ++i)
	{
		array[i] = (int*) malloc(size *  sizeof(int));
		memset(array[i], 1, size *  sizeof(int));
	}

	for (int i = 0; i <= graph->frameSize; ++i)
	{
		for (int j = 0; j < graph->frames[i].verticesSize; ++j)
		{
			for (int k = 0; k < graph->frames[i].vertices[j].edgesSize; ++k)
			{
				int idPlus = graph->frames[i].vertices[j].idRight / 2;
				int idMinus = (graph->frames[i].vertices[j].edges[k].to->idLeft - 1) / 2;
				array[idPlus][idMinus] = graph->frames[i].vertices[j].edges[k].weight;
			}
		}
	}	

	char **results = hungarian(array, size, size);
	struct Vertex *vertex;
	for (int i = 0; i < size - 2; ++i)
	{
		for (int j = 0; j < size; ++j)
		{
			if (results[i][j]){
				int idRight = i * 2;
				int idLeft = j * 2 + 1;
				int status = findVertexByIdInGraph(graph, idRight, &vertex);
				if (graph->frameSize == 4){
					removeEdgesFromVertexExceptToId(vertex, idLeft);
				}
			}
		}
	}
	    
	      
}

void calculateVertexDisjointMaximumWeightPathCover(struct Graph *graph) {
	if (graph->frameSize > 1) {
		hungarianMatch(graph);
	}
}

const float alpha = 0.1;

// Go through all frames and add edges to last frame vertices. 
// If the vertex is terminal it is extension edge, if the vertex is not terminal it is correction edge.
void addEdges(struct Graph *graph) {
	int edgesSize;
	if (graph->frameSize < 1 ) {
		return;
	}
	// All frames except last one
	for (int i = graph->frameSize - 1; i < graph->frameSize; ++i)
	{
		// All vertices in frame
		for (int j = 0; j < graph->frames[i].verticesSize; ++j) 
		{
			// Only iterating through last frame
			for (int k = 0; k < graph->frames[graph->frameSize].verticesSize; ++k)
			{
				edgesSize = graph->frames[i].vertices[j].edgesSize;
				graph->frames[i].vertices[j].edges[edgesSize].to = &graph->frames[graph->frameSize].vertices[k];

				graph->frames[i].vertices[j].edges[edgesSize].correctionEdge = graph->frames[i].vertices[j].edgesSize == 0 ? FALSE : TRUE;

				int from[2] = { graph->frames[i].vertices[j].human->x, graph->frames[i].vertices[j].human->y };
				int to[2] = { graph->frames[graph->frameSize].vertices[k].human->x, graph->frames[graph->frameSize].vertices[k].human->y };

				// Gain function specified in http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.2.7478&rep=rep1&type=pdf 
				float weight = alpha * (0.5 + (dotProduct(from, to, 2)/ 2 * norm(to, 2) * norm(from, 2))) + (1 - alpha) * (1 - (norm(subtract2dVector(from, to, 2), 2))/sqrt(pow(32,2) + pow(24,2)));
				
				graph->frames[i].vertices[j].edges[edgesSize].weight = weight;
				graph->frames[i].vertices[j].edgesSize++;
			}
		}
	}
}

void printPaths(struct Graph graph) {
	if (graph.frameSize < 2) {
		return;
	}
	printf("\n");
	int counter = 1;
	for (int i = 0; i < graph.frames[0].verticesSize; ++i)
	{
		struct Vertex *vertex = &graph.frames[0].vertices[i];
		while(vertex->edgesSize > 0){
			printf("Path %d - x: %d y: %d\n", counter, vertex->human->x, vertex->human->y);
			vertex = vertex->edges[0].to;
		}
		counter++;
	}
}

char isInAnotherPath(struct Man ***paths, struct Vertex *vertex, int numberOfPaths, int *sizesOfPaths ) {
	for (int i = 0; i < numberOfPaths; ++i)
	{
		for (int j = 0; j < sizesOfPaths[i]; ++j)
		{
			if (paths[i][j]->intensity == vertex->human->intensity) {
				return TRUE;
			}
		}
	}
	return FALSE;
}

void evaluateInsAndOuts(struct Graph *graph) {
	struct Man ***paths = (struct Man***) malloc(MAX_PEOPLE * sizeof(struct Man**));
	for (int i = 0; i < MAX_PEOPLE; ++i)
	{
		paths[i] = (struct Man**) malloc(MAX_PEOPLE * sizeof(struct Man*));
		
	}
	int counterPaths = 0;
	int counters[MAX_PEOPLE]; 
	memset(counters, 0, MAX_PEOPLE * sizeof(int));
	char newPath = FALSE;
	for (int i = 0; i < graph->frameSize; ++i)
	{
		for (int j = 0; j < graph->frames[i].verticesSize; ++j)
		{
			struct Vertex *vertex = &graph->frames[i].vertices[j];
			while(vertex->edgesSize > 0){
				if(!isInAnotherPath(paths, vertex, counterPaths, counters)){
					printf("Path %d - x: %d y: %d\n", counters[counterPaths], vertex->human->x, vertex->human->y);
					paths[counterPaths][counters[counterPaths]] = vertex->human;
					counters[counterPaths]++;
					newPath = TRUE;
					vertex = vertex->edges[0].to;
				}else {
					break;
				}
				
			}
			if (newPath) {
				counterPaths++;
			}
		}
	}
}

void detectDirection(struct Image *images, int currentImage, int *in, int *out) {
	int previousSize = 0;
	
	struct Graph graph;
	graph.frameSize = 0;
	int idCounter = 0;
	for (int i = currentImage; i >= currentImage - IMAGE_NUM; --i){
		int index = getIndexForImages(i);
		if (index == -1) {
			continue;
		}
		if (images[index].time > 5) {
			break;
		}
		
		struct Frame *frame = &graph.frames[graph.frameSize];
		frame->verticesSize = 0;
		struct Man *people = images[index].people;
		for (int j = 0; j < images[index].size; ++j){
			frame->vertices[frame->verticesSize].idRight = idCounter++;
			frame->vertices[frame->verticesSize].idLeft = idCounter++;
			frame->vertices[frame->verticesSize].human = &people[j];
			frame->vertices[frame->verticesSize].edges = (struct Edge*) malloc(MAX_PEOPLE_2 * sizeof(struct Edge));
			memset(frame->vertices[frame->verticesSize].edges, 0, MAX_PEOPLE_2 * sizeof(struct Edge));
			frame->vertices[frame->verticesSize].edgesSize = 0;
			frame->vertices[frame->verticesSize].frameIndex = graph.frameSize;
			frame->verticesSize++;
		}

		addEdges(&graph);
		calculateVertexDisjointMaximumWeightPathCover(&graph);
		printPaths(graph);
		graph.frameSize++;
	}

	evaluateInsAndOuts(&graph);

	
	for (int i = 0; i < graph.frameSize; ++i)
	{
		for (int j = 0; j < graph.frames[i].verticesSize; ++j)
		{
			free(graph.frames[i].vertices[j].edges);
		}
	}
}
struct Image* initImages() {
	struct Image *image = (struct Image*) malloc(IMAGE_NUM * sizeof(struct Image));
	image[0].size = 2;
	image[0].people[0].x = 5;
	image[0].people[0].y = 4;
	
	image[1].size = 2;
	image[1].people[0].x = 5;
	image[1].people[0].y = 5;
	image[1].people[1].x = 24;
	image[1].people[1].y = 15;

	image[2].size = 2;
	image[2].people[0].x = 20;
	image[2].people[0].y = 20;
	image[2].people[1].x = 23;
	image[2].people[1].y = 12;

	image[3].size = 2;
	image[3].people[0].x = 8;
	image[3].people[0].y = 15;
	image[3].people[1].x = 22;
	image[3].people[1].y = 8;
	
	image[4].size = 2;
	image[4].people[0].x = 7;
	image[4].people[0].y = 20;
	image[4].people[1].x = 21;
	image[4].people[1].y = 4;

	return image;
}

int main ( void )
{
	
	static const char filename[] = "in-gaussian-sideways";
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
	

	
	struct Image images[IMAGE_NUM];
	int imagesIndex = 0; 
	float numbers[768];
	memset(numbers, 0, 768);

	int *histogram;
	
	int peopleSize = 0;

	int lineNum = 0;
	float min, max;
	int AVG_TRAINING = 8;
	int trainingCycles = 0;

	float maxOfBackground = -99999999999;
	ssize_t read;
	char *line = NULL;
	size_t len = 0;
	int in = 0, out = 0;
	long currentImage = 0;

	// while ((read = getline(&line, &len, file)) != -1) {
	// 	trainingCycles++;
		
		
	// 	char *numberStrings[768];
	// 	memset(numberStrings, 0, 768);
	// 	int i = 0;
	// 	char *p = strtok( line, ",");
	// 	while (p != NULL) {
	// 		numberStrings[i++] = p;
	// 		p = strtok(NULL, ",");
	// 	}
	// 	for (int i = 0; i < 768; ++i)
	// 	{
	// 		numbers[i] = strtof(numberStrings[i], NULL);
	// 	}

	// 	float gaussians[768];
	// 	memcpy(gaussians, numbers, sizeof(gaussians));
	// 	// float *gaussians = applyGaussian(numbers, 32, 24, gausian);
	// 	// printFloatMatrix(normalizeFile, gaussians, 32, 24);

	// 	calculateHistogram(gaussians, 768, &min, &max);
	// 	float threshold = findThreshold(gaussians, 768, (max - min) / 10);
		
	// 	setThreshold(gaussians, 768, threshold);
	// 	imagesIndex = getIndexForImages(imagesIndex);
	// 	peopleSize = images[imagesIndex].size = 0;
	// 	int *detected = detectPeople(gaussians, numbers, 32, 24, images[imagesIndex].people, &peopleSize);
	// 	images[imagesIndex].size = peopleSize;
	// 	images[imagesIndex].time = currentImage;
		

	// 	printIntMatrix(outputfile, detected, 32, 24);
		
	// 	// printf("Image n. %d - Threshold: %f\n", trainingCycles, threshold);
	// 	struct Man *people = images[imagesIndex].people;
	// 	printf("Image n. %d\n", imagesIndex);
	// 	for (int i = 0; i < images[imagesIndex].size; ++i)
	// 	{
	// 		printf("Man detected at x - %d, y - %d, width - %d, height - %d, space - %d, intensity - %f\n", people[i].x, people[i].y, people[i].width, people[i].height, people[i].space, people[i].intensity);
	// 	}
		
	// 	if (imagesIndex == 4) {
	// 		detectDirection(images, imagesIndex, &in, &out);
	// 	}	
		
	// 	// printf("In - %d, Out - %d \n", in, out);

	// 	imagesIndex++;
	// 	currentImage++;
	// }
	struct Image *imageTest = initImages();
		
	detectDirection(imageTest, 4, &in, &out);
		
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
