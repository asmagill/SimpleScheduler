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
#include <SimpleScheduler.h>

// Getter functions for task attributes we don't want the user changing
uint8_t    t_scheduledTask::getTaskFlags() const { return taskFlags ; }

// Status functions for task
bool t_scheduledTask::isFirstRun() const {
  return (taskFlags & _m_taskFirstRun) ? true : false ;
}
bool t_scheduledTask::isLastRun() const {
  return (taskFlags & _m_taskLastRun) ? true : false ;
}

// Constructor -- simply initialize tasklist as empty.

SimpleScheduler::SimpleScheduler(void) {
  taskList           = NULL ;
  microsOfLastCheck  = 0 ;
  averageGranularity = 0 ;
  sampleSize         = 0 ;
}

// Destructor -- remove all tasks to free up heap space.
//    probably unnecessary as Scheduler is expected to be created in the global sketch space
//    so it is accessible in both setup() and loop(), but it's good programming practice, and
//    I do intend to experiment with non-ArduinoIDE development at some point, and it will be
//    good to minimize the things which I'll need to change in my libraries.  Since we do
//    use dynamically allocated memory, let's make sure we play nice and return it if/when
//    this goes out of scope.

SimpleScheduler::~SimpleScheduler(void) {
  SimpleTask current = taskList ;

  while (current) {
    SimpleTask next = current ;
    removeTask(current) ;
    current = next ;
  }
  taskList = NULL ;
}

// set up a repeating task -- this is the public task constructor for tasks which do not receive their task ptr as an argument.

SimpleTask SimpleScheduler::doTaskEvery(void (*theTask)(void), uint32_t timing, uint16_t count /* = 0 */, bool immediateRun /* = true */) {
  return taskBuilder(theTask, timing, count, immediateRun, false, false) ;
}

// set up a single shot task (which does not receive it's pointer) that will occur after the specified number of millis

SimpleTask SimpleScheduler::doTaskAfter(void (*theTask)(void), uint32_t timing) {
  return taskBuilder(theTask, timing, 1, false, false, false) ;
}

// set up a repeating task -- this is the public task constructor for tasks which do receive their task ptr as an argument.

SimpleTask SimpleScheduler::doTaskEvery(void (*theTask)(SimpleTask), uint32_t timing, uint16_t count /* = 0 */, bool immediateRun /* = true */) {
  return taskBuilder((void (*)()) theTask, timing, count, immediateRun, true, false) ;
}

// set up a single shot task (which does receive it's pointer) that will occur after the specified number of millis

SimpleTask SimpleScheduler::doTaskAfter(void (*theTask)(SimpleTask), uint32_t timing) {
  return taskBuilder((void (*)()) theTask, timing, 1, false, true, false) ;
}

// sames as above, but use microsecond instead of millisecond loops
SimpleTask SimpleScheduler::doTaskEveryMicros(void (*theTask)(void), uint32_t timing, uint16_t count /* = 0 */, bool immediateRun /* = true */) {
  return taskBuilder(theTask, timing, count, immediateRun, false, true) ;
}
SimpleTask SimpleScheduler::doTaskAfterMicros(void (*theTask)(void), uint32_t timing) {
  return taskBuilder(theTask, timing, 1, false, false, true) ;
}
SimpleTask SimpleScheduler::doTaskEveryMicros(void (*theTask)(SimpleTask), uint32_t timing, uint16_t count /* = 0 */, bool immediateRun /* = true */) {
  return taskBuilder((void (*)()) theTask, timing, count, immediateRun, true, true) ;
}
SimpleTask SimpleScheduler::doTaskAfterMicros(void (*theTask)(SimpleTask), uint32_t timing) {
  return taskBuilder((void (*)()) theTask, timing, 1, false, true, true) ;
}

// private function which does the heavy lifting for the public task creation functions above

SimpleTask SimpleScheduler::taskBuilder(void (*theTask)(void), uint32_t timing, uint16_t count, bool immediateRun, bool sendSelf, bool inMicros) {

  SimpleTask newTask = (SimpleTask) malloc(sizeof(t_scheduledTask)) ;
  newTask->taskFlags = _m_taskFirstRun ;
  if (count != 1) newTask->taskFlags |= _m_taskRepeats ;
  if (sendSelf)   newTask->taskFlags |= _m_taskSendSelf ;
  if (inMicros)   newTask->taskFlags |= _m_taskUseMicros ;

  newTask->theTask = theTask ;
  newTask->lastRun = immediateRun ? 0 : (inMicros ? micros() : millis()) ;
  newTask->period = timing ;
  newTask->loopMax = count ;
  newTask->loopCount = 0 ;

//   SimpleTask taskPtr = taskList, prevPtr = NULL ;
//
//   while (taskPtr) {
//     prevPtr = taskPtr ;
//     taskPtr = taskPtr->next ;
//   }
//
//   newTask->next = NULL ;
//   newTask->prev = prevPtr ;
//   if (prevPtr)
//     prevPtr->next = newTask ;
//   else
//     taskList = newTask ;

// on the theory that newer tasks are probably the most transitory and time
// sensitive as opposed to early defined tasks, we stick the new ones at the
// front of the list.

  newTask->next = taskList ; newTask->prev = NULL ;

  if (taskList) taskList->prev = newTask ;

  taskList = newTask ;

  return newTask ;
}

// pause the specified task

SimpleTask SimpleScheduler::pauseTask(SimpleTask theTask) {
  if (theTask) theTask->taskFlags |= _m_taskPaused ;
#ifdef _SimpleScheduler_Debug_
    else
      Serial.println(F("\r\n! NULL task in pauseTask")) ;
#endif
  return theTask ;
}

// resume the specified task

SimpleTask SimpleScheduler::resumeTask(SimpleTask theTask, bool resetCount /* = false */) {
  if (theTask) {
    theTask->taskFlags &= ~_m_taskPaused ;
    if (resetCount) theTask->lastRun = (theTask->taskFlags | _m_taskUseMicros) ? micros() : millis() ;
  }
#ifdef _SimpleScheduler_Debug_
    else
      Serial.println(F("\r\n! NULL task in resumeTask")) ;
#endif
  return theTask ;
}

// returns true if a task is currently paused, otherwise false

bool  SimpleScheduler::isTaskPaused(SimpleTask theTask) {
  if (theTask)
    return (theTask->taskFlags & _m_taskPaused) ? true : false ;
  else {
    return false ;
#ifdef _SimpleScheduler_Debug_
    Serial.println(F("\r\n! NULL task in isTaskPaused")) ;
#endif
  }
}

// remove a task after it has run one more time, no matter how many times it may have left

SimpleTask SimpleScheduler::removeTaskAfterNext(SimpleTask theTask) {
  if (theTask) {
    theTask->loopMax = 1 ;
    theTask->loopCount = 0 ;
    theTask->taskFlags &= ~_m_taskRepeats ;
  }
#ifdef _SimpleScheduler_Debug_
    else
      Serial.println(F("\r\n! NULL task in removeTaskAfterNext")) ;
#endif
  return theTask ;
}

// remove a task immediately, before it runs again

SimpleTask SimpleScheduler::removeTask(SimpleTask theTask) {
  if (theTask) {
    SimpleTask prevPtr, nextPtr ;

    prevPtr = theTask->prev ;
    nextPtr = theTask->next ;

    free(theTask) ; theTask = NULL ;

    if (prevPtr) {
      prevPtr->next = nextPtr ;
    } else {
      taskList = nextPtr ;
    }
    if (nextPtr) {
      nextPtr->prev = prevPtr ;
    }
  }
#ifdef _SimpleScheduler_Debug_
    else
      Serial.println(F("\r\n! NULL task in removeTask")) ;
#endif
  return theTask ;
}

// uint32_t SimpleScheduler::queueCheckCount() {
//     return visitsCounter ;
// }

// this is the function called in loop() which actually runs through the tasks and
// determines if it is time to run them again.

void SimpleScheduler::checkQueue() {
  SimpleTask current = taskList, next = NULL ;

  if (micros() > microsOfLastCheck) {
      currentGranularity = micros() - microsOfLastCheck ;
  } else {
      currentGranularity = ((uint32_t)(-1l) - microsOfLastCheck) + micros() ;
  }

// average will be biased a bit at the wrap-around point, but it should be close enough for all but the
// most picky... in which case, you're probably doing your own thing because you need more precision
// than this is capable of.

  if (sampleSize == 0) {
      averageGranularity = currentGranularity ;
  } else {
      averageGranularity = (averageGranularity * (sampleSize - 1) + currentGranularity) / sampleSize ;
  }
  if (sampleSize != AVG_SAMPLE_SIZE) sampleSize++ ;

  while(current) {
    next = current->next ;
    if (!(current->taskFlags & _m_taskPaused)) {
      uint32_t delta ;

      if (current->taskFlags & _m_taskUseMicros)
          delta = (micros() < current->lastRun) ?
              ((micros() + current->period) - (current->lastRun + current->period)) :
              (micros() - current->lastRun) ;
      else
          delta = (millis() < current->lastRun) ?
              ((millis() + current->period) - (current->lastRun + current->period)) :
              (millis() - current->lastRun) ;
      if (delta > current->period) {
        if (current->loopMax != 0) {
          current->loopCount++ ;
          if (current->loopCount == current->loopMax) current->taskFlags |= _m_taskLastRun ;
        }
        if (current->taskFlags & _m_taskSendSelf) {
         ((void (*)(SimpleTask)) (current->theTask))(current) ;
        } else {
          (current->theTask)() ;
        }
        current->taskFlags &= ~_m_taskFirstRun ;
        current->lastRun = (current->taskFlags | _m_taskUseMicros) ? micros() : millis() ;
        if(current->taskFlags & _m_taskLastRun) removeTask(current) ;
      }
    }
    current = next ;
  }

  microsOfLastCheck = micros() ;
}

// returns a pointer to the next task, or the taskList head, if currentTask is NULL
// most useful for generating a task list or calculating the number of queued tasks
SimpleTask SimpleScheduler::getNextTask(SimpleTask currentTask) {
  return (currentTask) ? currentTask->next : taskList ;
}