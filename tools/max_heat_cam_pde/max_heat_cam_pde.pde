// When there is a sizeable amount of data on the serial port
// read everything up to the first linefeed


String[] lines;
int j = 0 ;
void setup() {
  size(480, 400);  // Size must be the first statement
  //noStroke();
  
  colorMode(HSB, 360, 100, 100);
  lines = loadStrings("/Users/simonharvan/Documents/Development/Arduino/counting-people/lib/tmp-8");
}

void draw() {
  print(j + " - " + lines.length + "\n");
  if (j >= lines.length - 1) {
    noLoop();
  }
  
  String splitString[] = new String[1000];
  float maxTemp = 0;
  float minTemp = 500;
  String myString = null;
  float[] temps = new float[768];
   myString = lines[j];
  // Limit the size of this array so that it doesn't throw
  // OutOfBounds later when calling "splitTokens"
  if (myString.length() > 16000) {
    myString = myString.substring(0, 16000);
  }
  
  // generate an array of strings that contains each of the comma
  // separated values
  splitString = splitTokens(myString, ",");
  
  // Reset our min and max temperatures per frame
  maxTemp = 0;
  minTemp = 1;  
  
  // For each floating point value, double check that we've acquired a number,
  // then determine the min and max temperature values for this frame
  for (int q = 0; q < 768; q++) {
  
    if (!Float.isNaN(float(splitString[q])) && float(splitString[q]) > maxTemp) {
      maxTemp = float(splitString[q]);
    } else if (!Float.isNaN(float(splitString[q])) && float(splitString[q]) < minTemp) {
      minTemp = float(splitString[q]);
    }
  }  
  
  // for each of the 768 values, map the temperatures between min and max
  // to the blue through red portion of the color space
  for (int q = 0; q < 768; q++) {
    if (!Float.isNaN(float(splitString[q]))) {
      temps[q] = constrain(map(float(splitString[q]), minTemp, maxTemp, 180, 360), 160, 360);
    } else {
      temps[q] = 0;
    }
  }
  
  
  
  // Prepare variables needed to draw our heatmap
  int x = 0;
  int y = 0;
  int i = 0;
  background(0);   // Clear the screen with a black background
  
  
  
  while (y < 360) {
  
  
    // for each increment in the y direction, draw 8 boxes in the 
    // x direction, creating a 64 pixel matrix
    while (x < 480) {
      // before drawing each pixel, set our paintcan color to the 
      // appropriate mapped color value
      fill(temps[i], 100, 100);
      rect(x, y, 15, 15);
      x = x + 15;
      i++;
    }
  
    y = y + 15;
    x = 0;
  }
  saveFrame("line-###.png");
  
  // Add a gaussian blur to the canvas in order to create a rough
  // visual interpolation between pixels.
  //filter(BLUR, 7);
  
  
  // Generate the legend on the bottom of the screen
  //textSize(32);
  
  // Find the difference between the max and min temperatures in this frame
  //float tempDif = maxTemp - minTemp; 
  //// Find 5 intervals between the max and min
  //int legendInterval = round(tempDif / 5); 
  //// Set the first legend key to the min temp
  //int legendTemp = round(minTemp);
  
  //// Print each interval temperature in its corresponding heatmap color
  //for (int intervals = 0; intervals < 6; intervals++) {
  //  fill(constrain(map(legendTemp, minTemp, maxTemp, 180, 360), 160, 360), 100, 100);
  //  text(legendTemp+"°", 70*intervals, 390);
  //  legendTemp += legendInterval;
  //}
  j++;
  delay(1000);
}

void mousePressed(){
   looping =  !looping;  //EDIT******
}
