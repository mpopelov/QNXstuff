/*
 * 2005 (c) -={ MikeP }=-
 */





#ifndef _QXC_LOGGING_H_
#define	_QXC_LOGGING_H_


int log_parse_pkt( qx_log_t *pkt_parsed, uint8_t *pkt_raw, int pkt_raw_len, int pkt_dir );


int log_init( int num_slots );
void log_destroy();
void log_swap_log(int *rgn, int *cnt);
int log_store_pkt( qx_log_t *pkt );



#endif /* _QXC_LOGGING_H_ */
