
#ifndef _MLX_count_people_
#define _MLX_count_people_
   struct Man
   {
       int x;
       int y;
       int width;
       int height;
   };

   static const float gausian[5][5] = {
                                 {0.002969,0.013306,0.021938,0.013306,0.002969},
                                 {0.013306,0.059634,0.098320,0.059634,0.013306},
                                 {0.021938,0.098320,0.162103,0.098320,0.021938},
                                 {0.013306,0.059634,0.098320,0.059634,0.013306},
                                 {0.002969,0.013306,0.021938,0.013306,0.002969}
                              };


   float* applyGaussian(float *src, int widthOfImage, int heightOfImage);
   float findAvg(float *src, int size);
   float findThreshold(float *src, int size, float minimumStep);
   void findMinMax(float *src, int size, float *min, float *max);
   void setThreshold(float *src, int size, float threshold);
   void detectPeople(float *src, int width, int height, struct Man *people, int *peopleSize);

#endif