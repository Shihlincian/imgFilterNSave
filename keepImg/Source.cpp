#include <opencv2/opencv.hpp>
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/objdetect/objdetect.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/ml/ml.hpp"
#include "opencv2/video/background_segm.hpp"
#include "opencv2/highgui.hpp"

#include "vector"
#include "string"

#include "ScreenVideoCapture.h"
#include <time.h>
#include <iostream>
#include <stdio.h> 

#define START_BUF_SIZE 100

//BGS
// Create empy input img, foreground and background image and foreground mask.
cv::Mat img, img8U, foregroundMask, backgroundImage, foregroundImg;
// Init background substractor
cv::Ptr<cv::BackgroundSubtractor> bg_model = cv::createBackgroundSubtractorMOG2(300);

//save
std::vector<cv::Mat> imgtmp;
int saveCount = 0;
int saveEndCount = 0;
int bufCount = 0;
bool saveBeginFlag = false;
bool saveEndFlag = false;
int frameCounts = 0;
int compareNum = 31;

void BGS()
{
	static int frameBGSCounts = 0;

	// loadRawImgFrame(img, endNum_cp);
	//img.convertTo(img8U, CV_8U, 255.0 / 3000);

	// create foreground mask of proper size
	if (foregroundMask.empty())
	{
		foregroundMask.create(img.size(), img.type());
	}

	// compute foreground mask 8 bit image
	// -1 is parameter that chose automatically your learning rate

	if (frameBGSCounts <= 100)
	{
		bg_model->apply(img, foregroundMask);
		frameBGSCounts++;
	}
	else
	{
		bg_model->apply(img, foregroundMask, 0);
	}
	// bg_model->apply(img8U, foregroundMask, 0);

	// smooth the mask to reduce noise in image
	GaussianBlur(foregroundMask, foregroundMask, cv::Size(11, 11), 3.5, 3.5);

	// threshold mask to saturate at black and white values
	threshold(foregroundMask, foregroundMask, 10, 255, cv::THRESH_BINARY);

	// create black foreground image
	foregroundImg = cv::Scalar::all(0);
	// Copy source image to foreground image only in area with white mask
	img.copyTo(foregroundImg, foregroundMask);

	////Get background image
	//bg_model->getBackgroundImage(backgroundImage);
}

int computeForeground(cv::Mat foregroundImg)
{
	int foregroundCount = 0;
	for (int i = 0; i < foregroundImg.rows; i++)
	{
		for (int j = 0; j < foregroundImg.cols; j++)
		{
			if (foregroundImg.at<uchar>(i, j) > 10)
			{
				foregroundCount++;
			}
		}
	}

	return foregroundCount;
}

void captureHwnd(HWND window, cv::Rect2d targetArea, cv::Mat& dest)
{
	HDC hwindowDC, hwindowCompatibleDC;

	HBITMAP hbwindow;
	BITMAPINFOHEADER  bi;

	hwindowDC = GetDC(window);
	hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
	SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

	dest.create(targetArea.height, targetArea.width, CV_8UC4);

	// Initialize a bitmap
	hbwindow = CreateCompatibleBitmap(hwindowDC, targetArea.width, targetArea.height);
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = targetArea.width;
	// The negative height is required -- removing the inversion will make the image appear upside-down.
	bi.biHeight = -targetArea.height;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	SelectObject(hwindowCompatibleDC, hbwindow);
	// Copy from the window device context to the bitmap device context
	// Use BitBlt to do a copy without any stretching -- the output is of the same dimensions as the target area.
	BitBlt(hwindowCompatibleDC, 0, 0, targetArea.width, targetArea.height, hwindowDC, targetArea.x, targetArea.y, SRCCOPY);
	// Copy into our own buffer as device-independent bitmap
	GetDIBits(hwindowCompatibleDC, hbwindow, 0, targetArea.height, dest.data, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

	// Clean up memory to avoid leaks
	DeleteObject(hbwindow);
	DeleteDC(hwindowCompatibleDC);
	ReleaseDC(window, hwindowDC);
}

std::string to_format(const int number) 
{
	std::stringstream ss;
	ss << std::setw(2) << std::setfill('0') << number;
	return ss.str();
}

std::string getTime2Str() 
{
	time_t tt;
	time(&tt);
	tt = tt + 8 * 3600;  // transform the time zone
	tm* t = gmtime(&tt);

	std::string output = std::to_string(t->tm_year + 1900) + to_format(t->tm_mon + 1) +
						 to_format(t->tm_mday) + "_" + to_format(t->tm_hour) +
						 to_format(t->tm_min) + to_format(t->tm_sec);

	return output;
}

int main()
{
	HWND targetWindow = NULL;
	targetWindow = GetDesktopWindow();
	cv::Rect2d test(0, 0, 480, 270);



	for(int a = 0; a < 200; a++)
	{

		captureHwnd(targetWindow, test, img);

		if (cv::waitKey(15) == 27) break;


		if (imgtmp.size() < START_BUF_SIZE)
		{
			imgtmp.push_back(img);
			continue;
		}
		else
		{
			imgtmp[START_BUF_SIZE - 1] = img;
			for (int i = 0; i < imgtmp.size() - 1; i++)
			{
				imgtmp[i] = imgtmp[i + 1];
			}
		}


		BGS();
		cv::imshow("img", img);
		cv::imshow("foregroundImg", foregroundImg);
		if (cv::waitKey(30) == 27) break;
		frameCounts++;
	}

	while (1)
	{

		captureHwnd(targetWindow, test, img);

		if (cv::waitKey(15) == 27) break;


		if (imgtmp.size() < START_BUF_SIZE)
		{
			imgtmp.push_back(img);
			continue;
		}
		else
		{
			for (int i = 0; i < imgtmp.size() - 1; i++)
			{
				imgtmp[i] = imgtmp[i + 1].clone();
			}
			imgtmp[START_BUF_SIZE - 1] = img;
		}


		BGS();
		cv::imshow("img", img);
		cv::imshow("foregroundImg", foregroundImg);
		if (cv::waitKey(30) == 27) break;

		saveEndFlag = false;
		if (computeForeground(foregroundImg) > 10000)
		{			
			if ((frameCounts - compareNum) > 10)
			{
				if (!saveBeginFlag)
				{
					std::cout << "save" << std::endl;

					for (int i = 0; i < imgtmp.size(); i++)
					{
						cv::imwrite("C:\\Projects\\keepImg\\keepImg\\img\\" + std::to_string(saveCount) + "_" + getTime2Str() + ".png", imgtmp[i]);
						saveCount++;
					}
					saveBeginFlag = true;
				}
			}
			else
			{
				cv::imwrite("C:\\Projects\\keepImg\\keepImg\\img\\" + std::to_string(saveCount) + "_" + getTime2Str() + ".png", imgtmp[START_BUF_SIZE - 1]);
				saveCount++;
				saveBeginFlag = false;
			}
			compareNum = frameCounts;

			saveEndFlag = true;
			saveEndCount = 1;
		}

		if (!saveEndFlag)
		{
			if (saveEndCount == 1) 
			{
				static int endIndex = 1;
				cv::imwrite("C:\\Projects\\keepImg\\keepImg\\img\\" + std::to_string(saveCount) + "_" + getTime2Str() + ".png", imgtmp[START_BUF_SIZE - 1]);
				saveCount++;
				if (endIndex == START_BUF_SIZE)
				{
					endIndex = 0;
					saveEndCount = 0;
					std::cout << "save end" << std::endl;
				}
				endIndex++;
			}
		}

		frameCounts++;
	}
}



