#ifndef GENERATOR_H
#define GENERATOR_H
#include <string>
enum GeneratorType { Abstract, Gradient, Voronoi };

class Generator {
 public:
  virtual ~Generator() {}
  virtual void refresh() {}
  virtual GeneratorType type() { return GeneratorType::Abstract; }
  virtual bool accept(Generator*) { return true; }
  virtual void render() {}
  virtual std::string title() { return "Abstract"; }
};

#endif  // GENERATOR_H
