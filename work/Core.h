#pragma once
#include <string>
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include <imgui.h>
class CorePrivData {
 public:
  CorePrivData() {};
  bool _debug;
  int _width;
  int _height;
  std::string _font;
};
class Core {
 public:
  Core();
  void configure();
  void run();

 private:
  CorePrivData _priv;
};
