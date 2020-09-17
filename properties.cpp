#include <iostream>

#include "properties.hpp"

//////////////////////////////////////////////////////////////////////////////
int main()
{
  using json = nlohmann::json;

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
          {
            "joke",
            []{return "just a joke";},
            [](json const& j){std::cout << "knock: " << j << std::endl;}
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
