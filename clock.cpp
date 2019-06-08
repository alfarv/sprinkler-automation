#include "clock.h"

// #include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include "Arduino.h"

const String kWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const long kTimeZone = -4*3600;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", kTimeZone);

void Clock::HandleTime() {
  if (int(millis() - next_millis_) >= 0) {
    Update();
    second_ = (second_ + 1) % 60;
    if (second_ == 0) {
      minute_ = (minute_ + 1) % 60;
      if (minute_ == 0) {
        hour_ = (hour_ + 1) % 24;
        if (hour_ == 0) {
          day_ = (day_ + 1) % 7;
          needs_update_ = true;
        }
      }
    }
    next_millis_ += 1000;
    if (timer_seconds_ > 0) {
      timer_seconds_--;
    }
  }
}

int Clock::GetDay() {
  return day_;
}

int Clock::GetHour() {
  return hour_;
}

int Clock::GetMinute() {
  return minute_;
}

void Clock::SetTimer(int seconds) {
  timer_seconds_ = seconds;
}

bool Clock::IsTimerDone() {
  return timer_seconds_ == 0;
}

String Clock::GetTime() {
  char time[10];
  sprintf(time, "%s %02d:%02d", kWeek[day_].c_str(), hour_, minute_);
  return String(time);
}

void Clock::Update() {
  if (!needs_update_) {
    return;
  }

  timeClient.begin();
  while (!timeClient.update()){
   timeClient.forceUpdate();
  }

  day_ = timeClient.getDay();
  hour_ = timeClient.getHours();
  minute_ = timeClient.getMinutes();
  second_ = timeClient.getSeconds();
  next_millis_ = millis() + 1000;
  needs_update_ = false;
  timeClient.end();
}
