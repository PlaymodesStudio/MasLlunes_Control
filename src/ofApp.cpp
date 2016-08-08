#include "ofApp.h"

# pragma mark ---------- Setup and loop ----------

void ofApp::setup()
{
    ofSetBackgroundColor(48, 48, 48);
    deviceSelected = -1;
    networkDevices       = getDevicesIPs();
    networkDevicesAndIPs = buildDevicesIPsString();
    timeLastConnection = ofGetElapsedTimef();
    volume = 0;
    
    /// READ CONF
    readConfig();
    
    readCommands();
    initialTimeStamp = 0;
    timer = 0;

    // TCP
    ///////
    setupTCPConnection(confTCPPort);
}

//-------------------------------------------------------------------------------
void ofApp::update()
{
    // TCP
    tcpLock.lock();
    handleTcpIn();
    handleTcpOut();
    tcpLock.unlock();
    
    // if we got too much time without connection ...
    if(!tcpServer.isConnected() && (ofGetElapsedTimef()-timeLastConnection>35.0))
    {
        // reset TCP connection
        resetTCPConnection(confTCPPort);
        
        timeLastConnection=ofGetElapsedTimef();
        
        // send ping to all
        sendTCPPingAll();
    }
    
    //Timer to execute commands
    if(selectedCommandSequence.size() != 0){
        timer = ofGetElapsedTimef() - initialTimeStamp;
        while(selectedCommandSequence[0].first < timer){
            cout<<"Send command: "<<selectedCommandSequence[0].second<<endl;
            processTcpCommand(selectedCommandSequence[0].second);
            selectedCommandSequence.pop_front();
            if(selectedCommandSequence.size() == 0)
                break;
        }
    }
}

//-------------------------------------------------------------------------------
void ofApp::draw()
{

}

//--------------------------------------------------------------
void ofApp::exit()
{
    cout << "Trying to close TCP Server on exit() !!" << endl;
    
    tcpLock.lock();
    
    if(tcpServer.disconnectAllClients())
    {
        if(tcpServer.close())
        {
            cout << "TCP Closed and All Clients Disconnected !! " << endl;
        }
        else
        {
            cout << "Couldn't Close TCP Connection" << endl;
        }
    }
    else
    {
        cout << "Couldn't Disconect All clients !! ERROR " << endl;
    }
    
    tcpLock.unlock();
    
}

# pragma mark ---------- Auxiliari methods ----------
//-------------------------------------------------------------------------------
string ofApp::getIP(string device)
{
    vector<string> list = LocalAddressGrabber :: availableList();
    return LocalAddressGrabber :: getIpAddress(device);
}


//-------------------------------------------------------------------------------
vector<string> ofApp::getDevicesIPs()
{
    vector<string> v = LocalAddressGrabber :: availableList();
    cout << "....................................." << endl;
    cout << " ...... NETWORK DEVICES AND IPs" << endl;
    for(int i=0;i<v.size();i++)
    {
        networkIPs.push_back(LocalAddressGrabber :: getIpAddress(v[i]));
        cout << " ... " << v[i] << " : " << networkIPs[i] << endl;
    }
    cout << "....................................." << endl;
    return v;
}

//-------------------------------------------------------------------------------
vector<string> ofApp::buildDevicesIPsString()
{
    vector<string> ndi;
    for(int i=0;i<networkDevices.size();i++)
    {
        ndi.push_back(networkDevices[i] + " : " + networkIPs[i] );
    }
    return ndi;
}

//--------------------------------------------------------------
void ofApp::readConfig()
{
    ofxXmlSettings configXML;
    configXML.load("./app/config.xml");
    configXML.pushTag("config");
    
    /// NETWORK DEVICE
    confNetworkDevice = configXML.getValue("networkDevice","error");
    
    /// TCP SETUP
    confTCPPort = configXML.getValue("TCPPort",11999);
    
    //Vol
    volume = configXML.getValue("volume", 0.5);
    
    /// add to LOG
    //ofLog(OF_LOG_NOTICE) << "ofApp :: readConfig :: networkDevice " << confNetworkDevice << " :: TCP Port " << confTCPPort << endl;
    cout << "ofApp :: readConfig :: networkDevice " << confNetworkDevice << " :: TCP Port " << confTCPPort << endl;
    
}

//--------------------------------------------------------------
void ofApp::readCommands()
{
    ofxXmlSettings configXML;
    configXML.load("./app/tcpCommands.xml");
    configXML.pushTag("tcpCommands");
    for(int i = 0; configXML.pushTag("order", i) ; i++){
        pair<string, vector<pair<float, string>>> tempOrder;
        tempOrder.first = configXML.getValue("keycode", "error");
        for(int j = 0; configXML.pushTag("command", j); j++){
            pair<float, string> tempCommand;
            tempCommand.first = configXML.getValue("t", 0.0);
            tempCommand.second = configXML.getValue("s", " ");
            tempOrder.second.push_back(tempCommand);
            configXML.popTag();
        }
        tcpCommands.push_back(tempOrder);
        configXML.popTag();
    }
    //configXML.popTag();

    
    /// TCP SETUP
    confTCPPort = configXML.getValue("TCPPort",11999);
    
    /// add to LOG
    //ofLog(OF_LOG_NOTICE) << "ofApp :: readConfig :: networkDevice " << confNetworkDevice << " :: TCP Port " << confTCPPort << endl;
    cout << "ofApp :: readConfig :: networkDevice " << confNetworkDevice << " :: TCP Port " << confTCPPort << endl;
    
}

//-------------------------------------------------------------------------------
int ofApp::getIdFromSlave(int i)
{
    //ofxDatGuiToggle* t = slavesListFolder->getToggleAt(i);
    //string whichIdString = ofSplitString(t->getLabel()," ")[0];
    
    //return (ofToInt(whichIdString));
}

#pragma mark ---------- System events ----------
//-------------------------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs &args)
{
    int key = args.key;
    if(key == '+'){
        volume += 0.05;
        volume = ofClamp(volume, 0, 1);
        processTcpCommand("2 volume "+ofToString(volume));
    }
    else if(key == '-'){
        volume -= 0.05;
        volume = ofClamp(volume, 0, 1);
        processTcpCommand("2 volume "+ofToString(volume));
    }
    else if(key == 10 || key == OF_KEY_RETURN){ //IN RPI KEY RETURN IS 10 and has to be 13
        cout<<"Send commands from keycombination: "<<keybuffer<<endl;
        for(auto preset : tcpCommands){
            if(preset.first == keybuffer){
                cout<<"Start Counter"<<endl;
                initialTimeStamp = ofGetElapsedTimef();
                //Clear command sequence deque and fill with new info.
                selectedCommandSequence.clear();
                for(auto command : preset.second)
                    selectedCommandSequence.push_back(command);
            }
        }
        keybuffer.clear();
    }
    else
        keybuffer+=key;
    
    cout<<char(key)  << " " << key << "--" << args.keycode << " --- Enter keycode: " << OF_KEY_RETURN << " " << volume<<endl;
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{

}


//-------------------------------------------------------------------------------
// TCP
//-------------------------------------------------------------------------------
///--------------------------------------------------------------

# pragma mark ---------- TCP ----------

void ofApp::handleTcpOut()
{
    
    if(!tcpServer.isConnected())
    {
        return;
    }
    
    
    int numMessages = 0;
    
    /*
     
     //any bangs that came our way this frame send them out too
     for(int i = 0; i < bangsReceived.size(); i++)
     {
     cout << "TCP FOUND BANGS! oscAddress : " << bangsReceived[i].getAddress() << " :: " << bangsReceived[i].getArgAsString(0) << endl;
     string addressWithoutSlash = bangsReceived[i].getAddress().substr(1,bangsReceived[i].getAddress().size()-1);
     // SPLIT STRING ?
     string buf; // Have a buffer string
     stringstream ss(addressWithoutSlash); // Insert the string into a stream
     vector<string> tokens; // Create vector to hold our words
     while (ss >> buf)
     {
     tokens.push_back(buf);
     }
     
     // send a TCP MESSAGE FOR EVERY ADDRESS ITEM
     for(int j=0;j<tokens.size();j++)
     {
     string messageTcp = tokens[j] + " " + bangsReceived[i].getArgAsString(0);
     sendTcpMessageToAll(messageTcp);
     }
     
     }
     */
}

//-------------------------------------------------------------------------------
void ofApp::handleTcpIn()
{
    if((!tcpServer.isConnected()))
    {
        return;
    }
    
    for(int i = 0; i < tcpServer.getLastID(); i++)
    {
        if( !tcpServer.isClientConnected(i) ) continue;
        
        string str = tcpServer.receive(i);
        
        if(str.length() > 0)
        {
            
            vector<string> tokens = ofSplitString(str, " ");
            
            cout << "Received TCP message : " << str << endl;
            
            if(tokens[0]=="pong")
            {
                //slavesListFolder->collapse();
                
                int theId = ofToInt(tokens[1]);
                cout << "Hi !! I got a PONG TCP message !! >> " << str <<" <<  from client : " << i << " with ID : " << theId << endl;
                
                // to slave info
                slaveInfo s;
                s.id = theId;
                if(tokens.size()>=2)
                {
                    s.name = tokens[2];
                    s.ip = tcpServer.getClientIP(i);
                }
                else
                {
                    s.name = "defaultName";
                    s.ip = tcpServer.getClientIP(i);
                }
                connectedClients.push_back(ofToString(i) + " " + ofToString(s.id) + " " +s.name + " " + s.ip);
            }
            if(tokens[0]=="awake")
            {
                cout << "Received : awake . So sending ping to all !! " << endl;
                sendTCPPingAll();
            }
        }
        
    }
}

//-------------------------------------------------------------------------------
void ofApp::sendTcpMessageToAll(string mess)
{
    if(tcpServer.isConnected())
    {
        
        /// SEND TO ALL TCP CLIENTS !!
        //for each client lets send them a message
        for(int j = 0; j < tcpServer.getLastID(); j++)
        {
            if( !tcpServer.isClientConnected(j) )
            {
                continue;
            }
            tcpServer.send(j,mess);
            cout << "TCP Send : " << mess << endl;
        }
    }
}

//--------------------------------------------------------------
void ofApp::sendTcpMessageToSlave(string mess, int pos)
{
    if(tcpServer.isConnected())
    {
        if( tcpServer.isClientConnected(pos))
        {
            tcpServer.send(pos,mess);
            cout << "TCP Send : " << mess << endl;
        }
    }
}

//--------------------------------------------------------------
void ofApp::processTcpCommand(string command){
    sendMessageToSlavesFolder(command);
}

//--------------------------------------------------------------
void ofApp::sendMessageToSlavesFolder(string m)
{
    int num = connectedClients.size();
    istringstream iss(m);
    vector<string> tokens;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter(tokens));
    
    string request_id = tokens[0];
    for(int i=0;i<num;i++)
    {
        istringstream iss2(connectedClients[i]);
        vector<string> tokens2;
        copy(istream_iterator<string>(iss2),
             istream_iterator<string>(),
             back_inserter(tokens2));
        string id = tokens2[1];
        if(id == request_id){
            string messageTcp = m;
            sendTcpMessageToSlave(messageTcp, ofToInt(tokens2[0]));
        }
    }
}


//--------------------------------------------------------------
void ofApp::setupTCPConnection(int _port)
{
    tcpLock.lock();
    
    if(true)
    {
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        isTcpConnected = tcpServer.setup(_port);
        
        if (isTcpConnected)
        {
            cout << "TCP Server Setup. OK!! Port : " << _port << endl;
            isTcpConnected = true;
            
        }
        else
        {
            cout << "TCP Server reSetup FAIL!! . Port : " << _port<< endl;
            isTcpConnected = false;
            
        }
        cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
    }
    tcpLock.unlock();
}

//--------------------------------------------------------------
void ofApp::resetTCPConnection(int _port)
{
    tcpLock.lock();
    
    string messageTcp = "all close";
    sendTcpMessageToAll(messageTcp);
    sleep(1);
    if(tcpServer.disconnectAllClients())
    {
        cout << "oooooooooooooooooooooooooooooooooooooooooooooooo" << endl;
        cout << "oo TCP Server has disconnected from All Clients ..." << endl;
        if(tcpServer.close())
        {
            tcpLock.unlock();
            cout << "oo TCP Server has been closed for reset ..." << endl;
            setupTCPConnection(_port);
            tcpLock.lock();
            
        }
        else
        {
            cout << "oo Couldn't Close TCP Connection" << endl;
        }
    }
    else
    {
        cout << "oo Couldn't Disconect All clients !! ERROR " << endl;
    }
    tcpLock.unlock();
}

//--------------------------------------------------------------
void ofApp::sendTCPPingAll()
{
    if(tcpServer.isConnected())
    {
        string messageTcp = "all ping";
        sendTcpMessageToAll(messageTcp);
        cout << "Sending PING to ALL clients" << endl;
    }
}