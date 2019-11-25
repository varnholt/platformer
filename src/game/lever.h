#pragma once

#include <filesystem>
#include <functional>
#include <stdint.h>

#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#include "gamemechanism.h"

struct TmxLayer;
struct TmxObject;
struct TmxTileSet;


class Lever : public GameMechanism
{

public:

   using Callback = std::function<void(int32_t)>;

   enum class Type {
      TwoState,
      TriState
   };

   enum class State {
      Left   = -1,
      Middle = 0,
      Right  = 1,
   };

   Lever() = default;

   void update(const sf::Time& dt);

   void toggle();
   void setCallbacks(const std::vector<Callback>& callbacks);

   static void addSearchRect(TmxObject* rect);
   static void merge(
      std::vector<std::shared_ptr<GameMechanism>> lasers,
      std::vector<std::shared_ptr<GameMechanism>> fans,
      std::vector<std::shared_ptr<GameMechanism>> platforms,
      std::vector<std::shared_ptr<GameMechanism>> belts
   );

   static std::vector<std::shared_ptr<GameMechanism>> load(
      TmxLayer* layer,
      TmxTileSet* tileSet,
      const std::filesystem::path& basePath,
      const std::shared_ptr<b2World>&
   );

   bool getPlayerAtLever() const;
   void setPlayerAtLever(bool playerAtLever);


private:

   Type mType = Type::TwoState;
   State mState = State::Left;
   State mPreviousState = State::Left;
   std::vector<Callback> mCallbacks;
   sf::Rect<int32_t> mRect;
   bool mPlayerAtLever = false;

   static std::vector<TmxObject*> mRectangles;
   static std::vector<std::shared_ptr<Lever>> sLevers;
};


