#include "Painter.hpp"
#include "Color.hpp"
#include "Image.hpp"

long move_ptr_to_xy(const int x, const int y, const int w)
{
  return x + y*w;
}

bool in_bounds(const int x, const int y, const int w, const int h)
{
  if (x >= w) return false;
  if (y >= h) return false;
  if (x < 0) return false;
  if (y < 0) return false;

  return true;
}

Painter::Painter(std::shared_ptr<Image> image)
  : target{image}
{
}

Painter::Painter(std::shared_ptr<uint32_t[]> data, const Size &size, PixelFormat::Type format)
{
  target = Image::create(size, format);
  target->data = data;
}

void Painter::set_pixel(const int x, const int y, const Color &color)
{
  if (x < 0) return;
  if (y < 0) return;
  if (x >= target->width()) return;
  if (y >= target->height()) return;

  target->set_pixel(x, y, color);
}

void Painter::draw_image(const int x, const int y, const Image &image)
{
  auto src = image.get_data();
  auto dest = target->data.get();

  dest += move_ptr_to_xy(x, y, target->width());
  
  #pragma omp parallel for
  for (int i = 0; i < image.size.area(); i++)
  {
    ssize_t image_x = i%image.width();
    ssize_t image_y = i/image.width();

    if (x + image_x >= target->width() || x + image_x < 0) continue;
    if (y + image_y >= target->height() || y + image_y < 0) continue;

    size_t target_idx = image_x+(image_y*target->width());

    dest[target_idx] = src[i];
  }
}

void Painter::draw_image(const int x, const int y, const Image &image, double scale_x, double scale_y)
{
  auto src = image.get_data();
  auto dest = target->data.get();

  dest += move_ptr_to_xy(x, y, target->width());
  
  const double abs_scale_x = std::abs(scale_x);
  const double abs_scale_y = std::abs(scale_y);
  const int new_width = image.width()*abs_scale_x;
  const int new_height = image.height()*abs_scale_y;
  const int new_area = new_width * new_height;;

  #pragma omp parallel for
  for (int i = 0; i < new_area; i++)
  {
    const ssize_t image_x = i%new_width;
    const ssize_t image_y = i/new_width;

    if (x + image_x >= target->width() || x + image_x < 0) continue;
    if (y + image_y >= target->height() || y + image_y < 0) continue;

    const ssize_t target_idx = image_x+(image_y*target->width());

    ssize_t scaled_image_x = image_x/abs_scale_x;
    ssize_t scaled_image_y = image_y/abs_scale_y;

    if (scale_x < 0) scaled_image_x = image.width()-scaled_image_x;
    if (scale_y < 0) scaled_image_y = image.height()-scaled_image_y;

    const ssize_t image_idx = scaled_image_x+scaled_image_y*image.width();
    
    if (image_idx >= image.size.area()) continue;

    dest[target_idx] = src[image_idx];
  }

}

void Painter::draw_line(const int x1, const int y1, const int x2, const int y2, const Color &color)
{
	bool yLonger = false;
	int incrementVal;

	int shortLen = y2-y1;
	int longLen  = x2-x1;

	if (abs(shortLen)>abs(longLen)) 
  {
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;
		yLonger=true;
	}

	if (longLen<0) incrementVal = -1; else incrementVal = 1;

	double divDiff;
	if (shortLen == 0) divDiff = longLen;
	else divDiff = (double)longLen / (double)shortLen;
	if (yLonger) 
  {
		for (int i = 0; i != longLen; i += incrementVal) 
    {
      int x = x1 + (int)((double)i/divDiff);
      int y = y1+i;
      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= target->width()) continue;
      if (y >= target->height()) continue;

			target->set_pixel(x, y, color);
		}
	} else 
  {
		for (int i = 0; i != longLen; i += incrementVal) 
    {
      int x = x1+i;
      int y = y1 + (int)((double)i/divDiff);

      if (x < 0) continue;
      if (y < 0) continue;
      if (x >= target->width()) continue;
      if (y >= target->height()) continue;

			target->set_pixel(x, y, color);
		}
	}
}

void Painter::draw_rectangle(const int x1, const int y1, const int x2, const int y2, const Color &color)
{
  if (x1 < 0 && y1 < 0 && x2 < 0 && y2 < 0) return;
  if (x1 >= target->width() && y1 >= target->height() 
      && x2 >= target->width() && y2 >= target->height()) return;

  //TODO: handle special case when x1==x2 || y1==y2

  int xx = x1;
  int w = x2-x1;
  if (w < 0)  {
    xx = x2; w = x1 - x2;
  }
  int yy = y1;
  int h = y2-y1;
  if (h < 0)  {
    yy = y2; h = y1 - y2;
  }

  if (xx < 0) {
    w -= -xx+1;
    xx = 0;
  }
  if (yy < 0) { 
    h -= -yy+1;
    yy = 0;
  }
  if (xx >= target->width()) {
    w -= target->width() - xx+1;
    xx = target->width();
  }
  if (yy >= target->height()) {
    h -= target->height()-yy+1;
    yy = target->height();
  }

  #pragma omp parallel for
  for (int i = 0; i <= w; i++)
  {
    if (xx+i >= target->width()) {
      continue;
    }

    if (y1 >= 0 && y1 < target->height())
      target->set_pixel(xx+i, y1, color);
    
    if (y2 >= 0 && y2 < target->height())
      target->set_pixel(xx+i, y2, color);
  }

  #pragma omp parallel for
  for (int i = 0; i <= h; i++)
  {
    if (yy+i >= target->height()) {
      continue;
    }

    if (x1 >= 0 && x1 < target->width())
      target->set_pixel(x1, yy+i, color);

    if (x2 >= 0 && x2 < target->width())
      target->set_pixel(x2, yy+i, color);
  }
}

void Painter::draw_circle(const int x, const int y, const int radius, const Color &color)
{
  if (x + radius < 0) return;
  if (y + radius < 0) return;
  if (x - radius >= target->width()) return;
  if (y - radius >= target->height()) return;

  auto in_target_bounds = [&](const int t_x, const int t_y)->bool
  {
    return in_bounds(t_x, t_y, target->width(), target->height());
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

    if (in_target_bounds(x - xx, y + xx))
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
      err += 2*yy + 1;
    }

    if (err > 0)
    {
      xx -= 1;
      err -= 2*xx + 1;
    }
  }
}