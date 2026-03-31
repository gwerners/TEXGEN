#ifndef GRADIENT_H
#define GRADIENT_H

#include <raylib.h>
#include "Generator.h"
#include "Utils.h"

enum GradientType { White, BlackAndWhite, WhiteAndBlack };

class Gradient : public Generator {
 public:
  Gradient();
  void setRange(sU32 start, sU32 end);
  void setGradient(enum GradientType gt);
  void refresh() override;
  GenTexture texture();
  GeneratorType type() override;
  bool accept(Generator* other) override;
  void render() override;
  std::string title() override;

 private:
  GenTexture m_tex;
  sU32 m_start;
  sU32 m_end;
  GradientType m_gradientType;
};

#endif  // GRADIENT_H
