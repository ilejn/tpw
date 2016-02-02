#ifndef TPW_IPP_
#define TPW_IPP_

#include "tpw.hpp"

namespace TPW
{
	/// Response debug output
  template <typename Elem, typename Traits>
  std::basic_ostream<Elem, Traits>&
  operator<<(std::basic_ostream<Elem, Traits>& os, const Response& res)
  {
    tp_rewind(res.get_response());
    os << "reqid: " << res.get_reqid() << std::endl;
    
    int tuples = tp_tuplecount(res.get_response());
    os << tuples << " tuples returned " << (void*)res.get_response() << std::endl;
    if (tuples)
    {
      while(tp_next(res.get_response()))
      {
        os << "tuple:" << std::endl;
        while(tp_nextfield(res.get_response()))
        {
          enum tp_type t = tp_typeof(*tp_getfield(res.get_response()));
          if (t == TP_NIL)
          {
            os << "NIL ";
          }
          else if (t == TP_MAP)
          {
            os << "MAP ";
          }
          else if (t == TP_ARRAY)
          {
            os << "ARRAY ";
          }
          else if (t == TP_BIN)
          {
            os << "BIN ";
          }
          else if (t == TP_EXT)
          {
            os << "EXT ";
          }
          else if (t == TP_BOOL)
          {
            os << "(bool)"<< tp_get_bool(tp_getfield(res.get_response())) << ' ';
          }
          else if (t == TP_DOUBLE)
          {
            os << "DOUBLE: " << tp_get_double(tp_getfield(res.get_response())) << ' ';
          }
          else if (t == TP_INT)
          {
            os << "INT: " << tp_get_int(tp_getfield(res.get_response())) << ' ';
          }
          else if (t == TP_UINT)
          {
            os << "UINT: " << tp_get_uint(tp_getfield(res.get_response())) << ' ';
          }
          else if (t == TP_STR)
          {
            uint32_t len;
            const char *s = tp_get_str(tp_getfield(res.get_response()), &len);
            os << "STR: " << std::string(s, len) << ' ';
          }
          else
          {
            os << "<<unknown>>";
          }
        }
        os << std::endl;
      }
    }
    tp_rewind(res.get_response());
    return os;
  }

  template <typename Elem, typename Traits>
  std::basic_ostream<Elem, Traits>&
  operator<<(std::basic_ostream<Elem, Traits>& os, const Connection& conn)
  {
    os << conn.response_;
  }

  template
  <
    int Index,
    int Max,
    typename... Args
    >
  struct OTStream::Binder
  {
    static void
    bind(TPW::OTStream& out, const std::tuple<Args...>& args)
    {
      out << std::get<Index>(args);
      Binder<Index + 1, Max, Args...>::bind(out, args);
    }
  };

  template
  <
    int Max,
    typename... Args
    >
  struct OTStream::Binder<Max, Max, Args...>
  {
    static void
    bind(TPW::OTStream&, const std::tuple<Args...>&)
    {
      return;
    }
  };
  

  
  template <typename V>
  OTStream&
  OTStream::operator<<(const UpdateOp<V>& f)
  {
    if (!update_op_)
    {
      tuple_shift_ = tp_used(handle()) + mp_sizeof_uint(TP_TUPLE);;
      tp_updatebegin(handle(), 1);
      update_op_ = true;
      items_ = 0;
    }
    tp_op(handle(), f.op_.c_str()[0], f.field_);
    return operator<<(f.value_);
  }

  template <typename... Args> 
  OTStream&
  OTStream::operator<<(const std::tuple<Args...>& args)
  {
    Binder<0, sizeof...(Args), Args...>::bind(*this, args);
    return *this;
  }
  
  

  template
  <
    int Index,
    int Max,
    typename... Args
    >
  struct ITStream::Binder
  {
    static void
    bind(TPW::ITStream&& in, std::tuple<Args...>& args)
    {
      in >> std::get<Index>(args);
      Binder<Index + 1, Max, Args...>::bind(std::move(in), args);
    }
  };

  template
  <
    int Max,
    typename... Args
    >
  struct ITStream::Binder<Max, Max, Args...>
  {
    static void
    bind(TPW::ITStream&& in, std::tuple<Args...>& args)
    {
      return;
    }
  };
  

  template <typename... Args> 
  ITStream&
  ITStream::operator>>(std::tuple<Args...>& args)
  {
    Binder<0, sizeof...(Args), Args...>::bind(std::move(*this), args);
    return *this;
  }
  
  template <typename Arg> inline
  void
  Connection::call_pass(TPW::OTStream&& ots, Arg arg)
  {
    ots << arg;
  }
  
  template <typename Arg, typename... ArgTail> inline
  void
  Connection::call_pass(TPW::OTStream&& ots, Arg arg, ArgTail... arg_tail)
  {
    ots << arg;
    return call_pass(std::move(ots), arg_tail...);
  }
  
  template <typename... ArgTail> inline
  void
  Connection::call_execute(const std::string& name, ArgTail... arg_tail)
  {
    call_pass(std::move(call(name)), arg_tail...);
    OTStream(*this) << TPW::FLUSH;
  }
  
  template <typename... ArgTail> inline
  void
  Connection::call_tuple_execute(const std::string& name, ArgTail... arg_tail)
  {
    call_pass(std::move(call_tuple(name)), arg_tail...);
    OTStream(*this) << TPW::FLUSH;
  }

  template <typename... ArgTail> inline
  void
  Connection::call_table_execute(const std::string& name, ArgTail... arg_tail)
  {
    call_pass(std::move(call_table(name)), arg_tail...);
    OTStream(*this) << TPW::FLUSH;
  }
} 

#endif // TPW_IPP_
