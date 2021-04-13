#include "rope.h"

#include "framework/math/sfmlmath.h"
#include "framework/tmxparser/tmxobject.h"
#include "framework/tmxparser/tmxpolyline.h"
#include "texturepool.h"

#include <array>
#include <iostream>

Rope::Rope(GameNode* parent)
 : GameNode(parent)
{
   _joint_def.collideConnected = false;

   setZ(16);

   // chain element setup
   _rope_element_shape.SetAsBox(0.0125f, 0.0125f);
   _rope_element_fixture_def.shape = &_rope_element_shape;
   _rope_element_fixture_def.density = 20.0f;
   _rope_element_fixture_def.friction = 0.2f;

   _texture = TexturePool::getInstance().get("data/level-demo/tilesheets/catacombs-level-diffuse.png");

   // rope 1
   // 971,  73 .. 973,  73
   // 971, 211 .. 973, 211
   _texture_rect_px.left = 971;
   _texture_rect_px.top = 73;
   _texture_rect_px.width = 3;
   _texture_rect_px.height = 138;

   // rope 2
   // 1019,  72 .. 1021,  72
   // 1019, 153 .. 1021, 153
   //
   // _texture_rect_px.left = 1019;
   // _texture_rect_px.top = 72;
   // _texture_rect_px.width = 3;
   // _texture_rect_px.height = 81;
}


void Rope::draw(sf::RenderTarget& color, sf::RenderTarget& /*normal*/)
{
   std::optional<b2Vec2> q1_prev;
   std::optional<b2Vec2> q4_prev;

   std::vector<sf::Vertex> quads;

   for (auto i = 0u; i < _chain_elements.size() - 1; i++)
   {
      const auto c1 = _chain_elements[i];
      const auto c2 = _chain_elements[i + 1];

      const auto c1_pos_m = c1->GetPosition();
      const auto c2_pos_m = c2->GetPosition();

      static constexpr auto thickness_m = 0.025f;

      const auto dist = (c2_pos_m - c1_pos_m);
      auto normal = b2Vec2(dist.y, -dist.x);
      normal.Normalize();

      const auto q1 = q1_prev.value_or(c1_pos_m - (thickness_m * normal));
      const auto q2 = c2_pos_m - (thickness_m * normal);
      const auto q3 = c2_pos_m + (thickness_m * normal);
      const auto q4 = q4_prev.value_or(c1_pos_m + (thickness_m * normal));

      q1_prev = q2;
      q4_prev = q3;

      auto u0 = static_cast<float>(i    ) / static_cast<float>(_segment_count);
      auto u1 = static_cast<float>(i + 1) / static_cast<float>(_segment_count);

      const auto v1 = sf::Vertex(sf::Vector2f(q1.x * PPM, q1.y * PPM), sf::Vector2f(_texture_rect_px.left,                          _texture_rect_px.top + u0 * _texture_rect_px.height));
      const auto v2 = sf::Vertex(sf::Vector2f(q2.x * PPM, q2.y * PPM), sf::Vector2f(_texture_rect_px.left,                          _texture_rect_px.top + u1 * _texture_rect_px.height));
      const auto v3 = sf::Vertex(sf::Vector2f(q3.x * PPM, q3.y * PPM), sf::Vector2f(_texture_rect_px.left + _texture_rect_px.width, _texture_rect_px.top + u1 * _texture_rect_px.height));
      const auto v4 = sf::Vertex(sf::Vector2f(q4.x * PPM, q4.y * PPM), sf::Vector2f(_texture_rect_px.left + _texture_rect_px.width, _texture_rect_px.top + u0 * _texture_rect_px.height));

      quads.push_back(v1);
      quads.push_back(v2);
      quads.push_back(v3);
      quads.push_back(v4);
   }

   // render out those quads
   sf::RenderStates states;
   states.texture = _texture.get();
   color.draw(quads.data(), quads.size(), sf::Quads, states);
}


void Rope::update(const sf::Time& dt)
{
   // slightly push the rope all the way while it's moving from the right to the left
   auto f = dt.asSeconds() * 0.01f;

   auto last_element = _chain_elements.back();
   if (last_element->GetLinearVelocity().x <= 0.0f)
   {
      last_element->ApplyLinearImpulse(b2Vec2{-f, f}, last_element->GetWorldCenter(), true);
   }
}


void Rope::setup(TmxObject* tmxObject, const std::shared_ptr<b2World>& world)
{
   // init segment length
   std::vector<sf::Vector2f> pixel_path = tmxObject->mPolyLine->mPolyLine;
   auto path_0_px = pixel_path.at(0);
   auto path_1_px = pixel_path.at(1);
   _segment_length_m = (SfmlMath::length(path_1_px - path_0_px) * MPP) / static_cast<float>(_segment_count);
   // std::cout << _segment_length_m << std::endl;

   // init start position
   setPixelPosition(
      sf::Vector2i{
         static_cast<int32_t>(tmxObject->mX),
         static_cast<int32_t>(tmxObject->mY)
      }
   );

   // pin the rope to the starting point (anchor)
   auto pos_m = b2Vec2{static_cast<float>(_position_px.x * MPP), static_cast<float>(_position_px.y * MPP)};
   _anchor_a_body = world->CreateBody(&_anchor_a_def);
   _anchor_a_shape.Set(b2Vec2(pos_m.x - 0.1f, pos_m.y), b2Vec2(pos_m.x + 0.1f, pos_m.y));
   auto anchor_fixture =_anchor_a_body->CreateFixture(&_anchor_a_shape, 0.0f);
   anchor_fixture->SetSensor(true);

   auto previous_body = _anchor_a_body;

   for (auto i = 0; i < _segment_count; ++i)
   {
      // create chain element
      b2BodyDef chain_body_def;
      chain_body_def.type = b2_dynamicBody;
      chain_body_def.position.Set(pos_m.x, pos_m.y + 0.01f + i * _segment_length_m);
      auto chain_body = world->CreateBody(&chain_body_def);
      auto chain_fixture = chain_body->CreateFixture(&_rope_element_fixture_def);
      chain_fixture->SetSensor(true);

      // attach chain element to the previous one
      b2Vec2 anchor(pos_m.x, pos_m.y + i * _segment_length_m);
      _joint_def.Initialize(previous_body, chain_body, anchor);
      world->CreateJoint(&_joint_def);

      // store chain elements
      previous_body = chain_body;
      _chain_elements.push_back(chain_body);
   }
}


sf::Vector2i Rope::getPixelPosition() const
{
   return _position_px;
}


void Rope::setPixelPosition(const sf::Vector2i& pixelPosition)
{
   _position_px = pixelPosition;
}


