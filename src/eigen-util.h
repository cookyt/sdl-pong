#ifndef EIGEN_UTIL_H_
#define EIGEN_UTIL_H_

#include <boost/format.hpp>
#include <Eigen/Dense>

namespace eigen_util {

inline boost::format FormatVec2d(const Eigen::Vector2d& vec) {
  return boost::format("(%lf, %lf)") % vec.x() % vec.y();
}

}  // namespace eigen_util

#endif  // EIGEN_UTIL_H_
