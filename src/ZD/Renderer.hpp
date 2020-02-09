#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "Image.hpp"
#include "Model.hpp"
#include "Texture.hpp"
#include "Window.hpp"

class Painter;

class Renderer
{
  public:
    virtual size_t add_window(const WindowParameters &params) = 0;

    virtual void set_window_current(size_t index) { 
      current_window_index = index; 
    }

    virtual size_t get_current_window() const { 
      return current_window_index; 
    }

    virtual void remove_window(size_t index) {
      windows.erase(windows.begin() + index);
    }

    bool is_window_open() {
      return window()->is_open();
    }

    virtual void clear() = 0;
    virtual void update() = 0;
    virtual void render() = 0;
  protected:
    std::vector<std::shared_ptr<Window>> windows;
    size_t current_window_index{0};

    inline std::shared_ptr<Window> window() {
      return windows.at(current_window_index);
    }

    virtual void initialize_main_screen_image() {
      int w = window()->get_width();
      int h = window()->get_height();
      auto format = window()->get_format();
      main_screen_image = Image::create(Size(w, h), format);
    }

    virtual std::shared_ptr<Image> get_main_screen_image() {
      if (!main_screen_image) 
      {
        initialize_main_screen_image();
      }
      return main_screen_image;
    }

    std::shared_ptr<Image> main_screen_image;

    friend class Painter;
  private:

};