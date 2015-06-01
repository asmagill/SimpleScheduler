
//  Heartbeat pulsing led similar to the one found in ArduinoISP

#include <SimpleScheduler.h>

#define LED  3  // Must be on a pin that supports PWM

// Define the heartbeat parameters... this approximates the heartbeat in ArduinoISP
//#define _hb_MAX    192  // Maximum "brightness" -- 255 is full power
//#define _hb_MIN     32  // Minimum "brightness" -- 0 is off
//#define _hb_DELTA    8  // step size for each change in brightness for the pulse
//#define _hb_WIDTH  800  // full pulse (low to high to low) time

// I like a more subdued heartbeat pulse...
#define _hb_MAX     30
#define _hb_MIN      0
#define _hb_DELTA    1
#define _hb_WIDTH 4000

// How often should we adjust the brightness of the heartbeat led?
#define _hb_DELAY   (_hb_WIDTH / ((_hb_MAX - _hb_MIN) * 2 / _hb_DELTA))

void HeartBeat() {
  // if _hb_DELTA != 1 and (_hb_MAX + _hb_DELTA > 255 or _hb_MIN - _hb_DELTA < 0), then change
  // hbval to uint16_t and change the >= and <= to > and < in the two if's below... it will cost
  // you approx 30 bytes of flash ram, but it's the only safe way I've found to have one of the
  // end-points near the boundries and still work with a delta != 1.  Attempts to detect rollover
  // all seem to cost more than 30 bytes, unless I'm missing something...
  static uint8_t hbval   = ((_hb_MAX - _hb_MIN) / 2) + _hb_MIN ; // start at halfway point
  static int8_t  hbdelta = _hb_DELTA ;

  hbval += hbdelta;
  if  (hbval >= _hb_MAX) { hbdelta = -hbdelta; hbval = _hb_MAX ; }
  if  (hbval <= _hb_MIN) { hbdelta = -hbdelta; hbval = _hb_MIN ; }

  analogWrite(LED, hbval);
}

SimpleScheduler tasks = SimpleScheduler() ;
SimpleTask      HB_Task ;

void setup() {
  pinMode(LED, OUTPUT);

  analogWrite(LED, 0) ;

// Variable assignment not strictly necessary -- but by capturing it, logic in loop(),
// perhaps a button on another pin, perhaps some other activity can pause and resume
// the task with:
//    tasks.pauseTask(HB_Task) and tasks.resumeTask(HB_Task)

  HB_Task = tasks.doTaskEvery(&HeartBeat, _hb_DELAY) ;

}

void loop() {

  tasks.checkQueue() ;
}
