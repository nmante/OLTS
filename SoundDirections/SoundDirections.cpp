// SoundDirections.cpp : Defines the entry point for the console application.
//

/*
 * Written by Nii Mante
 * Use this program to train the test subject.  Specifically, this program will
 * allow the tester to demo the synthesized speech for the test subject.
 */
#include <iostream>
#include <map>
#include <string>

using namespace System;
using namespace System::Speech::Synthesis;
using namespace std;

#define MAX_LENGTH 5

enum StringValue{
	UP,
	DOWN,
	RIGHT,
	LEFT,
	UPPER_LEFT,
	UPPER_RIGHT,
	DOWN_LEFT,
	DOWN_RIGHT,
	CENTER
};

static map<string, StringValue> stringMap;
static char strInput[MAX_LENGTH];

ref class SpeechSyn{
public:
	SpeechSynthesizer speaker;
};
void doSpeech();
void initStrMap();


int main(int argc, char* argv[])
{
	initStrMap();
	doSpeech();
	return 0;
}

void doSpeech(){
	SpeechSynthesizer ^speaker;
	int myObjectPosition = -1;
	cout << "------------ Developed by Nii Mante ---------------" << endl;
	cout << "Welcome to the Speech Synthesis Prompt Software" << endl;
	cout << "This software allows the tester to play different voice commands" << endl;
	cout << "which the subject will hear.  The commands will be explained below." << endl;
	cout << endl;
	cout << "=============================" << endl;
	cout << "You can quit at anytime by pressing \"q\" or \"Q\"" << endl;
	cout << "=============================" << endl;
	cout << endl;
	cout << "Type this" << "  |  " << "Hear This" << endl;
	cout << "    u    " << "  |  " << "UP" << endl;
	cout << "    d    " << "  |  " << "DOWN" << endl;
	cout << "    l    " << "  |  " << "LEFT" << endl;
	cout << "    r    " << "  |  " << "RIGHT" << endl;
	cout << "    ul   " << "  |  " << "UP and LEFT" << endl;
	cout << "    dl   " << "  |  " << "DOWN and LEFT" << endl;
	cout << "    ur   " << "  |  " << "UP and RIGHT" << endl;
	cout << "    dr   " << "  |  " << "DOWN and RIGHT" << endl;
	cout << "    c    " << "  |  " << "CENTER" << endl;
	

	while(true){
		cout << "Type one of these: " << endl;
		cout << "u, d, l, r, ul, dl, ur, dr, c" << endl;
		cin.getline(strInput,MAX_LENGTH);
		if(strInput == NULL){
			cout << "Please enter a valid string" << endl;
			continue;
		}else if(_strnicmp(strInput,"Q", 1) == 0){
			cout << "Exiting the synthesis program" << endl;
			exit(0);
		}
		
		try {
				
				
				speaker = gcnew SpeechSynthesizer();
				speaker->Volume = 100;
				speaker->SelectVoiceByHints(VoiceGender::Female);
				speaker->Rate = 0;
				switch(stringMap[strInput]){
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
				case DOWN_LEFT:
					speaker->Speak("Down and Left");
					break;
				case DOWN_RIGHT:
					speaker->Speak("Down and Right");
					break;
				case CENTER:
					speaker->Speak("Center");
					break;
				default:
					cout << "Please enter one of the default strings" << endl;
					break;
				}
				delete speaker;
				
		}
		catch (System::Exception^ ex){
			cout << "Exception caught" << endl;
		}
	}
}

void initStrMap(){
	stringMap["u"] = UP;
	stringMap["d"] = DOWN;
	stringMap["r"] = RIGHT;
	stringMap["l"] = LEFT;
	stringMap["dl"] = DOWN_LEFT;
	stringMap["dr"] = DOWN_RIGHT;
	stringMap["ur"] = UPPER_RIGHT;
	stringMap["ul"] = UPPER_LEFT;
	stringMap["c"] = CENTER;
	cout << "s_mapStringValues contains " 
		<< stringMap.size() 
       << " entries." << endl;
}

