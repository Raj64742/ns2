//  Definition of the scoreboard class:

#define SBSIZE 1024

class ScoreBoard {
  public:
	ScoreBoard() {first_=0; length_=0;}
	int IsEmpty () {return (length_ == 0);}
	void ClearScoreBoard (); 
	int GetNextRetran ();
	void MarkRetran (int retran_seqno);
	void UpdateScoreBoard (int last_ack_, Packet *pkt);
	
  protected:
        int first_, length_;
        struct ScoreBoardNode {
		int seq_no_;		/* Packet number */
		int ack_flag_;		/* Acked by cumulative ACK */
		int sack_flag_;		/* Acked by SACK block */
		int retran_;		/* Packet retransmitted */
	} SBN[SBSIZE+1];
};

