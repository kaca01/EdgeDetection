#include <iostream>
#include <stdlib.h>
#include <tbb/task_group.h>
#include "BitmapRawConverter.h"

#define __ARG_NUM__				6
#define FILTER_SIZE				3
#define THRESHOLD				128

using namespace std;
using namespace tbb;

// Prewitt operators
int filterHor[FILTER_SIZE * FILTER_SIZE] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
int filterVer[FILTER_SIZE * FILTER_SIZE] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

/**
* @brief Serial version of edge detection algorithm implementation using Prewitt operator
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_prewitt(int *inBuffer, int *outBuffer, int width, int height)  //TODO obrisati
{
	for (int i = 1; i < width - 1; i++) { 
		for (int j = 1; j < height - 1; j++) {
			int Gx = 0, Gy = 0, G = 0;
			
			for (int m = -1; m <= 1; m++) {
				for (int n = -1; n <= 1; n++) {
					int index = (j + n) * width + (i + m);
					Gx += inBuffer[index] * n; 
					Gy += inBuffer[index] * m;
				}
			}
			G = abs(Gx) + abs(Gy);

			// transferring to black or white color
			if (G >= THRESHOLD) outBuffer[j * width + i] = 255;
			else outBuffer[j * width + i] = 0;
			
		}
	}
}


/**
* @brief Parallel version of edge detection algorithm implementation using Prewitt operator
* 
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/


void filter_parallel_prewitt(int row, int col, int width, int height, int *inBuffer, int *outBuffer, int _width)
{
	if (width <= 16 || height <= 16) {
		for (int i = 1; i < width; i++) {
			for (int j = 1; j < height; j++) {
				int Gx = 0, Gy = 0, G = 0;

				for (int m = -1; m <= 1; m++) {
					for (int n = -1; n <= 1; n++) {
						int index = (j + n + col) * _width + (i + m + row);
						Gx += inBuffer[index] * n;
						Gy += inBuffer[index] * m;
					}
				}
				G = abs(Gx) + abs(Gy);

				// transferring to black or white color
				if (G >= THRESHOLD) outBuffer[(j + col) * _width + i + row] = 255;
				else outBuffer[(j + col) * _width + i + row] = 0;

			}
		}
	}
	else {
		task_group t;
		t.run([&] {filter_parallel_prewitt(row - 1, col, width / 2 + 1, height / 2 + 1, inBuffer, outBuffer, _width); });
		t.run([&] {filter_parallel_prewitt(row + width / 2 - 1, col, width / 2 + 1, height / 2 + 1, inBuffer, outBuffer, _width); });
		t.run([&] {filter_parallel_prewitt(row - 1, col + height / 2 - 1, width / 2 + 1, height / 2 + 1, inBuffer, outBuffer, _width); });
		t.run([&] {filter_parallel_prewitt(row + width / 2 - 1, col + height / 2 - 1, width / 2 + 1, height / 2 + 1, inBuffer, outBuffer, _width); });
		t.wait();
	}
}

/**
* @brief Serial version of edge detection algorithm
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_serial_edge_detection(int *inBuffer, int *outBuffer, int width, int height)	//TODO obrisati
{
	// setting every element to 0 or 1, depending on threshold
	for (int i = 0; i < width; i++) {
		for (int j = 0; j < height; j++) {
			int index = j * width + i;
			if (inBuffer[index] >= THRESHOLD) inBuffer[index] = 0;
			else inBuffer[index] = 1;
		}
	}

	for (int i = 1; i < width - 1; i++) {
		for (int j = 1; j < height - 1; j++) {
			int P = 0, O = 1, G = 0;

			for (int m = -1; m <= 1; m++) {
				for (int n = -1; n <= 1; n++) {
					int index = (j + n) * width + (i + m);
					if (m == 0 && n == 0) continue;
					if (inBuffer[index] == 1) P = 1;
					else if (inBuffer[index] == 0) O = 0;
				}
			}

			G = abs(P) - abs(O);
			if (G == 0) outBuffer[j * width + i] = 0;
			else outBuffer[j * width + i] = 255;
		}
	}
}

void next_iter_parallel_edge_detection(int row, int col, int width, int height, int* inBuffer, int* outBuffer, int _width) {
	if (width <= 16 && height <= 16) {
		for (int i = 0; i < width; i++) {
			for (int j = 0; j < height; j++) {
				int index = (j + col) * _width + i + row;
				if (inBuffer[index] >= THRESHOLD) inBuffer[index] = 0;
				else inBuffer[index] = 1;
			}
		}

		for (int i = 1; i < width - 1; i++) {
			for (int j = 1; j < height - 1; j++) {
				int P = 0, O = 1, G = 0;

				for (int m = -1; m <= 1; m++) {
					for (int n = -1; n <= 1; n++) {
						int index = (j + n + col) * _width + (i + m + row);
						if (m == 0 && n == 0) continue;
						if (inBuffer[index] == 1) P = 1;
						else if (inBuffer[index] == 0) O = 0;
					}
				}

				G = abs(P) - abs(O);
				if (G == 0) outBuffer[(j + col) * _width + i + row] = 0;
				else outBuffer[(j + col) * _width + i + row] = 255;
			}
		}
	}
	else {
		task_group t;
		t.run([&] {next_iter_parallel_edge_detection(row, col, width / 2, height / 2, inBuffer, outBuffer, _width); });
		t.run([&] {next_iter_parallel_edge_detection(row + width / 2, col, width / 2, height / 2, inBuffer, outBuffer, _width); });
		t.run([&] {next_iter_parallel_edge_detection(row, col + height / 2, width / 2, height / 2, inBuffer, outBuffer, _width); });
		t.run([&] {next_iter_parallel_edge_detection(row + width / 2, col + height / 2, width / 2, height / 2, inBuffer, outBuffer, _width); });
		t.wait();
	}
}

/**
* @brief Parallel version of edge detection algorithm
* 
* @param inBuffer buffer of input image
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/
void filter_parallel_edge_detection(int *inBuffer, int *outBuffer, int width, int height)
{
	next_iter_parallel_edge_detection(0, 0, width, height, inBuffer, outBuffer, width);
}

/**
* @brief Function for running test.
*
* @param testNr test identification, 1: for serial version, 2: for parallel version
* @param ioFile input/output file, firstly it's holding buffer from input image and than to hold filtered data
* @param outFileName output file name
* @param outBuffer buffer of output image
* @param width image width
* @param height image height
*/


void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, unsigned int width, unsigned int height)
{

	// TODO: start measure
	

	switch (testNr)
	{
		case 1:
			cout << "Running serial version of edge detection using Prewitt operator" << endl;
			filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
			break;
		case 2:
			cout << "Running parallel version of edge detection using Prewitt operator" << endl;
			filter_parallel_prewitt(0, 0, width, height, ioFile->getBuffer(), outBuffer, width);
			break;
		case 3:
			cout << "Running serial version of edge detection" << endl;
			filter_serial_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
			break;
		case 4:
			cout << "Running parallel version of edge detection" << endl;
			filter_parallel_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
			break;
		default:
			cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
			break;
	}
	// TODO: end measure and display time

	ioFile->setBuffer(outBuffer);
	ioFile->pixelsToBitmap(outFileName);
}

/**
* @brief Print program usage.
*/
void usage()
{
	cout << "\n\ERROR: call program like: " << endl << endl; 
	cout << "ProjekatPP.exe";
	cout << " input.bmp";
	cout << " outputSerialPrewitt.bmp";
	cout << " outputParallelPrewitt.bmp";
	cout << " outputSerialEdge.bmp";
	cout << " outputParallelEdge.bmp" << endl << endl;
}

int main(int argc, char * argv[])
{

	if(argc != __ARG_NUM__)
	{
		usage();
		return 0;
	}

	BitmapRawConverter inputFile(argv[1]);
	BitmapRawConverter outputFileSerialPrewitt(argv[1]);
	BitmapRawConverter outputFileParallelPrewitt(argv[1]);
	BitmapRawConverter outputFileSerialEdge(argv[1]);
	BitmapRawConverter outputFileParallelEdge(argv[1]);

	unsigned int width, height;

	int test;
	
	width = inputFile.getWidth();
	height = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, argv[2], outBufferSerialPrewitt, width, height);

	// parallel version Prewitt
	run_test_nr(2, &outputFileParallelPrewitt, argv[3], outBufferParallelPrewitt, width, height);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, argv[4], outBufferSerialEdge, width, height);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, argv[5], outBufferParallelEdge, width, height);

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if(test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if(test != 0)
	{
		cout << "Edge detection FAIL!" << endl;
	}
	else
	{
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;

	delete outBufferSerialEdge;
	delete outBufferParallelEdge;

	return 0;
} 