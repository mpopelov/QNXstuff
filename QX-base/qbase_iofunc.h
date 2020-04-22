#ifndef _QBASE_IOFUNC_H_
#define _QBASE_IOFUNC_H_


/*
 * as we need to store information about each connection
 * to database - the best place is to store it in
 * Open Control Block - ocb
 * That's why we need to redefine these two functions
 */
IOFUNC_OCB_T * Qbase_OCB_calloc( resmgr_context_t * ctp, IOFUNC_ATTR_T *attr);
void Qbase_OCB_free( IOFUNC_OCB_T * ocb );
int Qbase_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb);
int Qbase_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);
int Qbase_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *o);

#endif /* _QBASE_IOFUNC_H_ */
