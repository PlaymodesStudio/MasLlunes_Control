#include "ofMain.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"

//========================================================================
int main( )
{
#ifdef TARGET_RASPBERRY_PI
    ofSetupOpenGL(1920, 1080, OF_FULLSCREEN);
#else
    ofSetupOpenGL(1400, 400, OF_WINDOW);		// <-------- setup the GL context
#endif
    ofRunApp(new ofApp());
}