#include <cstdio>
#include <iostream>

class Food {
 public:
  virtual ~Food() = default;
  virtual void get_cooked(void) = 0;
};

class Vegetable: public Food {
 public:
  virtual ~Vegetable() {}
  virtual void get_cooked(void) {
    puts("Cooking vegies..., smells good!");
  }
};

class Fruit: public Food {
 public:
  virtual ~Fruit() {}
  virtual void get_cooked(void) {
    puts("Cooking fruit..., ah... gross!");
  }
};

class Tomato: public Fruit, public Vegetable {
 public:
  Tomato() {}
  virtual ~Tomato() {}
  virtual void get_cooked(void) {
    puts("Cooking tomato, weirdly good!");
  }
};

int main(int argc, char** argv) {
  Tomato* tomato = new Tomato();
  
  puts("[Tomato]");
  tomato->get_cooked();

  puts("[Vegie]");
  auto veg_tomato = dynamic_cast<Vegetable*>(tomato);
  veg_tomato->get_cooked();

  return 0;
}
