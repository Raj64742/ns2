/* 
   imep_rt.cc
   $Id

   Routing Protocol (or more generally, ULP) Specific Functions
   */

#include <imep/imep.h>
#include <tora/tora_packet.h>
#include <packet.h>

// ======================================================================
// computes the length of a TORA header, copies the header to "dst"
// and returns the length of the header in "length".

int
imepAgent::toraHeaderLength(struct hdr_tora *t)
{
	switch(t->th_type) {
	case TORATYPE_QRY:
		return sizeof(struct hdr_tora_qry);
		break;
	case TORATYPE_UPD:
		return sizeof(struct hdr_tora_upd);
		break;
	case TORATYPE_CLR:
		return sizeof(struct hdr_tora_clr);
		break;
	default:
		abort();
	}
}

void
imepAgent::toraExtractHeader(Packet *p, char* dst)
{
  struct hdr_tora *t = HDR_TORA(p);
  int length = toraHeaderLength(t);
  bcopy((char*) t, dst, length);

  switch(t->th_type) {
  case TORATYPE_QRY:
    stats.qry_objs_created++;
    break;
  case TORATYPE_UPD:
    stats.upd_objs_created++;
    break;
  case TORATYPE_CLR:
    stats.clr_objs_created++;
    break;
  default:
    abort();
  }
}

void
imepAgent::toraCreateHeader(Packet *p, char *src, int length)
{
  struct hdr_cmn *ch = HDR_CMN(p);
  struct hdr_tora *t = HDR_TORA(p);

  ch->size() = IP_HDR_LEN + length;
  ch->ptype() = PT_TORA;

  bcopy(src, (char*) t, length);

  switch(t->th_type) {
  case TORATYPE_QRY:
    stats.qry_objs_recvd++;
    break;
  case TORATYPE_UPD:
    stats.upd_objs_recvd++;
    break;
  case TORATYPE_CLR:
    stats.clr_objs_recvd++;
    break;
  default:
    abort();
  }
}
