
extern "C" _declspec(dllimport) void init(IplImage * img, CvRect roi, bool context);

extern "C" _declspec(dllimport) void track(IplImage * img);

extern "C" _declspec(dllimport) void getRoi(CvRect * roi, 
                                            float * confidence, 
                                            bool * valid);
