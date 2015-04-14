#include "tpw.hpp"

void
simple_test()
{
  TPW::Connection conn("localhost");
  TPW::Space space = conn.get_space("tester");

  std::cout << "This example follows " <<
    "http://tarantool.org/doc/getting_started.html" << std::endl;
  
  std::cout << "'tester' space has internal number " << space.num() <<
    " while TPW able to use symbolic names" << std::endl;

  //
  // Uncomment to start fresh
  // conn.write( conn.call("box.space.tester:truncate") ).read();
  //

  try
  {
    std::cout << "Insert records in slightly different ways" << std::endl;
    conn.write( conn.insert(space) << 4U << "Painting" ).read();
    conn.insert(space) << 5U << "Diameter" << 12U <<
      TPW::ENDR;
    conn.write( conn.insert(space) << (uint64_t)6 << "Weight"
      << (uint64_t)80 ).read();
  }
  catch(const TPW::Connection::Error& err)
  {
    std::cout << "we've got error " << err.what() <<
      ", but lets try to go further" << std::endl;
  }

  uint64_t metric_value;
  std::string metric_name;
  conn.select(space) << 5U << TPW::ENDR >>
    TPW::SKIP >> metric_name >> metric_value;

  std::cout << "We've got value " << metric_value << " for " <<
    metric_name << std::endl;

  std::cout << "All known tester tools are:" << std::endl;

  std::cout << conn.write( conn.select(space) ).read().explain();
  
  for(;;)
  {
    TPW::ITStream&& its(conn.get_itstream());
    if (!its.good())
    {
      std::cout << "all done" << std::endl;
      break;
    }
    uint64_t id;
    std::string tool_name;
    
    its >> id >> tool_name;
    std::cout << " > " << id << ':' << tool_name << std::endl;
  }
  
}


int
main(int, char**)
{
  simple_test();
}
