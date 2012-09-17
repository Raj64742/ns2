
int dgtree_pkt::offset_;
static class DGTreeHeaderClass : public PacketHeaderClass {
public:
	DGTreeHeaderClass() : PacketHeaderClass("PacketHeader/DGTree",sizeof(hdr_dgtree_pkt)){
		bind_offset(&hdr_dgtree_pkt::offset_);

	}


}class_DGTree_hdr;


