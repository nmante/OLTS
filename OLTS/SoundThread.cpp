#include "SoundThread.h"
#include "GlobalHeader.h"
using namespace std;
using namespace nii;


/*
	This is an instantiation of our speech synthesizer. The syntax is very strange. 
	Windows specific way of intitializing the synthesizer.
*/
#ifdef WIN32
using namespace System;
using namespace System::Speech::Synthesis;
/*ref class SpeechSyn{
public:
	SpeechSynthesizer speaker;
};
*/

#endif //WIN32

/**********************************************/
/**************  Sound Thread *****************/
/**********************************************/


void doSpeech(int argc, char *argv[]){
	
	SpeechSynthesizer ^speaker;
	int myObjectPosition = -1;
	int lastValidObjectPosition = 0;
	unsigned long long milliSharedCopy;
	FILE * output_txt = NULL;
	int length = 50;
	int newLength = 57;
	char *txtFile = new char[length];
	char newTextFile[57];
	int soundToPlay = -5;
	unsigned long long lastValidObjectTime;
	bool wavOutputFileCreated = false;
	memset(newTextFile, '\0', length+7);
	strcpy(newTextFile, "sound_");

	std::vector<int> localSoundCodeVector;
	

	memset(txtFile, '\0', length);

	//int isFile = strcmp(txtFile, "");
	if (!shouldUseVirtualSoundSource)
	{
		
	
		if(argc == 9)
		{

			strncpy(txtFile, argv[7],50);
			
			strncat(newTextFile, txtFile, length);	
			std::cout << "txtFile: " << txtFile << std::endl;
			std::cout << "newTextFile: " << newTextFile << std::endl;
		}else if(argc == 4){
			strncpy(txtFile, argv[1],50);
			
			strncat(newTextFile, txtFile, length);
		}
	}

	if (strcmp(txtFile, "") != 0){
		std::cout << "txtFile: " << txtFile << std::endl;
		std::cout << "newTextFile: " << newTextFile << std::endl;
		cout << "Creating sound file " << newTextFile << " for context tracker output" << endl;

        output_txt = fopen(newTextFile, "wt");
		fprintf(output_txt, "Column 1 = shared_time, Column 2 = new_time, Column 3 = object_position\n");
	}
	
	int newI = 0;
	while(true){
		try {
							
				// Grab the position from a shared variable 'objectPosition'
				// This variable is set in the computer vision thread 'VisionThread.cpp'
				{
					boost::mutex::scoped_lock lock(positionMutex);
					myObjectPosition = objectPosition;
					std::cout << "My Object Position" << myObjectPosition << std::endl;
					milliSharedCopy = milliGlobal;	
				}

				if(myObjectPosition == -2){
					break;
				}
				std::time_t t = std::time(0);
				std::cout << t << "In Sound Seconds since January 1st 1970" << std::endl;
				/*
				 * The code below grabs the system time using windows standards.  
				 * It grabs the milliseconds passed, and then these milliseconds
				 * are essentially concatenated to UNIX time.
				 */
#ifdef WIN32
				SYSTEMTIME stMS;
				GetSystemTime(&stMS);
				unsigned long long newMilli = t*1000 + stMS.wMilliseconds;
				DEBUG2("In sound millisecond timestamp");
				DEBUG2(newMilli);

				if(output_txt != NULL)
				{
					//int key = cvWaitKey(0);
					fprintf(output_txt, "%llu %llu %d\n", milliSharedCopy, newMilli, myObjectPosition);
				}
#endif
				if(myObjectPosition != -1) {
					lastValidObjectPosition = myObjectPosition;
					lastValidObjectTime = t;
				}

				speaker = gcnew SpeechSynthesizer();
				speaker->Voice->Gender;
				speaker->SelectVoiceByHints(VoiceGender::Female);

				speaker->Volume = 100;
				speaker->Rate = SPEAKING_RATE;
				
				
				// Not using this if block below; Hence the 'if(false)' 
				// Purpose is to remember the last valid position of the 
				// object. We do this because the object can leave the field
				// of view, so to circumvent that can save the position 
				// and keep repeating it for 5 seconds if the object is lost
				if (useLastValidObjectPosition){

					if ( (myObjectPosition == -1) && ((int)(std::time(0) - lastValidObjectTime) > 5) ) {
						speaker->Rate = SPEAKING_RATE;
						speaker->Speak("Re Scan");
						soundToPlay = -1;
					}else if ( (lastValidObjectPosition != CENTER) 
						&& (myObjectPosition == -1) && ((int) (std::time(0) - lastValidObjectTime) <= 5)) {
						soundToPlay = lastValidObjectPosition;
				
					}
				} else if (!useLastValidObjectPosition) {
					soundToPlay = myObjectPosition;
				}

				DEBUG2("In Sound My object position = ");
				DEBUG2(soundToPlay);
				
				// Depending on the code we get from the vision thread
				// Play a speech synthesized tone
				switch(soundToPlay){
				case UP:
					speaker->Speak("Up");
					break;
				case DOWN:
					speaker->Speak("Down");
					break;
				case LEFT:
					speaker->Speak("Left");
					break;
				case RIGHT:
					speaker->Speak("Right");
					break;
				case UPPER_LEFT:
					speaker->Speak("Up and Left");
					break;
				case UPPER_RIGHT:
					speaker->Speak("Up and Right");
					break;
				case LOWER_LEFT:
					speaker->Rate = SPEAKING_RATE_SLOW;
					speaker->Speak("Down and Left");
					break;
				case LOWER_RIGHT:
					speaker->Rate = SPEAKING_RATE_SLOW;
					speaker->Speak("Down and Right");
					break;
				case CENTER:
					speaker->Speak("Center");
					break;
				default:
					speaker->Speak(" ");
					break;
				}
				myObjectPosition = -1;
				
				//soundCodeVectorPointer.push_back(5);//(int)soundToPlay);
				delete speaker;
				
				for(int i = 0; i < 10000; i++)
					boost::this_thread::yield();
		}
		catch (System::Exception^ ex){
			cout << "Exception caught" << endl;
		}
		cout << "pushed below" << endl;
		localSoundCodeVector.push_back(soundToPlay);
		cout << "pushed" << endl;
		newI++;
		
		delete speaker;
	}

	// Store the sounds we've played in a global vector 'soundCodeVectorPointer'
	// We record each sound to wav files 
	soundCodeVectorPointer = localSoundCodeVector;
	cout << "newI in sound thread is: " << newI << endl;
	if (output_txt != NULL)
        fclose(output_txt);
}



