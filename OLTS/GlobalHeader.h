#ifndef _GLOBALHEADER_H_
#define _GLOBALHEADER_H_

/* Includes from general C/C++ libraries */
#include <cstdio>
#include <iostream>
#include "time.h"
#include <stdio.h>
#include <math.h>
#include <string>

/* Includes from Boost and Synthesis portions of the project */
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
//#include <voce.h>

/* Platform Specific Libraries */
#ifdef WIN32
#include <Windows.h>
#include <BluetoothAPIs.h>
#endif


/* Includes from the OpenCV side of the project */

#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "ctracker.h"


/* Includes from the PortAudio side of the project */
//#include <asio.h>
//#include <pa_asio.h>
//#include <portaudio.h>

/* Includes from Nii Mante */
#include "GenericThread.h"
#include "VisionThread.h"
#include "RegionChooser.h"
#include "SoundThread.h"
#include <vector>
#include <string>
#include <map>

//static std::string POST = "POST";
//static std::string GET = "GET";
//static std::string PUT = "PUT";
//static std::string DELETE = "DELETE";
//static std::string PATCH = "PATCH";

/*
	Need these namespacing declarations to make Boost work. 
	Boost is a library that gives us powerful C++ tools.  We're using it in this
	program specifically for Multithreading.
*/


namespace boost {
	struct thread::dummy {};
}

namespace boost { 
    namespace detail { 

#ifdef WIN32
        namespace win32 { 
            struct _SECURITY_ATTRIBUTES: public ::_SECURITY_ATTRIBUTES {}; 
        };

#endif /* WIN32 */

    };
};


namespace nii{

	/**
	This enum type is used to pass information between the 'Sound' and 'Vision' threads.  It is
	a shared variable.

	The 'Vision' thread determines the position of the object (x,y in pixels).  It then determines
	the proper enum based on the objects position on screen.
	
*/

	extern enum ObjectPosition {
		UPPER_LEFT = 0,
		UP,
		UPPER_RIGHT,
		LEFT,
		CENTER,
		RIGHT,
		LOWER_LEFT,
		DOWN,
		LOWER_RIGHT
	};
	#define SPEAKING_RATE 2
	#define SPEAKING_RATE_SLOW 3

	/* Use positionMutex when manipulating the shared variable 'objectPosition' */
	extern boost::mutex positionMutex;

	/* This mutex isn't in use. For future use, when we start using multiple trackers */
	extern boost::mutex multipleTrackerMutex;

	/* Shared variable we will use to pass position between our Sound and Vision threads */
	extern int objectPosition;

	/* Shared variable we will use to pass timeStamps between the all of our threads */
	extern unsigned long long int milliGlobal;

	/* 
		We'll pass around the time stamp generated in the Full stream recording by using 
		this mutex and shared variable
	*/
	extern boost::mutex fullStreamTimeMutex;
	extern unsigned long long int fullStreamTime;

	/* Shared variable. Not in use */
	//bool globValid = false;

	//void doProcess(VisionThread *vthrd1, int yieldCount);

	/* These are variables for the generating the sound map */
	extern int RADIUS;
	extern int VAR_RADIUS;

	/* 
		Using this to determine when we should fully release the Streaming,
		and recording threads 
	*/

	extern int numberOfVideoRecordingThreads;
	extern boost::mutex videoRecordingThreadMutex;

	/* When user quits this program, threads check this shared variable */
	extern boost::mutex programShouldExitMutex;
	extern bool programShouldExit;

	/* Let's create an Array/Vector which contains all of the sound codes */
	extern std::vector<int> soundCodeVectorPointer;

	/* 
		Use this to turn on virtual sound source, and server communication. 
		This value is set if a command line option '--vss' is passed to the program
	*/
	extern bool shouldUseVirtualSoundSource;


	/* 
	 * Write the position of the object and other info to this JSON file 
	 */

	extern FILE *output_json;

	/*
	 * Remember the last valid object position.  Used in SoundThread.cpp
	 */

	extern bool useLastValidObjectPosition;
}



#define _DEBUG_ 1

#define ABS(x) (((x) > 0) ? (x) : (-(x)))

#define DEBUG(x) do { \
  if (_DEBUG_) { std::cerr << x << std::endl; } \
} while (0)

#define DEBUG2(x) do { \
	if(_DEBUG_) {std::cerr << #x << ": " << x << std::endl; } \
} while (0)

#define SHOULD_OUTPUT_JSON_TEST 0
/*
	Helper functions for writing position, logo and text to an opencv Image/frame
*/
//void writeText(IplImage * img, const char * text, 
//					  CvPoint position, CvScalar color);

//void writeLogo(IplImage * img, char* str);


//void writePos(IplImage * img, char* str);

/*
	Responsible for recording the full webcam stream at a high frame rate
*/
void recordFullStream(CvCapture *cvCap, int argc, char *argv[]);

/*
	Responsible for recording the sound(s) to a wave file(s).
*/

void recordSoundToWaveFiles();

/* Utiltity function for making */

std::string createRequestString(std::vector<std::string> & request);
#endif /* _GLOBALHEADER_H_ */
