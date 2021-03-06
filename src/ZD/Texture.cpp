#include "Texture.hpp"
#include "Shader.hpp"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "OpenGLRenderer.hpp"

namespace ZD
{
  static std::unordered_map<const Image *, std::shared_ptr<Texture>> loaded_textures;

  static std::optional<std::shared_ptr<Texture>> find_in_loaded(const Image *ptr)
  {
    for (auto &&ptr_tex_pair : loaded_textures)
    {
      if (ptr_tex_pair.first == ptr)
        return ptr_tex_pair.second;
    }
    return {};
  }

  std::shared_ptr<Texture> Texture::create(const TextureParameters params)
  {
    return std::shared_ptr<Texture>(new Texture { params });
  }

  std::shared_ptr<Texture> Texture::load(
    std::shared_ptr<Image> image, const TextureParameters params, ForceReload reload)
  {
    if (image && reload != ForceReload::Yes)
    {
      if (auto model = find_in_loaded(image.get()))
        return model.value();
    }

    std::shared_ptr<Texture> texture { new Texture { image, params } };
    loaded_textures.insert({ image.get(), texture });
    return texture;
  }

  std::shared_ptr<Texture> Texture::load(
    const std::string image_name, const TextureParameters params, ForceReload reload)
  {
    return Texture::load(Image::load(image_name), params, reload);
  }

  Texture::Texture(const TextureParameters params)
  : texture_wrap { params.wrap }
  , generate_mipmap { params.generate_mipmap }
  {
    this->generate(params);
  }

  Texture::Texture(const std::shared_ptr<Image> image, const TextureParameters params)
  : image { image }
  , texture_wrap { params.wrap }
  , generate_mipmap { params.generate_mipmap }
  {
    this->generate(params);
    this->set_buffer_data();
    width = image->width();
    height = image->height();
  }

  Texture::~Texture()
  {
    glDeleteTextures(1, &id);
    glDeleteBuffers(2, pbo);
  }

  void Texture::generate(const TextureParameters params)
  {
    glGenTextures(1, &this->id);
    glBindTexture(GL_TEXTURE_2D, this->id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, params.wrap_mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.mag_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.min_filter);

    static const bool IS_GL_4_5_SUPPORTED =
      glewGetExtension("ARB_get_texture_sub_image") && glewGetExtension("ARB_texture_barrier");

    if (
      glewGetExtension("GL_ARB_pixel_buffer_object") &&
      IS_GL_4_5_SUPPORTED /* to be sure implementation is proper and reliable */)
    {
      glGenBuffers(2, pbo);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
      printf("Generated Pixel Buffer Object id 1=%u id 2=%u\n", pbo[0], pbo[1]);
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }

    glCheckError();
  }

  void Texture::set_buffer_data()
  {
    if (!image)
      return;

    glBindTexture(GL_TEXTURE_2D, this->id);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      image->get_size().width(),
      image->get_size().height(),
      0,
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8,
      &image->get_data()[0]);

    if (pbo[0] > 0)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[0]);
      glBufferData(
        GL_PIXEL_UNPACK_BUFFER, image->get_size().area() * sizeof(uint32_t), image->get_data(), GL_STREAM_DRAW);
    }
    if (pbo[1] > 0)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[1]);
      glBufferData(
        GL_PIXEL_UNPACK_BUFFER, image->get_size().area() * sizeof(uint32_t), image->get_data(), GL_STREAM_DRAW);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    if (generate_mipmap)
    {
      glGenerateMipmap(GL_TEXTURE_2D);
    }
  }

  void Texture::set_image(std::shared_ptr<Image> new_image)
  {
    int current_width = -1;
    int current_height = -1;
    if (this->image)
    {
      current_width = this->image->width();
      current_height = this->image->height();
    }
    this->image = new_image;
    width = this->image->width();
    height = this->image->height();
    if (current_width != new_image->width() || current_height != new_image->height())
    {
      set_buffer_data();
    }
    update();
  }

  void Texture::update()
  {
    if (!image)
      return;

    auto *data_ptr = &image->get_data()[0];

    if (pbo[frame % 2] > 0)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[frame % 2]);
      data_ptr = 0;
    }

    glBindTexture(GL_TEXTURE_2D, this->id);
    glTexSubImage2D(
      GL_TEXTURE_2D,
      0,
      0,
      0,
      image->get_size().width(),
      image->get_size().height(),
      GL_BGRA,
      GL_UNSIGNED_INT_8_8_8_8,
      data_ptr);

    if (generate_mipmap)
    {
      glBindTexture(GL_TEXTURE_2D, this->id);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    if (pbo[(frame + 1) % 2] > 0)
    {
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo[(frame + 1) % 2]);
      auto *pbo_ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
      if (pbo_ptr)
      {
        memcpy(pbo_ptr, &image->get_data()[0], image->get_size().area() * sizeof(uint32_t));
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
      }
      glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
      frame++;
    }
  }

  void Texture::bind(const ShaderProgram &shader, GLuint sampler_id, std::string_view sampler_name)
  {
    glActiveTexture(GL_TEXTURE0 + sampler_id);
    glBindTexture(GL_TEXTURE_2D, this->id);

    if (auto sampler_uniform = shader.get_uniform(sampler_name.data()))
    {
      assert(sampler_uniform->type == GL_SAMPLER_2D);
      glUniform1i(sampler_uniform->location, sampler_id);
    }

    if (auto wrap_uniform = shader.get_uniform("texture_wrap"))
    {
      assert(wrap_uniform->type == GL_FLOAT_VEC2);
      glUniform2f(wrap_uniform->location, this->texture_wrap.x, this->texture_wrap.y);
    }

    glCheckError();

    if (frame % 2 == 1)
    {
      update();
    }
  }

  int Texture::get_width(int mip_level)
  {
    if (mip_level <= 0 && width > 0)
      return width;

    glGetTexLevelParameteriv(GL_TEXTURE_2D, mip_level, GL_TEXTURE_WIDTH, &width);
    return width;
  }

  int Texture::get_height(int mip_level)
  {
    if (mip_level <= 0 && height > 0)
      return height;

    glGetTexLevelParameteriv(GL_TEXTURE_2D, mip_level, GL_TEXTURE_HEIGHT, &height);
    return height;
  }

} // namespace ZD
