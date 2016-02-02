#ifndef TP_EXT_H_
#define TP_EXT_H_

//////////////////////////////////////////////////////
//  Missed tp methods
//////////////////////////////////////////////////////

static inline ssize_t
tp_reqbuf(const char *buf, size_t size)
{
  const ssize_t header_size = 5;
  
	if (tpunlikely(size < header_size))
		return header_size - size;

	/* len */
	if (mp_typeof(*buf) != MP_UINT)
  {
    /* wrong data*/
		return -1;
  }

	ssize_t len = mp_decode_uint(&buf);
	return header_size + len - size;
}

static inline ssize_t
tp_req(struct tp *p)
{
  return tp_reqbuf(p->s, tp_used(p));
}

static inline ssize_t
tp_recv2(struct tnt_stream_net *s, struct tp *p, size_t to_recv)
{
  // ssize_t received = tb_sesrecv(s, p->p, to_recv, false);
  ssize_t received = tnt_io_recv_raw(s, p->p, to_recv, false);
  p->p += received;
  return received;
}

#endif // TP_EXT_H_

