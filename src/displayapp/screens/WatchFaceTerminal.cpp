#include "WatchFaceTerminal.h"

#include <date/date.h>
#include <lvgl/lvgl.h>
#include <cstdio>
#include "BatteryIcon.h"
#include "BleIcon.h"
#include "NotificationIcon.h"
#include "Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "../DisplayApp.h"

using namespace Pinetime::Applications::Screens;

WatchFaceTerminal::WatchFaceTerminal(DisplayApp* app,
                                     Controllers::DateTime& dateTimeController,
                                     Controllers::Battery& batteryController,
                                     Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificatioManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
    : Screen(app),
      currentDateTime{{}},
      dateTimeController{dateTimeController},
      batteryController{batteryController},
      bleController{bleController},
      heartRateController{heartRateController},
      motionController{motionController} {
  settingsController.SetClockFace(2);
  
  displayedChar[0] = 0;
  displayedChar[1] = 0;
  displayedChar[2] = 0;
  displayedChar[3] = 0;
  displayedChar[4] = 0;
  
  backgroundLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_click(backgroundLabel, true);
  lv_label_set_long_mode(backgroundLabel, LV_LABEL_LONG_CROP);
  lv_obj_set_size(backgroundLabel, 240, 240);
  lv_obj_set_pos(backgroundLabel, 0, 0);
  lv_label_set_text(backgroundLabel, "");
  
  label_shell = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_shell, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_set_pos(label_shell, 0, 20);
  lv_label_set_text(label_shell, "pt$ status");
  
  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text(label_time, "t: (loading)");
  lv_obj_set_pos(label_time, 0, 40);
  
  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_date, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_label_set_text(label_date, "dt: (loading)");
  lv_obj_set_pos(label_date, 0, 60);
  
  label_ble = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_ble, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
  lv_label_set_text(label_ble, "ble: (loading)");
  lv_obj_set_pos(label_ble, 0, 80);
  
  label_battery = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_battery, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
  lv_label_set_text(label_battery, "batt: (loading)");
  lv_obj_set_pos(label_battery, 0, 100);
  
  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
  lv_label_set_text(heartbeatValue, "bpm: -");
  lv_obj_set_pos(heartbeatValue, 0, 120);
  
  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(stepValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x00FFE7));
  lv_label_set_text(stepValue, "st: 0");
  lv_obj_set_pos(stepValue, 0, 140);
  
  label_shellend = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_shellend, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x999999));
  lv_obj_set_pos(label_shellend, 0, 160);
  lv_label_set_text(label_shellend, "pt$_");
  
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceTerminal::~WatchFaceTerminal() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceTerminal::Refresh() {
  
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated()) {
    auto batteryPercent = batteryPercentRemaining.Get();
    auto isCharging = batteryController.IsCharging() || batteryController.IsPowerPresent();
    char batteryStr[22];
    sprintf(batteryStr, "batt: %d", batteryPercent);
    lv_label_set_text(label_battery, batteryStr);
    if (isCharging) {
      lv_obj_set_style_local_text_color(label_battery, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
    }
  }
  
  bleState = bleController.IsConnected();
  if (bleState.IsUpdated()) {
    if (bleState.Get() == true) {
      lv_obj_set_style_local_text_color(label_ble, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x0000FF));
      lv_label_set_text(label_ble, "ble: connected");
    } else {
      lv_obj_set_style_local_text_color(label_ble, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text(label_ble, "ble: disconnected");
    }
  }
  
  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();
    
    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    auto yearMonthDay = date::year_month_day(dp);
    
    auto year = (int) yearMonthDay.year();
    auto month = static_cast<Pinetime::Controllers::DateTime::Months>((unsigned) yearMonthDay.month());
    auto day = (unsigned) yearMonthDay.day();
    auto dayOfWeek = static_cast<Pinetime::Controllers::DateTime::Days>(date::weekday(yearMonthDay).iso_encoding());
    
    int hour = time.hours().count();
    auto minute = time.minutes().count();
    
    char minutesChar[3];
    sprintf(minutesChar, "%02d", static_cast<int>(minute));
    
    char hoursChar[3];
    sprintf(hoursChar, "%02d", hour);
    
    if (hoursChar[0] != displayedChar[0] || hoursChar[1] != displayedChar[1] || minutesChar[0] != displayedChar[2] ||
        minutesChar[1] != displayedChar[3]) {
      displayedChar[0] = hoursChar[0];
      displayedChar[1] = hoursChar[1];
      displayedChar[2] = minutesChar[0];
      displayedChar[3] = minutesChar[1];
      
      char timeStr[6];
      
      sprintf(timeStr, "t: %c%c:%c%c", hoursChar[0], hoursChar[1], minutesChar[0], minutesChar[1]);
      lv_label_set_text(label_time, timeStr);
    }
    
    if ((year != currentYear) || (month != currentMonth) || (dayOfWeek != currentDayOfWeek) || (day != currentDay)) {
      char dateStr[22];
      sprintf(dateStr, "dt: %s, %d.%d.%d", dateTimeController.DayOfWeekShortToStringLow(), day, (uint8_t) month, year);
      lv_label_set_text(label_date, dateStr);
      
      currentYear = year;
      currentMonth = month;
      currentDayOfWeek = dayOfWeek;
      currentDay = day;
    }
  }
  
  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xCE1B1B));
      lv_label_set_text_fmt(heartbeatValue, "bpm: %d", heartbeat.Get());
    } else {
      lv_obj_set_style_local_text_color(heartbeatValue, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x1B1B1B));
      lv_label_set_text_static(heartbeatValue, "bpm: -");
    }
  }
  
  stepCount = motionController.NbSteps();
  motionSensorOk = motionController.IsSensorOk();
  if (stepCount.IsUpdated() || motionSensorOk.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "st: %lu", stepCount.Get());
  }
}
