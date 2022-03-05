/************************************************************
ARDUINO MEGA COMMUNICATION WITH MODBUS DEVICE - VARIABLE FREQUENCY DRIVE (VFD)

Connect the Arduino to any Modbus device using MAX485 module.

This code was tested in setup of Arduino MEGA and MAX485 module.
If you use other boards or RS485 modules you might need to change some things in the code.

In this code we just implemented basic connection routine with a VFD. We also have three
buttons connected to the Arduino board to start, stop or change the frequency over Modubus.

Modified 5 March 2022
By spehj

************************************************************/

#include <Arduino.h>
#include <ModbusMaster.h>
#include <OneWire.h>

#define Slave_ID  1         // Define Modbus device ID (read from the datasheet of your device - most common it's 1)

#define MAX485_DE_RE      8 // Define pins for communication with MAX485 module (enable pin)
#define RXD0 19             // RX0 pin
#define TXD0 18             // TX0 pin

#define RUN 9               // Button pin to run the VFD
#define STOP 10             // Button pin to stop the VFD
#define SET 11              // Button pin to change frequency on the VFD over Modbus

// Instance of ModbusMaster object
ModbusMaster node;

// Function prototype
bool getResultMsg(ModbusMaster *node, uint8_t result);

void preTransmission()
{
  digitalWrite(MAX485_DE_RE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_DE_RE, 0);
}


bool setBool = true;  // SET button is enabled. Once pressed, you need to release it to fire the function again
bool runPrev = true;  // RUN button is enabled
bool stopPrev = true; // STOP button is enabled
int frequency = 0;    // Frequency is not in Hz but Hz*100

// Function for checking if buttons are pressed
void checkButtons(){

  int runCurrent = digitalRead(RUN);
  int stopCurrent = digitalRead(STOP);
  int setCurr = digitalRead(SET);

  if (runCurrent == LOW && runPrev == true)
  {
    // If button RUN is pressed (LOW state) write run command to VFD register
    node.writeSingleRegister(0x2000, 0x0001);
    runPrev = false;
    stopPrev = true;
    Serial.println("FWD pressed!");
  }else if ( stopCurrent == LOW && stopPrev == true)
  {
    // If button STOP is pressed (LOW state) write stop command to VFD register
    node.writeSingleRegister(0x2000, 0x0006);
    runPrev = true;
    stopPrev = false;
    Serial.println("STOP pressed!");
  }

  if (setCurr == LOW && setBool == true)
  { 
    // If SET button is pressed frequency goes up for a certain interval
    if (frequency < 10000)
    {
      frequency +=1000;
    }
    else if (frequency >= 10000)
    { // If frequency is too high, start from lowest frequency
      frequency = 1000;
    }
    
    // Write a frequency to the register of the VFD
    Serial.println("SET pressed!");
    node.writeSingleRegister(0x1000, frequency);

    // Make sure function only fires once
    setBool = false;
  }
  else if (setCurr == HIGH)
  { 
    // Setting frequency is enabled once the button is released
    setBool = true;
  }
}

void setup() {

  // Set inputs and outputs
  pinMode(MAX485_DE_RE, OUTPUT);
  pinMode(RUN, INPUT_PULLUP);
  pinMode(STOP, INPUT_PULLUP);
  pinMode(SET, INPUT_PULLUP);
  
  // Use Serial (port 0) for USB communication and debug
  Serial.begin(9600);
  Serial.println("Start setup...");


  // Start new Serial 1 (port 1) for the Modbus communication
  Serial1.begin(9600);

  // Communicate with Modbus slave ID 2 over Serial (port 0)
  node.begin(Slave_ID, Serial1);
  node.idle(yield);

  Serial.println("Done setup");

  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);

  // Set frequency
  node.writeSingleRegister(0x1000, 10000);
}

unsigned long lastMillis = 0;

void loop() {

  // Check if any button is pressed
  checkButtons();

  // Current millis
  long currentMillis = millis();

  // Polling Modbus device every 5 seconds
  if (currentMillis - lastMillis > 5000) 
  { 
    // Get set frequency from VFD
    uint8_t result = node.readHoldingRegisters(0x1001, 1);

    if (getResultMsg(&node, result)) 
    {
      Serial.println();
      double res_dbl = node.getResponseBuffer(0) / 100;
      String res = "Operating frequency: " + String(res_dbl) + " Hz\r\n";

      // Print result from the VFD
      Serial.println(res);

    }
    
    lastMillis = currentMillis;

  }
}


// Get Modbus message
bool getResultMsg(ModbusMaster *node, uint8_t result) 
{ 

  // Every time we need information from the Modbus device we can check which answer we get
  // If communication is not succesful we get the answer where the problem could be
  String tmpstr2 = "\r\n";
  switch (result) 
  {
  case node->ku8MBSuccess:
    return true;
    break;
  case node->ku8MBIllegalFunction:
    tmpstr2 += "Illegal Function";
    break;
  case node->ku8MBIllegalDataAddress:
    tmpstr2 += "Illegal Data Address";
    break;
  case node->ku8MBIllegalDataValue:
    tmpstr2 += "Illegal Data Value";
    break;
  case node->ku8MBSlaveDeviceFailure:
    tmpstr2 += "Slave Device Failure";
    break;
  case node->ku8MBInvalidSlaveID:
    tmpstr2 += "Invalid Slave ID";
    break;
  case node->ku8MBInvalidFunction:
    tmpstr2 += "Invalid Function";
    break;
  case node->ku8MBResponseTimedOut:
    tmpstr2 += "Response Timed Out";
    break;
  case node->ku8MBInvalidCRC:
    tmpstr2 += "Invalid CRC";
    break;
  default:
    tmpstr2 += "Unknown error: " + String(result);
    break;
  }

  // Print Modbus returned message
  Serial.println(tmpstr2);
  return false;

}
