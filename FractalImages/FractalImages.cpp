#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>

using namespace cv;
using namespace std;

static Mat computeFractals(Mat image, int iteration, Point offsetPoints[10], int reductionDenominator);
static void copyImageToOffset(Point point, Mat img, Mat reducedImg);
static Mat reduceImage(Mat originalImg, /*int reductionNumerator,*/ int reductionDenominator);

// This is a program that creates a "fractal" image from an input image.
// Arguments:
// * Image file path
// * Reduction factor. How much smaller the fractals should be. (optional, default is 2, i.e. 1/2 size)
// * Iterations. How many times we should create smaller fractals. (optional, default is 2, i.e. 3 times)
// * set of x/y offset values for the fractals (optional, default is the center of the input image)
// Example calls:
// FractalImages.exe C:\Users\DMC\Pictures\redBox.png
// FractalImages.exe C:\Users\DMC\Pictures\redBox.png 2 2 0 0 550 340
// FractalImages.exe C:\Users\DMC\Pictures\redBox.png 2 2 -100 -100 0 0 100 100 200 200 300 300 400 400 500 500 600 600
// FractalImages.exe C:\Users\DMC\Pictures\basicTree.png 3 2 150 100 500 50
// FractalImages.exe C:\Users\DMC\Pictures\basicLeaf.png 2 4 72 300
// FractalImages.exe C:\Users\DMC\Pictures\snowflake.png 4 2 40 100 230 20 430 100 40 300 430 300 230 380
int main(int argc, char** argv)
{
    const int MINIMUM_NUMBER_OF_ARGS = 2;
    const int MAXIMUM_NUMBER_OF_ARGS = 24;
    if (argc < MINIMUM_NUMBER_OF_ARGS)
    {
        cout << " Usage: " << argv[0] << " ImageToLoadAndDisplay (optional reduction, iterations, and offsets)" << endl;
        return -1;
    }
    if (argc > MAXIMUM_NUMBER_OF_ARGS) {
        cout << "Too many offset values provided. Max 10." << endl;
        return -1;
    }
    if ((argc - MINIMUM_NUMBER_OF_ARGS - 2) % 2 != 0) {
        cout << "Must have an even number of offset values" << endl;
        return -1;
    }

    // Read image
    Mat img = imread(argv[1], IMREAD_COLOR);
    imshow("Original", img);
    //waitKey(0);

    Point offsetPoints[10]; // Max 10.
    int reductionDenominator = 2;
    int iterations = 2;
    if (argc > MINIMUM_NUMBER_OF_ARGS) {
        reductionDenominator = atoi(argv[2]);
        iterations = atoi(argv[3]);
        // Offset included
        for (int i = 0; i < (argc - MINIMUM_NUMBER_OF_ARGS - 2) / 2; i++) {
            int index = 2 + 2 * (1 + i);
            int xval = atoi(argv[index]);
            int yval = atoi(argv[index + 1]);
            offsetPoints[i] = Point(xval, yval);
            //cout << i << endl;
            //cout << index << endl;
            //cout << "Point(" << xval << ", " << yval << ")" << endl;
        }
    }
    else {
        // Default center
        offsetPoints[0] = Point(img.cols / 2, img.rows / 2);
    }
    if (argc < MAXIMUM_NUMBER_OF_ARGS) {
        int startIndex = 1;
        if (argc > MINIMUM_NUMBER_OF_ARGS) {
            startIndex = (argc - MINIMUM_NUMBER_OF_ARGS - 2) / 2;
        }
        //cout << startIndex << endl;
        for (int i = startIndex; i < 10; i++) {
            // Pad the rest of the array with dummy points
            //cout << i << endl;
            offsetPoints[i] = Point(-1, -1);
        }
    }

    // Process image
    Mat fractalImg = computeFractals(img, iterations, offsetPoints, reductionDenominator);
    imshow("Out", fractalImg);
    while(getWindowProperty("Out", 0) >= 0) {
        char c = waitKey(33);
        if (c == 27) break;
    }
    //destroyWindow(argv[1]);
    return 0;
}

// Function called recursively to create smaller versions of the image and copy them into the main image at specified locations.
static Mat computeFractals(Mat image, int iteration, Point offsetPoints[10], int reductionDenominator) {
    Mat returnValue;
    Mat fractalImg;
    Mat reducedImg = reduceImage(image, reductionDenominator);
    image.copyTo(returnValue);
    Point childOffsets[10];
    for (int i = 0; i < 10; i++) {
        Point initialOffset = offsetPoints[i];
        if (initialOffset.x == -1 && initialOffset.y == -1) {
            childOffsets[i] = Point(-1, -1); // Do I need to handle -1 special?
        }
        else {
            childOffsets[i] = Point(initialOffset.x / reductionDenominator, initialOffset.y / reductionDenominator);
        }
        //cout << "Iteration: " << iteration << ", i: " << i << ", childOffset: " << childOffsets[i] << endl;
    }
    if (iteration > 0) {
        fractalImg = computeFractals(reducedImg, iteration - 1, childOffsets, reductionDenominator);
    }
    else {
        reducedImg.copyTo(fractalImg);
    }
    //cout << "About to copy to offsets. Iteration: " << iteration << endl;
    int i = 0;
    Point point = offsetPoints[0];
    while (point.x != -1 && point.y != -1) {
        //cout << "About to copy image to offset x=" << point.x << ", y=" << point.y << endl;
        copyImageToOffset(point, returnValue, fractalImg);
        i++;
        point = offsetPoints[i];
    }
    return returnValue;
}

// Copies a smaller image into the main image at a specified point.
// It does some error checking to ensure it doesn't go out of the bounds of the main image when copying.
// Arguments:
// * point - where in the main image the smaller image should be copied.
// * img - the main image
// * reducedImg - the smaller image
static void copyImageToOffset(Point point, Mat img, Mat reducedImg) {
    if (point.x > img.cols || point.y > img.rows) {
        // Skip if out of range offsets.
        return;
    }
    if (point.x < 0) { // x is out of range to the left
        if ((point.x + reducedImg.cols) > 0) { // Check if x values partially in range
            reducedImg = reducedImg(Rect(-point.x, 0, reducedImg.cols + point.x, reducedImg.rows)); // Unsure if assignment is necessary?
            point.x = 0;
        }
        else {
            return; // Really out of range.
        }
    }
    if (point.y < 0) { // y is out of range to the top
        if ((point.y + reducedImg.rows) > 0) {
            reducedImg = reducedImg(Rect(0, -point.y, reducedImg.cols, reducedImg.rows + point.y));
            point.y = 0;
        }
        else {
            return; // Really out of range.
        }
    }
    if ((point.x + reducedImg.cols) > img.cols) { // Out of range to the right
        reducedImg = reducedImg(Rect(0, 0, img.cols - point.x, reducedImg.rows));
    }
    if ((point.y + reducedImg.rows) > img.rows) { // Out of range to the bottom
        reducedImg = reducedImg(Rect(0, 0, reducedImg.cols, img.rows - point.y));
    }
    //cout << "point.x = " << point.x << ", point.y = " << point.y << endl;
    reducedImg.copyTo(img(Rect(point.x, point.y, reducedImg.cols, reducedImg.rows)));
    imshow("Out", img);
    return;
}

// Creates a smaller version of an image
// Arguments:
// originalImg - image to reduce.
// reductionDenominator - How much smaller we want the resulting image to be. For example: 2 = 1/2 the size.
static Mat reduceImage(Mat originalImg, /*int reductionNumerator,*/ int reductionDenominator) {
    Mat smoothedImg;
    GaussianBlur(originalImg, smoothedImg, Size(3, 3), 0, 0);
    Mat reducedImg;
    //float reduction = reductionNumerator / reductionDenominator;
    resize(smoothedImg, reducedImg, Size(smoothedImg.cols / reductionDenominator, smoothedImg.rows / reductionDenominator));
    return reducedImg;
}