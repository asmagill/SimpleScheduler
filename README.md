SimpleScheduler for Arduino
===========================

This is a very simple scheduler for tasks which are to be repeated every X ms or are to occur after X ms has passed.  There is no attempt at true "real time" -- the scheduler only promises that the task won't run *before* X ms have past.

The goal of this library is to get rid of `delay(...)` statements in `loop()` and other functions in order to make all functions more responsive and simulate, as much as possible with a single threaded controller running non-interupt driven code, the appearance of simultaneous operations.

### Environment
This software has been tested on the Arduino UNO and the Arduino Leonardo.  Other than the example which displays memory addresses in the task list, there is no reason to expect that this code would not work on other Arduino platforms including those in which `size_t` is 32 rather than 16 bits -- *however it has not been tested in these environments.*

This code does use `malloc` and `free` to dynamically create the data structure which contains task state information.  Some may have concerns with using dynamic memory in such a limited environment, but in my experience, this has not been a problem with this library and care is taken to make sure dynamic memory is freed as soon as it is no longer required. On controllers with 16 bit addresses, each task consumes 21 bytes of heap space (this includes heap block-size overhead).

### How it Works

This scheduler works by keeping a list of tasks (functions fitting the declaration of `void (*func)(void)` or `void (*func)(SimpleTask)`, which thankfully includes captureless lambdas: `[](){ whatever ; }` and `[](SimpleTask){ whatever ; }`

Within this list, a task is designated as a repeater or as a single shot.  A repeater may have a `loopMax` set (how many times to run before removing itself from the task list) and a single shot is really just a task with a `loopMax` of 1.

You must invoke `SimpleScheduler.checkQueue()` from within `loop()`, ideally as often as possible to get the most accurate results... Note that this only checks tasks when `checkQueue()` is invoked, and then only runs tasks which were last run > `period` ms ago... each task that matches is run in the order of the `taskList` which is currently more like a stack then a queue (last-in-first-out); at present there is no checking for which task is "most behind", though this may change in the future.

The rationale for this last-in-first-out approach is the assumption that many of the tasks added outside of `setup()` will likely be short lived or one-shot, clean-up type tasks (e.g. detaching a servo after giving it enough time to complete its movement) and are likely a little more sensitive to delays or shorter lived than tasks created earlier or in `setup()`.

Other functions which are invoked from within `loop()` or tasks which take a long time to run can throw off the timing of other tasks, as everything is still executed sequentially, so if you really need accurate "real time" timing, consider an interupt driven approach (beyond the scope of this library and me, at present) or break up your tasks into smaller, shorter tasks using task chaining (i.e. create sub tasks from within a task) so that other tasks have a chance to run.

### Usage

~~~cpp
#include <SimpleScheduler.h>

SimpleScheduler tasks = SimpleScheduler() ;

void setup() {
  // ... setup code ...
}

void loop() {
  // ... other loop() code ...
  tasks.checkQueue() ;
}
~~~

### Functions

###### SimpleScheduler
~~~cpp
SimpleScheduler() ;
~~~
Constructor for the SimpleScheduler class.

It is expected to be used as follows outside of `setup()` or `loop()` so that it is available to all functions within the sketch:

	SimpleScheduler tasks = SimpleScheduler() ;

~~~cpp
~SimpleScheduler() ;
~~~
Destructor for the SimpleScheduler class.

I'm not actually certain if/when this might be called, as I haven't yet experimented with limiting the scope of `SimpleScheduler()` or coding for the Arduino platform outside of _ArduinoIDE_, but to make sure we "play nice with others" and release dynamically allocated memory if/when the class instance does go out of scope, this function visits each task defined in the task queue and calls `SimpleScheduler.removeTask(SimpleTask)` on it.

~~~cpp
void SimpleScheduler.checkQueue() ;
~~~
This function is expected to be invoked from within `loop()` and checks each defined task to determine if it is time to execute the task again.  Remember that this is the only place where tasks are actually given a chance to execute, so it should be called as often as possible... `loop()` with minimal-to-no unnecessary `delay()` invocations is ideal.

~~~cpp
SimpleTask SimpleScheduler.doTaskAfter(void (*theTask)(void), uint32_t timing) ;
SimpleTask SimpleScheduler.doTaskAfter(void (*theTask)(SimpleTask), uint32_t timing) ;
~~~
Schedule a task for a single execution in `timing` ms.  The task may be defined as a function which takes no parameters (see Heartbeat example), or as a function which takes a single parameter of the type `SimpleTask` (see Blink and InteractiveTest examples.)  When the task is completed, it will be automatically removed from the queue.

Captureless lambda functions are also supported, as they reduce to one of the above functional casts (see Blink and InteractiveTest examples.)

Returns a pointer to the new task.

~~~cpp
SimpleTask SimpleScheduler.doTaskEvery(void (*theTask)(void), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;
SimpleTask SimpleScheduler.doTaskEvery(void (*theTask)(SimpleTask), uint32_t timing, uint16_t count = 0, bool immediateRun = true) ;
~~~
Schedule a task for a multiple executions, `timing` ms apart.  If `count` is supplied and is not 0, then the task will only execute that many times before removing itself from the queue. If `immediateRun` is `true` or not supplied, then the task is scheduled to run for the first time immediately (during the next `SimpleScheduler.checkQueue()` call), otherwise `SimpleTask->lastRun` is set to the current value of `millis()`, resulting in the task's first run occuring in `timing` ms.

The task may be defined as a function which takes no parameters (see Heartbeat example), or as a function which takes a single parameter of the type `SimpleTask` (see Blink and InteractiveTest examples.)

Captureless lambda functions are also supported, as they reduce to one of the above functional casts (see Blink and InteractiveTest examples.)

Note that if `count` is 0 or not supplied, then the task will run forever unless it is paused or removed (or the Arduino is powered off.)

Returns a pointer to the new task.

~~~cpp
SimpleTask SimpleScheduler.pauseTask(SimpleTask theTask) ;
~~~
Pauses a queued task (or does nothing if it is currently paused).  Returns the task pointer.

~~~cpp
bool SimpleScheduler.isTaskPaused(SimpleTask theTask) ;
~~~
Returns `true` if the task is currently paused, otherwise `false`.

~~~cpp
SimpleTask SimpleScheduler.resumeTask(SimpleTask theTask, bool resetCycle = false) ;
~~~
Resumes a paused task (or does nothing if it is not currently paused).  If `resetCycle` is passed in and is `true`, then the `SimpleTask->lastRun` field is set to the current value of `millis()` so that the next run will occur after `SimpleTask->period` ms have occured. If is `false` or not specified, then `SimpleTask->lastRun` is untouched, and it is likely that the task will trigger immediately (during the next `SimpleScheduler.checkQueue()` call), depending upon how much time has passed. Returns the task pointer.

~~~cpp
SimpleTask SimpleScheduler.removeTask(SimpleTask theTask) ;
~~~
Immediately removes the task from the queue.  Should always return NULL.

~~~cpp
SimpleTask SimpleScheduler.removeTaskAfterNext(SimpleTask theTask) ;
~~~
Sets the task's `loopMax` to 1, the `loopCount` to 0, and unsets the `_m_taskRepeats` flag for the task so that it will be removed from the queue after it's next invocation.  Returns the task pointer.

###### SimpleTask
~~~cpp
uint8_t SimpleTask->getTaskFlags() ;
~~~
Returns the flags for the task as an 8-bit number.  A getter is used for this field so that it is read-only and cannot be changed from outside of the scheduler.

~~~cpp
SimpleTask SimpleTask->getPrev() ;
~~~
Returns the previous task in the task queue, or `NULL` if we are the first in the list.  A getter is used for this field so that it is read-only and cannot be changed from outside of the scheduler. Probably not much use except for a task list, and may go away if I can find a better way to generate a running task list.

~~~cpp
SimpleTask SimpleTask->getNext() ;
~~~
Returns the next task in the task queue, or `NULL` if we are the last in the list.  A getter is used for this field so that it is read-only and cannot be changed from outside of the scheduler. Probably not much use except for a task list, and may go away if I can find a better way to generate a running task list.

~~~cpp
bool SimpleTask->isFirstRun() ;
~~~
Returns `true` if the task is running for the first time since it was created.

~~~cpp
bool SimpleTask->isLastRun() ;
~~~
Returns `true` if the task is running for the last time either because it was given a loop count or because `SimpleScheduler.removeTaskAfterNext(SimpleTask)` was called for the task.

### `SimpleTask` definition:
~~~cpp
typedef struct t_scheduledTask {
  public:
    uint32_t          lastRun ;
    uint32_t          period ;
    uint16_t          loopMax ;
    uint16_t          loopCount ;
    uint8_t           getTaskFlags() const ;
    t_scheduledTask  *getPrev()      const ;
    t_scheduledTask  *getNext()      const ;
    bool              isFirstRun()   const ;
    bool              isLastRun()    const ;

  private:
    uint8_t           taskFlags ;
    void            (*theTask)(void) ;
    t_scheduledTask  *prev ;
    t_scheduledTask  *next ;

  friend class SimpleScheduler ;
} *SimpleTask ;
~~~
###### Public Members
* `lastRun` -- timestamp from millis() when task was last run.
* `period` -- time in millis between invocations of the task.
* `loopMax` -- number of times to invoke task before autoremoval, or 0 if the task is to be run indefinately.
* `loopCount` -- number of times a task with a non-zero loopMax has been invoked.
* `getTaskFlags()` -- getter function so `taskFlags` can be read-only
* `getPrev()` -- getter function so `prev` can be read-only
* `getNext()` -- getter function so `next` can be read-only
* `isFirstRun()` -- returns `true` if this is the first time the task has been called by `SimpleScheduler.checkQueue()`
* `isLastRun()` -- returns `true` when the task is on its last run either because it was given a loop count or because `SimpleScheduler.removeTaskAfterNext(SimpleTask)` was called for the task.

###### Private Members
* `taskFlags` -- flags indicating task state -- see below.
* `theTask` -- the callback function for the task.
* `next`/`prev` -- pointers to other tasks in the queue.

### Constants

~~~cpp
#define _SimpleScheduler_Debug_
~~~
If this macro is set in `SimpleScheduler.h`, then the SimpleScheduler methods will print an error message via the serial port whenever the SimpleTask pointer passed into them is NULL.  Hopefully this never happens!  Comment out this macro if you're not using the serial port or if you're expecting specific output via the serial port and this extraneous output might break or confuse the receiver.

###### Task Flags `SimpleTask->taskFlags`
~~~cpp
#define _m_taskRepeats B00000001
~~~
Set whenever a task is defined via `SimpleScheduler.doTaskEvery(...)`.  Unset for tasks defined via `SimpleScheduler.doTaskAfter(...)` or when a task is modified with `SimpleScheduler.removeTaskAfterNext(...)`.  This flag may go away in the future because it isn't really used and the same information can be determined by comparing `SimpleTask->loopMax` and `SimpleTask->loopCount`.

~~~cpp
#define _m_taskPaused B00000010
~~~
Set whenever a task is paused via `SimpleScheduler.pauseTask(SimpleTask)`. Unset by default or whenever a task is resumed via `SimpleScheduler.resumeTask(SimpleTask)`.

~~~cpp
#define _m_taskSendSelf B00000100
~~~
Set when the callback function for the task uses the `void (*func)(SimpleTask)` cast.  Unset when the callback function uses the `void (*func)(void)` cast.

~~~cpp
#define _m_taskLastRun  B01000000
~~~
Set when the task is on its last run either because it was given a loop count or because `SimpleScheduler.removeTaskAfterNext(SimpleTask)` was called for the task.

~~~cpp
#define _m_taskFirstRun B10000000
~~~
Set when the task is called for the very first time from `SimpleScheduler.checkQueue()`.

### Examples
* _Blink_ -- This example recreates the standard Blink example using tasks.  It shows a lambda function based task which takes the `SimpleTask` parameter and triggers another task each time it is invoked as a simple example of task chaining.
* _Heartbeat_ -- This example recreates the Heartbeat pulse found in _ArduinoISP_ as a task.
* _InteractiveTest_ -- This example is more complex which starts with the Heartbeat task but allows you to trigger the Blink task with a count limit, pause and resume the Heartbeat task, and list the currently running tasks.  The output of running tasks currently requires [Streaming.h](http://arduiniana.org/libraries/streaming/) to simplify the format of the output.

### Known Caveats, Questions, or Issues
* I am not sure how this library will handle the approximate 45 day wraparound of millis()... hopefully well, but this hasn't been tested yet.
* Lambda functions with captures are not supported, which means tasks have to keep their own state information and other variances within static variables or utilize Global variables.  Initial investigation into supporting lambdas with variable capture is not promising and looks like it may significantly affect the memory footprint, but if anyone has any ideas, I am all ears!
* Should a task be able to modify its function Similar to the [Arduino Playground's TaskScheduler](http://playground.arduino.cc/Code/TaskScheduler)?  I started this before finding the above code, and while I think my code is maybe simpler to use (at least for the way I think and organize things!), this is one feature I didn't include and I can't decide if changing to support this is a feature or a risk.

### License

> Released under MIT license.
>
> Copyright (c) 2015 Aaron Magill
>
> Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
>
> The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
>
> THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
