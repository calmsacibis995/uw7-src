#ifndef _STATE_TABLE_H
#define _STATE_TABLE_H
#ident	"@(#)state_table.h	1.2"

void snd_cnf_req(proto_hdr_t *, db_t *),
	snd_trm_req(proto_hdr_t *, db_t *),
	snd_trm_ack(proto_hdr_t *, db_t *),
	snd_cnf_ack(proto_hdr_t *, db_t *),
	snd_cnf_rqack(proto_hdr_t *, db_t *),
	snd_cnf_nak(proto_hdr_t *, db_t *),
	snd_cnf_rqnak(proto_hdr_t *, db_t *),
	snd_cd_rej(proto_hdr_t *, db_t *),
	snd_echo_rply(proto_hdr_t *, db_t *);

void init_rst_cnf_cnt(proto_hdr_t *, db_t *),
	init_rst_trm_cnt(proto_hdr_t *, db_t *),
	zero_restart_cnt(proto_hdr_t *, db_t *),
	illegal_event(proto_hdr_t *, db_t *);

struct state_tbl_ent	ppp_state_tbl[PPP_EVENTS][PPP_STATES] =
{
#define no_act	0,0,0
#define illegal	0,illegal_event,0


	/* UP = lower layer is Up:		0   */
	/* INITIAL */	no_act, CLOSED,
	/* STARTING */	snd_cnf_req, init_rst_cnf_cnt, 0, REQSENT,
	/* CLOSED */	illegal, CLOSED,
	/* STOPPED */	illegal, STOPPED,
	/* CLOSING */	illegal, CLOSING,
	/* STOPPING */	illegal, STOPPING,
	/* REQSENT */	illegal, REQSENT,
	/* ACKRCVD */	illegal, ACKRCVD,
	/* ACKSENT */	illegal, ACKSENT,
	/* OPENED */	illegal, OPENED,

	/* DOWN = lower layer is Down:			1 */
	/* INITIAL*/	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	no_act, INITIAL,
	/* STOPPED */	0, 0, tls, STARTING,
	/* CLOSING */	no_act, INITIAL,
	/* STOPPING */	no_act, STARTING,
	/* REQSENT */	no_act, STARTING,
	/* ACKRCVD */	no_act, STARTING,
	/* ACKSENT */	no_act, STARTING,
	/* OPENED */	0, 0, tld, STARTING,

	/* OPEN = administrative Open:			2 */
	/* INITIAL */	0, 0, tls, STARTING,
	/* STARTING */	no_act, STARTING,
	/* CLOSED */	snd_cnf_req, init_rst_cnf_cnt, 0, REQSENT,
	/* STOPPED */	0, 0, restrt, STOPPED,
	/* CLOSING */	0, 0, restrt, STOPPING,
	/* STOPPING */	0, 0, restrt, STOPPING,
	/* REQSENT */	no_act, REQSENT,
	/* ACKRCVD */	no_act, ACKRCVD,
	/* ACKSENT */	no_act, ACKSENT,
	/* OPENED */	0, 0, restrt, OPENED,

	/* CLOSE = administrative Close:		3 */
	/* INITIAL */	no_act, INITIAL,
	/* STARTING */	no_act, INITIAL,
	/* CLOSED */	no_act, CLOSED,
	/* STOPPED */	no_act, CLOSED,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, CLOSING,
	/* REQSENT */	snd_trm_req, init_rst_trm_cnt, 0, CLOSING,
	/* ACKRCVD */	snd_trm_req, init_rst_trm_cnt, 0, CLOSING,
	/* ACKSENT */	snd_trm_req, init_rst_trm_cnt, 0, CLOSING,
	/* OPENED */	snd_trm_req, init_rst_trm_cnt, tld, CLOSING,

	/* TO_P = Timeout with counter > 0:		4 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	illegal, CLOSED,
	/* STOPPED */	illegal, STOPPED,
	/* CLOSING */	snd_trm_req, 0, 0, CLOSING,
	/* STOPPING */	snd_trm_req, 0, 0, STOPPING,
	/* REQSENT */	snd_cnf_req, 0, 0, REQSENT,
	/* ACKRCVD */	snd_cnf_req, 0, 0, REQSENT,
	/* ACKSENT */	snd_cnf_req, 0, 0, ACKSENT,
	/* OPENED */	illegal, OPENED,

	/* TO_M = Timeout with counter expired:		5 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	illegal, CLOSED,
	/* STOPPED */	illegal, STOPPED,
	/* CLOSING */	0, 0, tlf, CLOSED,
	/* STOPPING */	0, 0, tlf, STOPPED,
	/* REQSENT */	0, 0, tlf, STOPPED,
	/* ACKRCVD */	0, 0, tlf, STOPPED,
	/* ACKSENT */	0, 0, tlf, STOPPED,
	/* OPENED */	illegal, OPENED,

	/* RCR_P = Receive-Configure-Request (Good):	6 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_trm_ack, 0, 0, CLOSED,
	/* STOPPED */	snd_cnf_rqack, init_rst_cnf_cnt, 0, ACKSENT,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	snd_cnf_ack, 0, 0, ACKSENT,
	/* ACKRCVD */	snd_cnf_ack, 0, tlu, OPENED,
	/* ACKSENT */	snd_cnf_ack, 0, 0, ACKSENT,
	/* OPENED */	snd_cnf_rqack, 0, /*tld*/0, ACKSENT,

	/* RCR_M = Receive-Configure-Request (Bad):	7 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_trm_ack, 0, 0, CLOSED,
	/* STOPPED */	snd_cnf_rqnak, init_rst_cnf_cnt, 0, REQSENT,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	snd_cnf_nak, 0, 0, REQSENT,
	/* ACKRCVD */	snd_cnf_nak, 0, 0, ACKRCVD,
	/* ACKSENT */	snd_cnf_nak, 0, 0, REQSENT,
	/* OPENED */	snd_cnf_rqnak, 0, /*tld*/0, REQSENT,

	/* RCA = Receive-Configure-Ack:			8 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_trm_ack, 0, 0, CLOSED,
	/* STOPPED */	snd_trm_ack, 0, 0, STOPPED,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	0, init_rst_cnf_cnt, 0, ACKRCVD,
	/* ACKRCVD */	snd_cnf_req, 0, cross, REQSENT,
	/* ACKSENT */	0, init_rst_cnf_cnt, tlu, OPENED,
	/* OPENED */	snd_cnf_req, 0, c_tld, REQSENT,

	/* RCN = Receive-Configure-Nak/Rej:		9 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_trm_ack, 0, 0, CLOSED,
	/* STOPPED */	snd_trm_ack, 0, 0, STOPPED,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	snd_cnf_req, init_rst_cnf_cnt, 0, REQSENT,
	/* ACKRCVD */	snd_cnf_req, 0, cross, REQSENT,
	/* ACKSENT */	snd_cnf_req, init_rst_cnf_cnt, 0, ACKSENT,
	/* OPENED */	snd_cnf_req, 0, /*c_tld*/cross, REQSENT,

	/* RTR = Receive-Terminate-Request:		10 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_trm_ack, 0, 0, CLOSED,
	/* STOPPED */	snd_trm_ack, 0, 0, STOPPED,
	/* CLOSING */	snd_trm_ack, 0, 0, CLOSING,
	/* STOPPING */	snd_trm_ack, 0, 0, STOPPING,
	/* REQSENT */	snd_trm_ack, 0, 0, REQSENT,
	/* ACKRCVD */	snd_trm_ack, 0, 0, REQSENT,
	/* ACKSENT */	snd_trm_ack, 0, 0, REQSENT,
	/* OPENED */	snd_trm_ack, zero_restart_cnt, tld, STOPPING,

	/* RTA = Receive-Terminate-Ack:			11 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	no_act, CLOSED,
	/* STOPPED */	no_act, STOPPED,
	/* CLOSING */	0, 0, tlf, CLOSED,
	/* STOPPING */	0, 0, tlf, CLOSED,
	/* REQSENT */	no_act, REQSENT,
	/* ACKRCVD */	no_act, REQSENT,
	/* ACKSENT */	no_act, ACKSENT,
	/* OPENED */	snd_cnf_req, 0, tld, REQSENT,

	/* RUC = Receive-Unknown-Code:			12 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	snd_cd_rej, 0, 0, CLOSED,
	/* STOPPED */	snd_cd_rej, 0, 0, CLOSED,
	/* CLOSING */	snd_cd_rej, 0, 0, CLOSING,
	/* STOPPING */	snd_cd_rej, 0, 0, STOPPING,
	/* REQSENT */	snd_cd_rej, 0, 0, REQSENT,
	/* ACKRCVD */	snd_cd_rej, 0, 0, ACKRCVD,
	/* ACKSENT */	snd_cd_rej, 0, 0, ACKSENT,
	/* OPENED */	snd_cd_rej, 0, 0, OPENED,

	/* RXJ_P = Receive-Code-Reject (permitted):	13 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	no_act, CLOSED,
	/* STOPPED */	no_act, STOPPED,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	no_act, REQSENT,
	/* ACKRCVD */	no_act, REQSENT,
	/* ACKSENT */	no_act, ACKSENT,
	/* OPENED */	no_act, OPENED,

	/* RXJ_M = Receive-Code-Reject (catastrophic):	14 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	0, 0, tlf, CLOSED,
	/* STOPPED */	0, 0, tlf, STOPPED,
	/* CLOSING */	0, 0, tlf, CLOSED,
	/* STOPPING */	0, 0, tlf, STOPPED,
	/* REQSENT */	0, 0, tlf, STOPPED,
	/* ACKRCVD */	0, 0, tlf, STOPPED,
	/* ACKSENT */	0, 0, tlf, STOPPED,
	/* OPENED */	snd_trm_req, init_rst_trm_cnt, tld, STOPPING,

	/* RXR = Receive-Echo-Request:			15 */
	/* INITIAL */	illegal, INITIAL,
	/* STARTING */	illegal, STARTING,
	/* CLOSED */	no_act, CLOSED,
	/* STOPPED */	no_act, STOPPED,
	/* CLOSING */	no_act, CLOSING,
	/* STOPPING */	no_act, STOPPING,
	/* REQSENT */	no_act, REQSENT,
	/* ACKRCVD */	no_act, ACKRCVD,
	/* ACKSENT */	no_act, ACKSENT,
	/* OPENED */	snd_echo_rply, 0, 0, OPENED,
};
#endif /*_STATE_TABLE_H*/
