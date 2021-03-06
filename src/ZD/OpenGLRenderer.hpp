#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <memory>

#include "Framebuffer.hpp"
#include "Renderer.hpp"
#include "Screen.hpp"
#include "Size.hpp"
#include "Texture.hpp"
#include "Window.hpp"

namespace ZD
{
  extern GLenum glCheckError_(const char *, int);
#define glCheckError() glCheckError_(__FILE__, __LINE__)

  class OGLRenderer : public Renderer
  {
  public:
    OGLRenderer();
    ~OGLRenderer();

    void set_window_current(size_t index);
    void remove_window(size_t index);
    std::shared_ptr<Window> add_window(const WindowParameters &params);

    void clear();
    void update();
    void render();

    void render_screens();

    void enable_blend(GLenum sfactor = GL_SRC_ALPHA, GLenum dfactor = GL_ONE_MINUS_SRC_ALPHA);
    void disable_blend();
    void enable_cull_face(GLenum mode = GL_FRONT, GLenum front_face = GL_CW);
    void disable_cull_face();
    void enable_depth_test(GLenum func = GL_LEQUAL);
    void disable_depth_test();

    inline void clear_background_color(const Color &c)
    {
      glClearColor(c.red_float(), c.green_float(), c.blue_float(), 1.0);
    }

    inline void set_events_poll_rate(double rate) { this->poll_rate = rate; }

    FramebufferObject generate_framebuffer(
      size_t width, size_t height, const TextureParameters parameters = TextureParameters {});

    void unbind_framebuffer()
    {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      clear();
      window()->set_current();
    }

    void bind_framebuffer(const FramebufferObject &fbo);

    // should be set before window creation
    void set_multisampling(int value)
    {
      if (value > 0)
        glEnable(GL_MULTISAMPLE);

      glfwWindowHint(GLFW_SAMPLES, value);
    }

  private:
    void generate_vertex_array_object();
    void initialize_gl();
    void uninitialize_gl();
    GLuint vao;
    double poll_rate { 0.0 };
    bool clear_depth { false };
    bool screens_rendered { false };
  };
} // namespace ZD
