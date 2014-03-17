#ifndef _VISIONTHREAD_H_
#define _VISIONTHREAD_H_


/*
	Nii Mante
	University of Southern California

	The VisionThread class encapsulates all of the necessary features for the 
	Context Tracker algorithm, as well as video recording info

	The Context Tracker reads in a video stream from the Camera and processes
	a frame.  

	We also need to record the frames in which an object are detected, and 
	the position of object detected.

	Thus, the Vision thread allows us to:
		
		Record the 'tracked' frames to a video file
		Record the objects position over time to a text file
		Pass information to other threads

*/

class VisionThread : public GenericThread{
public:
	IplImage  * mFrame;
	CvRect *mInitRect;
	int mContext;
	IplImage  * mDrawImg;
	FILE * mOutput_txt;
	CvVideoWriter * mOutput_avi;	
	CvCapture * mCapture;
	cv::VideoCapture *mVidCap;
	cv::VideoWriter *mVidWrt;
	SOCKET mSocket;

public:
	using GenericThread::doProcess;
	void doProcess(void*,int, int, char**);
	//Context Tracker encapsulated to this thread
	void doProcess(VisionThread *, int);

	//Context tracker encapsulated to this thread, talks to server in this thread as well
	void doVssProcess(VisionThread *, int);

	/*
	Responsible for openning connection with server
	*/
	bool openConnection(char *ipAddr, int portNo);

	/*
		Responsible for closing our connection with the server
	*/

	void closeConnection();

	/*
		Error printing/handling for our sockets
	*/
	void killConnectionWithError(char *message);

	/* 
	 	Testing msbuild compilation 
	 */
	void testMSBuild();
	VisionThread();
	VisionThread(CvCapture *cvCap, 
		int mCon,
		IplImage *mDraw, 
		IplImage *mFra, 
		CvRect *mInit, 
		CvVideoWriter *mOutAvi, 
		FILE *mOutTxt);

	VisionThread(cv::VideoCapture *vidCap, 
		int mCon,
		IplImage *mDraw, 
		IplImage *mFra, 
		CvRect *mInit, 
		cv::VideoWriter *mOutAvi, 
		FILE *mOutTxt);
	~VisionThread();
};

#endif
