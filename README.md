#Object Localization and Tracking System

Nii Mante
University of Southern California

##Preface

Code on Github merely for demonstration purposes.  Proprietary external libraries have been left out of this code repo. As such, you won't be able to run the program, but by all means feel free to view the code, and adapt it to your needs.

##Purpose

The purpose of this software is to detect and localize objects via computer
computer vision algorithms, and generate sounds based on the position(s) of 
the aforementioned objects.

It's main goal is to assist blind subjects in reaching and grasping for 
objects of importance.

I created this software to test on Blind subjects for my PhD research.

##Description

####tl;dr

C++ Windows only software. The code depends on a library [Context Tracker](http://iris.usc.edu/outlines/papers/2011/dinh-vo-medioni-cvpr11.pdf) which was ONLY written for Windows, hence the Windows only restriction (Here's a [youtube video](http://www.youtube.com/watch?v=ifke42DryNM)) 

- This opencv program takes in video input from a webcam
- The video stream is interpreted by the Context Tracker
- The Context Tracker gives us the position of a previously selected object
- Feedback algorithms generate Synthesized Voice commands based on the position of the object

####Detailed Description
This code combines Computer Vision algorithms with Speech Synthesis to aid the Visually Impaired with Object Localization.

It's a multithreaded program which consists of three worker threads and the main thread.  The three worker threads are:
- Vision Thread (VisionThread.h/.cpp) - Does object detection, passes object position around to other threads
- Sound Thread (SoundThread.h/.cpp) - Takes object position, synthesizes speech OR sends HTTP requests to a [node-server]()
- Video Recording Thread (GlobalHeaders.h/.cpp) - records video of the vision thread's stream

Computer Vision related tasks are generated via OpenCV, and the 
[Context Tracker](http://iris.usc.edu/outlines/papers/2011/dinh-vo-medioni-cvpr11.pdf). Threading related tasks are accomplished via [Boost](http://www.boost.org/doc/libs/1_55_0/doc/html/thread.html).  Speech Synthesis is accomplished via Microsoft's Speech Synthesis class.

#####Program Flow

The three threads share information such as:
- Object position
- Video capture object


In main()

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