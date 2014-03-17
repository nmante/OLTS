/**
	Nii Mante
	University of Southern California

	This code combines Computer Vision algorithms with Speech Synthesis software to aid the Visually Impaired with Object Localization.

	This code consists of three worker threads and the main thread.  The three threads are:
		- Vision Thread
		- Sound Thread
		- Video Recording Thread

	Overall Flow of the Program
	=====================
	- In main()
	1. In main() - Program starts, and a live stream of the webcam's field of view is displayed on screen
	2. Press X
	3. Draw a box around the object of interest
	4. Pass the 'streaming' webcam object to the threads in step 5
	5. Three threads are spawned
		5a. Vision, Sound or Video Recording Thread. Because any of these threads can occur first, the first
		step in each one is '5a'.
	
	- In Vision Thread
	5a. Grab a frame from the 'stream' from step 5. Pass the frame through the three main funcions (init, getRoi, track)
	6. Position is outputted by algorithm
	7. An integer code (soundCode) is sent to the sound thread.  The code represents where in the field of view the 
	object is
	8. Loop back to 5a and grab a new frame

	- In Sound Thread
	5a1. If we're doing sound localization, then we need to communicate via sockets to the node/express server
	5a2. Checks the (soundCode). Plays the sound corresponding to that code.  For example "0" corresponds to 
	'upperLeft'
	6. Loop back to 5a

	- In Recording thread
	5a. Grab a frame from the 'stream'
	6. Write it to a file
	7. Loop back

*/

#include "GlobalHeader.h"
#include <queue>

using namespace std;
using namespace cv;
using std::vector;
using namespace nii;


int main(int argc, char * argv[])
{
	/*
		This program gives the user of using command line arguments to prime
		the program with important info.  The syntax is below.

		<program_name_exe> <video_file> <x> <y> <width> <height> <context> <output_text_file> <output_video_file>

		Example:
			OLTS.exe "" 0 0 0 0 1 outputText.txt outputVideo.avi

		The video_file can be passed in for processing, or if a "" is passed instead then the program
		will use the webcam.

		The x, y, width, and height, allow you to pass in an initial bounding box rectangle to the program

	*/

	bool webcam = true;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
    int context = 1;
	int length = 50;
	char *vidFile = "";
	char *txtFile = "";
	char *outVidFile = "";
	char *jsonFile = ""; //new char[50];
	int numTrackers = 2;
	VisionThread *vthrd;
	vector<VisionThread*> visVector;
	vector<char*> args(argv + 1, argv + argc);

	// Iterate through command line arguments for options
	// Options are prefixed with a '--'
	// We can pass file names or integer values to use in the program
	for (vector<char *>::iterator i = args.begin(); i != args.end(); ++i) {

        if (*i == "-h" || *i == "--help") {
			cout << "To turn on virtual sound source, pass --vss as a command line option" << endl;
			cout << "=========================" << endl;
			cout << "For standard context tracking, with recording, and WITHOUT varied sound map" << endl;
            cout << "***.exe video_file x y width height context output_txt output_avi" << endl;
			cout << "=========================" << endl;
			cout << "For standard context tracking, with recording, and WITH varied sound map" << endl;
			cout << "***.exe <patientName_mmddyy_bbSize_testNum>.txt <patientName_mmddyy_bbSize_testNum>.avi bbSize" << endl;
			cout << "=========================" << endl;
			cout << "For standard context, without recording or varied soundmap" << endl;
			cout << "***.exe" << endl;
            return 0;
        } else if (strcmp(*i, "--vss") == 0) {
        	DEBUG("Should use virtual sound source");
            //Turn on sound source by setting a flag used in the vision thread
			shouldUseVirtualSoundSource = true;
        } else if (strcmp(*i, "--ivid") == 0) {
			vidFile = *++i;
			DEBUG2(vidFile);
		} else if (strcmp(*i, "--otxt") == 0){
			txtFile = *++i;
			DEBUG2(txtFile);
		} else if (strcmp(*i, "--ovid") == 0) {
			outVidFile = *++i;
			DEBUG2(outVidFile);
		} else if (strcmp(*i, "--x") == 0) {
			x = atoi(*++i);
		} else if (strcmp(*i, "--y") == 0) {
			y = atoi(*++i);
		} else if (strcmp(*i, "--w") == 0) {
			w = atoi(*++i);
		} else if (strcmp(*i, "--h") == 0) {
			h = atoi(*++i);
		} else if (strcmp(*i, "--context") == 0) {
			context = atoi(*++i);
		} else if (strcmp(*i, "--radius") == 0) {
			VAR_RADIUS = atoi(*++i);
		} else if (strcmp(*i, "--json") == 0) {
			jsonFile = *++i;
			DEBUG2(jsonFile);
		}
    }

	// Shorthand notation for using the program
	// If they're not using the Virtual sound source, they can use 1, 4 or 9 
	// arguments to start the program
	if(!shouldUseVirtualSoundSource){
		if(argc == 9){
		
			//They passed in 9 arguments, so lets store their provided arguments
			webcam = strcmp(argv[1], "") == 0;
			x = atoi(argv[2]);
			y = atoi(argv[3]);
			w = atoi(argv[4]);
			h = atoi(argv[5]);
			context = atoi(argv[6]);
			strncpy(vidFile, argv[1],50);
			strncpy(txtFile, argv[7],50);
			strncpy(outVidFile, argv[8],50);
			//numTrackers = 1;

		}else if (argc == 1){
			cout << "Using default parameters" << endl;
		}else if(argc == 4){
	 			/*
	 				they passed in 3 arguments, so we'll create an output text and vid file.
	 				additionally, we'll alter the sound thread's processing technique by using
	 				the last argument
	 			*/
	 			//strncpy(txtFile, argv[1], strlen(argv[1]));
				txtFile = argv[1];
				DEBUG2(txtFile);
	 			//strncpy(outVidFile, argv[2], strlen(argv[2]));
				outVidFile = argv[2];
				DEBUG2(outVidFile);
	 			VAR_RADIUS = atoi(argv[3]);
		}
	}

	/*
		Let's initialize the stream we're capturing frames from
	*/
    CvCapture * capture = NULL;
    if (webcam){
		cout << "Creating Webcam" << endl;
        capture = cvCreateCameraCapture(1);
	}else{
        capture = cvCreateFileCapture(vidFile);
	}

	/*
		Grab an image, then create a copy for drawing a bounding box on
	*/
    IplImage  * frame   = cvQueryFrame(capture);
    IplImage  * drawImg = cvCloneImage(frame);

    /*
		Create file pointers/buffers for our output text and video files
	*/
    FILE * output_txt = NULL;
    CvVideoWriter * output_avi = NULL;
	cout << txtFile << endl;
	cout << outVidFile << endl;
    if (strcmp(txtFile, "") != 0){
		cout << "Creating text file for context tracker output" << endl;
        output_txt = fopen(txtFile, "wt");
		fprintf(output_txt, "x, y, width, height, full_stream_time_stamp, time_stamp, object_position\n");
	}
    if (strcmp(outVidFile, "") != 0){
		cout << "Creating video file for context tracker output" << endl;
        output_avi = cvCreateVideoWriter(outVidFile, 0, 25, cvGetSize(frame));
	}
	if (strcmp(jsonFile, "") != 0){
		cout << "Creating json file for context tracker output" << endl;
        	output_json = fopen(jsonFile, "wt");
	}
    
	/*
		Allow the user to freeze a frame (frame) from the camera stream, then draw a 
		bounding box (initRect) around the object of choice.  They can freeze the
		frame by pressing 'X'
	*/
	CvRect initRect = {x, y, w, h};
	int key=0;
		
	if (w == 0 && h == 0){
			
		while (key!='q'&&key!='Q'&&frame)
		{
			cvCopyImage(frame, drawImg);

			writeLogo(drawImg,"USC-IRIS");
			cvShowImage("Press x to draw. q to quit.", drawImg);
			key = cvWaitKey(1);
			if (key == 'x'||key=='X' || !webcam)
			{
				RegionChooser rc;
				initRect = rc.chooseRegion(drawImg);
				break;
			}else if(key == 'q' || key == 'Q'){
				if(output_txt){
					fclose(output_txt);
				}
				if(output_avi){
					cvReleaseVideoWriter(&output_avi);
				}
				cvReleaseCapture(&capture);
				exit(0);
			}
			frame = cvQueryFrame(capture);
		}
	}

	
	/* 
		We'll create an object which encapsulates the necessary features for the tracker
		This object needs the 
			video stream (capture)
			drawableImage (drawImg)
			frame
			output video (output_avi)
			output text (output_txt)
	*/
	vthrd = new VisionThread(capture,
		context, drawImg,
		frame, &initRect,
		output_avi,
		output_txt);

	/*
		Create a thread_group so we can queue up all of our worker threads (sound, vision, record)
	*/
	boost::thread_group thread_group;
	
	/*
	//Not in use, eventually use this to create multiple trackers
	for(int i = 0; i < numTrackers; i++){

		//thread_group.create_thread(boost::bind(&VisionThread::doProcess, boost::ref(visVector.at(i)), visVector.at(i), 20));
	}
	*/

	
	if(!shouldUseVirtualSoundSource){
		/*
			Create the vision thread, and pass the VisionThread object's 
				'doProcess(VisionThread *, int yieldCount)' 
			as a call back to the thread.  We bind arguments to thread by passing them
			as arguments to the bind function
		*/
		thread_group.create_thread(boost::bind(&VisionThread::doProcess, boost::ref(vthrd), vthrd, 1));
		numberOfVideoRecordingThreads++;

		// Create a sound thread for Synthesizing speech
		thread_group.create_thread(boost::bind(&doSpeech, argc, argv));
	}else{
		// Create a Computer Vision thread which talks to theserver
		thread_group.create_thread(boost::bind(&VisionThread::doVssProcess, boost::ref(vthrd), vthrd, 1));
		numberOfVideoRecordingThreads++;

	}
	

	// Create a thread for recording the full video stream (fps of 30)
	thread_group.create_thread(boost::bind(&recordFullStream, capture, argc, argv));
	numberOfVideoRecordingThreads++;

	// Invoke the threads and leave the main thread
	thread_group.join_all();
	
	/*
		We reach here once the other threads have completed/been quit from:
			Vision thread - tracks and detects object
			Sound thread - generates speech based on object position
		Call a the function which writes our sound to a file
	*/

	//recordSoundToWaveFiles();
    return 1;
}
