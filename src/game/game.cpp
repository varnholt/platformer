#include "game.h"

#include "animationpool.h"
#include "audio.h"
#include "projectilehitanimation.h"
#include "camerapane.h"
#include "debugdraw.h"
#include "displaymode.h"
#include "eventserializer.h"
#include "fadetransitioneffect.h"
#include "framework/joystick/gamecontroller.h"
#include "framework/tools/callbackmap.h"
#include "framework/tools/globalclock.h"
#include "framework/tools/timer.h"
#include "gameclock.h"
#include "gameconfiguration.h"
#include "gamecontactlistener.h"
#include "gamecontrollerdata.h"
#include "gamecontrollerintegration.h"
#include "gamejoystickmapping.h"
#include "gamestate.h"
#include "level.h"
#include "levels.h"
#include "messagebox.h"
#include "luainterface.h"
#include "player/player.h"
#include "player/playerinfo.h"
#include "physics/physicsconfiguration.h"
#include "savestate.h"
#include "screentransition.h"
#include "weapon.h"
#include "weather.h"

#include "menus/menuscreenmain.h"
#include "menus/menuscreenpause.h"
#include "menus/menuscreenvideo.h"

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <time.h>

// override WinUser.h
#ifdef MessageBox
#undef MessageBox
#endif

#ifdef __linux__
#define setUniform setParameter
#endif

// screen concept
//
// +-----------+------------------------------------------------------------------------------+-----------+        -
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// +-----------+---------------------------------------+--------------------------------------+-----------+  -     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|                V I E W                |                                      |...........|  |     |
// |...........||--------------- 640px ---------------||                                      |...........|
// |...........+---------------------------------------+--------------------------------------+...........| 720px 1080px
// |...........|#######################################|  -                                   |...........|
// |...........|---------------------------------------|  |                                   |...........|  |     |
// |...........|                                       |  |                                   |...........|  |     |
// |...........|                                       |                                      |...........|  |     |
// |...........|       O /                        +----| 360px                                |...........|  |     |
// |...........|     :()                          |####|                                      |...........|  |     |
// |...........|      / \                         |####|  |                                   |...........|  |     |
// |...........|-----'---"+     +-----------------+####|  |                                   |...........|  |     |
// |...........|##########|     |######################|  |                                   |...........|  |     |
// |...........|##########|:...:|######################|  -                                   |...........|  |     |
// +-----------+---------------------------------------+--------------------------------------+-----------+  -     |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// |...........|..............................................................................|...........|        |
// +-----------+------------------------------------------------------------------------------+-----------+        -
//
//             |------------------------------------ 1280px ----------------------------------|
//                                         R E N D E R T E X T U R E
//
// |------------------------------------------------ 1920px ----------------------------------------------|
//                                                 W I N D O W
//
// window width:           1280px
// window height:           720px
//
// view width:              640px
// view height:             360px
//
// ratio width:            1280px / 640px = 2
// ratio height:            720px / 360px = 2
//
// render texture width:    640 * 2 = 1280px
// render texture height:   360 * 2 = 720px

//----------------------------------------------------------------------------------------------------------------------
void Game::initializeWindow()
{
   GameConfiguration& gameConfig = GameConfiguration::getInstance();

   // since stencil buffers are used, it is required to enable them explicitly
   sf::ContextSettings context_settings;
   context_settings.stencilBits = 8;

   if (_window != nullptr)
   {
      _window->close();
      _window.reset();
   }

   // the window size is whatever the user sets up or whatever fullscreen resolution the user has
   _window = std::make_shared<sf::RenderWindow>(
      sf::VideoMode(
         static_cast<uint32_t>(gameConfig.mVideoModeWidth),
         static_cast<uint32_t>(gameConfig.mVideoModeHeight)
      ),
      GAME_NAME,
      gameConfig.mFullscreen ? sf::Style::Fullscreen : sf::Style::Default,
      context_settings
    );

   _window->setVerticalSyncEnabled(gameConfig.mVSync);
   _window->setFramerateLimit(60);
   _window->setKeyRepeatEnabled(false);
   _window->setMouseCursorVisible(!gameConfig.mFullscreen);

   // reset render textures if needed
   if (_window_render_texture != nullptr)
   {
      _window_render_texture.reset();
   }

   // this the render texture size derived from the window dimensions. as opposed to the window
   // dimensions this one takes the view dimensions into regard and preserves an integer multiplier
   const auto ratio_width = gameConfig.mVideoModeWidth / gameConfig.mViewWidth;
   const auto ratio_height = gameConfig.mVideoModeHeight / gameConfig.mViewHeight;

   const auto size_ratio = std::min(ratio_width, ratio_height);

   int32_t texture_width = size_ratio * gameConfig.mViewWidth;
   int32_t texture_height = size_ratio * gameConfig.mViewHeight;

   std::cout
      << "[x] video mode: "
      << gameConfig.mVideoModeWidth << " x " << gameConfig.mVideoModeHeight
      << ", view size: "
      << gameConfig.mViewWidth << " x " << gameConfig.mViewHeight
      << ", ratio: "
      << size_ratio
      << std::endl;

   _render_texture_offset.x = static_cast<uint32_t>((gameConfig.mVideoModeWidth - texture_width) / 2);
   _render_texture_offset.y = static_cast<uint32_t>((gameConfig.mVideoModeHeight - texture_height) / 2);

   _window_render_texture = std::make_shared<sf::RenderTexture>();
   _window_render_texture->create(
      static_cast<uint32_t>(texture_width),
      static_cast<uint32_t>(texture_height)
   );

   std::cout << "[x] created window render texture: " << texture_width << " x " << texture_height << std::endl;

   if (!_level)
   {
      std::cerr << "[!] level not initialized" << std::endl;
   }
   else
   {
      _level->initializeTextures();
   }

   EventSerializer::getInstance().setCallback([this](const sf::Event& event){processEvent(event);});
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initializeController()
{
   if (GameControllerIntegration::initializeAll() > 0)
   {
      auto gji = GameControllerIntegration::getInstance(0);

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_Y,
         [this](){
            openInventory();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_A,
         [this](){
            checkCloseInventory();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_B,
         [this](){
            checkCloseInventory();
         }
      );

      gji->getController()->addButtonPressedCallback(
         SDL_CONTROLLER_BUTTON_START,
         [this](){
            showPauseMenu();
         }
      );
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::showMainMenu()
{
   Menu::getInstance()->show(Menu::MenuType::Main);
   GameState::getInstance().enqueuePause();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::showPauseMenu()
{
   // while the game is loading, don't bother to open the pause screen
   if (!_level_loading_finished)
   {
      return;
   }

   if (Menu::getInstance()->getCurrentType() == Menu::MenuType::None)
   {
      Menu::getInstance()->show(Menu::MenuType::Pause);
      GameState::getInstance().enqueuePause();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::loadLevel()
{
   _level_loading_finished = false;
   _level_loading_finished_previous = false;

   _level_loading_thread = std::async(
      std::launch::async, [this](){

         // pick a level
         auto levels = Levels::getInstance();
         levels.deserializeFromFile();
         auto level_item = levels.mLevels.at(SaveState::getCurrent().mLevelIndex);

         _player->resetWorld();
         _level.reset();

         // load it
         _level = std::make_shared<Level>();
         _level->setDescriptionFilename(level_item.mLevelName);
         _level->initialize();
         _level->initializeTextures();

         // put the player in there
         _player->setWorld(_level->getWorld());
         _player->initializeLevel();

         // jump back to stored position
         if (_stored_position_valid)
         {
            _player->setBodyViaPixelPosition(_stored_position.x, _stored_position.y);
            _stored_position_valid = false;
         }

         _player->updatePlayerPixelRect();

         std::cout << "[x] level loading finished" << std::endl;

         _level_loading_finished = true;

         GameClock::getInstance().reset();
      }
   );
}


//----------------------------------------------------------------------------------------------------------------------
void Game::nextLevel()
{
   SaveState::getCurrent().mLevelIndex++;

   auto levels = Levels::getInstance();
   if (SaveState::getCurrent().mLevelIndex == levels.mLevels.size())
   {
       SaveState::getCurrent().mLevelIndex = 0;
   }

   loadLevel();
}


//----------------------------------------------------------------------------------------------------------------------
Game::~Game()
{
   // _event_serializer.serialize();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::initialize()
{
   initializeController();

   _player = std::make_shared<Player>();
   _player->initialize();

   // loadLevel();

   _info_layer = std::make_unique<InfoLayer>();
   _inventory_layer = std::make_unique<InventoryLayer>();
   _controller_overlay = std::make_unique<ControllerOverlay>();
   _test_scene = std::make_unique<ForestScene>();

   CallbackMap::getInstance().addCallback(CallbackMap::CallbackType::EndGame, [this](){_draw_states._draw_test_scene = true;});

   Audio::getInstance();

   // initially the game should be in main menu and paused
   std::dynamic_pointer_cast<MenuScreenMain>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Main))->setExitCallback(
      [this](){_window->close();}
   );

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setFullscreenCallback(
      [this](){toggleFullScreen();}
   );

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setResolutionCallback(
      [this](int32_t w, int32_t h){changeResolution(w, h);}
   );

   std::dynamic_pointer_cast<MenuScreenVideo>(Menu::getInstance()->getMenuScreen(Menu::MenuType::Video))->setVSyncCallback(
      [this](){
      initializeWindow();
      _level->createViews();
      }
   );

   initializeWindow();

   showMainMenu();

   Timer::add(std::chrono::milliseconds(1000), [this](){updateWindowTitle();}, Timer::Type::Repeated);

   GameState::getInstance().addCallback(
      [this](ExecutionMode current, ExecutionMode previous){
         if (current == ExecutionMode::Paused && previous == ExecutionMode::Running)
         {
            _player->getControls().setKeysPressed(0);
            CameraPane::getInstance().updateLookState(Look::LookActive, false);
         }
      }
   );

   // it is quite likely that after going into paused state different keys are
   // pressed compared to before. actually it's always going to happen. that results
   // in unpredictable player behavior, such as running into a 'random' direction.
   // this is why, after going into pause and back, the keyboard needs to be synced
   GameState::getInstance().addCallback(
      [this](ExecutionMode current, ExecutionMode previous){
         if (current == ExecutionMode::Running && previous == ExecutionMode::Paused)
         {
            _player->getControls().forceSync();
         }
      }
   );
}


// frambuffers
// - the window render texture
//    - the level render texture
//       - the level background render texture
//    - info layer
//    - menus
//    - inventory
//    - message boxes


//----------------------------------------------------------------------------------------------------------------------
void Game::draw()
{
   _fps++;

   _window_render_texture->clear();
   _window->clear(sf::Color::Black);
   _window->pushGLStates();

   _window_render_texture->clear();

   const auto mapEnabled = DisplayMode::getInstance().isSet(Display::DisplayMap);

   if (_level_loading_finished)
   {
      _level->draw(_window_render_texture, _screenshot);
   }

   _screenshot = false;

   // refactoring idea:
   // move this into the level class
   if (_draw_states._draw_weather)
   {
      Weather::getInstance().draw(*_window_render_texture.get());
   }

   // draw screen transitions here
   if (ScreenTransitionHandler::getInstance()._transition)
   {
      ScreenTransitionHandler::getInstance()._transition->draw(_window_render_texture);
   }

   if (!mapEnabled)
   {
      _info_layer->setLoading(!_level_loading_finished);
      _info_layer->draw(*_window_render_texture.get());
   }

   if (_draw_states._draw_debug_info)
   {
      _info_layer->drawDebugInfo(*_window_render_texture.get());
   }

   if (_draw_states._draw_console)
   {
      _info_layer->drawConsole(*_window_render_texture.get());
   }

   if (_draw_states._draw_camera_system)
   {
      DebugDraw::debugCameraSystem(*_window_render_texture.get());
   }

   if (_draw_states._draw_controller_overlay)
   {
      _controller_overlay->draw(*_window_render_texture.get());
   }

   if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
   {
      _inventory_layer->draw(*_window_render_texture.get());
   }

   if (_draw_states._draw_test_scene)
   {
      _test_scene->draw(*_window_render_texture.get());
   }

   Menu::getInstance()->draw(*_window_render_texture.get(), {sf::BlendAlpha});
   MessageBox::draw(*_window_render_texture.get());

   _window_render_texture->display();
   auto windowTextureSprite = sf::Sprite(_window_render_texture->getTexture());

   if (GameConfiguration::getInstance().mFullscreen)
   {
      // scale window texture up to available window size
      const auto scaleX = _window->getSize().x / static_cast<float>(_window_render_texture->getSize().x);
      const auto scaleY = _window->getSize().y / static_cast<float>(_window_render_texture->getSize().y);
      const auto scaleMin = std::min(static_cast<int32_t>(scaleX), static_cast<int32_t>(scaleY));
      const auto dx = (scaleX - scaleMin) * 0.5f;
      const auto dy = (scaleY - scaleMin) * 0.5f;
      windowTextureSprite.setPosition(_window_render_texture->getSize().x * dx, _window_render_texture->getSize().y * dy);
      windowTextureSprite.scale(static_cast<float>(scaleMin), static_cast<float>(scaleMin));
   }
   else
   {
      windowTextureSprite.setPosition(
         static_cast<float>(_render_texture_offset.x),
         static_cast<float>(_render_texture_offset.y)
      );
   }

   _window->draw(windowTextureSprite);

   _window->popGLStates();

   _window->display();

   if (_recording)
   {
      const auto image = windowTextureSprite.getTexture()->copyToImage();

      std::thread record([this, image](){
            std::ostringstream num;
            num << std::setfill('0') << std::setw(5) << _recording_counter++;
            image.saveToFile(num.str() + ".bmp");
         }
      );

      record.detach();
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameController()
{
   auto gji = GameControllerIntegration::getInstance(0);
   if (gji != nullptr)
   {
      gji->getController()->update();
      _player->getControls().setJoystickInfo(gji->getController()->getInfo());
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForGame()
{
  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     auto info = gji->getController()->getInfo();
     _player->getControls().setJoystickInfo(info);
     GameControllerData::getInstance().setJoystickInfo(info);
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateGameControllerForInventory()
{
  auto gji = GameControllerIntegration::getInstance(0);
  if (gji != nullptr)
  {
     _inventory_layer->setJoystickInfo(gji->getController()->getInfo());
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::updateWindowTitle()
{
   std::ostringstream sStream;
   sStream << GAME_NAME << " - " << _fps << "fps";
   _window->setTitle(sStream.str());
   _fps = 0;
}


//----------------------------------------------------------------------------------------------------------------------
void Game::resetAfterDeath(const sf::Time& dt)
{
   // not 100% if the screen transitions should actually drive the
   // level loading and game workflow. it should rather be the other
   // way round. on the other hand this approach allows very simple
   // timing and the fading is very unlikely to fail anyway.

   if (_player->isDead())
   {
      _death_wait_time_ms += dt.asMilliseconds();

      if (_death_wait_time_ms > 1000)
      {
         if (!ScreenTransitionHandler::getInstance()._transition)
         {
            // fade out/in
            auto screen_transition = std::make_unique<ScreenTransition>();
            sf::Color fade_color{60, 0, 0};
            auto fade_out = std::make_shared<FadeTransitionEffect>(fade_color);
            auto fade_in = std::make_shared<FadeTransitionEffect>(fade_color);
            fade_out->_direction = FadeTransitionEffect::Direction::FadeOut;
            fade_out->_speed = 1.0f;
            fade_in->_direction = FadeTransitionEffect::Direction::FadeIn;
            fade_in->_value = 1.0f;
            fade_in->_speed = 2.0f;
            screen_transition->_effect_1 = fade_out;
            screen_transition->_effect_2 = fade_in;
            screen_transition->_delay_between_effects_ms = std::chrono::milliseconds{500};
            screen_transition->_autostart_effect_2 = false;
            screen_transition->startEffect1();

            // do the actual level reset once the fade out has happened
            screen_transition->_callbacks_effect_1_ended.push_back(
               [this](){
                  SaveState::deserializeFromFile();
                  _player->reset();
                  loadLevel();

                  // update the camera system to point to the player position immediately
                  CameraSystem::getCameraSystem().syncNow();
               }
            );

            screen_transition->_callbacks_effect_2_ended.push_back(
               [](){
                  ScreenTransitionHandler::getInstance()._transition.release();
               }
            );

            ScreenTransitionHandler::getInstance()._transition = std::move(screen_transition);
         }
      }
   }

   if (_level_loading_finished && !_level_loading_finished_previous)
   {
      _level_loading_finished_previous = true;

      if (ScreenTransitionHandler::getInstance()._transition)
      {
         ScreenTransitionHandler::getInstance()._transition->startEffect2();
      }
   }
}


void Game::updateGameState(const sf::Time& dt)
{
   // check if just died
   auto deathReason = _player->checkDead();
   if (!_player->isDead() && deathReason != DeathReason::None)
   {
      _death_wait_time_ms = 0;
      _level->resetDeathShader();

      switch (deathReason)
      {
         case DeathReason::TouchesDeadly:
         {
            std::cout << "[i] dead: touched something deadly" << std::endl;
            break;
         }
         case DeathReason::TooFast:
         {
            std::cout << "[i] dead: too fast" << std::endl;
            break;
         }
         case DeathReason::OutOfHealth:
         {
            std::cout << "[i] dead: out of health" << std::endl;
            break;
         }
         case DeathReason::Smashed:
         {
            std::cout << "[i] dead: player got smashed" << std::endl;
            break;
         }
         case DeathReason::None:
         {
            break;
         }
      }

      _player->die();
   }

   // fade out when the player dies
   // when the level is faded out, then start reloading
   resetAfterDeath(dt);
}


//----------------------------------------------------------------------------------------------------------------------
void Game::update()
{
   const auto dt = _delta_clock.getElapsedTime();
   _delta_clock.restart();

   Audio::getInstance()->updateMusic();

   // update screen transitions here
   if (ScreenTransitionHandler::getInstance()._transition)
   {
      ScreenTransitionHandler::getInstance()._transition->update(dt);
   }

   // reload the level when the save state has been invalidated
   if (SaveState::getCurrent().mLoadLevelRequested)
   {
      SaveState::getCurrent().mLoadLevelRequested = false;
      loadLevel();
   }

   if (GameState::getInstance().getMode() == ExecutionMode::Paused)
   {
      updateGameController();
      updateGameControllerForInventory();
      _inventory_layer->update(dt);

      // this is not beautiful. simplify!
      if (DisplayMode::getInstance().isSet(Display::DisplayMap))
      {
         CameraPane::getInstance().update();
      }
   }
   else if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      Timer::update();

      if (_level_loading_finished)
      {
         AnimationPool::getInstance().updateAnimations(dt);
         Projectile::update(dt);
         updateGameController();
         updateGameControllerForGame();
         _level->update(dt);
         _player->update(dt);

         if (_draw_states._draw_test_scene)
         {
            _test_scene->update(dt);
         }

         if (_draw_states._draw_weather)
         {
            Weather::getInstance().update(dt);
         }

         // this might trigger level-reloading, so this ought to be the last drawing call in the loop
         updateGameState(dt);
      }
   }

   GameState::getInstance().sync();
   DisplayMode::getInstance().sync();
}


//----------------------------------------------------------------------------------------------------------------------
int Game::loop()
{
   while (_window->isOpen())
   {
      processEvents();
      update();
      draw();
   }

   return 0;
}


//----------------------------------------------------------------------------------------------------------------------
void Game::reset()
{
   _player->reset();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::checkCloseInventory()
{
  if (DisplayMode::getInstance().isSet(Display::DisplayInventory))
  {
     GameState::getInstance().enqueueResume();
     DisplayMode::getInstance().enqueueUnset(Display::DisplayInventory);
  }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::openInventory()
{
   if (GameState::getInstance().getMode() == ExecutionMode::Running)
   {
      GameState::getInstance().enqueuePause();
      DisplayMode::getInstance().enqueueSet(Display::DisplayInventory);
      _inventory_layer->setActive(true);
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::toggleFullScreen()
{
    GameConfiguration::getInstance().mFullscreen = !GameConfiguration::getInstance().mFullscreen;
    initializeWindow();
    _level->createViews();
}


//----------------------------------------------------------------------------------------------------------------------
void Game::changeResolution(int32_t w, int32_t h)
{
    GameConfiguration::getInstance().mVideoModeWidth = w;
    GameConfiguration::getInstance().mVideoModeHeight = h;
    GameConfiguration::getInstance().serializeToFile();

    initializeWindow();

    if (_level)
    {
       _level->createViews();
    }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::takeScreenshot()
{
   _screenshot = true;
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processEvent(const sf::Event& event)
{
   if (event.type == sf::Event::Closed)
   {
      _window->close();
   }

   else if (event.type == sf::Event::KeyPressed)
   {
      if (MessageBox::keyboardKeyPressed(event.key.code))
      {
         // nom nom nom
         return;
      }

      // todo: process keyboard events in the console class, just like done in the message box
      if (!Console::getInstance().isActive())
      {
         if (Menu::getInstance()->isVisible())
         {
            Menu::getInstance()->keyboardKeyPressed(event.key.code);
            return;
         }
         else
         {
            _player->getControls().keyboardKeyPressed(event.key.code);
         }
      }

      processKeyPressedEvents(event);
   }

   else if (event.type == sf::Event::KeyReleased)
   {
      if (Menu::getInstance()->isVisible())
      {
         Menu::getInstance()->keyboardKeyReleased(event.key.code);
         return;
      }
      else
      {
         _player->getControls().keyboardKeyReleased(event.key.code);
      }

      processKeyReleasedEvents(event);
   }

   else if (event.type == sf::Event::TextEntered)
   {
      if (Console::getInstance().isActive())
      {
         auto unicode = event.text.unicode;

         if (unicode > 0x1F && unicode < 0x80)
         {
            Console::getInstance().append(static_cast<char>(unicode));
         }
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processKeyPressedEvents(const sf::Event& event)
{
   if (Console::getInstance().isActive())
   {
      // these should be moved to the console itself
      if (event.key.code == sf::Keyboard::Return)
      {
         Console::getInstance().execute();
      }
      if (event.key.code == sf::Keyboard::F12)
      {
         _draw_states._draw_console = !_draw_states._draw_console;
         Console::getInstance().setActive(_draw_states._draw_console);
      }
      else if (event.key.code == sf::Keyboard::Backspace)
      {
         Console::getInstance().chop();
      }
      else if (event.key.code == sf::Keyboard::Up)
      {
         Console::getInstance().previousCommand();
      }
      else if (event.key.code == sf::Keyboard::Down)
      {
         Console::getInstance().nextCommand();
      }

      return;
   }

   switch (event.key.code)
   {
      case sf::Keyboard::Num0:
      {
         Audio::getInstance()->playSample("powerup.wav");

         if (SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills & ExtraSkill::SkillWallClimb)
         {
            SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills &= ~ ExtraSkill::SkillWallClimb;
         }
         else
         {
            SaveState::getPlayerInfo().mExtraTable.mSkills.mSkills |= ExtraSkill::SkillWallClimb;
         }
         break;
      }
      case sf::Keyboard::F1:
      {
         DisplayMode::getInstance().enqueueToggle(Display::DisplayDebug);
         break;
      }
      case sf::Keyboard::F2:
      {
         _draw_states._draw_controller_overlay = !_draw_states._draw_controller_overlay;
         break;
      }
      case sf::Keyboard::F3:
      {
         _draw_states._draw_camera_system = !_draw_states._draw_camera_system;
         break;
      }
      case sf::Keyboard::F4:
      {
         _draw_states._draw_debug_info = !_draw_states._draw_debug_info;
         break;
      }
      case sf::Keyboard::F5:
      {
         _draw_states._draw_weather = !_draw_states._draw_weather;
         break;
      }
      case sf::Keyboard::F6:
      {
         _draw_states._draw_test_scene = !_draw_states._draw_test_scene;
         break;
      }
      case sf::Keyboard::F7:
      {
         Player::getCurrent()->getPlayerAnimation().toggleVersion();
         break;
      }
      case sf::Keyboard::F12:
      {
         _draw_states._draw_console = !_draw_states._draw_console;
         Console::getInstance().setActive(_draw_states._draw_console);
         break;
      }
      case sf::Keyboard::F:
      {
         toggleFullScreen();
         break;
      }
      case sf::Keyboard::I:
      {
         openInventory();
         break;
      }
      case sf::Keyboard::L:
      {
         if (_level_loading_finished)
         {
            _stored_position_valid = true;
            _stored_position = _player->getPixelPositionf();
            loadLevel();
         }
         break;
      }
      case sf::Keyboard::M:
      {
         _recording = !_recording;
         break;
      }
      case sf::Keyboard::N:
      {
         nextLevel();
         break;
      }
      case sf::Keyboard::P:
      case sf::Keyboard::Escape:
      {
         const auto mapEnabled = DisplayMode::getInstance().isSet(Display::DisplayMap);
         if (!mapEnabled)
         {
            showPauseMenu();
         }
         else
         {
            // do the same as if the player had pressed tab again
            GameState::getInstance().enqueueTogglePauseResume();
            DisplayMode::getInstance().enqueueToggle(Display::DisplayMap);
         }
         break;
      }
      case sf::Keyboard::R:
      {
         reset();
         break;
      }
      case sf::Keyboard::S:
      {
         takeScreenshot();
         break;
      }
      case sf::Keyboard::V:
      {
         _player->setVisible(!_player->getVisible());
         break;
      }
      case sf::Keyboard::LShift:
      {
         CameraPane::getInstance().updateLookState(Look::LookActive, true);
         break;
      }
      case sf::Keyboard::Left:
      {
         _inventory_layer->left();
         CameraPane::getInstance().updateLookState(Look::LookLeft, true);
         break;
      }
      case sf::Keyboard::Right:
      {
         _inventory_layer->right();
         CameraPane::getInstance().updateLookState(Look::LookRight, true);
         break;
      }
      case sf::Keyboard::Return:
      {
         checkCloseInventory();
         break;
      }
      case sf::Keyboard::Up:
      {
         CameraPane::getInstance().updateLookState(Look::LookUp, true);
         break;
      }
      case sf::Keyboard::Down:
      {
         CameraPane::getInstance().updateLookState(Look::LookDown, true);
         break;
      }
      case sf::Keyboard::Tab:
      {
         GameState::getInstance().enqueueTogglePauseResume();
         DisplayMode::getInstance().enqueueToggle(Display::DisplayMap);
         break;
      }
      case sf::Keyboard::PageUp:
      {
         Level::getCurrentLevel()->getLightSystem()->increaseAmbient(0.1f);
         break;
      }
      case sf::Keyboard::PageDown:
      {
         Level::getCurrentLevel()->getLightSystem()->decreaseAmbient(0.1f);
         break;
      }
      default:
      {
         break;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processKeyReleasedEvents(const sf::Event& event)
{
   switch (event.key.code)
   {
      case sf::Keyboard::LShift:
      {
         CameraPane::getInstance().updateLookState(Look::LookActive, false);
         break;
      }
      case sf::Keyboard::Left:
      {
         CameraPane::getInstance().updateLookState(Look::LookLeft, false);
         break;
      }
      case sf::Keyboard::Right:
      {
         CameraPane::getInstance().updateLookState(Look::LookRight, false);
         break;
      }
      case sf::Keyboard::Up:
      {
         CameraPane::getInstance().updateLookState(Look::LookUp, false);
         break;
      }
      case sf::Keyboard::Down:
      {
         CameraPane::getInstance().updateLookState(Look::LookDown, false);
         break;
      }

      default:
      {
         break;
      }
   }
}


//----------------------------------------------------------------------------------------------------------------------
void Game::processEvents()
{
   sf::Event event;
   while (_window->pollEvent(event))
   {
      processEvent(event);

      EventSerializer::getInstance().add(event);
   }
}


