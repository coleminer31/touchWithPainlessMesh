#include <painlessMesh.h>
#include <M5Core2.h>
#include <Fonts/EVA_20px.h>
#include <stdio.h>

//define the painless mesh vars
#define   MESH_SSID       "whateverYouLike"
#define   MESH_PASSWORD   "somethingSneaky"
#define   MESH_PORT       5555

// Initialize the painlessMesh prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);

// Initialize the scheduler and painlessMesh objects
Scheduler     userScheduler; // to control your personal task
painlessMesh  mesh;

// Set the calc delay flag to false
bool calc_delay = false;

// Initialize the nodes list
SimpleList<uint32_t> nodes;

// Initialize the send message task
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, &sendMessage ); // start with a one second interval

// Set the messag recieved vars to 
// about the ~ color white, or 61375
String msgRecieved = "61375";
String lastMsgRecieved = "61375";

// Initialize the touch point vars
int recievedX = 0;
int recievedY = 0;
int touchX = 0;
int touchY = 0;

// Initialize the screen vars
int screenWidth = 320;
int screenHeight = 240;

// Define the showNum func
void showNum(short int X, short int Y)
{

  // Initialize the string char array
  char Str[30];

  // Clear the M5stack screen and 
  // set it to the color recived from
  // the other node
  M5.Lcd.clear(msgRecieved.toInt());

  // Print the test header
  M5.Lcd.setCursor(10, 10);
  M5.Lcd.printf("painlessMesh test!");

  // Print the x touch point
  M5.Lcd.setCursor(10, 26);
  sprintf(Str,"X:%d", X);
  M5.Lcd.printf(Str);

  // Print the y touch point
  M5.Lcd.setCursor(10, 42);
  sprintf(Str,"Y:%d", Y);
  M5.Lcd.printf(Str);

  // Print the number of nodes in the mesh
  M5.Lcd.setCursor(10, 62);
  sprintf(Str,"number of nodes: %d", (mesh.getNodeList().size() + 1));
  M5.Lcd.printf(Str);

  // Print the message recieved
  M5.Lcd.setCursor(10, 82);
  sprintf(Str,"message recieved: %s", msgRecieved.c_str());
  M5.Lcd.printf(Str);
  
}

// Define the touch setup func
void touchsetup()
{

    // Call the show num func
    showNum(0,0);
    
}

// Define the touch flush func
void touchflush()
{

    // Initialize the touch point char arrays
    char X[4];
    char Y[4];

    // Set the cursor
    M5.Lcd.setCursor(10, 10);

    // Get the touch point
    TouchPoint_t pos = M5.Touch.getPressPoint();

    // Get the touch state
    bool touchStateNow = ( pos.x == -1 ) ? false : true;

    // If the touch state is true
    // call the showNum func for the
    // current touch points, and store
    // the curren touch points in 
    // the global touch point vars
    if( touchStateNow )
    {
      
        showNum(pos.x,pos.y);
        touchX = pos.x;
        touchY = pos.y;
        
    }
}

// Define the setup func
void setup() {

  // Setup the M5stack
  M5.begin(true, true, true, true);
  Serial.begin(115200);
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.setTextSize(2);
  touchsetup();

  // Set up the mesh network 
  mesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages
  mesh.init(MESH_SSID, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  mesh.onNodeDelayReceived(&delayReceivedCallback);
  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
  randomSeed(analogRead(0.777));
  
}

// Define the loop func
void loop() {

  // Update the mesh
  mesh.update();

  // Get the touch point
  TouchPoint_t pos = M5.Touch.getPressPoint();

  // Set the text color according to
  // a press of the capacative buttons,
  // which fall within these limits
  if(pos.y > 240)
    if(pos.x < 109)
      M5.Lcd.setTextColor(RED);
    else if(pos.x > 218)
      M5.Lcd.setTextColor(BLUE);
    else if(pos.x >= 109 && pos.x <= 218)
      M5.Lcd.setTextColor(GREEN);

  // Call the touch flush func
  touchflush();

  // Delay for small moment
  // (need to keep as small as possible
  // for a smooth operation of
  // the painlessMesh network
  delay(1);
  
}

// Define the send message func
void sendMessage() {

  // Get 8-bit RGB colors based on touch point input
  // normalized over the x and y size of the screen
  int colorRed = 255 * (320 - touchX) / (320);
  int colorGreen = 255 * (240 - touchY) / (240);
  int colorBlue = 255 * ((320 * 240) - (touchX * touchY)) / (320 * 240);

  // Pass the 8-bit RGB colors to the  color565 function,
  // which returns the hex num used to drive the LCD screen colors
  uint16_t sendColor = M5.Lcd.color565(colorRed, colorGreen, colorBlue);

  // Convert the hex color number to
  // a string, so that it can be sent
  // across the painlessMesh network
  String msg = String(sendColor);

  // Broadcast the color string
  mesh.sendBroadcast(msg);

  // Calc delay if needed
  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }

  // Set the task interval
  taskSendMessage.setInterval( random(TASK_SECOND * 1, TASK_SECOND * 5));  // between 1 and 5 seconds
  
}

// Define the recieved callback functoin
void receivedCallback(uint32_t from, String & msg) {

  // Set the global last message recived var 
  // to the global msgRecieved var value
  lastMsgRecieved = msgRecieved;

  // Set the global message recieved var
  // to the local message recieved var value
  msgRecieved = msg;

  // If the global message recieved var value
  // does not equal the global last message
  // recieved var value, call the show num
  // func with the global touch point vars
  if (msgRecieved != lastMsgRecieved) {
    
    showNum(0, 0);
    
  }
  
}

// Define the new connection call back func
void newConnectionCallback(uint32_t nodeId) {

  // Do nothing here for now. 
  
}

// Define the changed connection call back func
void changedConnectionCallback() {

  // Update the nodes list
  nodes = mesh.getNodeList();

  // Set the calc delay flag to true
  calc_delay = true;
    
}

// Define the node time adjusted call back func
void nodeTimeAdjustedCallback(int32_t offset) {

  // Do nothing here for now.

}

// Define the delay recieved call back func
void delayReceivedCallback(uint32_t from, int32_t delay) {

  // Do nothing here for now.

}
