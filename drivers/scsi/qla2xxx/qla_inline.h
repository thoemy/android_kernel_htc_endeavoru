/*
 * QLogic Fibre Channel HBA Driver
 * Copyright (c)  2003-2011 QLogic Corporation
 *
 * See LICENSE.qla2xxx for copyright and licensing details.
 */

/*
 * qla2x00_debounce_register
 *      Debounce register.
 *
 * Input:
 *      port = register address.
 *
 * Returns:
 *      register value.
 */
static __inline__ uint16_t
qla2x00_debounce_register(volatile uint16_t __iomem *addr)
{
	volatile uint16_t first;
	volatile uint16_t second;

	do {
		first = RD_REG_WORD(addr);
		barrier();
		cpu_relax();
		second = RD_REG_WORD(addr);
	} while (first != second);

	return (first);
}

static inline void
qla2x00_poll(struct rsp_que *rsp)
{
	unsigned long flags;
	struct qla_hw_data *ha = rsp->hw;
	local_irq_save(flags);
	if (IS_QLA82XX(ha))
		qla82xx_poll(0, rsp);
	else
		ha->isp_ops->intr_handler(0, rsp);
	local_irq_restore(flags);
}

static inline uint8_t *
host_to_fcp_swap(uint8_t *fcp, uint32_t bsize)
{
       uint32_t *ifcp = (uint32_t *) fcp;
       uint32_t *ofcp = (uint32_t *) fcp;
       uint32_t iter = bsize >> 2;

       for (; iter ; iter--)
               *ofcp++ = swab32(*ifcp++);

       return fcp;
}

static inline int
qla2x00_is_reserved_id(scsi_qla_host_t *vha, uint16_t loop_id)
{
	struct qla_hw_data *ha = vha->hw;
	if (IS_FWI2_CAPABLE(ha))
		return (loop_id > NPH_LAST_HANDLE);

	return ((loop_id > ha->max_loop_id && loop_id < SNS_FIRST_LOOP_ID) ||
	    loop_id == MANAGEMENT_SERVER || loop_id == BROADCAST);
}

static inline void
qla2x00_clean_dsd_pool(struct qla_hw_data *ha, srb_t *sp)
{
	struct dsd_dma *dsd_ptr, *tdsd_ptr;

	/* clean up allocated prev pool */
	list_for_each_entry_safe(dsd_ptr, tdsd_ptr,
	    &((struct crc_context *)sp->ctx)->dsd_list, list) {
		dma_pool_free(ha->dl_dma_pool, dsd_ptr->dsd_addr,
		    dsd_ptr->dsd_list_dma);
		list_del(&dsd_ptr->list);
		kfree(dsd_ptr);
	}
	INIT_LIST_HEAD(&((struct crc_context *)sp->ctx)->dsd_list);
}

static inline void
qla2x00_set_fcport_state(fc_port_t *fcport, int state)
{
	int old_state;

	old_state = atomic_read(&fcport->state);
	atomic_set(&fcport->state, state);

	/* Don't print state transitions during initial allocation of fcport */
	if (old_state && old_state != state) {
		ql_dbg(ql_dbg_disc, fcport->vha, 0x207d,
		    "FCPort state transitioned from %s to %s - "
		    "portid=%02x%02x%02x.\n",
		    port_state_str[old_state], port_state_str[state],
		    fcport->d_id.b.domain, fcport->d_id.b.area,
		    fcport->d_id.b.al_pa);
	}
}

static inline int
qla2x00_hba_err_chk_enabled(srb_t *sp)
{
	/*
	 * Uncomment when corresponding SCSI changes are done.
	 *
	if (!sp->cmd->prot_chk)
		return 0;
	 *
	 */

	switch (scsi_get_prot_op(sp->cmd)) {
	case SCSI_PROT_READ_STRIP:
	case SCSI_PROT_WRITE_INSERT:
		if (ql2xenablehba_err_chk >= 1)
			return 1;
		break;
	case SCSI_PROT_READ_PASS:
	case SCSI_PROT_WRITE_PASS:
		if (ql2xenablehba_err_chk >= 2)
			return 1;
		break;
	case SCSI_PROT_READ_INSERT:
	case SCSI_PROT_WRITE_STRIP:
		return 1;
	}
	return 0;
}
