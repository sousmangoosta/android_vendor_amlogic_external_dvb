#ifdef _FORTIFY_SOURCE
#undef _FORTIFY_SOURCE
#endif
/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief 解复用设备测试程序
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-07: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_dmx.h>
#include <am_fend.h>
#include <string.h>
#include <unistd.h>

#define FEND_DEV_NO   (0)

#define  PAT_TEST
#define  EIT_TEST
#define  NIT_TEST
#define  BAT_TEST

#if 0
static FILE *fp;
static int sec_mask[255];
static int sec_cnt = 0;
#else
typedef struct {
	char data[4096];
	int len;
	int got;
} Section;
static Section sections[255];
#endif

static int s_last_num =-1;

int freq = 0;
int layer = -1;
int src=0;
int dmx=0;
int timeout = 60*3;

static int bat=0;
static int nit=0;
static int user=0;
static int pat=0;
static int eit=0;
static int pall=0;

#define USER_MAX 10
static int u_pid[USER_MAX]={[0 ... USER_MAX-1] = -1};
static int u_para[USER_MAX]={[0 ... USER_MAX-1] = 0};
static int u_para_g;
static FILE *fp[USER_MAX];
/*
   u_para format:
	d1 - 0:sec 1:pes
	d2 - 1:crc : sec only
	d3 - 1:print
	d5 - 1:swfilter
	d6 - 1:ts_tap :pes only
	d4 - 1:w2file
*/
#define UPARA_TYPE     0xf
#define UPARA_CRC      0xf0
#define UPARA_PR       0xf00
#define UPARA_SF       0xf000
#define UPARA_DMX_TAP  0xf0000
#define UPARA_FILE     0xf00000

#define get_upara(_i) (u_para[(_i)]? u_para[(_i)] : u_para_g)

static void dump_bytes(int dev_no, int fid, const uint8_t *data, int len, void *user_data)
{
	int u = (int)user_data;

	if(pall) {
		int i;
		printf("data:\n");
		for(i=0;i<len;i++)
		{
			printf("%02x ", data[i]);
			if(((i+1)%16)==0) printf("\n");
		}
		if((i%16)!=0) printf("\n");
	}
#if 1
	if(bat&UPARA_PR) {
		if(data[0]==0x4a) {
			printf("sec:tabid:0x%02x,bunqid:0x%02x%02x,section num:%4d,lat_section_num:%4d\n",data[0],
				data[3],data[4],data[6],data[7]);
		}

	}
	else if(nit&UPARA_PR) {

		if(data[0]==0x40) {
			printf("section:%8d,max:%8d\n",data[6],data[7]);
			if((data[6] !=s_last_num+1)&&(s_last_num!=-1))//��һ�����߲�����
			{
				if(s_last_num ==data[7])//��һ����MAX
				{
					if(data[6] != 0)//��һ��MAX ������� 0
					{
						printf("##drop packet,tabid:0x%4x,cur:%8d,last:%8d,max:%8d\n",data[0],
						data[6],s_last_num,data[7]);
						//stop_section_flag =1;
					}
					else
					{
					}
				}
				else//��һ������
				{
					printf("##drop packet,tabid:%4x,cur:%8d,last:%8d,max:%8d\n",data[0],
					data[6],s_last_num,data[7]);
					//stop_section_flag =1;
				}

				
			}
			else
			{
				//printf("section:%8d,",sectiondata->m_pucData[6]);
			}
			s_last_num = data[6];
		}
	}
	else if(pat&UPARA_PR) {
		if(data[0]==0x0)
			printf("%02x: %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8]);
	}
	else {
		if(!user_data) {
			printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4],
				data[5], data[6], data[7], data[8]);
			return;
		}
		
		if(get_upara(u-1)&UPARA_PR)
			printf("[%d:%d] %02x %02x %02x %02x %02x %02x %02x %02x %02x\n", u-1, u_pid[u-1],
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
		if(get_upara(u-1)&UPARA_FILE){
			int ret = fwrite(data, 1, len, fp[(int)user_data-1]);
			if(ret!=len)
				printf("data w lost\n");
		}
	}
#endif
}

static int get_section(int dmx, int timeout)
{
#ifdef PAT_TEST
	int fid;
#endif

#ifdef EIT_TEST
	int fid_eit_pf, fid_eit_opf;
#endif

#ifdef NIT_TEST
	int fid_nit;
#endif
#ifdef BAT_TEST
	int fid_bat;
#endif

	int fid_user[USER_MAX];
	int i;

	struct dmx_sct_filter_params param;
	struct dmx_pes_filter_params pparam;
#ifdef PAT_TEST
	if(pat&0xf) {
	printf("start pat...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid));
	AM_TRY(AM_DMX_SetCallback(dmx, fid, dump_bytes, NULL));
	}
#endif

#ifdef EIT_TEST
	if(eit&0xf) {
	printf("start eit...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_eit_pf));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_eit_pf, dump_bytes, NULL));
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_eit_opf));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_eit_opf, dump_bytes, NULL));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xf) {
	printf("start nit...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_nit));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_nit, dump_bytes, NULL));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xf) {
	printf("start bat...\n");
	AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_bat));
	AM_TRY(AM_DMX_SetCallback(dmx, fid_bat, dump_bytes, NULL));
	}
#endif

	
#ifdef PAT_TEST
	if(pat&0xf) {
	memset(&param, 0, sizeof(param));
	param.pid = 0;
	param.filter.filter[0] = 0;
	param.filter.mask[0] = 0xff;
	//param.filter.filter[2] = 0x08;
	//param.filter.mask[2] = 0xff;

	param.flags = DMX_CHECK_CRC;
	if(pat&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid, &param));
	}
#endif

#ifdef EIT_TEST
	if(eit&0xf) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x12;
	param.filter.filter[0] = 0x4E;
	param.filter.mask[0] = 0xff;
	param.flags = DMX_CHECK_CRC;	
	if(eit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_eit_pf, &param));
	
	memset(&param, 0, sizeof(param));
	param.pid = 0x12;
	param.filter.filter[0] = 0x4F;
	param.filter.mask[0] = 0xff;
	param.flags = DMX_CHECK_CRC;	
	if(eit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_eit_opf, &param));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xF) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x10;
	param.filter.filter[0] = 0x40;
	param.filter.mask[0] = 0xff;
	if(nit&UPARA_CRC)
		param.flags = DMX_CHECK_CRC;	
	if(nit&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_nit, &param));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xF) {
	memset(&param, 0, sizeof(param));
	param.pid = 0x11;
	param.filter.filter[0] = 0x4a;
	param.filter.mask[0] = 0xff;
	if(bat&UPARA_CRC)
		param.flags = DMX_CHECK_CRC;	
	if(bat&UPARA_SF)
		param.flags |= 0x100;
	AM_TRY(AM_DMX_SetSecFilter(dmx, fid_bat, &param));
	}
#endif


#ifdef PAT_TEST
	if(pat&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid));
	}
#endif
#ifdef EIT_TEST
	if(eit&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_eit_pf, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_eit_opf, 32*1024));
	//AM_TRY(AM_DMX_StartFilter(dmx, fid_eit_opf));
	}
#endif

#ifdef NIT_TEST
	if(nit&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_nit, 32*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_nit));
	}
#endif

#ifdef BAT_TEST
	if(bat&0xF) {
	AM_TRY(AM_DMX_SetBufferSize(dmx, fid_bat, 64*1024));
	AM_TRY(AM_DMX_StartFilter(dmx, fid_bat));
	}
#endif

	for(i=0; i<USER_MAX; i++) {
		if(u_pid[i]!=-1) {
			AM_TRY(AM_DMX_AllocateFilter(dmx, &fid_user[i]));

			AM_TRY(AM_DMX_SetCallback(dmx, fid_user[i], dump_bytes, (void*)(i+1)));
		
			if(get_upara(i)&UPARA_TYPE) {/*pes*/
				memset(&pparam, 0, sizeof(pparam));
				pparam.pid = u_pid[i];
				pparam.pes_type = DMX_PES_OTHER;
				pparam.input = DMX_IN_FRONTEND;
				pparam.output = DMX_OUT_TAP;
				if(get_upara(i)&UPARA_DMX_TAP)
					pparam.output = DMX_OUT_TSDEMUX_TAP;
				if(get_upara(i)&UPARA_SF)
					pparam.flags |= 0x100;
				AM_TRY(AM_DMX_SetPesFilter(dmx, fid_user[i], &pparam));

			} else {/*sct*/
				memset(&param, 0, sizeof(param));
				param.pid = u_pid[i];
			/*	param.filter.filter[0] = 0xa2;
				param.filter.mask[0] = 0xff;*/
				if(get_upara(i)&UPARA_CRC)
					param.flags = DMX_CHECK_CRC;
				if(get_upara(i)&UPARA_SF)
					param.flags |= 0x100;
				AM_TRY(AM_DMX_SetSecFilter(dmx, fid_user[i], &param));
			}

			AM_TRY(AM_DMX_SetBufferSize(dmx, fid_user[i], 64*1024));

			if(get_upara(i)&UPARA_FILE) {
				char name[32];
				sprintf(name, "/storage/external_storage/u_%d.dump", i);
				fp[i] = fopen(name, "wb");
				if(fp[i])
					printf("file open:[%s]\n", name);
			}

			AM_TRY(AM_DMX_StartFilter(dmx, fid_user[i]));
		}
	}

	sleep(timeout);

#ifdef PAT_TEST
	if(pat&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid));
	}
#endif	
#ifdef EIT_TEST
	if(eit&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_eit_pf));
	AM_TRY(AM_DMX_StopFilter(dmx, fid_eit_opf));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_eit_opf));
	}
#endif
#ifdef NIT_TEST
	if(nit&0xF){
	AM_TRY(AM_DMX_StopFilter(dmx, fid_nit));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_nit));
	}
#endif	
#ifdef BAT_TEST
	if(bat&0xF) {
	AM_TRY(AM_DMX_StopFilter(dmx, fid_bat));
	AM_TRY(AM_DMX_FreeFilter(dmx, fid_bat));
	}
#endif	
	
	for(i=0; i<USER_MAX; i++) {
		if(u_pid[i]!=-1) {
			AM_TRY(AM_DMX_StopFilter(dmx, fid_user[i]));
			AM_TRY(AM_DMX_FreeFilter(dmx, fid_user[i]));
			if((get_upara(i)&UPARA_FILE) && fp[i])
				fclose(fp[i]);
		}
	}
	
	return 0;
}

static int setlayer(int layer/*1/2/4/7*/)
{
	AM_ErrorCode_t ret;

	struct dtv_property p = {.cmd=DTV_ISDBT_LAYER_ENABLED, .u.data = layer};
	struct dtv_properties props = {.num=1, .props=&p};
	printf("AM FEND SetProp layer:%d\n", props.props[0].u.data);
	ret = AM_FEND_SetProp(FEND_DEV_NO, &props);
	return 0;
}

int get_para(char *argv)
{
	#define CASE(name, len, type, var) \
		if(!strncmp(argv, name"=", (len)+1)) { \
			sscanf(&argv[(len)+1], type, &var); \
			printf("param["name"] => "type"\n", var); \
		}

	CASE("freq",      4, "%i", freq)
	else CASE("src",  3, "%i", src)
	else CASE("dmx",  3, "%i", dmx)
	else CASE("pat",  3, "%x", pat)
	else CASE("eit",  3, "%x", eit)
	else CASE("layer",5, "%i", layer)
	else CASE("bat",  3, "%x", bat)
	else CASE("nit",  3, "%x", nit)
	else CASE("timeout", 7, "%i", timeout)
	else CASE("pall", 4, "%i", pall)
	else CASE("pid0", 4, "%x", u_pid[0])
	else CASE("pid1", 4, "%x", u_pid[1])
	else CASE("pid2", 4, "%x", u_pid[2])
	else CASE("pid3", 4, "%x", u_pid[3])
	else CASE("pid4", 4, "%x", u_pid[4])
	else CASE("para0", 5, "%x", u_para[0])
	else CASE("para1", 5, "%x", u_para[1])
	else CASE("para2", 5, "%x", u_para[2])
	else CASE("para3", 5, "%x", u_para[3])
	else CASE("para4", 5, "%x", u_para[4])
	else CASE("para", 4, "%x", u_para_g)

	return 0;
}

int main(int argc, char **argv)
{
	AM_DMX_OpenPara_t para;
	AM_FEND_OpenPara_t fpara;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	int ret=0;
	int i;

	memset(&fpara, 0, sizeof(fpara));
#if 1

	if(argc==1)
	{
		printf(
			"Usage:%s [freq=] [src=] [dmx=] [layer=] [timeout=] [pat=] [eit=] [bat=] [nit=] [pidx=] [parax=] [para=]\n"
			"  default   - src:0 dmx:0 layer:-1 uparax:0\n"
			"  x         - 0~5\n"
			"  upara     - d6->|111111|<-d1\n"
			"    d1 - 0:sec 1:pes (means enable for pat/eit/bat/nit)\n"
			"    d2 - 1:crc : sec only\n"
			"    d3 - 1:print\n"
			"    d4 - 1:swfilter\n"
			"    d5 - 1:ts tap : pes only\n"
			"    d6 - 1:w2file\n"
			, argv[0]);
		return 0;
	}
	
	for(i=1; i< argc; i++)
		get_para(argv[i]);


	if(freq>0)
	{
		AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));

		p.frequency = freq;
#if 1
		p.inversion = INVERSION_AUTO;
		p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
		p.u.ofdm.code_rate_HP = FEC_AUTO;
		p.u.ofdm.code_rate_LP = FEC_AUTO;
		p.u.ofdm.constellation = QAM_AUTO;
		p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
		p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
		p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
#else		
		p.u.qam.symbol_rate = 6875000;
		p.u.qam.fec_inner = FEC_AUTO;
		p.u.qam.modulation = QAM_64;
#endif		
	
		AM_TRY(AM_FEND_Lock(FEND_DEV_NO, &p, &status));
		
		if(status&FE_HAS_LOCK)
			printf("locked\n");
		else
			printf("unlocked\n");
	}
	
	if(layer!=-1)
		setlayer(layer);

#endif	
	memset(&para, 0, sizeof(para));
	//para.use_sw_filter = AM_TRUE;
	//para.dvr_fifo_no = 1;
	AM_TRY(AM_DMX_Open(dmx, &para));

	AM_TRY(AM_DMX_SetSource(dmx, src));
	printf("TS SRC = %d\n", src);

	get_section(dmx, timeout);
	
	AM_DMX_Close(dmx);
	if(freq)
		AM_FEND_Close(FEND_DEV_NO);
	
	return ret;
}

