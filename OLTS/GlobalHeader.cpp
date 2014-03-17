#include "GlobalHeader.h"

using namespace nii;
using namespace std;

// Include these namespaces so we can synthesize voices
#ifdef WIN32
using namespace System;
using namespace System::Speech::Synthesis;
/*ref class SpeechSyn{
public:
	SpeechSynthesizer speaker;
};
*/

#endif //WIN32

namespace nii{
	

	boost::mutex positionMutex;

	/* This mutex isn't in use. For future use, when we start using multiple trackers */
	boost::mutex multipleTrackerMutex;

	/* Shared variable we will use to pass position between our Sound and Vision threads */
	int objectPosition = -1;

	/* Shared variable we will use to pass timeStamps between the all of our threads */
	unsigned long long int milliGlobal;

	/* 
		We'll pass around the time stamp generated in the Full stream recording by using 
		this mutex and shared variable
	*/
	boost::mutex fullStreamTimeMutex;
	unsigned long long int fullStreamTime;

	/* Shared variable. Not in use */
	//bool globValid = false;

	//void doProcess(VisionThread *vthrd1, int yieldCount);

	/* These are variables for the generating the sound map */
	int RADIUS = 50;
	int VAR_RADIUS = RADIUS;

	/* 
		Using this to determine when we should fully release the Streaming,
		and recording threads 
	*/

	int numberOfVideoRecordingThreads = 0;
	boost::mutex videoRecordingThreadMutex;

	/* When user quits this program, threads check this shared variable */
	boost::mutex programShouldExitMutex;
	bool programShouldExit = false;

	/* Let's create an Array/Vector which contains all of the sound codes */
	std::vector<int> soundCodeVectorPointer;

	/* 
		Use this to turn on virtual sound source, and server communication. 
		This value is set if a command line option '--vss' is passed to the program
	*/
	bool shouldUseVirtualSoundSource = false;


	/* 
	 * Write the position of the object and other info to this JSON file 
	 */

	FILE *output_json = NULL;

	/*
	 * Remember the last valid object position.  Used in SoundThread.cpp
	 */

	bool useLastValidObjectPosition = false;
}

/**********************************************/
/************** Socket code(s) ****************/
/**********************************************/


std::string createRequestString(std::vector<std::string> & request){
	std::ostringstream requestStream;
	for(unsigned int i = 0; i < request.size(); i++){
		requestStream << request[i];
	}

	std::string retVal = requestStream.str();
	return retVal;
}

/**********************************************/
/************  Recording Thread ***************/
/**********************************************/

/*
	The purpose of this function is to record a full version of 
	our webcam stream.  When recording from the 'Vision' thread, we 
	cannot record at a desired frame rate of 30fps.  This is because
	of the intense vision algorithms that are occuring while we process
	the a frame.

	Thus, we can spawn another thread (the Recording thread, the function below), 
	and record the camera's stream in parallel while we track the object in the 
	'Vision' thread.
*/

void recordFullStream(CvCapture *capture, int argc, char *argv[]){
	//CvCapture * capture = NULL;
    //capture = cvCreateCameraCapture(0);

	int length = 50;
	int newLength = 63;
	char *vidFileName = new char[length];
	char *txtFileName = new char[length];
	char newVidFileName[63];
	char newTxtFileName[63];
	int soundToPlay = -5;
	
	memset(newVidFileName, '\0', newLength);
	strcpy(newVidFileName, "full_stream_");
	
	memset(newTxtFileName, '\0', newLength);
	strcpy(newTxtFileName, "full_stream_");

	memset(vidFileName, '\0', length);

	//int isFile = strcmp(txtFile, "");
	if(argc == 9)
	{
		strncpy(txtFileName, argv[7],50);
		strncat(newTxtFileName, txtFileName, length);

		strncpy(vidFileName, argv[8],50);
		strncat(newVidFileName, vidFileName, length);	
		
		DEBUG2(vidFileName);
		DEBUG2(newVidFileName);
	}else if(argc == 4){
		//Create a file name that concatenates the command line argument we passed in
		strncpy(txtFileName, argv[1], length);
		strncat(newTxtFileName, txtFileName, length);

		strncpy(vidFileName, argv[2], length);
		strncat(newVidFileName, vidFileName, length);

	}else{
		strncat(newTxtFileName, ".txt", length);
		strncat(newVidFileName, ".avi", length); 
	}

	if(capture == NULL){
		cout << "Web cam not chosen" << endl;
		return;
	}

	FILE *output_txt = fopen(newTxtFileName, "wt");
	IplImage *img = cvQueryFrame(capture);
	CvVideoWriter *writer = cvCreateVideoWriter(newVidFileName, 0, 30, cvGetSize(img));
	DEBUG2("Made video writer");
	
	//int64 ticks; 
	//int fps=30; 
	//double freq,et; 
	//double mspf=1000.0/(double)fps; // desired milliseconds per frame 
	//freq=cvGetTickFrequency()*1000.0; // ticks per millisecond

	int i = 0;
	while(img){ 
		//ticks=cvGetTickCount(); // start tick 
		img=cvQueryFrame(capture); 

		std::time_t t = std::time(0);
		DEBUG2("In Record full stream Seconds since January 1st 1970: ");
		DEBUG2(t);
		/*
		* The code below grabs the system time using windows standards.  
		* It grabs the milliseconds passed, and then these milliseconds
		* are essentially concatenated to UNIX time.
		*/
#ifdef WIN32
		SYSTEMTIME stMS;
		GetSystemTime(&stMS);
		unsigned long long newMilli = t*1000 + stMS.wMilliseconds;
		DEBUG2("In record full stream millisecond timestamp");
		DEBUG2(newMilli);

		{
			boost::mutex::scoped_lock lock(fullStreamTimeMutex);
			fullStreamTime = newMilli;
		}

		if(output_txt != NULL)
		{
			//int key = cvWaitKey(0);
			fprintf(output_txt, "%llu\n", newMilli);
		}
#endif
		if(!img){ 
			break;
		}
		//cvShowImage("Full Stream",img); 

		//cout << "Writing Frame " << i++ << endl;

		cvWriteFrame(writer,img); 
		//et=(cvGetTickCount()-ticks)/freq; // elapsed time for current frame, in milliseconds 

  
		// this will throttle the writing of frames to the desired rate 
		//  assuming that the amount of time required to read and write frames is consistent 
		//  if et > mspf, then the desired fps is not achievable 

		char c;//=cvWaitKey(et<mspf?mspf-et:1.0); 
		//c = cvWaitKey(1);
		if(c == 33 || c == 'q' || c == 'Q' || programShouldExit) {
			{
				boost::mutex::scoped_lock lock(programShouldExitMutex);
				programShouldExit = true;
			}
			break;
		}
		//boost::this_thread::yield();
	}
	cvReleaseVideoWriter(&writer);
	{
		boost::mutex::scoped_lock lock(videoRecordingThreadMutex);
		numberOfVideoRecordingThreads--;
		DEBUG2(numberOfVideoRecordingThreads);
		if(numberOfVideoRecordingThreads == 0){
			DEBUG2("Releasing in recorder");
			
			cvReleaseCapture(&capture);
		}
	}
	
}

/**********************************************/
/******** Voice Recording Function ************/
/**********************************************/

void recordSoundToWaveFiles(){

	std::vector<int> localSoundCodeVector = soundCodeVectorPointer;
	/* Make sure sounds/words have been pushed to the vector */
	/*if(soundCodeVectorPointer == NULL){
		return;
	}*/
	
	/* Let's initialize a Synthesizer to use for writing to a wav file */
	
	/* 
		Iterate through the list of sound codes. Write a sound file
		on each iteration (using synthesizer from above).
	*/


	cout << "the sound code size is: " << localSoundCodeVector.size() << endl;
	int i = 0;
	for(std::vector<int>::iterator it=soundCodeVectorPointer.begin(); 
		it != soundCodeVectorPointer.end(); ++it){
	

	//for(unsigned int i = 0; i < localSoundCodeVector.size(); i++){
			
			try {
				cout << "in Sound try and the code is " << *it << endl;
				SpeechSynthesizer ^ theSpeaker = gcnew SpeechSynthesizer();
				theSpeaker->Rate = SPEAKING_RATE;
				theSpeaker->Volume = 100;
				if(*it != -1){
					theSpeaker->SetOutputToWaveFile("wavDir/soundCommand" + i + ".wav");
				}
				switch(*it){
					case UP:
						theSpeaker->Speak("Up");
						break;
					case DOWN:
						theSpeaker->Speak("Down");
						break;
					case LEFT:
						theSpeaker->Speak("Left");
						break;
					case RIGHT:
						theSpeaker->Speak("Right");
						break;
					case UPPER_LEFT:
						theSpeaker->Speak("Up and Left");
						break;
					case UPPER_RIGHT:
						theSpeaker->Speak("Up and Right");
						break;
					case LOWER_LEFT:
						theSpeaker->Rate = SPEAKING_RATE_SLOW;
						theSpeaker->Speak("Down and Left");
						break;
					case LOWER_RIGHT:
						theSpeaker->Rate = SPEAKING_RATE_SLOW;
						theSpeaker->Speak("Down and Right");
						break;
					case CENTER:
						theSpeaker->Speak("Center");
						break;
					default:
						break;
				}

				//theSpeaker delete;
			}catch (System::Exception ^ ex){
				cout << "Exception Caught. recordSoundToWaveFiles()" << endl;
			}

			i++;
	}

	/* Clean up */

	//theSpeaker delete;


}