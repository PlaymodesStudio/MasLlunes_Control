#include "ofMain.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"

//========================================================================
int main( )
{
#ifdef TARGET_RASPBERRY_PI
    ofSetupOpenGL(100, 100, OF_WINDOW);
#else
    ofSetupOpenGL(1400, 400, OF_WINDOW);		// <-------- setup the GL context
#endif
    ofRunApp(new ofApp());
}