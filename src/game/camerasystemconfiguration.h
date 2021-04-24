#pragma once

#include <string>


class CameraSystemConfiguration
{
   public:

      static CameraSystemConfiguration& getInstance();

      float getCameraVelocityFactorX() const;
      float getFocusZoneDivider() const;
      float getTargetShiftFactor() const;
      int32_t getBackInBoundsToleranceX() const;

      float getCameraVelocityFactorY() const;
      float getPanicLineDivider() const;
      float getViewRatioY() const;
      int32_t getBackInBoundsToleranceY() const;
      int32_t getPlayerOffsetY() const;


   private:

      std::string serialize();
      void deserialize(const std::string& data);
      void deserializeFromFile(const std::string& filename = "data/config/camera.json");
      void serializeToFile(const std::string& filename = "data/config/camera.json");

      // x
      float _camera_velocity_factor_x = 4.0f;
      float _focus_zone_divider = 6.0f;
      float _target_shift_factor = 0.75f;
      int32_t _back_in_bounds_tolerance_x = 10;

      // y
      float _camera_velocity_factor_y = 3.0f;
      float _panic_line_divider = 2.5f;
      float _view_ratio_y = 1.5f;
      int32_t _back_in_bounds_tolerance_y = 10;
      int32_t _player_offset_y = 0;
      float _panic_acceleration_factor_y = 2.0f;

      static bool sInitialized;
      static CameraSystemConfiguration sInstance;
};


