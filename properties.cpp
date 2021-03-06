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
      register_properties(
        {
          {"b", b},
          {"i", i},
          {"_invisible", b},
          {
            "joke",
            []{return "just a joke";},
            [](auto&& j){std::cout << "knock: " << j << std::endl;}
          },
          {
            "lol",
            []{return "haha";}
          }
        }
      );
    }
  } s;

  s.set("b", true)("i", 11.1)("joke", 1);

  std::cout << s.get("b") << std::endl;
  std::cout << s.state() << std::endl;

  return 0;
}
