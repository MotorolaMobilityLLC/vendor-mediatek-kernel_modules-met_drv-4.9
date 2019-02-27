/*
 * Copyright (C) 2016 MediaTek Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See http://www.gnu.org/licenses/gpl-2.0.html for more details.
 */
MET_SSPM_RTS_EVNET(SSPM_PTPOD, "_id,voltage")
MET_SSPM_RTS_EVNET(SSPM_MET_UNIT_TEST, "test")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_NON_WFX, "non_wfx_0,non_wfx_1,non_wfx_2,non_wfx_3,non_wfx_4,non_wfx_5,non_wfx_6,non_wfx_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_LOADING, "ratio,cps")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_POWER, "c_up_array_0,c_up_array_1,c_down_array_0,c_down_array_1,c_up_0,c_up_1,c_down_0,c_dwon_1,c_up,c_down,v_up,v_down,v2f_0,v2f_1")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_OPP, "v_dram_opp,v_dram_opp_cur,c_opp_cur_0,c_opp_cur_1,d_times_up,d_times_down")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_RATIO, "ratio_max_0,ratio_max_1,ratio_0,ratio_1,ratio_2,ratio_3,ratio_4,ratio_5,ratio_6,ratio_7")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_BW, "total_bw")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_CP_RATIO, "up0,up1,up2,up3,down0,down1,down2,down3")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_VP_RATIO, "up0,up1,up2,up3,down0,down1,down2,down3")
MET_SSPM_RTS_EVNET(SSPM_CM_MGR_DE_TIMES, "up0,up1,up2,up3,down0,down1,down2,down3,reset")
MET_SSPM_RTS_EVNET(SSPM_SWPM_PWR, "cpu_L,cpu_B,gpu,cam,img,mdp,disp,adsp,venc,vdec,vpu,dramc,infra_top,aphy_vcore,aphy_vddq_0p6v,dram_vddq_0p6v,aphy_vm_1p1v,dram_vdd2_1p1v,aphy_vio_1p8v,dram_vdd1_1p8v")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_ACTIVE_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_IDLE_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_OFF_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_STALL_RATIO, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_PMU_L3DCR, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_PMU_INST_SPEC, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_PMU_CYCLES, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CPU_NON_WFX_CTR, "cpu0,cpu1,cpu2,cpu3,cpu4,cpu5,cpu6,cpu7")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CLUSTER_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_CAM_STATE_RATIO, "RAW_A_active,RAW_B_active,RAW_C_active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_IMG_STATE_RATIO, "P2_active,P2_idle,MFB_active,WPE_active,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_MDP_STATE_RATIO, "active,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_DISP_STATE_RATIO, "active,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_ADSP_STATE_RATIO, "active,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VENC_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VDEC_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU0_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_VPU1_STATE_RATIO, "active,idle,off")
MET_SSPM_RTS_EVNET(SSPM_SWPM_INFRA_STATE_RATIO, "dact,cact,idle,dcm")
MET_SSPM_RTS_EVNET(SSPM_SWPM_EMI_BW, "total_r,total_w,cpu")
MET_SSPM_RTS_EVNET(SSPM_SWPM_MEM_IDX, "read_bw,write_bw,srr_pct,pdir_pct,phr_pct,acc_util,mr4")
