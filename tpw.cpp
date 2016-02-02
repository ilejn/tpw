#include "tpw.hpp"

namespace TPW
{
  void
  Connection::fill_spaces()
  {
    const Space meta_space(280);   // _space
    
    write(select(meta_space));

    // This is possible as well
    // for(;;)
    // {
    //   std::cout << "connection ptr in fill_spaces" << (void*)this << std::endl;
    //   ITStream&& its(get_itstream());
    //   if (!its.good())
    //   {
    //     break;
    //   }

    for(ITStream&& its(get_itstream()); its.good(); its.next_tuple())
    {
      uint64_t space_id;
      uint64_t to_ommit;
      std::string space_name;
    
      its >> space_id >> to_ommit >> space_name;
      space_map_.insert(std::make_pair(space_name, space_id));
    }
  }
  
  Connection::Connection(const std::string hostname,
    uint16_t port)
    : need_read_(false)
  {
    memset(static_cast<void*>(&s_), 0, sizeof(struct tnt_stream));
    tnt_net(&s_);
    

    std::string uri = /* "test:test@"  + */  hostname + ':' + std::to_string(port);
    tnt_set(&s_, TNT_OPT_URI, uri.c_str());

    
    int rc = tnt_connect(&s_);
    
    // tb_sesinit(&s_);
    // tb_sesset(&s_, TB_HOST, hostname.c_str());
    // tb_sesset(&s_, TB_PORT, port);
    // tb_sesset(&s_, TB_SENDBUF, 0);
    // tb_sesset(&s_, TB_READBUF, 0);
    // int rc = tb_sesconnect(&s_);
    if (rc == -1)
    {
      std::string error_text = tnt_strerror(&s_);
      throw Error("Failed to connect: " + error_text);
    }

    // /* handle the server's greeting */
    // const int greeting_buffer_size = 128;                        
    // char greeting_buffer[greeting_buffer_size];

    // ssize_t greet_recv = tnt_io_recv_raw(
    //   get_io(), greeting_buffer, greeting_buffer_size, 1);
    // if (greet_recv != greeting_buffer_size)
    // {
    //   throw Error("Failed to receive greeting");
    // }

    // tpgreeting greet;
    // rc = tp_greeting(&greet, greeting_buffer, 128);
 //   rc = tnt_authenticate(&s_);
    rc = tnt_deauth(&s_);
    
    if (rc == -1)
    {
      std::string error_text = tnt_strerror(&s_);
      throw Error("Failed to authenticate: " + error_text);
    }

    fill_spaces();
  }

  Space
  Connection::get_space(const std::string& space_name)
  {
    SpaceMap::iterator it = space_map_.find(space_name);
    if (it == space_map_.end())
    {
      return Space();
    }
    
    return Space(it->second, space_name);
  }

  int
  Connection::get_fd()
  {
    return get_io()->fd;
  }


  Connection&
  Connection::write(const OTStream& ots)
  {
    return write();
  }

  Connection&
  Connection::write()
  {
    request_.reqid();
    if (tnt_io_send_raw(get_io(), static_cast<char*>(const_cast<void*>(request_.data())),
        request_.size(), 1) < 0)
    {
      throw Error("Failed to write");
    }
    need_write_ = false;
    need_read_ = true;
    request_.request_init();
    return *this;
  }

  Connection&
  Connection::eread(bool initialized)
  {
    if (!initialized)
    {
      response_.free();
      response_.init();
    }
  
    while (1)
    {
      const ssize_t to_read = tp_req(response_.handle());
      if (to_read <= 0)
      {
        break;
      }
    
      const ssize_t new_size = tp_ensure(response_.handle(), to_read);
      if (new_size == -1)
      {
        throw Error("no memory");
      }
      const ssize_t res = tp_recv2(get_io(), response_.handle(), to_read);
      if (res == 0)
      {
        throw Error("eof");
      }
      else if (res < 0)
      {
        throw Error("read error");
      }
    }

    tp_reply(response_.get_response(),
      tp_buf(response_.handle()), tp_size(response_.handle()));
  
    const ssize_t server_code = response_.get_response_code();
    need_read_ = false;

    if (server_code != 0)
    {
      throw Error(std::string("Server reply error: ") +
        std::string(
          response_.get_response()->error,
          response_.get_response()->error_end));
    }

    return *this;
  }
  
  Connection&
  Connection::read(bool initialized) noexcept
  {
    try
    {
      eread(initialized);
    }
    catch(const Error& err)
    {
      std::cerr << err.what() << " suppressed" << std::endl;
    }
    return *this;
  }
  
  ITStream
  Connection::get_itstream()
  {
    if (need_write_)
    {
      write();
    }

    ITStream itr = ITStream(need_read_ ? eread() : *this);
    if (!response_.get_response_code() &&
      tp_hasnextfield(response_.get_response()))
    {
      // std::cout << "field exists in get_itstream" << std::endl;
    }
    else
    {
      itr.invalidate();
    }
    return itr;
  }

  OTStream
  Connection::insert(const Space& space)
  {
    tp_insert(handle(), space.num());
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::replace(const Space& space)
  {
    tp_replace(handle(), space.num());
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::del(const Space& space)
  {
    tp_delete(handle(), space.num());
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::call(const std::string& name)
  {
    tp_call(handle(), name.data(), name.size());
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::call_tuple(const std::string& name)
  {
    tp_call(handle(), name.data(), name.size());
    // only one parameter to pass
    tp_tuple(handle(), 1);
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::call_table(const std::string& name)
  {
    tp_call(handle(), name.data(), name.size());
    tp_tuple(handle(), 1);
    need_write_ = true;
    return OTStream(*this, true);
  }

  OTStream
  Connection::auto_increment(const Space& space)
  {
    std::string func = "box.space." + space.name() + ":auto_increment";
    need_write_ = true;
    return call_tuple(func);
  }

  OTStream
  Connection::select(const Space& space,
    uint32_t offset, uint32_t limit)
  {
    tp_select(handle(), space.num(), 0 /*ind*/, offset, TP_ITERATOR_EQ, limit); 
    need_write_ = true;
    return OTStream(*this);
  }

  OTStream
  Connection::update(const Space& space)
  {
    tp_update(handle(), space.num());
    need_write_ = true;
    return OTStream(*this);
  }

////////////////////
  
  std::string
  Response::explain() const
  {
    std::stringstream ss;
    ss << *this;
    return ss.str();
  }

  bool
  Response::next_tuple()
  {
    return tp_next(&tpr_);
  }

  bool
  Response::next_field()
  {
    if (tp_hasnextfield(const_cast<tpresponse*>(&tpr_)))
    {
      return tp_nextfield(const_cast<tpresponse*>(&tpr_));
    }
    else
    {
      return false;
    }
  }

  Field
  Response::get() const
  {

    if (tp_hasnextfield(const_cast<tpresponse*>(&tpr_)))
    {
      tp_nextfield(const_cast<tpresponse*>(&tpr_));
    }
    else
    {
      return Field();
    }
    return Field(tp_getfield(const_cast<tpresponse*>(&tpr_)),
      tp_getfieldsize(const_cast<tpresponse*>(&tpr_)));
  }

  Field
  Response::get(size_t n) const
  {
    tp_rewindfield(const_cast<tpresponse*>(&tpr_));
    for (size_t i = 0; i <= n; ++i)
    {
      if (tp_hasnextfield(const_cast<tpresponse*>(&tpr_)))
      {
        tp_nextfield(const_cast<tpresponse*>(&tpr_));
      }
      else
      {
        return Field();
      }
    }
    return Field(tp_getfield(const_cast<tpresponse*>(&tpr_)),
      tp_getfieldsize(const_cast<tpresponse*>(&tpr_)));
  }

  std::string
  Response::get_str(size_t n, std::string deflt) const
  {
    Field f = get(n);
    if (f.valid())
    {
      assert(tp_typeof(*f.data()) == TP_STR);
      uint32_t len;
      const char* str_begin = tp_get_str(f.data(), &len);
      return std::string(str_begin, len);
    }
    else
    {
      return deflt;
    }
  }

  int64_t
  Response::get_int(size_t n, int64_t deflt) const
  {
    Field f = get(n);
    assert(tp_typeof(*f.data()) == TP_INT);
    if (f.valid())
    {
      return tp_get_int(f.data());
    }
    else
    {
      return deflt;
    }
  }

  uint64_t
  Response::get_uint(size_t n, uint64_t deflt) const
  {
    Field f = get(n);
    assert(tp_typeof(*f.data()) == TP_INT);
    if (f.valid())
    {
      return tp_get_uint(f.data());
    }
    else
    {
      return deflt;
    }
  }

////////////////
  
  ITStream::ITStream(Connection& conn)
    : conn_(conn),
      good_(conn.next_tuple())
  {
  }

  ITStream&
  ITStream::operator>>(int64_t& i)
  {
    Field f = conn_.get();
    assert(f.valid());
    
    enum tp_type t = tp_typeof(*f.data());
    switch(t)
    {
    case TP_UINT:
      i = tp_get_uint(f.data());
      break;
    case TP_INT:
      i = tp_get_int(f.data());
      break;
    }
    return *this;
  }

  ITStream&
  ITStream::operator>>(uint64_t& i)
  {
    Field f = conn_.get();
    assert(f.valid());
    i = tp_get_uint(f.data());
    return *this;
  }

  ITStream&
  ITStream::operator>>(std::string& s)
  {
    Field f = conn_.get();
    uint32_t len;
    const char* str_begin = tp_get_str(f.data(), &len);
    s = std::string(str_begin, len);
    return *this;
  }

  ITStream&
  ITStream::operator>>(bool& b)
  {
    Field f = conn_.get();
    assert(f.valid());
    b = tp_get_bool(f.data());
    return *this;
  }

  ITStream&
  ITStream::skip()
  {
    if (!conn_.next_field())
    {
      invalidate();
    }
    
    return *this;
  }

  ITStream&
  ITStream::next_tuple()
  {
    if (!conn_.next_tuple())
    {
      invalidate();
    }
    
    return *this;
  }


////////////////
  
  void
  OTStream::advance()
  {
    assert(tuple_shift_);
    if (table_)
    {
      ++items_;
      if (items_ % 2)
      {
        mp_encode_map(tp_buf(handle()) + tuple_shift_, 1 + items_ / 2);
      }
    }
    else
    {
      ++items_;
      mp_encode_array(tp_buf(handle()) + tuple_shift_, items_);
    }
  }

  const tp*
  OTStream::handle() const
  {
    return conn_.handle();
  }

  tp*
  OTStream::handle()
  {
    return conn_.handle();
  }

  OTStream::OTStream(Connection& conn, bool table)
    : conn_(conn),
      items_(0),
      tuple_shift_(tp_used(handle())),
      table_(table)
  {
    if (table_)
    {
      tp_encode_map(handle(), 0);
    }
    else
    {
      tp_tuple(handle(), 0);
    }
  }

  OTStream::OTStream(const OTStream& otr)
    : conn_(otr.conn_),
      items_(otr.items_),
      update_op_(otr.update_op_),
      tuple_shift_(otr.tuple_shift_),
      table_(otr.table_)
  {
  }

  OTStream&
  OTStream::operator<<(const std::string& v)
  {
    tp_encode_str(handle(), v.data(), v.size());
    advance();
    return *this;
  }
  

  OTStream&
  OTStream::operator<<(const char* v)
  {
    tp_encode_str(handle(), v, strlen(v));
    advance();
    return *this;
  }

  OTStream&
  OTStream::operator<<(uint64_t v)
  {
    tp_encode_uint(handle(), v);
    advance();
    return *this;
  }

  OTStream&
  OTStream::operator<<(int64_t v)
  {
    if (v >= 0)
    {
      tp_encode_uint(handle(), v);
    }
    else
    {
      tp_encode_int(handle(), v);
    }
    advance();
    return *this;
  }

  OTStream&
  OTStream::operator<<(uint32_t v)
  {
    tp_encode_uint(handle(), (uint64_t)v);
    advance();
    return *this;
  }

  OTStream&
  OTStream::operator<<(int32_t v)
  {
    if (v >= 0)
    {
      tp_encode_uint(handle(), (uint64_t)v);
    }
    else
    {
      tp_encode_int(handle(), (int64_t)v);
    }
    advance();
    return *this;
  }

  OTStream&
  OTStream::operator<<(const Field& f)
  {
    tp_encode_bin(handle(), f.data(), f.size());
    advance();
    return *this;
  }

  ITStream
  OTStream::get_itstream()
  {
    return conn_.get_itstream();
  }

// Manipulators
  
  ITStream
  ENDR(OTStream& ots)
  {
    return ots.get_itstream();
  }

  void
  FLUSH(OTStream& ots)
  {
    ots.conn_.write(ots);
  }

// next field
  ITStream&
  SKIP(ITStream& its)
  {
    its.skip();
    return its;
  }

// next tuple (row) manipulator
  ITStream&
  NEXT(ITStream& its)
  {
    its.skip();
    return its;
  }
}




