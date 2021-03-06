// base
#include "gamecontactlistener.h"

// game
#include "audio.h"
#include "projectile.h"
#include "constants.h"
#include "fixturenode.h"
#include "framework/tools/timer.h"
#include "luanode.h"
#include "mechanisms/bouncer.h"
#include "mechanisms/conveyorbelt.h"
#include "mechanisms/movingplatform.h"
#include "player/player.h"

#include <iostream>


// http://www.iforce2d.net/b2dtut/collision-anatomy
//
// TODO: pass collision normal to projectile detonation
//       so animation can be aligned to detonation angle.

GameContactListener* GameContactListener::sInstance = nullptr;


GameContactListener::GameContactListener()
 : b2ContactListener()
{
  sInstance = this;
}


int32_t GameContactListener::getNumFootContacts() const
{
   return mNumFootContacts;
}


int32_t GameContactListener::getDeadlyContacts() const
{
   return mNumDeadlyContacts;
}


bool GameContactListener::isPlayer(FixtureNode* obj) const
{
   if (obj == nullptr)
   {
      return false;
   }

   auto p = dynamic_cast<Player*>(obj->getParent());

   if (p == nullptr)
   {
      return false;
   }

   return true;
}


void GameContactListener::processOneSidedWalls(b2Contact* contact, b2Fixture* playerFixture, b2Fixture* platformFixture)
{
   // decide whether an incoming contact to the platform should be disabled or not

   // if the head bounces against the one-sided wall, disable the contact
   // until there is no more contact with the head (EndContact)
   if (playerFixture != nullptr && (static_cast<FixtureNode*>(playerFixture->GetUserData()))->hasFlag("head"))
   {
      contact->SetEnabled(false);
   }

   if (!platformFixture)
   {
      return;
   }

   // if moving down, the contact should be solid
   if (playerFixture->GetBody()->GetLinearVelocity().y > 0.0f)
   {
      return;
   }

   // not all points are moving down towards the platform, the contact should not be solid
   contact->SetEnabled(false);
}


void GameContactListener::BeginContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   b2Fixture* platformFixture = nullptr;
   b2Fixture* playerFixture = nullptr;

   FixtureNode* fixtureNodeA = nullptr;
   FixtureNode* fixtureNodeB = nullptr;

   if (fixtureUserDataA)
   {
      fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);
   }

   if (fixtureUserDataB)
   {
      fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);
   }

   if (fixtureUserDataA)
   {
      switch (fixtureNodeA->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               // store ground body in player
               if (contact->GetFixtureB()->GetType() == b2Shape::e_chain)
               {
                  Player::getCurrent()->setGroundBody(contact->GetFixtureB()->GetBody());
               }
               mNumFootContacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumHeadContacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmLeftContacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmRightContacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));

            if (isPlayer(fixtureNodeB))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixtureNodeB && fixtureNodeB->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixtureNodeB->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            auto projectile = dynamic_cast<Projectile*>(fixtureNodeA);

            // if it's an arrow, let postsolve handle it. if the impulse is not
            // hard enough, the arrow should just fall on the ground
            if (!projectile->isSticky())
            {
               projectile->setScheduledForRemoval(true);
            }

            break;
         }
         case ObjectTypeSolidOneSided:
         {
            platformFixture = contact->GetFixtureA();
            playerFixture = contact->GetFixtureB();
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            // check if platform smashes the player
            auto fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);
            if (fixtureNodeB && fixtureNodeB->getType() == ObjectType::ObjectTypePlayerHeadSensor)
            {
               if (Player::getCurrent()->isOnGround())
               {
                  mSmashed = true;
               }
            }

            auto platformBody = contact->GetFixtureA()->GetBody();
            Player::getCurrent()->setPlatformBody(platformBody);

            mNumMovingPlatformContacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixtureNodeA)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixtureNodeB))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeA->getProperty("damage"));
               fixtureNodeA->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
        case ObjectTypeMoveableBox:
           break;
        case ObjectTypeDeathBlock:
           break;
        case ObjectTypeSolid:
           break;
      }
   }

   if (fixtureUserDataB)
   {
      FixtureNode* fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNodeB->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               // store ground body in player
               if (contact->GetFixtureA()->GetType() == b2Shape::e_chain)
               {
                  Player::getCurrent()->setGroundBody(contact->GetFixtureA()->GetBody());
               }

               mNumFootContacts++;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumHeadContacts++;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmLeftContacts++;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmRightContacts++;
            }
            break;
         }
         case ObjectTypeProjectile:
         {
            auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));

            if (isPlayer(fixtureNodeA))
            {
               Player::getCurrent()->damage(damage);
            }
            else if (fixtureNodeA && fixtureNodeA->getType() == ObjectTypeEnemy)
            {
               auto p = dynamic_cast<LuaNode*>(fixtureNodeA->getParent());
               if (p != nullptr)
               {
                  p->luaHit(damage);
               }
            }

            auto projectile = dynamic_cast<Projectile*>(fixtureNodeB);

            // if it's an arrow, let postsolve handle it. if the impulse is not
            // hard enough, the arrow should just fall on the ground
            if (!projectile->isSticky())
            {
               projectile->setScheduledForRemoval(true);
            }

            break;
         }
         case ObjectTypeSolidOneSided:
         {
            platformFixture = contact->GetFixtureB();
            playerFixture = contact->GetFixtureA();
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts++;
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts++;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            // check if platform smashes the player
            auto fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);
            if (fixtureNodeA && fixtureNodeA->getType() == ObjectType::ObjectTypePlayerHeadSensor)
            {
               if (Player::getCurrent()->isOnGround())
               {
                  mSmashed = true;
               }
            }

            auto platformBody = contact->GetFixtureB()->GetBody();
            Player::getCurrent()->setPlatformBody(platformBody);

            mNumMovingPlatformContacts++;
            break;
         }
         case ObjectTypeBouncer:
         {
            dynamic_cast<Bouncer*>(fixtureNodeB)->activate();
            break;
         }
         case ObjectTypeEnemy:
         {
            if (isPlayer(fixtureNodeA))
            {
               // printf("collision with enemy\n");
               auto damage = std::get<int32_t>(fixtureNodeB->getProperty("damage"));
               fixtureNodeB->collisionWithPlayer();
               Player::getCurrent()->damage(damage);
               break;
            }
            break;
         }
         case ObjectTypeDoor:
            break;
         case ObjectTypeConveyorBelt:
            break;
         case ObjectTypeMoveableBox:
            break;
         case ObjectTypeDeathBlock:
            break;
         case ObjectTypeSolid:
            break;
      }
   }

   // handle one sided walls
   processOneSidedWalls(contact, playerFixture, platformFixture);
}


void GameContactListener::EndContact(b2Contact* contact)
{
   auto fixtureUserDataA = contact->GetFixtureA()->GetUserData();
   auto fixtureUserDataB = contact->GetFixtureB()->GetUserData();

   FixtureNode* fixtureNodeA = nullptr;
   FixtureNode* fixtureNodeB = nullptr;

   if (fixtureUserDataA)
   {
      fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);
   }

   if (fixtureUserDataB)
   {
      fixtureNodeB = static_cast<FixtureNode*>(fixtureUserDataB);
   }

   if (fixtureUserDataA)
   {
      auto fixtureNodeA = static_cast<FixtureNode*>(fixtureUserDataA);

      switch (fixtureNodeA->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumFootContacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumHeadContacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmLeftContacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureB()->IsSensor())
            {
               mNumArmRightContacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts--;
            break;
         }
         case ObjectTypeSolidOneSided:
         {
            // reset the default state of the contact
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeB))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            mNumMovingPlatformContacts--;
            break;
         }
         default:
         {
            break;
         }
      }
   }

   if (fixtureUserDataB)
   {
      auto fixtureNode = static_cast<FixtureNode*>(fixtureUserDataB);

      switch (fixtureNode->getType())
      {
         case ObjectTypeCrusher:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypePlayerFootSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumFootContacts--;
            }
            break;
         }
         case ObjectTypePlayerHeadSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumHeadContacts--;
            }
            break;
         }
         case ObjectTypePlayerLeftArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmLeftContacts--;
            }
            break;
         }
         case ObjectTypePlayerRightArmSensor:
         {
            if (!contact->GetFixtureA()->IsSensor())
            {
               mNumArmRightContacts--;
            }
            break;
         }
         case ObjectTypePlayer:
         {
            mNumPlayerContacts--;
            break;
         }
         case ObjectTypeSolidOneSided:
         {
            // reset the default state of the contact
            contact->SetEnabled(true);
            break;
         }
         case ObjectTypeDeadly:
         {
            if (isPlayer(fixtureNodeA))
            {
               mNumDeadlyContacts--;
            }
            break;
         }
         case ObjectTypeMovingPlatform:
         {
            mNumMovingPlatformContacts--;
            break;
         }
         default:
         {
            break;
         }
      }
   }

   // debug();
   // std::cout << "left arm: " << mNumArmLeftContacts << " " << "right arm: " << mNumArmRightContacts << std::endl;
}


void GameContactListener::PreSolve(b2Contact* contact, const b2Manifold* /*oldManifold*/)
{
   ConveyorBelt::processContact(contact);
}


void GameContactListener::PostSolve(b2Contact* contact, const b2ContactImpulse *contactImpulse)
{
   // normal impulse
   //
   //    The normal force is the force applied at a contact point to prevent the shapes from penetrating.
   //    For convenience, Box2D works with impulses. The normal impulse is just the normal force multiplied
   //    by the time step.
   //
   //
   // tangent impulse
   //
   //    The tangent force is generated at a contact point to simulate friction. For convenience,
   //    this is stored as an impulse.
   //
   //
   // for debugging
   //
   // auto normalMax = 0.0f;
   // auto tangentMax = 0.0f;
   // for (auto i = 0; i < contactImpulse->count; i++)
   // {
   //    normalMax = std::max(normalMax, contactImpulse->normalImpulses[i]);
   //    tangentMax = std::max(tangentMax, contactImpulse->tangentImpulses[i]);
   // }
   //
   // if (normalMax > 0.025f || tangentMax > 0.01f)
   // {
   //    std::cout << "normal max: " << normalMax << " tangent max: " << tangentMax << std::endl;
   // }

   // check if the player hits something at a heigh speed or
   // if something hits the player at a nigh speed
   auto userDataA = contact->GetFixtureA()->GetUserData();
   auto userDataB = contact->GetFixtureB()->GetUserData();

   auto impulse = contactImpulse->normalImpulses[0];

   if (userDataA)
   {
      auto nodeA = static_cast<FixtureNode*>(userDataA);

      if (nodeA->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
      else if (nodeA->getType() == ObjectTypeProjectile)
      {
         auto projectile = dynamic_cast<Projectile*>(nodeA);

         if (projectile->isSticky())
         {
            if (projectile->hitSomething())
            {
               return;
            }

            projectile->setHitSomething(true);

            // this is only needed for arrows, so could be generalised
            Timer::add(
               std::chrono::milliseconds(1000),
               [projectile](){projectile->setScheduledForRemoval(true);},
               Timer::Type::Singleshot
            );

            if (impulse > 0.0003f)
            {
               // std::cout << "arrow hit with " << impulse << std::endl;
               projectile->setScheduledForInactivity(true);
            }
         }
      }
   }

   if (userDataB)
   {
      auto nodeB = static_cast<FixtureNode*>(userDataB);

      if (nodeB->getType() == ObjectTypePlayer)
      {
         processImpulse(impulse);
      }
      else if (nodeB->getType() == ObjectTypeProjectile)
      {
         auto projectile = dynamic_cast<Projectile*>(nodeB);

         if (projectile->isSticky())
         {
            if (projectile->hitSomething())
            {
               return;
            }

            projectile->setHitSomething(true);

            // this is only needed for arrows, so could be generalised
            Timer::add(
               std::chrono::milliseconds(1000),
               [projectile](){projectile->setScheduledForRemoval(true);},
               Timer::Type::Singleshot
            );

            if (impulse > 0.0003f)
            {
               // std::cout << "arrow hit with " << impulse << std::endl;
               projectile->setScheduledForInactivity(true);
            }
         }
      }
   }
}


void GameContactListener::debug()
{
   std::cout
      << "head contacts: " << getNumHeadContacts() << std::endl
      << "foot contacts: " << getNumFootContacts() << std::endl
      << "deadly contacts: " << getDeadlyContacts() << std::endl
      << "moving platform contacts: " << getNumMovingPlatformContacts() << std::endl
      << "player contacts: " << getNumPlayerContacts() << std::endl
   ;
}


void GameContactListener::processImpulse(float impulse)
{
   // filter just ordinary ground contact
   if (impulse < 0.03f)
   {
      return;
   }

   Player::getCurrent()->impulse(impulse);
}


int32_t GameContactListener::getNumArmRightContacts() const
{
   return mNumArmRightContacts;
}


bool GameContactListener::isSmashed() const
{
   return mSmashed;
}


int32_t GameContactListener::getNumArmLeftContacts() const
{
   return mNumArmLeftContacts;
}


int32_t GameContactListener::getNumHeadContacts() const
{
   return mNumHeadContacts;
}


void GameContactListener::reset()
{
   mNumHeadContacts = 0;
   mNumFootContacts = 0;
   mNumPlayerContacts = 0;
   mNumArmLeftContacts = 0;
   mNumArmRightContacts = 0;
   mNumDeadlyContacts = 0;
   mNumMovingPlatformContacts = 0;
   mSmashed = false;
}


GameContactListener* GameContactListener::getInstance()
{
  if (!sInstance)
  {
    new GameContactListener();
  }

  return sInstance;
}


int32_t GameContactListener::getNumPlayerContacts() const
{
   return mNumPlayerContacts;
}


int32_t GameContactListener::getNumMovingPlatformContacts() const
{
   return mNumMovingPlatformContacts;
}


