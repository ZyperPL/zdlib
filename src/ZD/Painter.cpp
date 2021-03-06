#include "Painter.hpp"
#include "Color.hpp"
#include "Image.hpp"

#pragma GCC optimize("O3")

namespace ZD
{
  long move_ptr_to_xy(const long x, const long y, const long w)
  {
    return x + y * w;
  }

  bool in_bounds(const int x, const int y, const int w, const int h)
  {
    if (x >= w)
      return false;
    if (y >= h)
      return false;
    if (x < 0)
      return false;
    if (y < 0)
      return false;

    return true;
  }

  Painter::Painter(std::shared_ptr<Image> image)
  : target { image }
  {
  }

  void Painter::set_pixel(const int x, const int y, const Color &color)
  {
    if (x < 0)
      return;
    if (y < 0)
      return;
    if (x >= target->width())
      return;
    if (y >= target->height())
      return;

    target->set_pixel(x, y, color);
    target->changes++;
  }

  void Painter::draw_image(const int x, const int y, const Image &image)
  {
    const auto t_width = target->width();
    const auto t_height = target->height();

    auto src = image.get_data();
    auto dest = target->data.get();

    dest += move_ptr_to_xy(x, y, t_width);

    const auto image_width = image.width();
    const auto image_height = image.height();

    int ys = 0;
    if (y < 0)
      ys = -y;

    for (ssize_t iy = ys; iy < image_height; ++iy)
    {
      if (y + iy < t_height)
      {
        for (ssize_t ix = 0; ix < image_width; ++ix)
        {
          const ssize_t i = ix + iy * image_width;
          if ((src[i] & 0xff) == 0)
            continue;

          if (x + ix >= t_width || x + ix < 0)
            continue;

          size_t target_idx = ix + (iy * t_width);

          dest[target_idx] = src[i];
        }
      }
    }
    target->changes++;
  }

  void Painter::draw_image(
    const int x, const int y, const Image &image, const int width,
    const int height, AspectRatioOptions aspect_ratio_options)
  {
    const double scale_x = (double)(width) / (double)(image.width());
    const double scale_y = (double)(height) / (double)(image.height());

    if (aspect_ratio_options == PreserveAspectRatio)
    {
      const double scale = scale_y < scale_x ? scale_y : scale_x;
      return draw_image(x, y, image, scale, scale);
    }

    return draw_image(x, y, image, scale_x, scale_y);
  }

  void Painter::draw_image(
    const int x, const int y, const Image &image, double scale_x,
    double scale_y)
  {
    if (scale_x == 1.0 && scale_y == 1.0)
    {
      return draw_image(x, y, image);
    }

    auto src = image.get_data();
    auto dest = target->data.get();

    dest += move_ptr_to_xy(x, y, target->width());

    const double abs_scale_x = std::abs(scale_x);
    const double abs_scale_y = std::abs(scale_y);
    const int new_width = image.width() * abs_scale_x;
    const int new_height = image.height() * abs_scale_y;

    const auto t_width = target->width();
    const auto t_height = target->height();

    const auto image_width = image.width();
    const auto image_height = image.height();
    const auto image_area = image.size.area();

    for (ssize_t iy = 0; iy < new_height; ++iy)
    {
      for (ssize_t ix = 0; ix < new_width; ++ix)
      {
        if (x + ix >= t_width || x + ix < 0)
          continue;
        if (y + iy >= t_height || y + iy < 0)
          continue;

        const ssize_t target_idx = ix + (iy * t_width);

        ssize_t scaled_image_x = ix / abs_scale_x;
        ssize_t scaled_image_y = iy / abs_scale_y;

        if (scale_x < 0)
          scaled_image_x = image_width - scaled_image_x;
        if (scale_y < 0)
          scaled_image_y = image_height - scaled_image_y;

        const ssize_t image_idx = scaled_image_x + scaled_image_y * image_width;

        if (image_idx >= image_area)
          continue;

        if ((src[image_idx] & 0xff) == 0)
          continue;

        dest[target_idx] = src[image_idx];
      }
    }
    target->changes++;
  }

  void Painter::draw_line(
    const int x1, const int y1, const int x2, const int y2, const Color &color)
  {
    const auto t_width = target->width();
    const auto t_height = target->height();

    //TODO: handle straight lines

    bool y_longer = false;
    int increment_val = 1;

    int64_t short_len = y2 - y1;
    int64_t long_len = x2 - x1;

    if (std::abs(short_len) > std::abs(long_len))
    {
      std::swap(short_len, long_len);
      y_longer = true;
    }

    if (long_len < 0)
      increment_val = -1;

    double div_diff = long_len;
    if (short_len != 0)
      div_diff = (double)long_len / (double)short_len;
    if (y_longer)
    {
      for (int i = 0; i != long_len; i += increment_val)
      {
        int x = x1 + (int)((double)i / div_diff);
        int y = y1 + i;
        if (x < 0)
          continue;
        if (y < 0)
          continue;
        if (x >= t_width)
          continue;
        if (y >= t_height)
          continue;

        target->set_pixel(x, y, color);
      }
    }
    else
    {
      for (int i = 0; i != long_len; i += increment_val)
      {
        int x = x1 + i;
        int y = y1 + (int)((double)i / div_diff);

        if (x < 0)
          continue;
        if (y < 0)
          continue;
        if (x >= t_width)
          continue;
        if (y >= t_height)
          continue;

        target->set_pixel(x, y, color);
      }
    }
    target->changes++;
  }

  void Painter::clear_rectangle(int x1, int y1, int x2, int y2)
  {
    auto dest = target->data.get();

    if (x2 < x1)
      std::swap(x1, x2);
    if (y2 < y1)
      std::swap(y2, y1);

    if (x1 < 0)
      x1 = 0;
    if (y1 < 0)
      y1 = 0;
    if (x2 < 0)
      return;
    if (y2 < 0)
      return;

    if (x2 > target->width())
      x2 = target->width();
    if (y2 > target->height())
      y2 = target->height();

    if (x1 > target->width())
      return;
    if (y1 > target->height())
      return;

    for (int y = y1; y < y2; ++y)
    {
      for (int x = x1; x < x2; ++x)
      {
        dest[x + y * target->width()] = 0;
      }
    }
    target->changes++;
  }

  void Painter::draw_rectangle(
    const int x1, const int y1, const int x2, const int y2, const Color &color)
  {
    const int t_width = target->width();
    const int t_height = target->height();

    if (x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0)
      return;
    if (x1 >= t_width && y1 >= t_height && x2 >= t_width && y2 >= t_height)
      return;

    if (x1 == x2 || y1 == y2)
    {
      return draw_line(x1, y1, x2, y2, color);
    }

    int xx = x1;
    int w = x2 - x1;
    if (w < 0)
    {
      xx = x2;
      w = x1 - x2;
    }
    int yy = y1;
    int h = y2 - y1;
    if (h < 0)
    {
      yy = y2;
      h = y1 - y2;
    }

    if (xx < 0)
    {
      w -= -xx + 1;
      xx = 0;
    }
    if (yy < 0)
    {
      h -= -yy + 1;
      yy = 0;
    }
    if (xx >= t_width)
    {
      w -= t_width - xx + 1;
      xx = t_width;
    }
    if (yy >= t_height)
    {
      h -= t_height - yy + 1;
      yy = t_height;
    }

    for (int i = 0; i <= w; i++)
    {
      if (xx + i >= t_width)
      {
        continue;
      }

      if (y1 >= 0 && y1 < t_height)
        target->set_pixel(xx + i, y1, color);

      if (y2 >= 0 && y2 < t_height)
        target->set_pixel(xx + i, y2, color);
    }

    for (int i = 0; i <= h; i++)
    {
      if (yy + i >= t_height)
      {
        continue;
      }

      if (x1 >= 0 && x1 < t_width)
        target->set_pixel(x1, yy + i, color);

      if (x2 >= 0 && x2 < t_width)
        target->set_pixel(x2, yy + i, color);
    }
    target->changes++;
  }

  void Painter::draw_circle(
    const int x, const int y, const int radius, const Color &color)
  {
    if (x + radius < 0)
      return;
    if (y + radius < 0)
      return;
    if (x - radius >= target->width())
      return;
    if (y - radius >= target->height())
      return;

    const int t_width = target->width();
    const int t_height = target->height();

    auto in_target_bounds = [&t_width,
                             &t_height](const int t_x, const int t_y) -> bool {
      return in_bounds(t_x, t_y, t_width, t_height);
    };

    int xx = radius;
    int yy = 0;
    int err = 0;

    while (xx >= yy)
    {
      if (in_target_bounds(x + xx, y + yy))
        target->set_pixel(x + xx, y + yy, color);

      if (in_target_bounds(x + yy, y + xx))
        target->set_pixel(x + yy, y + xx, color);

      if (in_target_bounds(x - yy, y + xx))
        target->set_pixel(x - yy, y + xx, color);

      if (in_target_bounds(x - xx, y + yy))
        target->set_pixel(x - xx, y + yy, color);

      if (in_target_bounds(x - xx, y - yy))
        target->set_pixel(x - xx, y - yy, color);

      if (in_target_bounds(x - yy, y - xx))
        target->set_pixel(x - yy, y - xx, color);

      if (in_target_bounds(x + yy, y - xx))
        target->set_pixel(x + yy, y - xx, color);

      if (in_target_bounds(x + xx, y - yy))
        target->set_pixel(x + xx, y - yy, color);

      if (err <= 0)
      {
        yy += 1;
        err += 2 * yy + 1;
      }

      if (err > 0)
      {
        xx -= 1;
        err -= 2 * xx + 1;
      }
    }
    target->changes++;
  }

} // namespace ZD
