#include "ofApp.h"

# pragma mark ---------- Setup and loop ----------

void ofApp::setup()
{
    ofSetBackgroundColor(48, 48, 48);
    deviceSelected = -1;
    networkDevices       = getDevicesIPs();
    networkDevicesAndIPs = buildDevicesIPsString();
    timeLastConnection = ofGetElapsedTimef();
    volume = 0.5;
    
    /// READ CONF
    readConfig();
    readCommands();
    initialTimeStamp = 0;
    timer = 0;

    // TCP
    ///////
    setupTCPConnection(confTCPPort);
    isClientAchieved.resize(10);
    isClientAchieved.assign(sizeof(bool), false);
    
    osc.setup(12345);
}

//-------------------------------------------------------------------------------
void ofApp::update()
{
    int equalHack = 0;
    while(osc.hasWaitingMessages()){
        ofxOscMessage m;
        osc.getNextMessage(m);
        cout<<m.getAddress() << " -- " << m.getArgAsString(0) << endl;
        lastChar = m.getArgAsString(0);
        if(equalHack == 0){
            if(lastChar == "'\\n'")
                keyPressed(OF_KEY_RETURN);
            else if(lastChar == "'\\x1b'"){
                equalHack = 4;
                keyPressed('=');
            }
            else
                keyPressed((int)lastChar[lastChar.size()-2]);
        }else
            equalHack--;
    }
    
    // TCP
    tcpLock.lock();
    handleTcpIn();
    handleTcpOut();
    tcpLock.unlock();
    
    // if we got too much time without connection...
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
            cout<<"Time: " << timer <<  " -- Send command: "<<selectedCommandSequence[0].second<<endl;
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
    ofDrawBitmapString(lastChar, 20, 50);
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
    for(int i = 0; configXML.pushTag("order", i); i++){
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
    
    // Order tcpCommands from .first
    
    //FOUND: http://stackoverflow.com/questions/279854/how-do-i-sort-a-vector-of-pairs-based-on-the-second-element-of-the-pair
    for(auto &tcpCommand : tcpCommands){
        std::sort(tcpCommand.second.begin(), tcpCommand.second.end(), [](pair<float, string> &left, pair<float, string> &right) {
            return left.first < right.first;
        });
    }
        
    cout<< "---- Loaded Info ----" << endl;
    for(auto tcpCommand : tcpCommands){
        cout << "KeyCombination: " << tcpCommand.first << endl;
        for (auto message : tcpCommand.second)
            cout << "--> " << message.first << " -- " << message.second << endl;
    }
}

#pragma mark ---------- System events ----------
//-------------------------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    if(key == '+'){
        volume += 0.005;
        volume = ofClamp(volume, 0, 1);
        processTcpCommand("2 volume "+ofToString(volume));
    }
    else if(key == '-'){
        volume -= 0.005;
        volume = ofClamp(volume, 0, 1);
        processTcpCommand("2 volume "+ofToString(volume));
    }
    else if(key == OF_KEY_RETURN){ //IN RPI KEY RETURN IS 10 and has to be 13, with osc your receive a n
        cout<<"Send commands from keycombination: "<<keybuffer<<endl;
        for(auto preset : tcpCommands){
            if(preset.first == keybuffer){
                cout<<"-----Start Counter------"<<endl;
                initialTimeStamp = ofGetElapsedTimef();
                //Clear command sequence deque and fill with new info.
                selectedCommandSequence.clear();
                for(auto command : preset.second)
                    selectedCommandSequence.push_back(command);
            }
        }
        keybuffer.clear();
    }
    else if(key == 'r'){
        tcpCommands.clear();
        readCommands();
        initialTimeStamp = -1000;
        cout << "Reload Commands" << endl;
    }
    else
        keybuffer+=key;
    
    //cout<<char(key)  << " " << key  << " --- Enter keycode: " << OF_KEY_RETURN << " " << volume<<endl;
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
                int theId = ofToInt(tokens[1]);
                
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
                if(!isClientAchieved[i]){
                    connectedClients.push_back(ofToString(i) + " " + ofToString(s.id) + " " + s.name + " " + s.ip);
                    isClientAchieved[i] = true;
                    cout << "New Client to list, list size: " << connectedClients.size() << endl;
                }
            }
            if(tokens[0]=="awake")
            {
                cout << "Received awake. So sending ping to all!!" << endl;
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
            cout << "TCP Send: " << mess << endl;
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
    for(int i=0; i<num; i++)
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
//            cout<< "id: " <<   id << " -- request id: " << request_id << endl;
        }
//        cout<< "id: " <<   id << " -- request id: " << request_id << endl;
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
        cout << "oo Couldn't Disconect All clients !! ERROR" << endl;
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