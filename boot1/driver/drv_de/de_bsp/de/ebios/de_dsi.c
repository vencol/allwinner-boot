#include "ebios_lcdc_tve.h"
#include "de_dsi.h"

static volatile __de_dsi_dev_t *dsi_dev[1];
static volatile __de_dsi_dphy_dev_t *dphy_dev[1];

__u32 const dsi_pixel_bits[4] = {24,24,18,16};
__u32 const dsi_lane_den[4] = {0x1,0x3,0x7,0xf};

__s32 dsi_set_reg_base(__u32 sel, __u32 base)
{
	dsi_dev[sel]=(__de_dsi_dev_t *)base;
	dphy_dev[sel]=(__de_dsi_dphy_dev_t *)(base+0x1000);
	return 0;
}

__u32 dsi_get_reg_base(__u32 sel)
{
	return (__u32)dsi_dev[sel];
}

__u32 dsi_dphy_get_reg_base(__u32 sel)
{
	return (__u32)dphy_dev[sel];
}


__u32 dsi_get_start_delay(__u32 sel)
{
	__u32 start_delay = dsi_dev[sel]->dsi_basic_ctl1.bits.video_start_delay;
	__u32 vt = dsi_dev[sel]->dsi_basic_size1.bits.vt;
	__u32 vsa = dsi_dev[sel]->dsi_basic_size0.bits.vsa;	
	__u32 vbp = dsi_dev[sel]->dsi_basic_size0.bits.vbp;	
	__u32 vfp = vt-vsa-vbp;	
	return start_delay+vfp;
}


__u32 dsi_get_cur_line(__u32 sel)
{
 	__u32 curr_line = dsi_dev[sel]->dsi_debug_video0.bits.video_curr_line;
	__u32 vt = dsi_dev[sel]->dsi_basic_size1.bits.vt;
	__u32 vsa = dsi_dev[sel]->dsi_basic_size0.bits.vsa;	
	__u32 vbp = dsi_dev[sel]->dsi_basic_size0.bits.vbp;	
	__u32 vfp = vt-vsa-vbp;	
	curr_line += vfp;
	if(curr_line>vt)
		curr_line -= vt;
    return curr_line;
}

__s32 dsi_irq_enable(__u32 sel, __dsi_irq_id_t id)
{
	dsi_dev[sel]->dsi_gint0.bits.dsi_irq_en |= (1<<id);
    return 0;
}

__s32 dsi_irq_disable(__u32 sel, __dsi_irq_id_t id)
{
	dsi_dev[sel]->dsi_gint0.bits.dsi_irq_en &= ~(1<<id);
    return 0;
}

__u32 dsi_irq_query(__u32 sel,__dsi_irq_id_t id)
{
	__u32 en,fl;
	en = dsi_dev[sel]->dsi_gint0.bits.dsi_irq_en;
	fl = dsi_dev[sel]->dsi_gint0.bits.dsi_irq_flag;
	if(en & fl & (1<<id))
	{
		dsi_dev[sel]->dsi_gint0.bits.dsi_irq_flag |= (1<<id);
		return 1;
	}
	else
	{
		return 0;
	}
}

__s32 dsi_start(__u32 sel,__dsi_start_t func)
{
	switch(func)
	{
		case DSI_START_TBA:
			dsi_dev[sel]->dsi_inst_jump_sel.dwval = DSI_INST_ID_TBA  <<(4*DSI_INST_ID_LP11)
												  | DSI_INST_ID_END  <<(4*DSI_INST_ID_TBA);
			break;
		case DSI_START_HSTX:
			dsi_dev[sel]->dsi_inst_jump_sel.dwval = DSI_INST_ID_END  <<(4*DSI_INST_ID_HSCEXIT)
												  |	DSI_INST_ID_HSC  <<(4*DSI_INST_ID_LP11)
												  | DSI_INST_ID_NOP  <<(4*DSI_INST_ID_HSC)
												  | DSI_INST_ID_HSD  <<(4*DSI_INST_ID_NOP)
												  | DSI_INST_ID_DLY  <<(4*DSI_INST_ID_HSD)
												  | DSI_INST_ID_NOP  <<(4*DSI_INST_ID_DLY);
			break;
		case DSI_START_LPTX:
			dsi_dev[sel]->dsi_inst_jump_sel.dwval = DSI_INST_ID_LPDT <<(4*DSI_INST_ID_LP11)
												  | DSI_INST_ID_END  <<(4*DSI_INST_ID_LPDT);
			break;
		case DSI_START_LPRX:
			dsi_dev[sel]->dsi_inst_jump_sel.dwval = DSI_INST_ID_LPDT <<(4*DSI_INST_ID_LP11)
												  | DSI_INST_ID_DLY  <<(4*DSI_INST_ID_LPDT)
										  	  	  | DSI_INST_ID_TBA  <<(4*DSI_INST_ID_DLY)
												  | DSI_INST_ID_END  <<(4*DSI_INST_ID_TBA);
			break;
		default:
			dsi_dev[sel]->dsi_inst_jump_sel.dwval = DSI_INST_ID_END  <<(4*DSI_INST_ID_LP11);
			break;
	}
	dsi_dev[sel]->dsi_basic_ctl0.bits.inst_st = 0;
	dsi_dev[sel]->dsi_basic_ctl0.bits.inst_st = 1;
	return 0;
}




__s32 dsi_open(__u32 sel,__panel_para_t * panel)
{
	if(panel->lcd_fresh_mode==0)
	{
		dsi_irq_enable(sel,DSI_IRQ_VIDEO_VBLK);
		dsi_start(sel,DSI_START_HSTX);
	}
	
	return 0;
}

__s32 dsi_close(__u32 sel)
{
	dsi_irq_disable(sel,DSI_IRQ_VIDEO_VBLK);
	dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_en = 1;	
	while(dsi_dev[sel]->dsi_basic_ctl0.bits.inst_st == 1);		
	return 0;
}

__s32 dsi_tri_start(__u32 sel)
{
	dsi_start(sel,DSI_START_HSTX);
/*	{
		__u32 dbg = 0xcc;
		__inf("infinit loop\n");
		while(dbg);
	}*/
	while(dsi_dev[sel]->dsi_debug_inst.bits.curr_instru_num != DSI_INST_ID_HSD);
//	while(dphy_dev[sel]->dphy_dbg0.bits.lptx_sta_d0 != 5);
	return 0;
}

void DSI_delay_ms(__u32 ms) 
{
#ifdef __LINUX_OSAL__
    __u32 timeout = ms*HZ/1000;
    
    set_current_state(TASK_INTERRUPTIBLE);
    schedule_timeout(timeout);
#endif
#ifdef __BOOT_OSAL__
    wBoot_timer_delay(ms);//assume cpu runs at 1000Mhz,10 clock one cycle
#endif
}


__s32 dsi_dcs_wr(__u32 sel,__u8 cmd,__u8* para_p,__u32 para_num)
{
	volatile __u8*  p = (__u8*)dsi_dev[sel]->dsi_cmd_tx;
	while(dsi_dev[sel]->dsi_basic_ctl0.bits.inst_st == 1);
	if(para_num==0)
	{

		*(p++) = DSI_DT_DCS_WR_P0;
		*(p++) = cmd;
		*(p++) = 0x00;
	}
	else if(para_num==1)
	{
		*(p++) = DSI_DT_DCS_WR_P1;
		*(p++) = cmd;
		*(p++) = *para_p;
	}
	else
	{
		*(p++) = DSI_DT_DCS_LONG_WR;
		*(p++) = (para_num+1)>>0 & 0xff;
		*(p++) = (para_num+1)>>8 & 0xff;
	}
	*(p++) = dsi_ecc_pro(dsi_dev[sel]->dsi_cmd_tx[0].dwval);

	if(para_num>1)
	{
		__u16 crc,i;
		*(p++) = cmd;
		for(i=0;i<para_num;i++)
		{
			*(p++)  = *(para_p+i);
		}
		crc = dsi_crc_pro((__u8*)(dsi_dev[sel]->dsi_cmd_tx+1),para_num+1);
		*(p++) = (crc>>0) & 0xff;
		*(p++) = (crc>>8) & 0xff;
		dsi_dev[sel]->dsi_cmd_ctl.bits.tx_size = 4+1+para_num+2-1;
	}
	else
	{
		dsi_dev[sel]->dsi_cmd_ctl.bits.tx_size = 4-1;
	}
//	__inf("enter delay 5 s\n");
//	DSI_delay_ms(5000);
	dsi_start(sel,DSI_START_LPTX);
	return 0;
}


__s32 dsi_dcs_wr_index(__u32 sel,__u8 index)
{

	return 0;
}

__s32 dsi_dcs_wr_data(__u32 sel,__u8 data)
{

	return 0;
}

__s32 dsi_dcs_wr_0para(__u32 sel,__u8 cmd)
{
	__u8 tmp;
	dsi_dcs_wr(0,cmd,&tmp,0);
	return 0;
}

__s32 dsi_dcs_wr_1para(__u32 sel,__u8 cmd,__u8 para)
{
	__u8 tmp = para;
	dsi_dcs_wr(0,cmd,&tmp,1);
	return 0;
}

__s32 dsi_dcs_wr_2para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2)
{
	__u8 tmp[2];
	tmp[0] = para1;
	tmp[1] = para2;
	dsi_dcs_wr(0,cmd,tmp,2);
	return 0;
}

__s32 dsi_dcs_wr_3para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3)
{
	__u8 tmp[3];
	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	dsi_dcs_wr(0,cmd,tmp,3);
	return 0;
}
__s32 dsi_dcs_wr_4para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4)
{
	__u8 tmp[4];
	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	dsi_dcs_wr(0,cmd,tmp,4);
	return 0;
}

__s32 dsi_dcs_wr_5para(__u32 sel,__u8 cmd,__u8 para1,__u8 para2,__u8 para3,__u8 para4,__u8 para5)
{
	__u8 tmp[5];
	tmp[0] = para1;
	tmp[1] = para2;
	tmp[2] = para3;
	tmp[3] = para4;
	tmp[4] = para5;
	dsi_dcs_wr(0,cmd,tmp,5);
	return 0;
}

__s32 dsi_dcs_wr_memory(__u32 sel,__u32* p_data,__u32 length)
{
	__u32 tx_cntr=length;
	__u32 tx_num;
	__u32* tx_p=p_data;
	__u8 para[256];
	__u32 i;
	__u32 start=1;

	while(tx_cntr)
	{
		if(tx_cntr>=83)
			tx_num = 83;
		else
			tx_num = tx_cntr;
		tx_cntr -= tx_num;

		for(i=0;i<tx_num;i++)
		{
			para[i*3+0] = (*tx_p >> 16) & 0xff;
			para[i*3+1] = (*tx_p >> 8)  & 0xff;
			para[i*3+2] = (*tx_p >> 0)  & 0xff;
			tx_p++;
		}

		if(start)
		{
			dsi_dcs_wr(sel,DSI_DCS_WRITE_MEMORY_START,para,tx_num*3);
			start = 0;
		}
		else
		{
			dsi_dcs_wr(sel,DSI_DCS_WRITE_MEMORY_CONTINUE,para,tx_num*3);
		}
	}
	return 0;
}


__s32 dsi_set_max_ret_size(__u32 sel,__u32 size)
{
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte0 = DSI_DT_MAX_RET_SIZE;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte1 = (size>>0) & 0xff;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte2 = (size>>8) & 0xff;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte3 =  dsi_ecc_pro(dsi_dev[sel]->dsi_cmd_tx[0].dwval);
	dsi_dev[sel]->dsi_cmd_ctl.bits.tx_size = 4-1;
	dsi_start(sel,DSI_START_LPTX);
	return 0;
}

__s32 dsi_dcs_rd(__u32 sel,__u8 cmd,__u8* para_p,__u32* num_p)
{
	__u32 num,i;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte0 = DSI_DT_DCS_RD_P0;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte1 = cmd;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte2 = 0x00;
	dsi_dev[sel]->dsi_cmd_tx[0].bits.byte3 = dsi_ecc_pro(dsi_dev[sel]->dsi_cmd_tx[0].dwval);
	dsi_dev[sel]->dsi_cmd_ctl.bits.tx_size = 4-1;

	dsi_start(sel,DSI_START_LPRX);

	while(dsi_dev[sel]->dsi_basic_ctl0.bits.inst_st == 1);
	if(dsi_dev[sel]->dsi_cmd_ctl.bits.rx_flag)
	{
		if(dsi_dev[sel]->dsi_cmd_ctl.bits.rx_overflow)
			return -1;
		if(dsi_dev[sel]->dsi_cmd_rx[0].bits.byte0==DSI_DT_ACK_ERR)
			return -1;

		if(dsi_dev[sel]->dsi_cmd_rx[0].bits.byte0==DSI_DT_DCS_RD_R1)
		{
			num = 1;
			*(para_p+0) = dsi_dev[sel]->dsi_cmd_rx[0].bits.byte1;
		}
		else if(dsi_dev[sel]->dsi_cmd_rx[0].bits.byte0==(__u8)DSI_DT_DCS_RD_R2)
		{
			num = 2;
			*(para_p+0) = dsi_dev[sel]->dsi_cmd_rx[0].bits.byte1;
			*(para_p+1) = dsi_dev[sel]->dsi_cmd_rx[0].bits.byte2;
		}
		else if(dsi_dev[sel]->dsi_cmd_rx[0].bits.byte0==(__u8)DSI_DT_DCS_LONG_RD_R)
		{
			num = dsi_dev[sel]->dsi_cmd_ctl.bits.rx_size+1-6;
			for(i=0;i<num;i++)
			{
				*(para_p+i) = *((__u8*)dsi_dev[sel]->dsi_cmd_rx+4+i);
			}
		}
        else
        {
            num = 0;
            return -1;
        }
		*num_p = num;
	}

	return 0;
}


__s32 dsi_dcs_rd_memory(__u32 sel,__u32* p_data,__u32 length)
{
	__u32 rx_cntr=length;
	__u32 rx_curr;
	__u32* rx_p=p_data;
	__u32 i;
	__u32 start=1;

	__u8 rx_bf[32];
	__u32 rx_num;

	dsi_set_max_ret_size(sel,24);

	while(rx_cntr)
	{
		if(rx_cntr>=8)
		{
			rx_curr = 8;
		}
		else
		{
			rx_curr = rx_cntr;
			dsi_set_max_ret_size(sel,rx_curr*3);
		}
		rx_cntr -= rx_curr;

		if(start)
		{
			dsi_dcs_rd(sel,DSI_DCS_READ_MEMORY_START,rx_bf,&rx_num);
			start = 0;
		}
		else
		{
			dsi_dcs_rd(sel,DSI_DCS_READ_MEMORY_CONTINUE,rx_bf,&rx_num);
		}

		if(rx_num != rx_curr*3)
			return -1;

		for(i=0;i<rx_curr;i++)
		{
			*rx_p &= 0xff000000;
			*rx_p |= 0xff000000;
			*rx_p |= (__u32)*(rx_bf + 4 + i*3 + 0)<<16;
			*rx_p |= (__u32)*(rx_bf + 4 + i*3 + 1)<<8;
			*rx_p |= (__u32)*(rx_bf + 4 + i*3 + 2)<<0;
			rx_p++;
		}
	}
	return 0;
}

__s32 dsi_dphy_cfg(__u32 sel,__panel_para_t * panel)
{
	__disp_dsi_dphy_timing_t* dphy_timing_p;
	__disp_dsi_dphy_timing_t dphy_timing_cfg1 = {
        14, //lp_clk_div        100ns
        6,  //hs_prepare        70ns
        10, //hs_trail          100ns
        7,  //clk_prepare       70ns
        50, //clk_zero          300ns
        3,  //clk_pre
        10, //clk_post          400*6.734 for nop inst  2.5us
        30, //clk_trail         200ns
        0,  //hs_dly_mode
        10, //hs_dly
        3,  //lptx_ulps_exit
        3,  //hstx_ana1
        3,  //hstx_ana0
	};	

	dphy_dev[sel]->dphy_gctl.bits.module_en = 0;
	dphy_dev[sel]->dphy_gctl.bits.lane_num = panel->lcd_dsi_lane-1;

	dphy_dev[sel]->dphy_tx_ctl.bits.hstx_clk_cont = 1;

//	if(panel->lcd_dsi_dphy_timing_en==2)
	{
		dphy_timing_p = &dphy_timing_cfg1;
	}
//	else
	{
//		dphy_timing_p = panel->lcd_dsi_dphy_timing_p;
	}

	dphy_dev[sel]->dphy_tx_time0.bits.lpx_tm_set = dphy_timing_p->lp_clk_div;
	dphy_dev[sel]->dphy_tx_time0.bits.hs_pre_set = dphy_timing_p->hs_prepare;
	dphy_dev[sel]->dphy_tx_time0.bits.hs_trail_set = dphy_timing_p->hs_trail;
	dphy_dev[sel]->dphy_tx_time1.bits.ck_prep_set = dphy_timing_p->clk_prepare;
	dphy_dev[sel]->dphy_tx_time1.bits.ck_zero_set = dphy_timing_p->clk_zero;
	dphy_dev[sel]->dphy_tx_time1.bits.ck_pre_set = dphy_timing_p->clk_pre;
	dphy_dev[sel]->dphy_tx_time1.bits.ck_post_set = dphy_timing_p->clk_post;
	dphy_dev[sel]->dphy_tx_time2.bits.ck_trail_set = dphy_timing_p->clk_trail;
	dphy_dev[sel]->dphy_tx_time2.bits.hs_dly_mode = 0;
	dphy_dev[sel]->dphy_tx_time2.bits.hs_dly_set = 0;
	dphy_dev[sel]->dphy_tx_time3.bits.lptx_ulps_exit_set = 0;
	dphy_dev[sel]->dphy_tx_time4.bits.hstx_ana0_set = 3;
	dphy_dev[sel]->dphy_tx_time4.bits.hstx_ana1_set = 3;

	dphy_dev[sel]->dphy_gctl.bits.module_en = 1;
	return 0;
}

__s32 DSI_delay_us(__u32 us)
{
    return 0;
}
__u32 dsi_io_open(__u32 sel,__panel_para_t * panel)
{
	dphy_dev[sel]->dphy_ana1.bits.reg_vttmode = 0;
	dphy_dev[sel]->dphy_ana0.bits.reg_selsck = 0;
	dphy_dev[sel]->dphy_ana0.bits.reg_pws = 0;//0	1
	dphy_dev[sel]->dphy_ana0.bits.reg_sfb = 2;//2	0
	dphy_dev[sel]->dphy_ana0.bits.reg_dmpc = 1;
	dphy_dev[sel]->dphy_ana0.bits.reg_dmpd = dsi_lane_den[panel->lcd_dsi_lane-1];
	dphy_dev[sel]->dphy_ana0.bits.reg_slv = 0x7;
	dphy_dev[sel]->dphy_ana0.bits.reg_den = dsi_lane_den[panel->lcd_dsi_lane-1];
	dphy_dev[sel]->dphy_ana1.bits.reg_svtt = 7;
	dphy_dev[sel]->dphy_ana4.bits.reg_ckdv = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_tmsc = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_tmsd = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_txdnsc = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_txdnsd = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_txpusc = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_txpusd = 0x1;	
	dphy_dev[sel]->dphy_ana4.bits.reg_dmplvc = 0x1;
	dphy_dev[sel]->dphy_ana4.bits.reg_dmplvd = dsi_lane_den[panel->lcd_dsi_lane-1];	

	dphy_dev[sel]->dphy_ana2.bits.enib = 1;
	DSI_delay_us(5);
	dphy_dev[sel]->dphy_ana3.bits.enldor = 1;
	dphy_dev[sel]->dphy_ana3.bits.enldoc = 1;
	dphy_dev[sel]->dphy_ana3.bits.enldod = 1;
	DSI_delay_us(1);
	dphy_dev[sel]->dphy_ana3.bits.envttc = 1;
	dphy_dev[sel]->dphy_ana3.bits.envttd = dsi_lane_den[panel->lcd_dsi_lane-1];
	DSI_delay_us(1);
	dphy_dev[sel]->dphy_ana3.bits.endiv = 1;
	DSI_delay_us(1);
	dphy_dev[sel]->dphy_ana2.bits.enck_cpu = 1;
	DSI_delay_us(1);	

    dphy_dev[sel]->dphy_ana1.bits.reg_vttmode = 1;
	dphy_dev[sel]->dphy_ana2.bits.enp2s_cpu = dsi_lane_den[panel->lcd_dsi_lane-1];

	return 0;	
}

__u32 dsi_io_close(__u32 sel)
{
	dphy_dev[sel]->dphy_ana2.bits.enp2s_cpu = 0;
    dphy_dev[sel]->dphy_ana1.bits.reg_vttmode = 0;

	dphy_dev[sel]->dphy_ana2.bits.enck_cpu = 0;
	DSI_delay_us(1);
	dphy_dev[sel]->dphy_ana3.bits.endiv = 0;
	DSI_delay_us(1);
	dphy_dev[sel]->dphy_ana3.bits.envttd = 0;	
	dphy_dev[sel]->dphy_ana3.bits.envttc = 0;
	DSI_delay_us(1);	
	dphy_dev[sel]->dphy_ana3.bits.enldod = 0;
	dphy_dev[sel]->dphy_ana3.bits.enldoc = 0;
	dphy_dev[sel]->dphy_ana3.bits.enldor = 0;	
	DSI_delay_us(5);	
	dphy_dev[sel]->dphy_ana2.bits.enib = 0;	
		
	dphy_dev[sel]->dphy_ana4.bits.reg_dmplvd = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_dmplvc = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_txpusd = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_txpusc = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_txdnsd = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_txdnsc = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_tmsd = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_tmsc = 0;	
	dphy_dev[sel]->dphy_ana4.bits.reg_ckdv = 0;	
	dphy_dev[sel]->dphy_ana1.bits.reg_svtt = 0;	
	dphy_dev[sel]->dphy_ana0.bits.reg_den = 0;	
	dphy_dev[sel]->dphy_ana0.bits.reg_slv = 0;	
	dphy_dev[sel]->dphy_ana0.bits.reg_dmpd = 0;	
	dphy_dev[sel]->dphy_ana0.bits.reg_dmpc = 0;	
	dphy_dev[sel]->dphy_ana0.bits.reg_sfb = 0;
    dphy_dev[sel]->dphy_ana0.bits.reg_pws = 0;
	dphy_dev[sel]->dphy_ana0.bits.reg_selsck = 0;	
	dphy_dev[sel]->dphy_ana1.bits.reg_vttmode = 0;	

	return 0;	
}


__s32 dsi_basic_cfg(__u32 sel,__panel_para_t * panel)
{

	if(panel->lcd_dsi_if==LCD_DSI_IF_VIDEO_MODE)
	{
		__s32 start_delay = panel->lcd_vt - panel->lcd_y -10;
        __u32 vfp = panel->lcd_vt - panel->lcd_y - panel->lcd_vbp;
        __u32 dsi_start_delay;
        
		if(start_delay<8)
			start_delay = 8;
		else if(start_delay>31)
			start_delay = 31;

        if(start_delay-vfp<1)
            dsi_start_delay = 1;
        else
            dsi_start_delay = start_delay - vfp;
        
		dsi_dev[sel]->dsi_basic_ctl0.bits.ecc_en = 1;
		dsi_dev[sel]->dsi_basic_ctl0.bits.crc_en = 1;
		dsi_dev[sel]->dsi_basic_ctl0.bits.hs_eotp_en = 0;	
		dsi_dev[sel]->dsi_basic_ctl1.bits.video_start_delay = dsi_start_delay;	
		dsi_dev[sel]->dsi_basic_ctl1.bits.video_precision_mode_align = 0;
		dsi_dev[sel]->dsi_basic_ctl1.bits.video_frame_start = 1;
		dsi_dev[sel]->dsi_basic_ctl1.bits.dsi_mode = 1;
		dsi_dev[sel]->dsi_trans_start.bits.trans_start_set = 10;
		dsi_dev[sel]->dsi_trans_zero.bits.hs_zero_reduce_set = 0;
	}
	else
	{
		dsi_dev[sel]->dsi_basic_ctl0.bits.ecc_en = 1;
		dsi_dev[sel]->dsi_basic_ctl0.bits.crc_en = 0;
		dsi_dev[sel]->dsi_basic_ctl0.bits.hs_eotp_en = 0;	
		dsi_dev[sel]->dsi_basic_ctl1.bits.dsi_mode = 0;
		dsi_dev[sel]->dsi_trans_start.bits.trans_start_set = 10;
		dsi_dev[sel]->dsi_trans_zero.bits.hs_zero_reduce_set = 0;
	}
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LP11].bits.instru_mode		= DSI_INST_MODE_STOP;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LP11].bits.lane_cen			= 1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LP11].bits.lane_den			= dsi_lane_den[panel->lcd_dsi_lane-1];
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_TBA].bits.instru_mode		= DSI_INST_MODE_TBA;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_TBA].bits.lane_cen			= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_TBA].bits.lane_den			= 0x1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSC].bits.instru_mode		= DSI_INST_MODE_HS;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSC].bits.trans_packet		= DSI_INST_PACK_PIXEL;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSC].bits.lane_cen			= 1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSC].bits.lane_den			= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSD].bits.instru_mode		= DSI_INST_MODE_HS;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSD].bits.trans_packet		= DSI_INST_PACK_PIXEL;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSD].bits.lane_cen			= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSD].bits.lane_den			= dsi_lane_den[panel->lcd_dsi_lane-1];
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LPDT].bits.instru_mode		= DSI_INST_MODE_ESCAPE;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LPDT].bits.escape_enrty		= DSI_INST_ESCA_LPDT;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LPDT].bits.trans_packet		= DSI_INST_PACK_COMMAND;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LPDT].bits.lane_cen			= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_LPDT].bits.lane_den			= 0x1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSCEXIT].bits.instru_mode	= DSI_INST_MODE_HSCEXIT;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSCEXIT].bits.lane_cen		= 1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_HSCEXIT].bits.lane_den		= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_NOP].bits.instru_mode		= DSI_INST_MODE_STOP;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_NOP].bits.lane_cen			= 0;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_NOP].bits.lane_den			= dsi_lane_den[panel->lcd_dsi_lane-1];
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_DLY].bits.instru_mode		= DSI_INST_MODE_NOP;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_DLY].bits.lane_cen			= 1;
	dsi_dev[sel]->dsi_inst_func[DSI_INST_ID_DLY].bits.lane_den			= dsi_lane_den[panel->lcd_dsi_lane-1];
	dsi_dev[sel]->dsi_inst_loop_sel.dwval = 2<<(4*DSI_INST_ID_LP11)
										  | 3<<(4*DSI_INST_ID_DLY);
	dsi_dev[sel]->dsi_inst_loop_num.bits.loop_n0 = 50;
	dsi_dev[sel]->dsi_inst_loop_num.bits.loop_n1 = 50;

	if(panel->lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE)
	{
		dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_en = 0;
		dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_num = 1;
	}
	else
	{
		dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_en = 1;
		dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_num = panel->lcd_y;
	}
	dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_point = DSI_INST_ID_NOP;
	dsi_dev[sel]->dsi_inst_jump_cfg[0].bits.jump_cfg_to = DSI_INST_ID_HSCEXIT;
	dsi_dev[sel]->dsi_debug_data.bits.test_data = 0xff;
	dsi_dev[sel]->dsi_gctl.bits.dsi_en = 1;
	return 0;
}

__s32 dsi_packet_cfg(__u32 sel,__panel_para_t * panel)
{
	if(panel->lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE)
	{
		dsi_dev[sel]->dsi_pixel_ctl0.bits.pd_plug_dis = 1;
		dsi_dev[sel]->dsi_pixel_ph.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_pixel_ph.bits.wc = panel->lcd_x*dsi_pixel_bits[panel->lcd_dsi_format]/8;
        dsi_dev[sel]->dsi_pixel_ph.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_pixel_ph.dwval);
		dsi_dev[sel]->dsi_pixel_pf0.bits.crc_force = 0xffff;
		dsi_dev[sel]->dsi_pixel_pf1.bits.crc_init_line0 = 0xffff;
		dsi_dev[sel]->dsi_pixel_pf1.bits.crc_init_linen = 0xffff;
		dsi_dev[sel]->dsi_pixel_ctl0.bits.pixel_format = 8+panel->lcd_dsi_format;
		dsi_dev[sel]->dsi_pixel_ph.bits.dt = DSI_DT_PIXEL_RGB888 - 0x10*panel->lcd_dsi_format;
	}
	else
	{
		dsi_dev[sel]->dsi_pixel_ctl0.bits.pd_plug_dis = 0;
		dsi_dev[sel]->dsi_pixel_ph.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_pixel_ph.bits.dt = DSI_DT_DCS_LONG_WR;
        dsi_dev[sel]->dsi_pixel_ph.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_pixel_ph.dwval);
        dsi_dev[sel]->dsi_pixel_ph.bits.wc = 1+panel->lcd_x*dsi_pixel_bits[panel->lcd_dsi_format]/8;
		dsi_dev[sel]->dsi_pixel_pd.bits.pd_tran0 = DSI_DCS_WRITE_MEMORY_START;
		dsi_dev[sel]->dsi_pixel_pd.bits.pd_trann = DSI_DCS_WRITE_MEMORY_CONTINUE;
		dsi_dev[sel]->dsi_pixel_pf0.bits.crc_force = 0xffff;
		dsi_dev[sel]->dsi_pixel_pf1.bits.crc_init_line0 = 0xe4e9;
		dsi_dev[sel]->dsi_pixel_pf1.bits.crc_init_linen = 0xf468;
		dsi_dev[sel]->dsi_pixel_ctl0.bits.pixel_format = panel->lcd_dsi_format;
	}

	if(panel->lcd_dsi_if == LCD_DSI_IF_VIDEO_MODE)
	{
		__u32 dsi_hsa,dsi_hact,dsi_hbp,dsi_hfp,dsi_hblk,dsi_vblk,tmp;
		
		dsi_hsa = panel->lcd_hspw *dsi_pixel_bits[panel->lcd_dsi_format]/8 - (4+2+4);
		dsi_hact = panel->lcd_x   *dsi_pixel_bits[panel->lcd_dsi_format]/8;
		dsi_hbp = (panel->lcd_hbp-panel->lcd_hspw)  *dsi_pixel_bits[panel->lcd_dsi_format]/8 - (4+2+4);
		dsi_hfp = (panel->lcd_ht-panel->lcd_x-panel->lcd_hbp)  *dsi_pixel_bits[panel->lcd_dsi_format]/8 - (2+4+2+4);
		dsi_hblk = (4+dsi_hbp+2)+(4+dsi_hact+2)+(4+dsi_hfp+2)-4-2;
		tmp = (4+(4+dsi_hsa+2)+4+(4+dsi_hblk+2))*(panel->lcd_vt-1);
		tmp = tmp+4+(4+dsi_hsa+2)+4+(4+2);
		dsi_vblk = 12-tmp%4;

		dsi_dev[sel]->dsi_sync_hss.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_sync_hss.bits.dt = DSI_DT_HSS;
		dsi_dev[sel]->dsi_sync_hss.bits.d0 = 0;
		dsi_dev[sel]->dsi_sync_hss.bits.d1 = 0;
		dsi_dev[sel]->dsi_sync_hss.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_sync_hss.dwval);
		dsi_dev[sel]->dsi_sync_hse.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_sync_hse.bits.dt = DSI_DT_HSE;
		dsi_dev[sel]->dsi_sync_hse.bits.d0 = 0;
		dsi_dev[sel]->dsi_sync_hse.bits.d1 = 0;
		dsi_dev[sel]->dsi_sync_hse.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_sync_hse.dwval);
		dsi_dev[sel]->dsi_sync_vss.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_sync_vss.bits.dt = DSI_DT_VSS;
		dsi_dev[sel]->dsi_sync_vss.bits.d0 = 0;
		dsi_dev[sel]->dsi_sync_vss.bits.d1 = 0;
		dsi_dev[sel]->dsi_sync_vss.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_sync_vss.dwval);
		dsi_dev[sel]->dsi_sync_vse.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_sync_vse.bits.dt = DSI_DT_VSE;
		dsi_dev[sel]->dsi_sync_vse.bits.d0 = 0;
		dsi_dev[sel]->dsi_sync_vse.bits.d1 = 0;
		dsi_dev[sel]->dsi_sync_vse.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_sync_vse.dwval);


		dsi_dev[sel]->dsi_basic_size0.bits.vsa = panel->lcd_vspw;
		dsi_dev[sel]->dsi_basic_size0.bits.vbp = panel->lcd_vbp-panel->lcd_vspw;
		dsi_dev[sel]->dsi_basic_size1.bits.vact = panel->lcd_y;
		dsi_dev[sel]->dsi_basic_size1.bits.vt = panel->lcd_vt;
		dsi_dev[sel]->dsi_blk_hsa0.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_blk_hsa0.bits.dt = DSI_DT_BLK;
		dsi_dev[sel]->dsi_blk_hsa0.bits.wc = dsi_hsa;
		dsi_dev[sel]->dsi_blk_hsa0.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_blk_hsa0.dwval);
		dsi_dev[sel]->dsi_blk_hsa1.bits.pd = 0;
		dsi_dev[sel]->dsi_blk_hsa1.bits.pf = dsi_crc_pro_pd_repeat(0,dsi_hsa);
		dsi_dev[sel]->dsi_blk_hbp0.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_blk_hbp0.bits.dt = DSI_DT_BLK;
		dsi_dev[sel]->dsi_blk_hbp0.bits.wc = dsi_hbp;
		dsi_dev[sel]->dsi_blk_hbp0.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_blk_hbp0.dwval);
		dsi_dev[sel]->dsi_blk_hbp1.bits.pd = 0;
		dsi_dev[sel]->dsi_blk_hbp1.bits.pf = dsi_crc_pro_pd_repeat(0,dsi_hbp);
		dsi_dev[sel]->dsi_blk_hfp0.bits.vc = panel->lcd_dsi_vc;
		dsi_dev[sel]->dsi_blk_hfp0.bits.dt = DSI_DT_BLK;
		dsi_dev[sel]->dsi_blk_hfp0.bits.wc = dsi_hfp;
		dsi_dev[sel]->dsi_blk_hfp0.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_blk_hfp0.dwval);
		dsi_dev[sel]->dsi_blk_hfp1.bits.pd = 0;
		dsi_dev[sel]->dsi_blk_hfp1.bits.pf = dsi_crc_pro_pd_repeat(0,dsi_hfp);
		dsi_dev[sel]->dsi_blk_hblk0.bits.dt = DSI_DT_BLK;
		dsi_dev[sel]->dsi_blk_hblk0.bits.wc = dsi_hblk;
		dsi_dev[sel]->dsi_blk_hblk0.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_blk_hblk0.dwval);
		dsi_dev[sel]->dsi_blk_hblk1.bits.pd = 0;
		dsi_dev[sel]->dsi_blk_hblk1.bits.pf = dsi_crc_pro_pd_repeat(0,dsi_hblk);
		dsi_dev[sel]->dsi_blk_vblk0.bits.dt = DSI_DT_BLK;
		dsi_dev[sel]->dsi_blk_vblk0.bits.wc = dsi_vblk;
		dsi_dev[sel]->dsi_blk_vblk0.bits.ecc = dsi_ecc_pro(dsi_dev[sel]->dsi_blk_vblk0.dwval);
		dsi_dev[sel]->dsi_blk_vblk1.bits.pd = 0xff;
		dsi_dev[sel]->dsi_blk_vblk1.bits.pf = 0xffff;
	}
	return 0;
}


__s32 dsi_cfg(__u32 sel,__panel_para_t * panel)
{
	dsi_basic_cfg(sel,panel);
	dsi_packet_cfg(sel,panel);
	dsi_dphy_cfg(sel,panel);
	return 0;
}


__s32 dsi_exit(__u32 sel)
{
	return 0;
}

__u8 dsi_ecc_pro(__u32 dsi_ph)
{
	dsi_ph_t ph;
	ph.bytes.byte012 = dsi_ph;

	ph.bits.bit29 = ph.bits.bit10 ^ ph.bits.bit11 ^ ph.bits.bit12 ^ ph.bits.bit13
				  ^ ph.bits.bit14 ^ ph.bits.bit15 ^ ph.bits.bit16 ^ ph.bits.bit17
				  ^ ph.bits.bit18 ^ ph.bits.bit19 ^ ph.bits.bit21 ^ ph.bits.bit22
				  ^ ph.bits.bit23 ;
	ph.bits.bit28 = ph.bits.bit04 ^ ph.bits.bit05 ^ ph.bits.bit06 ^ ph.bits.bit07
				  ^ ph.bits.bit08 ^ ph.bits.bit09 ^ ph.bits.bit16 ^ ph.bits.bit17
				  ^ ph.bits.bit18 ^ ph.bits.bit19 ^ ph.bits.bit20 ^ ph.bits.bit22
				  ^ ph.bits.bit23 ;
	ph.bits.bit27 = ph.bits.bit01 ^ ph.bits.bit02 ^ ph.bits.bit03 ^ ph.bits.bit07
			      ^ ph.bits.bit08 ^ ph.bits.bit09 ^ ph.bits.bit13 ^ ph.bits.bit14
			      ^ ph.bits.bit15 ^ ph.bits.bit19 ^ ph.bits.bit20 ^ ph.bits.bit21
			      ^ ph.bits.bit23 ;
	ph.bits.bit26 = ph.bits.bit00 ^ ph.bits.bit02 ^ ph.bits.bit03 ^ ph.bits.bit05
			      ^ ph.bits.bit06 ^ ph.bits.bit09 ^ ph.bits.bit11 ^ ph.bits.bit12
			      ^ ph.bits.bit15 ^ ph.bits.bit18 ^ ph.bits.bit20 ^ ph.bits.bit21
			      ^ ph.bits.bit22 ;
	ph.bits.bit25 = ph.bits.bit00 ^ ph.bits.bit01 ^ ph.bits.bit03 ^ ph.bits.bit04
			      ^ ph.bits.bit06 ^ ph.bits.bit08 ^ ph.bits.bit10 ^ ph.bits.bit12
			      ^ ph.bits.bit14 ^ ph.bits.bit17 ^ ph.bits.bit20 ^ ph.bits.bit21
			      ^ ph.bits.bit22 ^ ph.bits.bit23 ;
	ph.bits.bit24 = ph.bits.bit00 ^ ph.bits.bit01 ^ ph.bits.bit02 ^ ph.bits.bit04
				  ^ ph.bits.bit05 ^ ph.bits.bit07 ^ ph.bits.bit10 ^ ph.bits.bit11
				  ^ ph.bits.bit13 ^ ph.bits.bit16 ^ ph.bits.bit20 ^ ph.bits.bit21
				  ^ ph.bits.bit22 ^ ph.bits.bit23 ;
	return ph.bytes.byte3;
}

__u16 dsi_crc_pro(__u8* pd_p,__u32 pd_bytes)
{
	__u16 gen_code = 0x8408;
	__u16 byte_cntr;
	__u8 bit_cntr;
	__u8 curr_data;
	__u16 crc = 0xffff;

	if(pd_bytes>0)
	{
		for(byte_cntr=0;byte_cntr<pd_bytes;byte_cntr++)
		{
			curr_data = *(pd_p+byte_cntr);
			for(bit_cntr=0;bit_cntr<8;bit_cntr++)
			{
				if(((crc&0x0001)^((0x0001*curr_data)& 0x0001)) > 0 )
					crc = ((crc>>1) & 0x7fff) ^ gen_code;
				else
					crc = (crc>>1) 	& 0x7fff;
				curr_data = (curr_data>>1) & 0x7f;
			}
		}
	}
	return crc;
}

__u16 dsi_crc_pro_pd_repeat(__u8 pd,__u32 pd_bytes)
{
	__u16 gen_code = 0x8408;
	__u16 byte_cntr;
	__u8 bit_cntr;
	__u8 curr_data;
	__u16 crc = 0xffff;

	if(pd_bytes>0)
	{
		for(byte_cntr=0;byte_cntr<pd_bytes;byte_cntr++)
		{
			curr_data = pd;
			for(bit_cntr=0;bit_cntr<8;bit_cntr++)
			{
				if(((crc&0x0001)^((0x0001*curr_data)& 0x0001)) > 0 )
					crc = ((crc>>1) & 0x7fff) ^ gen_code;
				else
					crc = (crc>>1) 	& 0x7fff;
				curr_data = (curr_data>>1) & 0x7f;
			}
		}
	}
	return crc;
}



