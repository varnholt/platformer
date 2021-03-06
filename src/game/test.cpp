#include "test.h"

// things under test
#include "framework/math/maptools.h"
#include "detonationanimation.h"
#include "game/player/playeranimation.h"

#include <iostream>


void dumpAnimations()
{
   PlayerAnimation::generateJson();
}


void testBresenham()
{
   auto f = [](int32_t x, int32_t y) -> bool {
      std::cout << x << ", " << y << std::endl;
      return false;
   };

   MapTools::lineCollide(0,0, 10,5, f);
}


Test::Test()
{
   // testBresenham();
   dumpAnimations();
}
