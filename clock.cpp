#include "clock.h"

#include <ESP8266HTTPClient.h>

#include "Arduino.h"

const String kWeek[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
const String kTimeServer =
    "http://free.timeanddate.com/clock/i6s3ue10/n156/tt0/tm1/th1/tb4";

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
  HTTPClient http;
  http.begin(kTimeServer);
  if (http.GET() == HTTP_CODE_OK) {
    String response = http.getString();
    int x = int(response.length());
    while (response.substring(x - 3, x) != "br>") {
      x--;
    }
    hour_ = response.substring(x, x + 2).toInt();
    minute_ = response.substring(x + 3, x + 5).toInt();
    second_ = response.substring(x + 6, x + 8).toInt();
    while (response.substring(x - 3, x) != "t1>") {
      x--;
    }
    String weekday = response.substring(x, x + 3);
    for (int i = 0; i < 7; i++) {
      if (weekday.equals(kWeek[i])) {
        day_ = i;
        break;
      }
    }
    next_millis_ = millis() + 1000;
    needs_update_ = false;
  } else {
    // TODO: Better handle failures
    ESP.restart();
  }
  http.end();
}
