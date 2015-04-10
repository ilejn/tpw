# tpw
## *Tarantool binary Protocol c++ Wrapper*
## Tarantool?

Yes. According to [official site](http://tarantool.org/) it is *A NoSQL database running in a Lua application server*
Because it is In Memory Data Base, the most natural competitor is Redis.
Tarantool' evangelists say that Tarantool is better.
Redis' evangelists say nothing because do not know about Tarantool

## Status

The library is early alfa.
It is hardly that you can do anything besides simple example.
One of the reasons - you need not.
I've found calling Lua methods from C++ more practical than performing data manipalations directly.
So it is possible that ability to connect to Tarantool and to call Lua methods is Ok.

The library does not cover async usage, while it is possible of course.

## History

The library is created because C++ is my main programming language. Official C-connector did not exist at the moment when I started to evaluate Tarantool 1.6 so I had no choice but create something to go further. When tp was adapted for 1.6 by Tarantool team, I happily switched to it.

## Plans

Many many things are missed. 
I'll definitely make this library 
* more robust
** get rid of assertions 
** ensure there is no memory leaks
* as fast as possible
** adding essential features like aggregating requests

I am not sure that I am ready to
* care about build for different environments
 

## Example

```c++
  TPW::Connection conn("localhost");
  TPW::Space space = conn.get_space("tester");
  conn.select(space) << digit << TPW::ENDR >>  got_index >> str1 >> TPW::SKIP >> str3;;
  std::cout << "select response:" << got_index << ":" << str1 <<
    ":" << "skipped" << ":" << str3 << std::endl;
  conn.update(space) << digit << TPW::UpdateOp<std::string>("=", 2, "vasya");
  conn.write();
  std::cout << conn.explain();
```
