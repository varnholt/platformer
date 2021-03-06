#include "crusher.h"

#include "fixturenode.h"

#include "framework/easings/easings.h"

#include "framework/tmxparser/tmximage.h"
#include "framework/tmxparser/tmxlayer.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "framework/tmxparser/tmxproperty.h"
#include "framework/tmxparser/tmxproperties.h"
#include "framework/tmxparser/tmxtileset.h"
#include "texturepool.h"

#include <iostream>

int32_t Crusher::mInstanceCounter = 0;

static constexpr auto BLADE_HORIZONTAL_TILES = 5;
static constexpr auto BLADE_VERTICAL_TILES = 1;

static constexpr auto BLADE_SIZE_X = (BLADE_HORIZONTAL_TILES * PIXELS_PER_TILE) / PPM;
static constexpr auto BLADE_SIZE_Y = (BLADE_VERTICAL_TILES * PIXELS_PER_TILE) / PPM;

static constexpr auto BLADE_SHARPNESS = 0.1f;
static constexpr auto BLADE_TOLERANCE = 0.06f;


//-----------------------------------------------------------------------------
Crusher::Crusher(GameNode* parent)
   : GameNode(parent)
{
   setName("DeathBlock");

   mTexture = TexturePool::getInstance().get("data/level-crypt/tilesets/crushers.png");

   mInstanceId = mInstanceCounter;
   mInstanceCounter++;
}


//-----------------------------------------------------------------------------
void Crusher::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   color.draw(mSpriteSpike);
   color.draw(mSpritePusher);
   color.draw(mSpriteMount);
}


//-----------------------------------------------------------------------------
void Crusher::step(const sf::Time& dt)
{
   const auto distance_to_be_traveled = 48.0f;

   switch (mState)
   {
      case State::Idle:
      {
         mIdleTime += dt;
         break;
      }
      case State::Extract:
      {
         const auto val = distance_to_be_traveled * Easings::easeOutBounce<float>(mExtractionTime.asSeconds());

         switch (mAlignment)
         {
            case Alignment::PointsDown:
               mBladeOffset.y = val;
               break;
            case Alignment::PointsUp:
               mBladeOffset.y = -val;
               break;
            case Alignment::PointsLeft:
               mBladeOffset.x = -val;
               break;
            case Alignment::PointsRight:
               mBladeOffset.x = val;
               break;
            case Alignment::PointsNowhere:
               break;
         }


         mExtractionTime += dt * 1.0f;

         break;
      }
      case State::Retract:
      {
         const auto val = distance_to_be_traveled * (1.0f - Easings::easeInSine<float>(mRetractionTime.asSeconds()));

         switch (mAlignment)
         {
            case Alignment::PointsDown:
               mBladeOffset.y = val;
               break;
            case Alignment::PointsUp:
               mBladeOffset.y = -val;
               break;
            case Alignment::PointsLeft:
               mBladeOffset.x = -val;
               break;
            case Alignment::PointsRight:
               mBladeOffset.x = val;
               break;
            case Alignment::PointsNowhere:
               break;
         }

         mRetractionTime += dt * 0.4f;

         break;
      }
   }
}


void Crusher::update(const sf::Time& dt)
{
   updateState();
   step(dt);
   updateSpritePositions();
   updateTransform();
}


//-----------------------------------------------------------------------------
void Crusher::updateState()
{
   switch (mMode)
   {
      case Mode::Interval:
      {
         switch (mState)
         {
            case State::Idle:
            {
               // go to extract when idle time is elapsed
               if (mIdleTime.asSeconds() > 3.0f)
               {
                  mIdleTime = {};

                  if (mPreviousState == State::Retract || mPreviousState == State::Idle)
                  {
                     mState = State::Extract;
                  }
                  else
                  {
                     mState = State::Retract;
                  }
               }
               break;
            }
            case State::Extract:
            {
               // extract until normalised extraction time is 1
               if (mExtractionTime.asSeconds() >= 1.0f)
               {
                  mState = State::Idle;
                  mPreviousState = State::Extract;
                  mExtractionTime = {};
               }
               break;
            }
            case State::Retract:
            {
               // retract until normalised retraction time is 1
               if (mRetractionTime.asSeconds() >= 1.0f)
               {
                  mState = State::Idle;
                  mPreviousState = State::Retract;
                  mRetractionTime = {};
               }
               break;
            }
         }

         break;
      }
      case Mode::Distance:
      {
         break;
      }
   }
}


//-----------------------------------------------------------------------------
void Crusher::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   if (tmxObject->_properties)
   {
      const auto it = tmxObject->_properties->_map.find("alignment");

      if (it != tmxObject->_properties->_map.end())
      {
         // std::cout << it->second->mValueStr << std::endl;

         const auto alignment = it->second->_value_string;
         if (alignment == "up")
         {
            mAlignment = Alignment::PointsUp;
         }
         else if (alignment == "down")
         {
            mAlignment = Alignment::PointsDown;
         }
         else if (alignment == "left")
         {
            mAlignment = Alignment::PointsLeft;
         }
         else if (alignment == "right")
         {
            mAlignment = Alignment::PointsRight;
         }
      }
   }

   //    0123456789012
   //   +-------------+
   //  0|<#         #>|
   //  1|<#         #>|
   //  2|<#CBAA AABC#>|
   //  3|<#         #>|
   //  4|<#         #>|
   //  6|^^^^^    A   |
   //  7|#####    A   |
   //  8|  C      B   |
   //  9|  B      C   |
   //  0|  A    ##### |
   //  1|  A    VVVVV |
   //   +-------------+
   //    0123456789012

   mPixelPosition.x = tmxObject->_x_px;
   mPixelPosition.y = tmxObject->_y_px;

   mSpriteMount.setTexture(*mTexture);
   mSpritePusher.setTexture(*mTexture);
   mSpriteSpike.setTexture(*mTexture);

   switch (mAlignment)
   {
      case Alignment::PointsDown:
      {
         mSpriteMount.setTextureRect({
               7 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE
            }
         );

         mSpritePusher.setTextureRect({
               7 * PIXELS_PER_TILE,
               7 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               1 // * PIXELS_PER_TILE - i only want this to be one pixel in height so scaling is easy
            }
         );

         mSpriteSpike.setTextureRect({
               7 * PIXELS_PER_TILE,
               8 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE
            }
         );

         mPixelOffsetPusher.y = 2 * PIXELS_PER_TILE;
         mPixelOffsetSpike.y = 2 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsUp:
      {
         mSpriteMount.setTextureRect({
               0 * PIXELS_PER_TILE,
               9 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE
            }
         );

         mSpritePusher.setTextureRect({
               0 * PIXELS_PER_TILE,
               8 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               1
            }
         );

         mSpriteSpike.setTextureRect({
               0 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE
            }
         );

         mPixelOffsetPusher.y = 6 * PIXELS_PER_TILE;
         mPixelOffsetSpike.y = 3 * PIXELS_PER_TILE;
         mPixelOffsetMount.y = 6 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsLeft:
      {
         mSpritePusher.setTextureRect({
               3 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               1,
               5 * PIXELS_PER_TILE
            }
         );

         mSpriteMount.setTextureRect({
               4 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         mSpriteSpike.setTextureRect({
               0 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         mPixelOffsetPusher.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetPusher.x = 3 * PIXELS_PER_TILE;
         mPixelOffsetSpike.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetMount.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetMount.x = 3 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsRight:
      {
         mSpriteMount.setTextureRect({
               7 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               2 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         mSpritePusher.setTextureRect({
               9 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               1,
               5 * PIXELS_PER_TILE
            }
         );

         mSpriteSpike.setTextureRect({
              10 * PIXELS_PER_TILE,
               0 * PIXELS_PER_TILE,
               3 * PIXELS_PER_TILE,
               5 * PIXELS_PER_TILE
            }
         );

         mPixelOffsetPusher.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetPusher.x = -1 * PIXELS_PER_TILE;
         mPixelOffsetSpike.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetSpike.x = -1 * PIXELS_PER_TILE;
         mPixelOffsetMount.y = -1 * PIXELS_PER_TILE;
         mPixelOffsetMount.x = -3 * PIXELS_PER_TILE;

         break;
      }

      case Alignment::PointsNowhere:
         break;
   }

   setupBody(world);
}


//-----------------------------------------------------------------------------
void Crusher::updateTransform()
{
   auto x = (mBladeOffset.x + mPixelPosition.x) / PPM;
   auto y = (mBladeOffset.y + mPixelPosition.y - PIXELS_PER_TILE) / PPM + (5 * PIXELS_PER_TILE) / PPM;
   mBody->SetTransform(b2Vec2(x, y), 0.0f);
}


//-----------------------------------------------------------------------------
void Crusher::setupBody(const std::shared_ptr<b2World>& world)
{
   //       +-+
   //       | |
   //       | |
   //       | |
   //       | |
   // +-----+-+------+
   // \             /
   //  \___________/

   b2Vec2 bladeVertices[4];

   switch (mAlignment)
   {
      case Alignment::PointsLeft:
      {

         bladeVertices[0] = b2Vec2(0           ,                BLADE_SHARPNESS + BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[1] = b2Vec2(0           , BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[2] = b2Vec2(BLADE_SIZE_Y,                                  BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[3] = b2Vec2(BLADE_SIZE_Y, BLADE_SIZE_X                   - BLADE_TOLERANCE - BLADE_SIZE_X);
         break;
      }
      case Alignment::PointsRight:
      {
         bladeVertices[0] = b2Vec2(0            + PIXELS_PER_TILE / PPM,                                  BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[1] = b2Vec2(BLADE_SIZE_Y + PIXELS_PER_TILE / PPM,                BLADE_SHARPNESS + BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[2] = b2Vec2(BLADE_SIZE_Y + PIXELS_PER_TILE / PPM, BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE - BLADE_SIZE_X);
         bladeVertices[3] = b2Vec2(0            + PIXELS_PER_TILE / PPM, BLADE_SIZE_X                   - BLADE_TOLERANCE - BLADE_SIZE_X);
         break;
      }
      case Alignment::PointsDown:
      {
         bladeVertices[0] = b2Vec2(                                 BLADE_TOLERANCE, 0           );
         bladeVertices[1] = b2Vec2(               BLADE_SHARPNESS + BLADE_TOLERANCE, BLADE_SIZE_Y);
         bladeVertices[2] = b2Vec2(BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE, BLADE_SIZE_Y);
         bladeVertices[3] = b2Vec2(BLADE_SIZE_X                   - BLADE_TOLERANCE, 0           );
         break;
      }
      case Alignment::PointsUp:
      {
         bladeVertices[0] = b2Vec2(                                 BLADE_TOLERANCE, BLADE_SIZE_Y - PIXELS_PER_TILE / PPM);
         bladeVertices[1] = b2Vec2(               BLADE_SHARPNESS + BLADE_TOLERANCE, 0            - PIXELS_PER_TILE / PPM);
         bladeVertices[2] = b2Vec2(BLADE_SIZE_X - BLADE_SHARPNESS - BLADE_TOLERANCE, 0            - PIXELS_PER_TILE / PPM);
         bladeVertices[3] = b2Vec2(BLADE_SIZE_X                   - BLADE_TOLERANCE, BLADE_SIZE_Y - PIXELS_PER_TILE / PPM);
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   b2BodyDef deadlyBodyDef;
   deadlyBodyDef.type = b2_kinematicBody;
   mBody = world->CreateBody(&deadlyBodyDef);

   b2PolygonShape spikeShape;
   spikeShape.Set(bladeVertices, 4);
   auto deadlyFixture = mBody->CreateFixture(&spikeShape, 0);

   auto objectData = new FixtureNode(this);
   objectData->setType(ObjectTypeCrusher);
   deadlyFixture->SetUserData(static_cast<void*>(objectData));

   auto box_width = 0.0f;
   auto box_height = 0.0f;
   b2Vec2 box_center;

   switch (mAlignment)
   {
      case Alignment::PointsLeft:
      {
         box_width = BLADE_SIZE_Y * 0.5f;
         box_height = BLADE_SIZE_X * 0.5f;
         box_center = {box_width, box_height};
         box_center.x += PIXELS_PER_TILE / PPM;
         box_center.y -= BLADE_SIZE_X;
         break;
      }
      case Alignment::PointsRight:
      {
         box_width = BLADE_SIZE_Y * 0.5f;
         box_height = BLADE_SIZE_X * 0.5f;
         box_center = {box_width, box_height};
         box_center.y -= BLADE_SIZE_X;
         break;
      }
      case Alignment::PointsUp:
      {
         box_width = BLADE_SIZE_X * 0.5f;
         box_height = BLADE_SIZE_Y * 0.5f;
         box_center = {box_width, box_height};
         break;
      }
      case Alignment::PointsDown:
      {
         box_width = BLADE_SIZE_X * 0.5f;
         box_height = BLADE_SIZE_Y * 0.5f;
         box_center = {box_width, box_height};
         box_center.y -= PIXELS_PER_TILE / PPM;
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   b2PolygonShape boxShape;
   boxShape.SetAsBox(
      box_width,
      box_height,
      box_center,
      0.0f
   );

   mBody->CreateFixture(&boxShape, 0);
}


void Crusher::updateSpritePositions()
{

   switch (mAlignment)
   {
      case Alignment::PointsDown:
      {
         mSpritePusher.setScale(1.0f, mBladeOffset.y);
         break;
      }
      case Alignment::PointsUp:
      {
         mSpritePusher.setScale(1.0f, mBladeOffset.y);
         break;
      }
      case Alignment::PointsLeft:
      {
         mSpritePusher.setScale(mBladeOffset.x, 1.0f);
         break;
      }
      case Alignment::PointsRight:
      {
         mSpritePusher.setScale(mBladeOffset.x, 1.0f);
         break;
      }
      case Alignment::PointsNowhere:
      {
         break;
      }
   }

   mSpriteMount.setPosition(mPixelPosition + mPixelOffsetMount);
   mSpritePusher.setPosition(mPixelPosition + mPixelOffsetPusher);
   mSpriteSpike.setPosition(mPixelPosition + mPixelOffsetSpike + mBladeOffset);
}

