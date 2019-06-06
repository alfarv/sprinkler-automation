#ifndef SPRINKLER_AUTOMATION_CLOCK_H_
#define SPRINKLER_AUTOMATION_CLOCK_H_

#include "Arduino.h"

class Clock {
 public:
  Clock() : needs_update_(true){};
  ~Clock() = default;

  void HandleTime();
  int GetDay();
  int GetHour();
  int GetMinute();
  void SetTimer(int seconds);
  bool IsTimerDone();
  String GetTime();

 private:
  unsigned long next_millis_;
  bool needs_update_;
  int day_;
  int hour_;
  int minute_;
  int second_;
  int timer_seconds_;

  void Update();
};

#endif  // SPRINKLER_AUTOMATION_CLOCK_H_
