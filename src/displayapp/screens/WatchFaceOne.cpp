#include "WatchFaceOne.h"

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

WatchFaceOne::WatchFaceOne(DisplayApp* app,
                                     Controllers::DateTime& dateTimeController,
                                     Controllers::Battery& batteryController,
                                     Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificatioManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController)
    : Screen(app),
      currentDateTime {{}},
      dateTimeController {dateTimeController},
      batteryController {batteryController},
      bleController {bleController},
      settingsController {settingsController},
      heartRateController {heartRateController},
      motionController {motionController} {
  settingsController.SetClockFace(3);
  
  obj_background = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(obj_background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_click(obj_background, true);
  lv_obj_set_size(obj_background, 240, 240);
  lv_obj_set_pos(obj_background, 0, 0);
  lv_obj_move_background(obj_background);
  lv_obj_set_style_local_radius(obj_background, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  
  obj_line = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(obj_line, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_set_style_local_radius(obj_line, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, 0);
  lv_obj_set_size(obj_line, 50, 240);
  lv_obj_align(obj_line, lv_scr_act(), LV_ALIGN_CENTER, 0, 0);
  
  obj_uppereye = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(obj_uppereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_radius(obj_uppereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_size(obj_uppereye, 30, 30);
  lv_obj_align(obj_uppereye, lv_scr_act(), LV_ALIGN_CENTER, 0, 20);
  
  obj_lowereye = lv_obj_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_bg_color(obj_lowereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  lv_obj_set_style_local_radius(obj_lowereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
  lv_obj_set_size(obj_lowereye, 30, 30);
  lv_obj_align(obj_lowereye, lv_scr_act(), LV_ALIGN_CENTER, 0, -20);
  
  label_hours = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_hours, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_set_style_local_text_color(label_hours, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_hours, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 0);
  lv_label_set_text(label_hours, "01");
  
  label_minutes = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_font(label_minutes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_extrabold_compressed);
  lv_obj_set_style_local_text_color(label_minutes, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_BLACK);
  lv_obj_align(label_minutes, lv_scr_act(), LV_ALIGN_CENTER, 25, 0);
  lv_label_set_text(label_minutes, "01");
  
  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceOne::~WatchFaceOne() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceOne::Refresh() {
  auto isCharging = batteryController.IsCharging() || batteryController.IsPowerPresent();
  if (isCharging) {
    lv_obj_set_style_local_bg_color(obj_uppereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xfb6257));
    lv_obj_set_style_local_bg_color(obj_lowereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x8bfffe));
  } else {
    lv_obj_set_style_local_bg_color(obj_uppereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_obj_set_style_local_bg_color(obj_lowereye, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_WHITE);
  }
  
  currentDateTime = dateTimeController.CurrentDateTime();
  if (currentDateTime.IsUpdated()) {
    auto newDateTime = currentDateTime.Get();
    
    auto dp = date::floor<date::days>(newDateTime);
    auto time = date::make_time(newDateTime - dp);
    
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
  
      lv_label_set_text(label_hours, hoursChar);
      lv_label_set_text(label_minutes, minutesChar);
    }
    lv_label_set_text(label_hours, hoursChar);
    lv_label_set_text(label_minutes, minutesChar);
  }
}