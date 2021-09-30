#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include "Screen.h"
#include "ScreenList.h"
#include "components/datetime/DateTimeController.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }
  
  namespace Applications {
    namespace Screens {
      
      class WatchFaceOne : public Screen {
      public:
        WatchFaceOne(DisplayApp* app,
                          Controllers::DateTime& dateTimeController,
                          Controllers::Battery& batteryController,
                          Controllers::Ble& bleController,
                          Controllers::NotificationManager& notificatioManager,
                          Controllers::Settings& settingsController,
                          Controllers::HeartRateController& heartRateController,
                          Controllers::MotionController& motionController);
        ~WatchFaceOne() override;
        
        void Refresh() override;
      
      private:
        char displayedChar[5];
        
        uint16_t currentYear = 1970;
        Pinetime::Controllers::DateTime::Months currentMonth = Pinetime::Controllers::DateTime::Months::Unknown;
        Pinetime::Controllers::DateTime::Days currentDayOfWeek = Pinetime::Controllers::DateTime::Days::Unknown;
        uint8_t currentDay = 0;
        
        DirtyValue<int> batteryPercentRemaining {};
        DirtyValue<bool> bleState {};
        DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>> currentDateTime {};
        DirtyValue<bool> motionSensorOk {};
        DirtyValue<uint32_t> stepCount {};
        DirtyValue<uint8_t> heartbeat {};
        DirtyValue<bool> heartbeatRunning {};
        
        lv_obj_t* obj_background;
        lv_obj_t* obj_line;
        lv_obj_t* obj_uppereye;
        lv_obj_t* obj_lowereye;
        lv_obj_t* label_hours;
        lv_obj_t* label_minutes;
        
        Controllers::DateTime& dateTimeController;
        Controllers::Battery& batteryController;
        Controllers::Ble& bleController;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        
        lv_task_t* taskRefresh;
      };
    }
  }
}