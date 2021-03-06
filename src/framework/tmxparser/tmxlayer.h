#pragma once

#include "tmxchunk.h"
#include "tmxelement.h"

struct TmxProperties;

struct TmxLayer : TmxElement
{
   TmxLayer();
   ~TmxLayer() override;

   void deserialize(tinyxml2::XMLElement*) override;

   uint32_t _width_px = 0;
   uint32_t _height_px = 0;
   float _opacity = 1.0f;
   bool _visible = true;
   TmxProperties* _properties = nullptr;
   int32_t _z = 0;

   int32_t _offset_x_px = 0;
   int32_t _offset_y_px = 0;

   int32_t* _data = nullptr;
};

