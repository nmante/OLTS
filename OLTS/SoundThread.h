#ifndef _SOUNDTHREAD_H_
#define _SOUNDTHREAD_H_ 

/*
	Responsible for calling the Speech Synthesizer into action. We bind this function
	to the 'Sound' thread.
*/
void doSpeech(int argc, char *argv[]);


#endif // _SOUNDTHREAD_H_