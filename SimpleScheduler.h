/*
  Released under MIT license.

  Copyright (c) 2015 Aaron Magill

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef _SIMPLESCHEDULER_H
#define _SIMPLESCHEDULER_H

// sample size to max out at for loop granularity average
#define AVG_SAMPLE_SIZE 100000

#include <Arduino.h>

// Outputs error messages if task methods are called to act on a NULL task...
// shouldn't happen, but a nice check if it won't screw up other use of Serial output.
#define _SimpleScheduler_Debug_

#define _m_taskRepeats   B00000001 // don't really use this... do we need it?
#define _m_taskPaused    B00000010 // is task paused?
#define _m_taskSendSelf  B00000100 // whether task is invoked as 0 - task(void) or 1 - task(SimpleTask)
#define _m_taskUseMicros B00001000
#define _m_taskLastRun   B01000000 // is this our last run?
#define _m_taskFirstRun  B10000000 // is this our first run?

typedef struct t_scheduledTask {
  public:
    uint32_t          lastRun ;
    uint32_t          period ;
    uint16_t          loopMax ;
    uint16_t          loopCount ;
    uint8_t           getTaskFlags() const ;
    bool              isFirstRun()   const ;
    bool              isLastRun()    const ;

  private:
    uint8_t           taskFlags ;
    void            (*theTask)(void) ;
    t_scheduledTask  *prev ;
    t_scheduledTask  *next ;

  friend class SimpleScheduler ;
} *SimpleTask ;

class SimpleScheduler {
  public:
                SimpleScheduler() ;   // Constructor
               ~SimpleScheduler() ;   // Destructor
    void        checkQueue() ;

    SimpleTask  doTaskAfter(void (*theTask)(void), uint32_t timing) ;
    SimpleTask  doTaskEvery(void (*theTask)(void), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;
    SimpleTask  doTaskAfter(void (*theTask)(SimpleTask), uint32_t timing) ;
    SimpleTask  doTaskEvery(void (*theTask)(SimpleTask), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;
    SimpleTask  doTaskAfterMicros(void (*theTask)(void), uint32_t timing) ;
    SimpleTask  doTaskEveryMicros(void (*theTask)(void), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;
    SimpleTask  doTaskAfterMicros(void (*theTask)(SimpleTask), uint32_t timing) ;
    SimpleTask  doTaskEveryMicros(void (*theTask)(SimpleTask), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;

    SimpleTask  pauseTask(SimpleTask theTask) ;
    SimpleTask  resumeTask(SimpleTask theTask, bool resetCycle = false) ;
    bool        isTaskPaused(SimpleTask theTask) ;

    SimpleTask  removeTask(SimpleTask theTask) ;
    SimpleTask  removeTaskAfterNext(SimpleTask theTask) ;

    SimpleTask  getNextTask(SimpleTask currentTask) ;
    uint32_t    currentGranularity, averageGranularity ;

  private:
    SimpleTask  taskList ;
    SimpleTask  taskBuilder(void (*theTask)(void), uint32_t timing, uint16_t count, bool immediateRun, bool sendSelf, bool inMicros) ;
    uint32_t    microsOfLastCheck, sampleSize ;
};

#endif // _SIMPLESCHEDULER_H
