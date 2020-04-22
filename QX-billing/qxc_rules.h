/*
 * 2005 (c) -={ MikeP }=-
 */


#ifndef	_QXC_RULES_H_
#define	_QXC_RULES_H_

#include	"qx_rules.h"


/*
 * Tax exempt rules.
 * There are supposed to be a few so we maintain a queue of records.
 */
#define qxc_rule_exempt_t qx_rule_exempt_t


/*
 * Client access rules.
 * There supposed to be great ammounts of rules so we maintain
 * bi-tree.
 */
typedef struct _qxc_rule_nics qxc_rule_nics_t;
struct _qxc_rule_nics{
	qx_rule_nics_t	rule;
	qxc_rule_nics_t	*left;
	qxc_rule_nics_t	*right;
};



/* prototypes of functions that deal with rules matching */
int rule_match_exempt( qx_log_t *pkt );
int rule_match_client( qx_log_t *pkt );


int rule_add_exempt(qx_rule_exempt_t *rule);
int rule_add_nic(qx_rule_nics_t *rule);
int rule_init_lists( int ex_cnt, int nc_cnt );
void rule_swap_lists();



/*
 * This defines return codes of rule_match_exempt(...) function
 */
#define	RET_RME_NOEXEMPT	0
#define RET_RME_ACTIVEONLY	1
#define RET_RME_EVERYONE	2

#endif /* _QXC_RULES_H_ */
