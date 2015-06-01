//  More complex example of the use of Simple Scheduler with a serial interface for testing
//  Requires Streaming.h which can be acquired from http://arduiniana.org/libraries/streaming/

//  Attach a LED to a PWM pin, by default pin 3 here.  The blink example uses pin 13,
//  so attach a second LED if you change that parameter or your board doesn't have the
//  built in LED attached to digital pin 13.

//  Once the sketch is loaded, connect to your board with the Serial Monitor or other
//  communication software.  You should be greeted with a compile time tagline and an OK
//  prompt.  If you're using the Serial Monitor in the Arduino IDE, set a line ending (it
//  doesn't matter what, as long as it's not "No line ending".

//  Send 'B #' to blink LED 13 the number of times you specify.  While it is blinking,
//  send 'T' and you'll see a list of between 2 and 3 tasks... 1 for the pulsing
//  heartbeat task, 1 for the blinking task, and possibly you'll catch 1 for the task
//  to turn off the blinking LED.

//  Send 'L #' to stop the pulsing LED and set it to a specific brightness.  Send 'L'
//  by itself to restart the pulsing.

//  '?' will give you a list of these commands as well.

#include <Arduino.h>
#include <SimpleScheduler.h>
#include <Streaming.h>

#pragma mark Configuration Definitions
//************************************************************************

#define HB_LED      3 // Must be on a pin that supports PWM
#define BLINK_LED   13
#define SERIAL_BAUD 115200

// Heartbeat pulse parameters

#define _hb_MAX     30
#define _hb_MIN      0
#define _hb_DELTA    1
#define _hb_WIDTH 4000

#pragma mark Global Variables
//************************************************************************

SimpleScheduler tasks = SimpleScheduler() ;
SimpleTask      HB_Task ;
char            inputString[256] ;            // a string to hold incoming data
bool            stringComplete = false ;      // whether the string is complete
uint8_t         stringLength = 0 ;            // string length

#pragma mark Arduino Main
//************************************************************************

// Forward declaration for heartbeat so we can reference it in setup()
#define _hb_DELAY   (_hb_WIDTH / ((_hb_MAX - _hb_MIN) * 2 / _hb_DELTA))
void HeartBeat(void) ;

void setup() {
  pinMode(HB_LED, OUTPUT);  analogWrite(HB_LED, 0) ;
  pinMode(BLINK_LED, OUTPUT);

// Example of a task which is started with no end, but which we want to keep a
// link to so we can pause and resume it later.
  HB_Task = tasks.doTaskEvery(&HeartBeat, _hb_DELAY) ;

#if defined(__AVR_ATmega32U4__)     // Leonardo -- wait until serial opens
  while (!Serial) tasks.checkQueue() ;  // may as well pulse the pwm led
#endif
  Serial.begin(SERIAL_BAUD) ;
  inputString[0] = 'V' ; doParseSerial() ; // Display compile tag and OK indicating input readiness
}

void loop() {
  if (Serial.available()) doGetSerial() ;      // SerialEvent is not supported on Leonardo, so manually check
  if (stringComplete)     doParseSerial() ;    // Parse input

  tasks.checkQueue() ;
}

#pragma mark Support Functions
//************************************************************************

#define xstr(s) str(s)
#define str(s) #s

#define _hexByte(myByte)     ((((uint8_t) (myByte)) & 0xF0) ?  F("") : F("0")) << _HEX(((uint8_t) (myByte)))
#define _hexWord(myWord)     _hexByte(((((uint16_t) (myWord)) & 0xFF00) >> 8)) << _hexByte((((uint16_t) (myWord)) & 0x00FF))
#define _binaryFlag(myByte)  ((((uint8_t) (myByte)) & 0x80) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x40) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x20) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x10) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x08) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x04) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x02) ? F("@") : F(".")) \
                          << ((((uint8_t) (myByte)) & 0x01) ? F("@") : F("."))

void HeartBeat() {
  static uint8_t hbval   = ((_hb_MAX - _hb_MIN) / 2) + _hb_MIN ; // start at halfway point
  static int8_t  hbdelta = _hb_DELTA ;

  hbval += hbdelta;
  if  (hbval >= _hb_MAX) { hbdelta = -hbdelta; hbval = _hb_MAX ; }
  if  (hbval <= _hb_MIN) { hbdelta = -hbdelta; hbval = _hb_MIN ; }

  analogWrite(HB_LED, hbval);
}

void doGetSerial() {
  while (!stringComplete && Serial.available()) {
    char inChar = (char)Serial.read();
    Serial << inChar ;
    inputString[stringLength++] = inChar; // wrap... will deal with longer if and when...
    if (inChar == '\n' || inChar == '\r') {
      Serial << endl ;
      stringComplete = true;              // We have something to parse
      inputString[stringLength] = 0 ;     // null terminate so string funcs can work with it
      inputString[255] = 0 ;              // Just in case we wrapped exactly
    }
  }
}

void doParseSerial() {
  char *command, *current ;
  char *param[2] = { NULL, NULL } ;
  byte paramCount = 0 ;
  bool parseError = false ;

  current = inputString ;
  command = strsep(&current, " ") ;

  // This "language" has no more than 2 distinct parameters for a given command...
  while (current && paramCount < 2) { param[paramCount++] = strsep(&current, " ") ; }

  if (command[0] > '`' && command[0] < '{') command[0] &= B11011111 ; // if letter, make upper case for smaller switch list
  switch(command[0]) {
    case 0  :   // NULL  input -- // special case if want to change later
    case '\n':  // empty input -- new-line received, but nothing else    // special case if want to change later
    case '\r':  // empty input -- return received, but nothing else      // special case if want to change later
      break ;
    case '?':
      Serial << F(
        " B(link) # [#2] -- blink led # times with a cycle time of #2 (500 ms if blank)\r\n"
        " L(ight) [#]    -- set pulsing led to a specific brightness or return to pulsing if blank\r\n"
        " T(asks)        -- list tasks in SimpleScheduler queue\r\n"
        " V(ersion)      -- print compile time info\r\n"
      ) ;
      break ;
    case 'B':

    // Example of a task which runs a set number of times and uses a simple captureless lambda function
    // to define the task with a parameter (a task pointer -- SimpleTask me) so it can access it's own
    // state functions to determine if it is running for the first time or the last time.  In addition,
    // it creates another task which is to fire off halfway through it's period... this allows the
    // repeating task to just toggle the LED high without needing to capture state info -- it creates it's
    // own task, with a shorter period, to toggle the LED low.

      tasks.doTaskEvery([](SimpleTask me)
        {
              digitalWrite(BLINK_LED, HIGH) ;
              if (me->isFirstRun())
                Serial << F("First Run!") << endl ;
              if (me->isLastRun())
                Serial << F("Last Run!") << endl ;
              tasks.doTaskAfter([]()
                {
                  digitalWrite(BLINK_LED, LOW) ;
                },
                me->period / 2) ;
        },
        param[1] ? strtol(param[1], NULL, 0) : 500, strtol(param[0], NULL, 0), true) ;
    break ;
    case 'L':

    // Example of pausing and resuming a task

      if (param[0]) {
        tasks.pauseTask(HB_Task) ;
        analogWrite(HB_LED, strtol(param[0], NULL, 0)) ;
      } else {
        tasks.resumeTask(HB_Task) ;
      }
      break ;
    case 'V':
      Serial << F(" Compiled on " __DATE__ " at " __TIME__ " with Arduino IDE " xstr(ARDUINO)) << endl
             << F(" Type '?' for help.") << endl ;
      break ;
    case 'T': {
      SimpleTask current = tasks.getNextTask(NULL) ;
      uint8_t i = 0 ;

      while (current) {
        Serial << F(" ") << _DEC(i) << F(" ") << _hexWord(current)
               << F(" Flags: ") << _binaryFlag(current->getTaskFlags())
               << F(" lastRun: ") << _DEC(current->lastRun)
               << F(" period: ") << _DEC(current->period)
               << F(" loopMax: ") << _DEC(current->loopMax)
               << F(" loopCount: ") << _DEC(current->loopCount)
               << endl ;
        current = tasks.getNextTask(current) ;
        i++ ;
      }
      Serial << F(" Task Count: ") << _DEC(i) << F(" Time: ") << millis() << endl ;
    } break ;
    default:
      parseError = true ;
      break ;
  }

  if (parseError) { Serial << F("!! parse error") ; }

  Serial << endl << F("OK") << endl ; stringLength = 0 ; inputString[0] = 0 ; stringComplete = false ;
}



