#pragma once

#include "fovea/types.hpp"

namespace fovea {

class Predictor {
public:
  explicit Predictor(float horizon_seconds = 2.0F);

  void apply(SceneFrame& frame) const;

private:
  float horizon_seconds_;
};

}  // namespace fovea
