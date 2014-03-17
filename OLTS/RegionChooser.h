#ifndef _REGIONCHOOSER_H_
#define _REGIONCHOOSER_H_ value

#include <opencv/cv.h>
#include <opencv/highgui.h>
//#include <opencv2/opencv.hpp>
#include <time.h>


#define ABS(x) (((x) > 0) ? (x) : (-(x)))
static void writeText(IplImage * img, const char * text, 
					  CvPoint position = cvPoint(0, 30), CvScalar color = cvScalar(255, 255, 255))
{
	static CvFont font;
	static bool   init = false;
	if (!init)
	{
		cvInitFont(&font, CV_FONT_HERSHEY_PLAIN, 1, 1);
		init = true;
	}
	cvPutText(img, text, position, &font, color);
}

static void writeLogo(IplImage * img, char* str)
{
	static clock_t  time = clock();
	/*    MyUtils::writeText(img, MyUtils::makeString("[%d] %2dms/frame", c, clock() - time), 
	cvPoint(0, 30), COLOR_YELLOW);*/
	writeText(img, str, 
		cvPoint(10, img->height-10), CV_RGB(255,255,255));
	time = clock();
}

class RegionChooser
{
private:
    char * windowName;
    bool drawing;
    CvPoint p1, p2;
    CvRect rect;

public:
    RegionChooser(char * window = NULL):
      drawing(false), rect(cvRect(10, 10, 10, 10)),
          p1(cvPoint(10, 10)), p2(cvPoint(20, 20))
      {
          windowName = window;
          if (windowName == NULL)
              windowName = "RegionChooser";
      }

      CvRect chooseRegion(IplImage * img)
      {
          cvNamedWindow(windowName);
		      writeLogo(img,"USC-IRIS");
          cvShowImage(windowName, img);
          cvSetMouseCallback(windowName, mouseCB, this);

          IplImage * showImg = (IplImage *) cvClone(img);

          while (1)
          {
              cvCopy(img, showImg);
              cvRectangle(showImg, p1, p2, cvScalar(20, 30, 100));
              cvShowImage(windowName, showImg);
              if (cvWaitKey(20) > 0)
                  break;
          }

          cvReleaseImage(&showImg);
          cvDestroyWindow(windowName);
          return rect;
      }

	  CvRect chooseRegion2(cv::Mat *img)
      {
          cvNamedWindow(windowName);
		  //writeLogo(img,"USC-IRIS");
          cvShowImage(windowName, img);
          cvSetMouseCallback(windowName, mouseCB, this);

          IplImage * showImg = (IplImage *) cvClone(img);

          while (1)
          {
              cvCopy(img, showImg);
              cvRectangle(showImg, p1, p2, cvScalar(20, 30, 100));
              cvShowImage(windowName, showImg);
              if (cvWaitKey(20) > 0)
                  break;
          }

          cvReleaseImage(&showImg);
          cvDestroyWindow(windowName);
          return rect;
      }

      static void mouseCB(int event, int x, int y, int flags, void * param)
      {
          RegionChooser * me = (RegionChooser *) param;

          if (event == CV_EVENT_LBUTTONDOWN)
          {
              me->p1.x = x;
              me->p1.y = y;
              me->p2 = me->p1;
              me->drawing = true;
          }
          if (event == CV_EVENT_MOUSEMOVE && me->drawing == true)
          {
              me->p2.x = x;
              me->p2.y = y;
          }
          if (event == CV_EVENT_LBUTTONUP && me->drawing == true)
          {
              me->drawing = false;

              // Find the ROI region
              me->rect.x = MIN(me->p1.x, x);
              me->rect.y = MIN(me->p1.y, y);
              me->rect.width = ABS(x - me->p1.x);
              me->rect.height = ABS(y - me->p1.y);

              if (me->rect.width <= 0)
                  me->rect.width = 1;
              if (me->rect.height <= 0)
                  me->rect.height = 1;
          }
      }
};

#endif // _REGIONCHOOSER_H_