#include <iostream>

#include "properties.hpp"

//////////////////////////////////////////////////////////////////////////////
int main()
{
  struct S: properties
  {
    bool b{};
    int i{};

    S()
    {
      register_property("b", b)("i", i)
        ("joke",[]{return "just a joke";},
          [](nlm::json const& j){std::cout << "knock: " << j << std::endl;})
        ();
    }
  } s;

  s.set("b", true)("i", 11.1)("joke", 1);

  std::cout << s.get("b") << std::endl;
  std::cout << s.state() << std::endl;

  return 0;
}