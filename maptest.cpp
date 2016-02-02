#include "tpwmap.hpp"

void
tuple_test()
{
  TPW::Connection conn("localhost", 3301);

  using TesterMap =
    TPW::Map<uint64_t, std::tuple<std::string, uint64_t> >;
  TesterMap tester_map(conn, "tester");

  TesterMap::mapped_type tpl = tester_map[5];

  std::cout << std::get<0>(tpl) << ':' << std::get<1>(tpl) << std::endl;

  tester_map[5] = std::make_tuple<std::string, uint64_t>("Diameter",
    std::get<1>(tpl) * 2);

  tpl = static_cast<decltype(tester_map)::mapped_type>(tester_map[5]);
  std::cout << std::get<0>(tpl) << ':' << std::get<1>(tpl) << std::endl;

  std::cout << "There are " << tester_map.size() <<
    " elements in the container" << std::endl;
}

struct TesterObject
{
  std::string property;
  uint64_t value;
};

inline TPW::ITStream&
operator>>(TPW::ITStream& in, TesterObject& obj)
{
  in >> obj.property >> obj.value;
}

inline TPW::OTStream&
operator<<(TPW::OTStream& out, const TesterObject& obj)
{
  out << obj.property << obj.value;
}


inline std::ostream&
operator<<(std::ostream& out, const TesterObject& obj)
{
  out << "property:" << obj.property << ", value:" << obj.value << std::endl;
}

void
object_test()
{
  TPW::Connection conn("localhost", 3301);

  

  using TesterMap =
    TPW::Map<uint64_t, TesterObject>;
  TesterMap tester_map(conn, "tester");

  TesterObject tester_object = tester_map[5];
  std::cout << tester_object;

  tester_map[5] = TesterObject{"Diameter", tester_object.value * 2};

  std::cout << tester_map[5];
}

int
main(int, char**)
{
  tuple_test();
  object_test();
}
