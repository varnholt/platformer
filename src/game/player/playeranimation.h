#pragma once

#include <SFML/Graphics.hpp>
#include <Box2D/Box2D.h>

#include <optional>

#include "animation.h"
#include "constants.h"
#include "playercontrols.h"
#include "playerjump.h"

class PlayerAnimation
{

public:

   PlayerAnimation();

   enum class Version
   {
      V1 = 1,
      V2 = 2
   };

   struct PlayerAnimationData
   {
      bool _dead = false;
      bool _in_air = false;
      bool _in_water = false;
      bool _hard_landing = false;
      bool _crouching = false;
      bool _points_left = false;
      bool _points_right = false;
      bool _climb_joint_present = false;
      bool _moving_left = false;
      bool _moving_right = false;
      bool _wall_sliding = false;
      std::optional<Dash> _dash_dir;
      b2Vec2 _linear_velocity = b2Vec2{0.0f, 0.0f};
      int32_t _jump_steps = 0;
   };

   void update(
      const sf::Time& dt,
      const PlayerAnimationData& data
   );

   int32_t getJumpAnimationReference() const;

   std::shared_ptr<Animation> getCurrentCycle() const;

   void resetAlpha();

   void toggleVersion();

   static void generateJson();


private:

   void updateV1(
      const sf::Time& dt,
      const PlayerAnimationData& data
   );


   void updateV2(
      const sf::Time& dt,
      const PlayerAnimationData& data
   );

   // version 1
   std::shared_ptr<Animation> _idle_r;
   std::shared_ptr<Animation> _idle_l;
   std::shared_ptr<Animation> _swim_r;
   std::shared_ptr<Animation> _swim_l;
   std::shared_ptr<Animation> _run_r;
   std::shared_ptr<Animation> _run_l;
   std::shared_ptr<Animation> _dash_r;
   std::shared_ptr<Animation> _dash_l;
   std::shared_ptr<Animation> _crouch_r;
   std::shared_ptr<Animation> _crouch_l;

   std::shared_ptr<Animation> _jump_init_r;
   std::shared_ptr<Animation> _jump_up_r;
   std::shared_ptr<Animation> _jump_midair_r;
   std::shared_ptr<Animation> _jump_down_r;
   std::shared_ptr<Animation> _jump_landing_r;

   std::shared_ptr<Animation> _jump_init_l;
   std::shared_ptr<Animation> _jump_up_l;
   std::shared_ptr<Animation> _jump_midair_l;
   std::shared_ptr<Animation> _jump_down_l;
   std::shared_ptr<Animation> _jump_landing_l;

   // version 2
   std::shared_ptr<Animation> _idle_r_2;                 // supported
   std::shared_ptr<Animation> _idle_l_2;                 // supported
   std::shared_ptr<Animation> _bend_down_r_2;
   std::shared_ptr<Animation> _bend_down_l_2;
   std::shared_ptr<Animation> _idle_to_run_r_2;
   std::shared_ptr<Animation> _idle_to_run_l_2;
   std::shared_ptr<Animation> _runstop_r_2;
   std::shared_ptr<Animation> _runstop_l_2;
   std::shared_ptr<Animation> _run_r_2;                  // supported
   std::shared_ptr<Animation> _run_l_2;                  // supported
   std::shared_ptr<Animation> _dash_r_2;
   std::shared_ptr<Animation> _dash_l_2;

   std::shared_ptr<Animation> _crouch_r_2;
   std::shared_ptr<Animation> _crouch_l_2;

   std::shared_ptr<Animation> _jump_init_r_2;            // supported
   std::shared_ptr<Animation> _jump_up_r_2;              // supported
   std::shared_ptr<Animation> _jump_midair_r_2;          // supported
   std::shared_ptr<Animation> _jump_down_r_2;            // supported
   std::shared_ptr<Animation> _jump_landing_r_2;         // supported

   std::shared_ptr<Animation> _jump_init_l_2;            // supported
   std::shared_ptr<Animation> _jump_up_l_2;              // supported
   std::shared_ptr<Animation> _jump_midair_l_2;          // supported
   std::shared_ptr<Animation> _jump_down_l_2;            // supported
   std::shared_ptr<Animation> _jump_landing_l_2;         // supported

   std::shared_ptr<Animation> _double_jump_r_2;
   std::shared_ptr<Animation> _double_jump_l_2;
   std::shared_ptr<Animation> _swim_idle_r_2;
   std::shared_ptr<Animation> _swim_idle_l_2;
   std::shared_ptr<Animation> _swim_r_2;                 // supported
   std::shared_ptr<Animation> _swim_l_2;                 // supported

   std::shared_ptr<Animation> _wallslide_r_2;
   std::shared_ptr<Animation> _wallslide_l_2;
   std::shared_ptr<Animation> _wall_jump_r_2;
   std::shared_ptr<Animation> _wall_jump_l_2;
   std::shared_ptr<Animation> _appear_r_2;
   std::shared_ptr<Animation> _appear_l_2;

   int32_t _jump_animation_reference = 0;

   std::vector<std::shared_ptr<Animation>> _looped_animations;
   std::shared_ptr<Animation> _current_cycle;

   Version _version = Version::V1;
};

