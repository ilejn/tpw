/*
 * Tarantool Binary Protocol C++ Wrapper
 *   header
 */

#ifndef TPW_HPP_
#define TPW_HPP_


#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <stdexcept>
#include <string>
#include <iostream>
#include <map>

#include <boost/lexical_cast.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-fpermissive"
#include "tp.h"
#pragma GCC diagnostic pop

#include "tarantool/tarantool.h"
extern "C"
{
#include "tarantool/tnt_net.h"
#include "tarantool/tnt_io.h"
}


#include "tp_ext.h"

namespace TPW
{
  
//////////////////////////////////////////////////////
  class Connection;
  class Message;
  class Request;
  class Response;
  class OTStream;
  class ITStream;
  class Field;
  class Space;
  template <typename F> class UpdateOp;


//////////////////////////////////////////////////////
// Helper to represent Tarantool tables AKA spaces
//////////////////////////////////////////////////////
  class Space
  {
  private:
    const uint32_t num_;      // id
    const std::string name_;
  public:
    explicit Space(uint32_t num, std::string name = "not defined")
      : num_(num),
        name_(name)
    {
    }

    Space()
      : num_(0)
    {
    }

    bool
    valid()
    {
      return num_;
    }
  
    uint32_t
    num() const
    {
      return num_;
    }

    std::string
    name() const
    {
      return name_;
    }
  };


//////////////////////////////////////////////////////
// A message received (Response)  or to send (Request)
//   via Tarantool Binary protocol
//////////////////////////////////////////////////////
  class Message
  {
  private:
    uint32_t reqid_;   // AKA sync
    // required to link Response with proper
    //   Request if requests are muliplexed
    struct tp tp_;     // low level (tp.h) control structure
  
  public:
    friend class Connection; // namely Connection::operator<<
  
    const tp*
    handle() const
    {
      // why cast ??
      return const_cast<tp*>(&tp_);
    }

    tp*
    handle()
    {
      return &tp_;
    }

    const
    void*
    data() const
    {
      return tp_buf(const_cast<tp*>(&tp_));
    }
  
    ssize_t
    size() const
    {
      return tp_used(const_cast<tp*>(&tp_));
    }
  
  public:
    Message(uint32_t reqid = 0)
      : reqid_(reqid)
    {
      tp_init(&tp_, nullptr, 0, &tp_realloc, nullptr);
    }

    Message(const Message& rhs) = delete;

  
    virtual
    ~Message()
    {
      this->free();
    }

  protected:
    void
    free()
    {
      if (tp_.s)
      {
        tp_free(&tp_);
      }
    }

    void
    set_reqid(uint32_t rqid)
    {
      reqid_ = rqid;
    }

    void
    reqid() const
    {
      if (reqid_)
      {
        tp_reqid(const_cast<tp*>(&tp_), reqid_);
      }
    }
  
    void
    init()
    {
      tp_init(&tp_, nullptr, 0, &tp_realloc, nullptr);
    }
  };

//////////////////////////////////////////////////////
// A message to send
//   via Tarantool Binary protocol
//////////////////////////////////////////////////////
  class Request : public Message
  {
  public:
    Request(uint32_t reqid = 0)
      : Message(reqid)
    {
    }

    OTStream
    insert(const Space& space);

    OTStream
    del(const Space& space);
  
    OTStream
    call(const std::string& name);

    OTStream
    call_tuple(const std::string& name);

    OTStream
    auto_increment(const Space& space);

    OTStream
    select(const Space& space, 
      uint32_t offset = 0, uint32_t limit = 100);

    OTStream
    update(const Space& space);

    void
    request_init()
    {
      Message::free();
      Message::init();
    }
    
  };

//////////////////////////////////////////////////////
// A message received
//   via Tarantool Binary protocol
//////////////////////////////////////////////////////
  class Response: public Message
  {
    friend class Connection;
  
  private:
    tpresponse tpr_;  // low level (tp.h) control structure

    void
    response_init()
    {
      memset(&tpr_, 0, sizeof(struct tpresponse));
    }
  
  public:
    struct tpresponse*
    get_response()
    {
      return &tpr_;
    }

    /* const */
    struct tpresponse*
    get_response() const
    {
      return const_cast<struct tpresponse*>(&tpr_);
    }
    
    ssize_t
    get_response_code() const
    {
      return tpr_.code;
    }

    Response()
    {
      response_init();
    }


    Response& operator=(Response&& other) = delete;
  
    Field
    get() const;

    Field
    get(size_t i) const;

    std::string
    get_str(size_t n, std::string deflt = "") const;

    int64_t
    get_int(size_t n, int64_t deflt = 0) const;

    uint64_t
    get_uint(size_t n, uint64_t deflt = 0) const;

    bool
    next_tuple();

    bool
    next_field();

    size_t
    nfields() const
    {
      return tp_tuplecount(&tpr_);
    }

    // size_t
    // tuples() const
    // {
    //   return tp_tuplecount(&tpr_);
    // }

    bool
    tuple(size_t n);

    std::string
    explain() const;

    uint32_t
    get_reqid() const
    {
      return tp_getreqid(const_cast<tpresponse*>(&tpr_));
    }

    template <typename Elem, typename Traits>
    friend
    std::basic_ostream<Elem, Traits>&
    operator<< (std::basic_ostream<Elem, Traits>& os, Response& res);
  };


//////////////////////////////////////////////////////
// Input Tarantool Stream
//   obtained from response
//////////////////////////////////////////////////////
  class ITStream
  {
    friend Connection;
    friend OTStream;  // namely get_itstream
    friend ITStream ENDR(OTStream& ots);
  
  
  private:
    bool good_;
    // Response& response_;
    Connection& conn_;

  private:
    ITStream(const ITStream& itsr)
      : good_(itsr.good_),
        conn_(itsr.conn_)
    {
    }
  

    ITStream&
    operator=(const ITStream&) = delete;

    ITStream&
    operator=(ITStream&&) = delete;

    void
    invalidate()
    {
      good_ = false;
    }
  
  public:
    ITStream(Connection& conn);
  
    bool
    good()
    {
      return good_;
    }

    // get value, advance to next column
    ITStream&
    operator>>(int64_t& i);

    ITStream&
    operator>>(uint64_t& i);

    ITStream&
    operator>>(std::string& s);

    ITStream&
    operator>>(bool& b);

    // skip one filed (column)
    ITStream&
    skip();

    // proceed to next row
    ITStream&
    next_tuple();

    /**
     * Handle a manipulator
     */
    ITStream&
    operator>>(ITStream& (*f)(ITStream&))
    {
      return f(*this);
    }
  };


//////////////////////////////////////////////////////
// Output Tarantool Stream
//   to modify request
//////////////////////////////////////////////////////
  class OTStream
  {
    friend Connection;
    friend void FLUSH(OTStream& ots);
  
  private:
    // Request& msg_;
    Connection& conn_;
  
    size_t items_;
    bool update_op_ = false;
    size_t tuple_shift_;
    bool table_ = false;

  private:
    // to next field
    void
    advance();

    const tp*
    handle() const;

    tp*
    handle();
  
    explicit
    OTStream(Connection& conn, bool table = false);
  
    OTStream(const OTStream& otr);
    
    OTStream&
    operator=(const OTStream&) = delete;

    // Do we need this ??
    OTStream&
    operator=(OTStream&&) = delete;
  public:

    /**
     * Handle a manipulator
     */
    OTStream&
    operator<<(OTStream& (*f)(OTStream&))
    {
      return f(*this);
    }

    // ITStream from OTStream manipulator
    ITStream
    operator<<(ITStream (*f)(OTStream&))
    {
      return f(*this);
    }
  
    // flush manipulator
    void
    operator<<(void (*f)(OTStream&))
    {
      return f(*this);
    }
  
    OTStream&
    operator<<(const std::string& v);

    OTStream&
    operator<<(const char* v);

    OTStream&
    operator<<(uint64_t v);

    OTStream&
    operator<<(int64_t v);

    OTStream&
    operator<<(uint32_t v);

    OTStream&
    operator<<(int32_t v);

    OTStream&
    operator<<(const Field& f);

    template <typename V>
    OTStream&
    operator<<(const UpdateOp<V>& f);

    // make it private !!!
    ITStream
    get_itstream();
  };


//////////////////////////////////////////////////////
// Field (AKA column) raw representation
//////////////////////////////////////////////////////
  class Field
  {
  private:
    const char* data_;
    const size_t size_;
  
  public:
    Field()
      : data_(nullptr), size_(0)
    {
    }

    Field(const char* data, size_t size)
      : data_(data), size_(size)
    {
    }

    Field(const void* data, size_t size)
      : data_(static_cast<const char*>(data)), size_(size)
    {
    }

    // no shallow copy
    Field(const Field&) = delete;   

    Field(const Field&& r)
      : data_(r.data_), size_(r.size_)
    {
    }

    Field& operator=(const Field&) = delete;
    
    const char*
    data() const
    {
      return data_;
    }

    size_t
    size() const
    {
      return size_;
    }

    bool
    valid() const
    {
      return data_ != nullptr;
    }

    template <typename T>
    const T&
    cast() const
    {
      assert(sizeof(T) == size_);
      return *static_cast<const T*>(static_cast<const void*>(data_));
    }
  };



//////////////////////////////////////////////////////
// Tarantool binary protocol connection
//////////////////////////////////////////////////////
  class Connection
  {
    friend OTStream;
  
  private:
    // struct tbses s_;
    struct tnt_stream s_;
    typedef std::map<std::string, uint64_t> SpaceMap;
    SpaceMap space_map_;
    Response response_;
    Request request_;
    bool need_read_;  // internal status
    bool need_write_;  // internal status
  private:
    tnt_stream_net* get_io()
    {
      return TNT_SNET_CAST(&s_);
    }
    

  public:
    class Error : public std::logic_error
    {
      using logic_error::logic_error;
    };

    explicit
    Connection(const std::string hostname = "localhost",
      uint16_t port = 3301);

    Connection(const Connection&) = delete;

    Connection& operator=(const Connection&) = delete;
    
    ~Connection()
    {
      tnt_close(&s_);
      tnt_stream_free(&s_);
    }

    // AKA next row
    bool
    next_tuple()
    {
      return response_.next_tuple();
    }

    // AKA next column
    bool
    next_field()
    {
      return response_.next_field();
    }

    // debug representation
    std::string
    explain() const
    {
      return response_.explain();
    }

    Connection&
    set_reqid(uint32_t rqid)
    {
      request_.set_reqid(rqid);
      return *this;
    }

    int32_t get_reqid()
    {
      return response_.get_reqid();
    }

    //////////////////////////////////
    // tarantool box methods
    OTStream
    insert(const Space& space);
  

    OTStream
    del(const Space& space);
  
  
    OTStream
    call(const std::string& name);

    /// function that accepts tuple call
    OTStream
    call_tuple(const std::string& name);
  
    OTStream
    call_table(const std::string& name);
  

    OTStream
    auto_increment(const Space& space);
  

    OTStream
    select(const Space& space, 
      uint32_t offset = 0, uint32_t limit = 100);

  
    OTStream
    update(const Space& space);
    //////////////////////////////

  
    /**
     * Send request to Tarantool
     */
    Connection&
    write(const OTStream& os);
  
    Connection&
    write();
  
    Connection&
    operator<<(const OTStream& os)
    {
      return write(os);
    }

    /**
     * Get response from Tarantool
     */
    Connection&
    eread(bool initialized = false);

    Connection&
    read(bool initialized = false) noexcept;

    Space
    get_space(const std::string& space_name);

    ITStream
    get_itstream();

    int
    get_fd();

    Field
    get() const
    {
      return response_.get();
    }
  
  
    template <typename Elem, typename Traits>
    friend
    std::basic_ostream<Elem, Traits>&
    operator<< (std::basic_ostream<Elem, Traits>& os, const Connection& conn);
    
  private:
    void
    fill_spaces();

    const tp*
    handle() const
    {
      return request_.handle();
    }

    tp*
    handle()
    {
      return request_.handle();
    }
  };

  typedef std::vector<Connection*> ConnPtrVector;

//////////////////////////////////////////////////////
// Update helper
//    The update function supports operations on fields —
//       assignment, arithmetic (if the field is unsigned numeric),
//       cutting and pasting fragments of a field, deleting or inserting a field.
//////////////////////////////////////////////////////
  template <typename V>
  class UpdateOp
  {
  public:
    std::string op_;  // operation
    uint64_t field_;
    V value_;
  
  public:
    UpdateOp(const std::string& op, uint64_t field, V value)
      : op_(op), field_(field), value_(value)
    {
    }
  };

    /// manipulators
  // end of request 
  ITStream ENDR(OTStream& ots);

  // end of request 
  void FLUSH(OTStream& ots);

  // next field 
  ITStream& SKIP(ITStream& ots);
  
  // next tuple (row)
  ITStream& NEXT(ITStream& ots);
}

#include "tpw.ipp"

#endif // TPW_HPP_
