#pragma once

#include "constants.h"
#include "extramanager.h"
#include "extratable.h"
#include "gamenode.h"
#include "framework/joystick/gamecontrollerinfo.h"
#include "playeranimation.h"
#include "playerclimb.h"
#include "playercontrols.h"
#include "playerjump.h"
#include "shaders/deathshader.h"

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>
#include "Box2D/Box2D.h"

#include <deque>
#include <functional>
#include <memory>
#include <set>

class Animation;
class GameContactListener;
class Weapon;
struct WeaponSystem;

class Player : public GameNode
{
   struct JumpTrace
   {
      bool jumpStarted = false;
      sf::Time jumpStartTime;
      float jumpStartY = 0.0f;
      float jumpEpsilon = 0.00001f;
      float jumpPrevY = 0.0f;
   };


   struct PositionedAnimation
   {
      sf::Vector2f mPosition;
      std::shared_ptr<Animation> mAnimation;
   };


   struct PlayerSpeed
   {
      b2Vec2 currentVelocity;
      float velocityMax = 0.0f;
      float acceleration = 0.0f;
      float deceleration = 0.0f;
   };


public:

   Player(GameNode* parent = nullptr);
   virtual ~Player() = default;

   static Player* getCurrent();

   void initialize();
   void initializeLevel();
   void initializeController();
   void draw(sf::RenderTarget& color, sf::RenderTarget& normal);

   void update(const sf::Time& dt);

   void fire();
   void die();
   void reset();
   DeathReason checkDead() const;

   bool isPointingRight() const;
   bool isPointingLeft() const;

   void setStartPixelPosition(float x, float y);

   b2Vec2 getBodyPosition() const;
   const sf::Vector2f& getPixelPositionf() const;
   const sf::Vector2i& getPixelPositioni() const;
   void setPixelPosition(float x, float y);

   float getBeltVelocity() const;
   void setBeltVelocity(float beltVelocity);
   bool isOnBelt() const;
   void setOnBelt(bool onBelt);
   void applyBeltVelocity(float &desiredVel);

   const sf::IntRect& getPlayerPixelRect() const;


   b2Body* getBody() const;
   void setWorld(const std::shared_ptr<b2World>& world);
   void resetWorld();
   void updatePixelPosition();
   void updatePreviousBodyState();
   void updatePlayerPixelRect();
   void setBodyViaPixelPosition(float x, float y);
   void setFriction(float f);


   bool getVisible() const;
   void setVisible(bool visible);


   b2Body* getPlatformBody() const;
   void setPlatformBody(b2Body* body);
   void setGroundBody(b2Body* body);

   bool isInAir() const;
   bool isInWater() const;
   bool isOnPlatform() const;
   bool isOnGround() const;
   bool isDead() const;
   bool isCrouching() const;

   void setInWater(bool inWater);

   int getZ() const;
   void setZ(int z);

   int getId() const;

   void impulse(float intensity);
   void damage(int damage, const sf::Vector2f& force = sf::Vector2f{0.0f, 0.0f});

   std::shared_ptr<ExtraManager> getExtraManager() const;

   PlayerControls& getControls();

   std::shared_ptr<WeaponSystem> getWeaponSystem() const;

   PlayerAnimation& getPlayerAnimation();


private:

   void createPlayerBody();

   void updateAnimation(const sf::Time& dt);
   void updatePixelCollisions();
   void updateAtmosphere();
   void updateVelocity();
   void updatePlatformMovement(const sf::Time& dt);
   void updateFire();
   void updateFootsteps();
   void updatePortal();
   void updatePlayerOrientation();
   void updateDash(Dash dir = Dash::None);
   void updateCrouch();
   void updateHardLanding();
   void updateGroundAngle();
   void updateDeadFixtures();
   void updateWeapons(const sf::Time& dt);
   void updateImpulse();

   bool isDashActive() const;
   void resetDash();

   void createBody();
   void createFeet();
   void setCrouching(bool enabled);

   float getMaxVelocity() const;
   float getVelocityFromController(const PlayerSpeed& speed) const;
   float getVelocityFromKeyboard(const PlayerSpeed& speed) const;
   float getDesiredVelocity(const PlayerSpeed& speed) const;
   float getDesiredVelocity() const;
   float getDeceleration() const;
   float getAcceleration() const;

   void playDustAnimation();
   void traceJumpCurve();
   void keyPressed(sf::Keyboard::Key key);


   std::shared_ptr<WeaponSystem> mWeaponSystem;
   std::shared_ptr<ExtraManager> mExtraManager;

   std::shared_ptr<b2World> mWorld;
   b2Body* mBody = nullptr;

   static const int32_t sFootCount = 4u;
   b2Fixture* mBodyFixture = nullptr;
   b2Fixture* mFootFixtures[sFootCount];

   sf::Vector2f mPixelPositionf;
   sf::Vector2i mPixelPositioni;
   sf::Sprite mSprite;
   sf::Vector2u mSpritePrev;
   sf::Vector2u mSpriteAnim;
   sf::IntRect mPlayerPixelRect;

   sf::Time mTime;
   sf::Clock mClock;
   sf::Clock mPortalClock;
   sf::Clock mDamageClock;
   bool mDamageInitialized = false;

   int mAnimSpeed = 50;

   bool mPointsToLeft = false;
   bool mVisible = true;
   bool mCrouching = false;
   bool mInWater = false;
   bool mDead = false;

   b2Vec2 mPositionPrevious;
   b2Vec2 mVelocityPrevious;
   b2Body* mPlatformBody = nullptr;
   b2Body* mGroundBody = nullptr;
   b2Vec2 mGroundNormal;
   float mNextFootStepTime = 0.0f;

   int mZ = 0;
   int mId = 0;

   bool mHardLanding = false;
   int32_t mHardLandingCycles = 0;

   float mBeltVelocity = 0.0f;
   bool mIsOnBelt = false;

   float mImpulse = 0.0f;

   int mDashSteps = 0;
   Dash mDashDir = Dash::None;

   PlayerAnimation _player_animation;

   PlayerControls mControls;
   PlayerClimb mClimb;
   PlayerJump mJump;
   JumpTrace mJumpTrace;

   std::deque<PositionedAnimation> mLastAnimations;

   static Player* sCurrent;
};

