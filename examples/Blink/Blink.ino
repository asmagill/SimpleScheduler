
//  Varient on the basic blink example, using SimpleScheduler

#include <SimpleScheduler.h>

#define LED  13

SimpleScheduler tasks = SimpleScheduler() ;

void setup() {
  pinMode(LED, OUTPUT);

  //  do
  //    set LED to HIGH
  //    schedule task for now + 500 millis to set LED to LOW
  //  end,
  //  every 1000 millis,
  //  forever,
  //  start immediately

  tasks.doTaskEvery([](SimpleTask me)
    {
      digitalWrite(LED, HIGH) ;
      tasks.doTaskAfter([]()
        {
          digitalWrite(LED, LOW) ;
        },
        me->period / 2) ;
    },
    1000, 0, true) ;

}

void loop() {

  tasks.checkQueue() ;
}
