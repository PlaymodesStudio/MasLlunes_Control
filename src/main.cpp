#include "ofMain.h"
#include "ofApp.h"
#include "ofAppNoWindow.h"

//========================================================================
int main( )
{
    ofAppNoWindow headless;
    ofSetupOpenGL(&headless, 1400, 400, OF_WINDOW);
	ofRunApp(new ofApp());
}