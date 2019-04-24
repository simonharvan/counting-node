
#ifndef _MLX_count_people_
#define _MLX_count_people_

  #define IMAGE_NUM 10
  #define MAX_PEOPLE 5 /// Maximum possible people in one frame
  #define MAX_PEOPLE_2 25 /// Maximum possible people in two frame
  #define MAX_VERTICES 100
  #define INF (1<<29)

  struct Man
  {
    int x;
      int y;
      int width;
      int height;
      int space;
      float intensity;
      char alreadyCounted;
  };

  struct Image {
    int size;
    struct Man people[MAX_PEOPLE];
    long time;
  };




  static const float gausian[5][5] = {
                               {0.002969,0.013306,0.021938,0.013306,0.002969},
                               {0.013306,0.059634,0.098320,0.059634,0.013306},
                               {0.021938,0.098320,0.162103,0.098320,0.021938},
                               {0.013306,0.059634,0.098320,0.059634,0.013306},
                               {0.002969,0.013306,0.021938,0.013306,0.002969}
                            };


  float* applyGaussian(float *src, int widthOfImage, int heightOfImage);
  float* movingAverage(float *src, int widthOfImage, int heightOfImage);
  float findAvg(float *src, int size);
  float findThreshold(float *src, int size, float minimumStep);
  void findMinMax(float *src, int size, float *min, float *max);
  float* setThreshold(float *src, int size, float threshold);
  float getStdDev(float *src, int size);
  int* detectPeople(float *src, float *intensities, int width, int height, struct Man *people, int *peopleSize);
  void detectDirection(struct Image *images, int currentImage, int *in, int *out);
  int getIndexForImages(int index);
#endif