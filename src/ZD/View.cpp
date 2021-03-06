#include "View.hpp"

#include <cstdio>

#include "3rd/glm/ext/vector_float3.hpp" // vec3
#include "3rd/glm/ext/matrix_float4x4.hpp" // mat4x4
#include "3rd/glm/ext/matrix_transform.hpp" // translate, rotate, scale, identity
#include "3rd/glm/ext/quaternion_float.hpp" // quat
#include "3rd/glm/gtc/quaternion.hpp"

namespace ZD
{
  glm::mat4 View::get_projection_matrix() const
  {
    switch (projection)
    {
      case Camera::Projection::Perspective:
        return glm::perspective(
          fov.angle_rad, aspect, clipping_plane.near, clipping_plane.far);
      case Camera::Projection::Ortographic:
        return glm::ortho(
          ortographic_box.left,
          ortographic_box.right,
          ortographic_box.bottom,
          ortographic_box.top,
          clipping_plane.near,
          clipping_plane.far);
    }
    assert(false);
    return {};
  }

  glm::mat4 View::get_view_matrix() const
  {
    return glm::lookAt(position, target, glm::vec3(0, 1, 0));
  }

} // namespace ZD
