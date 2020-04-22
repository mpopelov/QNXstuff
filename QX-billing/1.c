		fprintf(logfd,"rx_up: _packet_ (dlc: %hx; ip_c: %x; ip_h: %x; ip_p: %hx; p_c: %hx; p_h: %hx;)\n", \
		pkt_parsed.DLC_proto,pkt_parsed.IP_clnt,pkt_parsed.IP_host,pkt_parsed.IP_proto,pkt_parsed.Port_clnt,pkt_parsed.Port_host );

