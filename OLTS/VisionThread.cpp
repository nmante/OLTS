#include "GlobalHeader.h"
//#include "VisionThread.h"

//boost::mutex positionMutex;
using namespace nii;
using namespace std;

void VisionThread::testMSBuild(){
	std::cout << "Success!!!" << std::endl;
}

/*
	Responsible for Bluetooth connections

*/

bool VisionThread::openBluetoothConnection(){
#ifdef WIN32
	WSADATA wsd;
	int error = WSAStartup (MAKEWORD(1, 0), &wsd);
	if (error)
	{
		std::cout << "Error making wsa bluetooth connection" << std::endl;
		return false;
	}

	if(wsd.wVersion != MAKEWORD(1,0)){
		WSACleanup();
		return false;
	}
#endif

	SOCKET server_socket = socket(af_bt, SOCK_STREAM, BTHPROTO_RFCOMM);
	SOCKADDR_BTH sa;
	memset(&sa, 0, sizeof(sa));
	int channel = 0
	sa.addressFamily = AF_BT;
	sa.port = channel & 0xff;

	if (bind(server_socket, (SOCKADDR *)&sa, sizeof(sa)))
	{
		// Server binding failed.  TODO error handling

		closesocket(server_socket);
		return 0;

	}

	if (listen(server_socket, 5)){
		// Do some error handling
		closesocket(server_socket);
		return 0;
	}


	SOCKADDR_BTH sa2;
	int size = sizeof(sa2);
	SOCKET s2 = accept (server_socket, (SOCKADDR *)&sa2, &size);
	mSocket = s2;

}

/*
	Responsible for openning connection with server
*/
bool VisionThread::openConnection(char *ipAddress, int portNo){
#ifdef WIN32
	//Startup the winsocket, the wsastartup calls a windows dll(aka .so)
	//responsible for making win socket calls
	WSADATA wsadata;
	int error = WSAStartup(0x0202, &wsadata);

	//Check to see if we connected to/started the winsocket dll
	if (error)
	{
		std::cout << "Error making wsa connection" << std::endl;
		return false;
	}	

	//Now check to see if we got winsock2 (not winsock)
	if (wsadata.wVersion != 0x0202)
	{
		WSACleanup();
		return false;
	}
#endif

	//Create object which holds the socket information, like portNo, ip addr,
	//familu 
	SOCKADDR_IN target;
	target.sin_family = AF_INET;
	target.sin_port = htons(portNo);
	target.sin_addr.s_addr = inet_addr(ipAddress);

	//Keep in mind AF_UNIX, SOCK_DGRAM, 0 for udp connection
	mSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); //create socket
	if (mSocket == INVALID_SOCKET)
	{
		return false;
	}

	//Now let's connect to the socket
	if (connect(mSocket, (SOCKADDR *)&target, sizeof(target)) == SOCKET_ERROR)
	{
		//connection failed
		return false;
	}else{
		//connection succeeded
		return true;
	}
}

/*
	Responsible for closing our connection with the server
*/

void VisionThread::closeConnection(){
	//Close the connection if it exists
	if (mSocket)
	{
		closesocket(mSocket);
	}	

#ifdef WIN32
	WSACleanup();
#endif
}

/*
	Error printing/handling for our sockets
*/
void VisionThread::killConnectionWithError(char *message){
#ifdef WIN32
	fprintf(stderr, "%s: %d\n", message, WSAGetLastError());
#else
	perror(message);
#endif
	exit(1);
}

/**********************************************/
/************** Vision Thread *****************/
/**********************************************/

/* Contructor with C based stream capture object cvCap*/
VisionThread::VisionThread(CvCapture *cvCap, int mCon,IplImage *mDraw, IplImage *mFra, CvRect *mInit, CvVideoWriter *mOutAvi, FILE *mOutTxt){
	mCapture = cvCap;
	mContext = mCon;
	mDrawImg = mDraw;
	mFrame = mFra;
	mInitRect = mInit;
	mOutput_avi = mOutAvi;
	mOutput_txt = mOutTxt;
}

/* Constructor with C++ based stream capture object vidCap */
VisionThread::VisionThread(cv::VideoCapture *vidCap, int mCon,IplImage *mDraw, IplImage *mFra, CvRect *mInit, cv::VideoWriter *mOutAvi, FILE *mOutTxt){
	mVidCap = vidCap;
	mContext = mCon;
	mDrawImg = mDraw;
	mFrame = mFra;
	mInitRect = mInit;
	mVidWrt = mOutAvi;
	mOutput_txt = mOutTxt;
}

/* Dectructor */
VisionThread::~VisionThread(){
	mCapture = NULL;
	mDrawImg = NULL;
	mFrame = NULL;
	mInitRect = NULL;
	mOutput_avi = NULL;
	mOutput_txt = NULL;
}


/***********************************/
/******** Vision Thread ************/
/***********************************/

void VisionThread::doVibProcess(VisionThread *vThrd1, int yieldCount)
{
	
}
void VisionThread::doVssProcess(VisionThread *vThrd1, int yieldCount){
	boost::mutex::scoped_lock lock(multipleTrackerMutex);
	int myObjectPosition = -1;
	double kCameraWidth = (double) vThrd1->mFrame->width;
	double kCameraHeight = (double) vThrd1->mFrame->height;

	char *ipAddr = "127.0.0.1";
	int portNumber = 3000;
	bool successfulConnection = openConnection(ipAddr, portNumber);
	if(!successfulConnection){
		killConnectionWithError("In VisionThread::doVssProcess... Could not connect to server");
		return;
	}	

    //cout << "Frame per second " <<(int) cvGetCaptureProperty(vThrd1->mCapture, CV_CAP_PROP_FPS) << endl;
	/*
		One of the 'black box' functions provided to us by the Context Tracker algorithm
		It initializes the tracker with a frame (from our camera stream), the bounding box
		to track, and a bool for whether or not we want to use context info
	*/
    init(vThrd1->mFrame, *vThrd1->mInitRect, vThrd1->mContext == 1);

	//line(Mat(vThrd1->mFrame), cvPoint((int)kCameraWidth/2.0 - RADIUS, 0), cvPoint((int)kCameraWidth/2.0 - RADIUS, kCameraHeight), cv::Scalar(0,0,200), 1, 8, 0);

	/*
		Take the frame (mFrame) and pass it through the tracking algorithms
	*/
	int fileNumber = 0;
    while (vThrd1->mFrame)
    {
		//ticks=cvGetTickCount(); // start tick 
        cvCopyImage(vThrd1->mFrame, vThrd1->mDrawImg);

        // Pass it through the tracker
        track(vThrd1->mFrame);

        // get result
        CvRect r;
        float  confidence;
        bool   valid;
		
		//Determine the position of the new rectangle which fits the object
		//Determine how 'good' of a fit the rectangle and give us a confidence score
		//Determine if it's a valid rectangle
        getRoi(&r, &confidence, &valid);

        // Draw a green or yellow rectangle if the object is right or wrong, respectively
        cvDrawRect(vThrd1->mDrawImg, cvPoint(r.x, r.y), 
            cvPoint(r.x + r.width - 1, r.y + r.height - 1),
            valid ? cvScalar(0, 255, 0) : cvScalar(0, 255, 255),
            2
        );

		//Determine the position of the CENTER of the object/bounding box
		//r.x and r.y give you the position of the top left corner
		double xpos = (double)((double)r.x + (double)r.width/2.0);
		double ypos = (double)((double)r.y + (double)r.height/2.0);
		
		double cameraWidth = (double) vThrd1->mFrame->width;
		double cameraHeight = (double) vThrd1->mFrame->height;
		double centerDistance = sqrt(xpos*xpos + ypos*ypos);

		//Some debug statements for a sanity check. This DEBUG2 macro is defined in GlobalHeader.h
		DEBUG2(r.x);
		DEBUG2(r.y);
		DEBUG2(xpos);
		DEBUG2(ypos);
		DEBUG2(r.width);
		DEBUG2(r.height);
		DEBUG2(cameraWidth);
		DEBUG2(cameraHeight);
		DEBUG2(centerDistance);

		/* 
			If the object we tracked this frame is right, then let's send that position to our server
			At the same time, our local web browser will be grabbing info from the server to play sounds
		*/
		
			
		if(valid){
			/*
				Create a request with HTTP headers and a body
				Each line of the request will be pushed to a vector. The vector then gets
				processed by the 'createRequestString()' function to create a full
				HTTP request.
			 */
			std::map<std::string, std::string> requestMap;
			std::ostringstream requestStream, jsonStream, contentLengthStream;
			std::vector<std::string> requestVector;
			std::ostringstream fileNumberStream;
			fileNumberStream << fileNumber;
			std::string jsonFileName = std::string("output") + fileNumberStream.str() + std::string(".json");
			

			requestVector.push_back(std::string("POST /position HTTP/1.1\r\n"));
			requestVector.push_back(std::string("Host: www.google.com\r\n"));
			
			requestVector.push_back(std::string("Content-Type: application/json\r\n"));
			jsonStream << "{\n\t\"xpos\": " << xpos << ",\n\t\"ypos\": " << ypos;
			jsonStream << ",\n\t\"cameraWidth\": " << cameraWidth << ",\n\t\"cameraHeight\":" << cameraHeight << "\n}";
			std::string jsonString = jsonStream.str();

			contentLengthStream << "Content-Length: " << jsonString.length() << "\r\n";
			requestVector.push_back(contentLengthStream.str());
			requestVector.push_back(std::string("Connection: Keep-Alive\r\n"));
			requestVector.push_back(std::string("\r\n"));
			requestVector.push_back(jsonString);


			std::string requestString = createRequestString(requestVector);
			DEBUG2("Printing to output_json"); 
			if (output_json)
			{
				fprintf(output_json, requestString.c_str());
			}
			
			DEBUG2("Printing to");
			DEBUG2(jsonFileName);
			if (SHOULD_OUTPUT_JSON_TEST)
			{
				FILE *jsonDocument = NULL;
				jsonDocument = fopen(jsonFileName.c_str(), "w");
				fprintf(jsonDocument, jsonString.c_str());
				fclose(jsonDocument);
				fileNumber++;
			}
			
			size_t reqBuflength = strlen(requestString.c_str());//strlen(request);

			//Send the request to our server via a socket
			int size = send(mSocket, requestString.c_str(), reqBuflength, 0);
			std::cout << "Sent: " << size << " bytes. Message is " << requestString << std::endl;
				
		}

		std::time_t t = std::time(0);

#ifdef WIN32
		SYSTEMTIME stMS;
		GetSystemTime(&stMS);
		unsigned long long milli = t*1000 + stMS.wMilliseconds;
		unsigned long long fullStreamTimeLocal;
		DEBUG2(milli);
#endif

		DEBUG2(myObjectPosition);
		{
			boost::mutex::scoped_lock lock(positionMutex);
			objectPosition = myObjectPosition;
			milliGlobal = milli;
		}



		{
			boost::mutex::scoped_lock lock(fullStreamTimeMutex);
			fullStreamTimeLocal = fullStreamTime;
		}

		writeLogo(vThrd1->mDrawImg,"USC-IRIS");

        cvShowImage("Tracking", vThrd1->mDrawImg);
        
		
        
		//Output the position/size info, and the frame to text and video files
        if (vThrd1->mOutput_txt)
		{
            fprintf(mOutput_txt, "%d %d %d %d %llu %llu %d\n", r.x, r.y, r.width, r.height, fullStreamTimeLocal, milli, myObjectPosition);
		}
		if (vThrd1->mOutput_avi)
        {
			cvWriteFrame(mOutput_avi, mDrawImg);
		}

        //Wait to see if the user presses something
		int key = cvWaitKey(1);
		/*
			Exit the program if the user presses 'q', in this thread or any of the other threads.

			We'll update the 'objectPosition' shared variable, so that the 'SoundThread' can 
			exit as well.
		*/
		if (key == 'q' || key == 'Q' || programShouldExit){
			myObjectPosition = -2;
			{
				/*
					If the user quits from the vision thread, then alert all the other threads
					by updating the 'programShouldExit' shared variable
				*/
				boost::mutex::scoped_lock lock(programShouldExitMutex);
				programShouldExit = true;
			}
			{
				boost::mutex::scoped_lock lock(positionMutex);
				objectPosition = myObjectPosition;
			}
            break;
		}
        vThrd1->mFrame = cvQueryFrame(vThrd1->mCapture);
		for(int i = 0; i < yieldCount; i++){
			boost::this_thread::yield();
		}
	}

	/*
		Here if the user quit the program.  Close all of the files, webcam streams
		that were functioning during the 
	*/
    if (vThrd1->mOutput_txt){
        fclose(vThrd1->mOutput_txt);
    }
    if (vThrd1->mOutput_avi){
		cvReleaseVideoWriter(&vThrd1->mOutput_avi);
		/*boost::mutex::scoped_lock lock(videoRecordingThreadMutex);
		numberOfVideoRecordingThreads--;

		if(numberOfVideoRecordingThreads == 0){
			DEBUG2("Releasing video writer and capture");
			
			cvReleaseCapture(&vThrd1->mCapture);
			return;
		}
		*/
	}
	if (output_json)
	{
		fclose(output_json);
	}

	// Create an HTTP Request.  We're POSTing an empty position to the
	// server.  This tells the server to stop it's sound playing service
	std::ostringstream contentLengthStream;
	std::vector<std::string> reqVector;
	reqVector.push_back(std::string("POST /stop HTTP/1.1\r\n"));
	reqVector.push_back(std::string("Host: www.google.com\r\n"));
	reqVector.push_back(std::string("Content-type: application/json\r\n"));
	std::string jsonString = "{}";
	contentLengthStream << "Content-Length: " << jsonString.length() << "\r\n";
	reqVector.push_back(contentLengthStream.str());
	reqVector.push_back(std::string("Connection: Keep-Alive\r\n"));
	reqVector.push_back(std::string("\r\n"));
	reqVector.push_back(jsonString);
	std::string reqString = createRequestString(reqVector);

	size_t reqBuflength = strlen(reqString.c_str());//strlen(request);

	//Send the request to our server via a socket
	int size = send(mSocket, reqString.c_str(), reqBuflength, 0);
	std::cout << "Sent: " << size << " bytes. Message is " << reqString << std::endl;

	// Kill the connection
	closeConnection();
	
	// Multiple threads are using the video stream from the camera
	// Thus, we can't kill the connection to the video stream  'cvReleaseCapture' until all video recording threads have exited
	// This block is written in all video recording feeds
	{
		boost::mutex::scoped_lock lock(videoRecordingThreadMutex);
		numberOfVideoRecordingThreads--;
		DEBUG2(numberOfVideoRecordingThreads);
		if(numberOfVideoRecordingThreads == 0){
			DEBUG2("Releasing the capture, in vision thread");
			cvReleaseCapture(&vThrd1->mCapture);
			
		}
	}


}
void VisionThread::doProcess(VisionThread *vThrd1, int yieldCount){
	/*
		Because this is a multithreaded application, we must use locks to 
		circumvent race conditions
	*/
	boost::mutex::scoped_lock lock(multipleTrackerMutex);
	int myObjectPosition = -1;
	double kCameraWidth = (double) vThrd1->mFrame->width;
	double kCameraHeight = (double) vThrd1->mFrame->height;

    //cout << "Frame per second " <<(int) cvGetCaptureProperty(vThrd1->mCapture, CV_CAP_PROP_FPS) << endl;
	/*
		One of the 'black box' functions provided to us by the Context Tracker algorithm
		It initializes the tracker with a frame (from our camera stream), the bounding box
		to track, and a bool for whether or not we want to use context info
	*/
    init(vThrd1->mFrame, *vThrd1->mInitRect, vThrd1->mContext == 1);

	//line(Mat(vThrd1->mFrame), cvPoint((int)kCameraWidth/2.0 - RADIUS, 0), cvPoint((int)kCameraWidth/2.0 - RADIUS, kCameraHeight), cv::Scalar(0,0,200), 1, 8, 0);

	/*
		Take the frame (mFrame) and pass it through the tracking algorithms
	*/
    while (vThrd1->mFrame)
    {
		//ticks=cvGetTickCount(); // start tick 
        cvCopyImage(vThrd1->mFrame, vThrd1->mDrawImg);

        // Pass it through the tracker
        track(vThrd1->mFrame);

        // get result
        CvRect r;
        float  confidence;
        bool   valid;
		
		//Determine the position of the new rectangle which fits the object
		//Determine how 'good' of a fit the rectangle and give us a confidence score
		//Determine if it's a valid rectangle
        getRoi(&r, &confidence, &valid);

        // Draw a green or yellow rectangle if the object is right or wrong, respectively
        cvDrawRect(vThrd1->mDrawImg, cvPoint(r.x, r.y), 
            cvPoint(r.x + r.width - 1, r.y + r.height - 1),
            valid ? cvScalar(0, 255, 0) : cvScalar(0, 255, 255),
            2
        );

		//Determine the position of the CENTER of the object/bounding box
		//r.x and r.y give you the position of the top left corner
		double xpos = (double)((double)r.x + (double)r.width/2.0);
		double ypos = (double)((double)r.y + (double)r.height/2.0);
		
		double cameraWidth = (double) vThrd1->mFrame->width;
		double cameraHeight = (double) vThrd1->mFrame->height;
		double centerDistance = sqrt(xpos*xpos + ypos*ypos);

		//Some debug statements for a sanity check. This DEBUG2 macro is defined in GlobalHeader.h
		DEBUG2(r.x);
		DEBUG2(r.y);
		DEBUG2(xpos);
		DEBUG2(ypos);
		DEBUG2(r.width);
		DEBUG2(r.height);
		DEBUG2(cameraWidth);
		DEBUG2(cameraHeight);
		DEBUG2(centerDistance);

		/* 
			If the object we tracked this frame is right, then let's calculate the 'objectPosition' code,
			and pass it to our sound thread
		*/
		if(valid){

			/*
				We'll calculate the distance of the object from the center of the frame
				The camera frame depends on the resolution of the camera in pixels.

				Our current camera is 640x480 pixels.  Thus, the center is (320, 240)

				Additionally, the top left corner of our frame is pixel (0,0)
			*/

			if(centerDistance < RADIUS){//center
				myObjectPosition = CENTER;
			}else{
				if(xpos < (cameraWidth/2.0 - RADIUS) 
					&& ypos < (cameraHeight/2.0 - RADIUS)){//upper left

						myObjectPosition = UPPER_LEFT;

				}else if(xpos > (cameraWidth/2.0 - RADIUS) 
					&& xpos < (cameraWidth/2.0 + RADIUS) 
					&& ypos < (cameraHeight/2.0 -RADIUS)
					|| 
					(ypos < (cameraHeight/2.0 - VAR_RADIUS) 
					&& xpos > (cameraWidth/2.0 - VAR_RADIUS) 
					&& xpos < (cameraWidth/2.0 + VAR_RADIUS) )){//up

						/* 
							I've changed the if statement slightly. A slightly higher bias will be given to
							the left and right portions if the radius is decreased. 
						 */
						myObjectPosition = UP;

				}else if(xpos > (cameraWidth/2.0 + RADIUS) 
					&& ypos < (cameraHeight/2.0 - RADIUS)){//upper right

						myObjectPosition = UPPER_RIGHT;

				}else if(xpos < (cameraWidth/2.0 - VAR_RADIUS) 
					&& ypos > (cameraHeight/2.0 - RADIUS)
					&& ypos < (cameraHeight/2.0 + RADIUS)){//left

						/* 
							I've changed the if statement slightly. A slightly higher bias will be given to
							the left and right portions if the radius is decreased. 
						 */
						myObjectPosition = LEFT;

				}else if(xpos > (cameraWidth/2.0 + VAR_RADIUS) 
					&& ypos > (cameraHeight/2.0 - RADIUS)
					&& ypos < (cameraHeight/2.0 + RADIUS)){//right

						/* 
							I've changed the if statement slightly. A slightly higher bias will be given to
							the left and right portions if the radius is decreased. 
						 */

						myObjectPosition = RIGHT;

				}else if(xpos < (cameraWidth/2.0 - RADIUS) 
					&& ypos > (cameraHeight/2.0 + RADIUS)){//lower left

						myObjectPosition = LOWER_LEFT;

				}else if(xpos > (cameraWidth/2.0 - RADIUS) 
					&& xpos < (cameraWidth/2.0 + RADIUS) 
					&& ypos > (cameraHeight/2.0 + RADIUS)
					|| 
					(ypos > (cameraHeight/2.0 + VAR_RADIUS) 
					&& xpos > (cameraWidth/2.0 - VAR_RADIUS) 
					&& xpos < (cameraWidth/2.0 + VAR_RADIUS) )){//down


						/* 
							I've changed the if statement slightly. A slightly higher bias will be given to
							the left and right portions if the radius is decreased. 
						 */
						myObjectPosition = DOWN;

				}else if(xpos > (cameraWidth/2.0 + RADIUS) 
					&& ypos > (cameraHeight/2.0 + RADIUS)) {//lower right

					myObjectPosition = LOWER_RIGHT;

				}else{
					/* 
					 * This is the portion outside of the center circle
					 * that is also not apart of the other regions above
					 */
					myObjectPosition = CENTER;

				}
			}
		}else{
			myObjectPosition = -1;
		}

		std::time_t t = std::time(0);

#ifdef WIN32
		SYSTEMTIME stMS;
		GetSystemTime(&stMS);
		unsigned long long milli = t*1000 + stMS.wMilliseconds;
		unsigned long long fullStreamTimeLocal;
		DEBUG2(milli);
#endif

		DEBUG2(myObjectPosition);
		{
			boost::mutex::scoped_lock lock(positionMutex);
			objectPosition = myObjectPosition;
			milliGlobal = milli;
		}



		{
			boost::mutex::scoped_lock lock(fullStreamTimeMutex);
			fullStreamTimeLocal = fullStreamTime;
		}

		writeLogo(vThrd1->mDrawImg,"USC-IRIS");

        cvShowImage("Tracking", vThrd1->mDrawImg);
        
		
        
		//Output the position/size info, and the frame to text and video files
        if (vThrd1->mOutput_txt)
		{
            fprintf(mOutput_txt, "%d %d %d %d %llu %llu %d\n", r.x, r.y, r.width, r.height, fullStreamTimeLocal, milli, myObjectPosition);
		}
		if (vThrd1->mOutput_avi)
        {
			cvWriteFrame(mOutput_avi, mDrawImg);
		}

        //Wait to see if the user presses something
		int key = cvWaitKey(1);
		/*
			Exit the program if the user presses 'q', in this thread or any of the other threads.

			We'll update the 'objectPosition' shared variable, so that the 'SoundThread' can 
			exit as well.
		*/
		if (key == 'q' || key == 'Q' || programShouldExit){
			myObjectPosition = -2;
			{
				/*
					If the user quits from the vision thread, then alert all the other threads
					by updating the 'programShouldExit' shared variable
				*/
				boost::mutex::scoped_lock lock(programShouldExitMutex);
				programShouldExit = true;
			}
			{
				boost::mutex::scoped_lock lock(positionMutex);
				objectPosition = myObjectPosition;
			}
            break;
		}
        vThrd1->mFrame = cvQueryFrame(vThrd1->mCapture);
		for(int i = 0; i < yieldCount; i++){
			boost::this_thread::yield();
		}
	}

	/*
		Here if the user quit the program.  Close all of the files, webcam streams
		that were functioning during the 
	*/
    if (vThrd1->mOutput_txt)
        fclose(vThrd1->mOutput_txt);
    if (vThrd1->mOutput_avi){
		cvReleaseVideoWriter(&vThrd1->mOutput_avi);
		/*boost::mutex::scoped_lock lock(videoRecordingThreadMutex);
		numberOfVideoRecordingThreads--;

		if(numberOfVideoRecordingThreads == 0){
			DEBUG2("Releasing video writer and capture");
			
			cvReleaseCapture(&vThrd1->mCapture);
			return;
		}
		*/
	}

	// Multiple threads are using the video stream from the camera
	// Thus, we can't kill the connection to the video stream  'cvReleaseCapture' until all video recording threads have exited
	// This block is written in all video recording feeds
	{
		boost::mutex::scoped_lock lock(videoRecordingThreadMutex);
		numberOfVideoRecordingThreads--;
		DEBUG2(numberOfVideoRecordingThreads);
		if(numberOfVideoRecordingThreads == 0){
			DEBUG2("Releasing the capture, in vision thread");
			cvReleaseCapture(&vThrd1->mCapture);
		}
	}
}


