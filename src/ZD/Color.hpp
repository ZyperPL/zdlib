#pragma once

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <random>

namespace ZD
{
  typedef unsigned char u8;

  struct PixelFormat
  {
    enum Type
    {
      Indexed,
      Gray,
      GrayAlpha,
      RGB,
      RGBA,
      BGR,
      BGRA,
      Invalid
    };
    static constexpr int get_components_num(Type format)
    {
      switch (format)
      {
        case Type::Indexed:
        case Type::Gray: return 1;

        case Type::GrayAlpha: return 2;

        case Type::RGB:
        case Type::BGR: return 3;

        case Type::RGBA:
        case Type::BGRA: return 4;

        case Type::Invalid: assert(false);
      }

      return 3;
    }
  };

  class Color
  {
  public:
    constexpr Color(uint32_t v)
    : color_value { v }
    {
    }

    constexpr Color(u8 red, u8 green, u8 blue) { set_value(red, green, blue); }

    constexpr Color(u8 red, u8 green, u8 blue, u8 alpha)
    {
      set_value(red, green, blue, alpha);
    }

    constexpr bool operator==(const Color &o)
    {
      return o.color_value == color_value;
    }
    constexpr bool operator!=(const Color &o)
    {
      return o.color_value != color_value;
    }

    static constexpr Color from_floats(
      float red, float green, float blue, float alpha = 1.0)
    {
      u8 r = red * 255;
      u8 g = green * 255;
      u8 b = blue * 255;
      u8 a = alpha * 255;

      return Color(r, g, b, a);
    }

    static Color from_random(int minimum, int maximum = 255)
    {
      static std::random_device rd;
      static std::uniform_int_distribution<int> dist(minimum, maximum);
      int r = dist(rd);
      int g = dist(rd);
      int b = dist(rd);
      return Color(r, g, b);
    }

    static constexpr Color from_value(uint32_t v) { return Color(v); }

    inline constexpr void set_value(u8 r, u8 g, u8 b, u8 a = 255)
    {
      // internally BGRA
      color_value =
        ((b << BLUE_BIT) | (g << GREEN_BIT) | (r << RED_BIT) |
         (a << ALPHA_BIT));
    }

    inline constexpr uint32_t value() const { return color_value; }

    inline constexpr u8 red() const { return (color_value >> RED_BIT) & 0xff; }
    inline constexpr u8 green() const
    {
      return (color_value >> GREEN_BIT) & 0xff;
    }
    inline constexpr u8 blue() const
    {
      return (color_value >> BLUE_BIT) & 0xff;
    }
    inline constexpr u8 alpha() const
    {
      return (color_value >> ALPHA_BIT) & 0xff;
    }

    constexpr float red_float() const { return ((float)red()) / 255.0; }
    constexpr float green_float() const { return ((float)green()) / 255.0; }
    constexpr float blue_float() const { return ((float)blue()) / 255.0; }
    constexpr float alpha_float() const { return ((float)alpha()) / 255.0; }

    inline void print() const
    {
      printf(
        "Color (%8x): %3d,%3d,%3d,%3d",
        value(),
        red(),
        green(),
        blue(),
        alpha());
    }

  private:
    uint32_t color_value { 0 };

    const int RED_BIT = 8;
    const int GREEN_BIT = 16;
    const int BLUE_BIT = 24;
    const int ALPHA_BIT = 0;
  };

} // namespace ZD
