#pragma once

#include "ofMain.h"
#include "ofxNetwork.h"
#include "LocalAddressGrabber.h"
#include "ofxXmlSettings.h"

typedef struct
{
    int                 id;
    string              name;
    string              ip;
} slaveInfo;


class ofApp : public ofBaseApp
{
public:
    void setup();
    void draw();
    void update();
    void exit();
    
    void keyPressed(int key);
    void mouseMoved(int x, int y);


    int                     getIdFromSlave(int i);
    
    // TCP
    void                setupTCPConnection(int port);
    void                resetTCPConnection(int port);
    int                 tcpPort;
    bool                isTcpConnected;
    ofxTCPServer        tcpServer;
    ofMutex             tcpLock;
    void                handleTcpOut();
    void                sendTcpMessageToAll(string mess);
    void                sendTcpMessageToSlave(string mess, int id);
    void                handleTcpIn();
    float               timeLastConnection;
    void                sendTCPPingAll();
    
    void                sendMessageToSlavesFolder(string messageWithoutId);

    // IP
    vector<string>      getDevicesIPs();
    vector<string>      buildDevicesIPsString();
    vector<string>      networkDevices;
    vector<string>      networkIPs;
    vector<string>      networkDevicesAndIPs;
    string              getIP(string device);
    int                 deviceSelected;
    
    /// CONFIG
    void                readConfig();
    string              confNetworkDevice;
    int                 confTCPPort;
    
    
    void                readCommands();
    string              keybuffer;
    
    
    // Send info
    vector<pair<string, vector<string>>> tcpCommands;
    

};
