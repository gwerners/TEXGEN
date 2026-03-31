#ifndef VORONOI_H
#define VORONOI_H

#include "Generator.h"
#include "Gradient.h"
#include "Utils.h"

class Voronoi : public Generator {
 public:
  Voronoi();
  ~Voronoi() override;
  void setGradient(GenTexture tex);
  GenTexture gradient();
  void refresh() override;
  GenTexture texture();
  GeneratorType type() override;
  bool accept(Generator* other) override;
  void render() override;
  std::string title() override;

 private:
  sInt m_side;
  sInt m_intensity;
  sInt m_count;
  sF32 m_distance;
  GenTexture m_gradient;
  GenTexture m_texture;
  Texture2D texture2D;
};

#endif  // VORONOI_H
