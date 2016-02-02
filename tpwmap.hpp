#include "tpw.hpp"

namespace TPW
{
  template <typename _Key, typename _Tp, typename _Compare = std::less<_Key>,
            typename _Alloc = std::allocator<std::pair<const _Key, _Tp> > >
  class Map
  {
  public:
    typedef _Key                                          key_type;
    typedef _Tp                                           mapped_type;
    typedef std::pair<const _Key, _Tp>                    value_type;
    typedef std::less<_Key>                               key_compare;
    typedef _Alloc                                        allocator_type;
    typedef size_t                                        size_type;

  private:
    TPW::Connection& conn_;
    TPW::Space space_;

    struct TupleProxy
    {
      TupleProxy(TPW::Connection& conn,
        const TPW::Space& space,
        _Key key)
        : conn_(conn), space_(space), key_(key)
      {
      }
      
      
      void /*mapped_type &*/
      operator=(const _Tp &r)
      {
        conn_.replace(space_) << key_ << r << TPW::ENDR;
        area_ = r;
      }

      operator const _Tp& () const
      {
        return area_;
      }

      _Tp&
      get()
      {
        return area_;
      }
      
    private:
      _Tp area_;
      TPW::Connection& conn_;
      const TPW::Space& space_;
      _Key key_;
    };

    // not implemented yes
    struct iterator
    {
      iterator(TPW::Connection& conn, const TPW::Space& space)
      {
        std::string pairs = "box.space." + space.name() + ":pairs";
        conn.write( conn.call(pairs.c_str() )).read();
      }
    };

  public:
    Map(TPW::Connection& conn, const std::string& space)
      : conn_(conn), space_(conn.get_space(space))
    {
    }

    TupleProxy
    operator[](const key_type& k)
    {
      TupleProxy mt(conn_, space_, k);
      
      conn_.select(space_) <<  k << TPW::ENDR >> TPW::SKIP >> mt.get();

      return mt;
    }

    iterator
    begin()
    {
      assert(!"not implemented");
      return iterator(conn_, space_);
    }

    size_type
    size()
    {
      std::string len_query = "box.space." + space_.name() + ":len";
      uint64_t len_result;
      conn_.call(len_query) << TPW::ENDR >> len_result;
      return len_result;
    }
      
    bool
    empty()
    {
      return !size();
    }
  };
}

    
