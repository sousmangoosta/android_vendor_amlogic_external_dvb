/***************************************************************************
 *  Copyright C 2009 by Amlogic, Inc. All Rights Reserved.
 */
/**\file
 * \brief DVB前端测试
 *
 * \author Gong Ke <ke.gong@amlogic.com>
 * \date 2010-06-08: create the document
 ***************************************************************************/

#define AM_DEBUG_LEVEL 5

#include <am_debug.h>
#include <am_fend.h>
#include <am_fend_diseqc_cmd.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <am_misc.h>

/****************************************************************************
 * Macro definitions
 ***************************************************************************/
#define FEND_DEV_NO    (0)

#define MAX_DISEQC_LENGTH  16

static unsigned int blindscan_process = 0;

/****************************************************************************
 * Functions
 ***************************************************************************/

static void fend_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	fe_status_t status;
	int ber, snr, strength;
	struct dvb_frontend_info info;

	AM_FEND_GetInfo(dev_no, &info);
	if(info.type == FE_QAM) {
		printf("cb parameters: freq:%d srate:%d modulation:%d fec_inner:%d\n",
			evt->parameters.frequency, evt->parameters.u.qam.symbol_rate,
			evt->parameters.u.qam.modulation, evt->parameters.u.qam.fec_inner);
	} else if(info.type == FE_OFDM) {
		printf("cb parameters: freq:%d bandwidth:%d \n",
			evt->parameters.frequency, evt->parameters.u.ofdm.bandwidth);
	} else if(info.type == FE_QPSK) {	
		printf("cb parameters: * can get fe type qpsk! *\n");
	}else {
		printf("cb parameters: * can not get fe type! *\n");
	}
	printf("cb status: 0x%x\n", evt->status);
	
	AM_FEND_GetStatus(dev_no, &status);
	AM_FEND_GetBER(dev_no, &ber);
	AM_FEND_GetSNR(dev_no, &snr);
	AM_FEND_GetStrength(dev_no, &strength);
	
	printf("cb status: 0x%0x ber:%d snr:%d, strength:%d\n", status, ber, snr, strength);
}

static void blindscan_cb(int dev_no, AM_FEND_BlindEvent_t *evt, void *user_data)
{
	if(evt->status == AM_FEND_BLIND_START)
	{
		printf("++++++blindscan_start %u\n", evt->freq);
	}
	else if(evt->status == AM_FEND_BLIND_UPDATE)
	{
		blindscan_process = evt->process;
		printf("++++++blindscan_process %u\n", blindscan_process);
	}
}

static void SetDiseqcCommandString(int dev_no, const char *str)
{
	struct dvb_diseqc_master_cmd diseqc_cmd;
	
	if (!str)
		return;
	diseqc_cmd.msg_len=0;
	int slen = strlen(str);
	if (slen % 2)
	{
		AM_DEBUG(1, "%s", "invalid diseqc command string length (not 2 byte aligned)");
		return;
	}
	if (slen > MAX_DISEQC_LENGTH*2)
	{
		AM_DEBUG(1, "%s", "invalid diseqc command string length (string is to long)");
		return;
	}
	unsigned char val=0;
	int i=0; 
	for (i=0; i < slen; ++i)
	{
		unsigned char c = str[i];
		switch(c)
		{
			case '0' ... '9': c-=48; break;
			case 'a' ... 'f': c-=87; break;
			case 'A' ... 'F': c-=55; break;
			default:
				AM_DEBUG(1, "%s", "invalid character in hex string..ignore complete diseqc command !");
				return;
		}
		if ( i % 2 )
		{
			val |= c;
			diseqc_cmd.msg[i/2] = val;
		}
		else
			val = c << 4;
	}
	diseqc_cmd.msg_len = slen/2;

	AM_FEND_DiseqcSendMasterCmd(dev_no, &diseqc_cmd); 

	return;
}

static void sec(int dev_no)
{
	int sec = -1;
	AM_Bool_t sec_exit = AM_FALSE;
	
	printf("sec_control\n");
	while(1)
	{
		printf("-----------------------------\n");
		printf("DiseqcResetOverload-0\n");
		printf("DiseqcSendBurst-1\n");
		printf("SetTone-2\n");
		printf("SetVoltage-3\n");
		printf("EnableHighLnbVoltage-4\n");
		printf("Diseqccmd_ResetDiseqcMicro-5\n");
		printf("Diseqccmd_StandbySwitch-6\n");
		printf("Diseqccmd_PoweronSwitch-7\n");
		printf("Diseqccmd_SetLo-8\n");
		printf("Diseqccmd_SetVR-9\n");
		printf("Diseqccmd_SetSatellitePositionA-10\n");
		printf("Diseqccmd_SetSwitchOptionA-11\n");
		printf("Diseqccmd_SetHi-12\n");
		printf("Diseqccmd_SetHL-13\n");
		printf("Diseqccmd_SetSatellitePositionB-14\n");
		printf("Diseqccmd_SetSwitchOptionB-15\n");
		printf("Diseqccmd_SetSwitchInput-16\n");
		printf("Diseqccmd_SetLNBPort4-17\n");
		printf("Diseqccmd_SetLNBPort16-18\n");		
		printf("Diseqccmd_SetChannelFreq-19\n");
		printf("Diseqccmd_SetPositionerHalt-20\n");
		printf("Diseqccmd_EnablePositionerLimit-21\n");
		printf("Diseqccmd_DisablePositionerLimit-22\n");
		printf("Diseqccmd_SetPositionerELimit-23\n");
		printf("Diseqccmd_SetPositionerWLimit-24\n");
		printf("Diseqccmd_PositionerGoE-25\n");
		printf("Diseqccmd_PositionerGoW-26\n");	
		printf("Diseqccmd_StorePosition-27\n");
		printf("Diseqccmd_GotoPositioner-28\n");	
		printf("Diseqccmd_GotoxxAngularPositioner-29\n");
		printf("Diseqccmd_GotoAngularPositioner-30\n");			
		printf("Diseqccmd_SetODUChannel-31\n");
		printf("Diseqccmd_SetODUPowerOff-32\n");	
		printf("Diseqccmd_SetODUUbxSignalOn-33\n");
		printf("Diseqccmd_SetString-34\n");
		printf("Exit sec-35\n");
		printf("-----------------------------\n");
		printf("select\n");
		scanf("%d", &sec);
		switch(sec)
		{
			case 0:
				AM_FEND_DiseqcResetOverload(dev_no); 
				break;
				
			case 1:
				{
					int minicmd;
					printf("minicmd_A-0/minicmd_B-1\n");
					scanf("%d", &minicmd);
					AM_FEND_DiseqcSendBurst(dev_no,minicmd);
					break;
				}
				
			case 2:
				{
					int tone;
					printf("on-0/off-1\n");
					scanf("%d", &tone);				
					AM_FEND_SetTone(dev_no, tone); 
					break;
				}

			case 3:
				{
					int voltage;
					printf("v13-0/v18-1/v_off-2\n");
					scanf("%d", &voltage);
					AM_FEND_SetVoltage(dev_no, voltage);				
					break;
				}

			case 4:
				{
					int enable;
					printf("disable-0/enable-1/\n");
					scanf("%d", &enable);				
					AM_FEND_EnableHighLnbVoltage(dev_no, (long)enable);  
					break;
				}

			case 5:
				AM_FEND_Diseqccmd_ResetDiseqcMicro(dev_no);
				break;

			case 6:
				AM_FEND_Diseqccmd_StandbySwitch(dev_no);
				break;
				
			case 7:
				AM_FEND_Diseqccmd_PoweronSwitch(dev_no);
				break;
				
			case 8:
				AM_FEND_Diseqccmd_SetLo(dev_no);
				break;
				
			case 9:
				AM_FEND_Diseqccmd_SetVR(dev_no); 
				break;
				
			case 10:
				AM_FEND_Diseqccmd_SetSatellitePositionA(dev_no); 
				break;
				
			case 11:
				AM_FEND_Diseqccmd_SetSwitchOptionA(dev_no);
				break;
				
			case 12:
				AM_FEND_Diseqccmd_SetHi(dev_no);
				break;
				
			case 13:
				AM_FEND_Diseqccmd_SetHL(dev_no);
				break;
				
			case 14:
				AM_FEND_Diseqccmd_SetSatellitePositionB(dev_no); 
				break;
				
			case 15:
				AM_FEND_Diseqccmd_SetSwitchOptionB(dev_no); 
				break;
				
			case 16:
				{
					int input;
					printf("s1ia-1/s2ia-2/s3ia-3/s4ia-4/s1ib-5/s2ib-6/s3ib-7/s4ib-8/\n");
					scanf("%d", &input);	
					AM_FEND_Diseqccmd_SetSwitchInput(dev_no, input);
					break;
				}
				
			case 17:
				{
					int lnbport, polarisation, local_oscillator_freq;
					printf("lnbport-1-4\n");
					scanf("%d", &lnbport);
					printf("polarisation:H-0/V-1/NO-2\n");
					scanf("%d", &polarisation);
					printf("local_oscillator_freq:L-0/H-1/NO-2\n");
					scanf("%d", &local_oscillator_freq);				
					AM_FEND_Diseqccmd_SetLNBPort4(dev_no, lnbport, polarisation, local_oscillator_freq);
					break;
				}

			case 18:
				{
					int lnbport, polarisation, local_oscillator_freq;
					printf("lnbport-1-16\n");
					scanf("%d", &lnbport);
					printf("polarisation:H-0/V-1/NO-2\n");
					scanf("%d", &polarisation);
					printf("polarisation:L-0/H-1/NO-2\n");
					scanf("%d", &local_oscillator_freq);				
					AM_FEND_Diseqccmd_SetLNBPort16(dev_no, lnbport, polarisation, local_oscillator_freq);
					break;
				}
                                                                  
			case 19:
				{
					int freq;
					printf("frequency(KHz): ");		
					scanf("%d", &freq);				
					AM_FEND_Diseqccmd_SetChannelFreq(dev_no, freq);
					break;
				}

			case 20:
				AM_FEND_Diseqccmd_SetPositionerHalt(dev_no);
				break;                                                                  

			case 21:
				AM_FEND_Diseqccmd_EnablePositionerLimit(dev_no);
				break;

			case 22:
				AM_FEND_Diseqccmd_DisablePositionerLimit(dev_no);
				break;

			case 23:
				AM_FEND_Diseqccmd_SetPositionerELimit(dev_no);
				break;

			case 24:
				AM_FEND_Diseqccmd_SetPositionerWLimit(dev_no);
				break;
				
			case 25:
				{
					unsigned char unit;
					printf("unit continue-0 second-1-127 step-128-255: ");		
					scanf("%d", &unit);				
					AM_FEND_Diseqccmd_PositionerGoE(dev_no, unit);
					break;
				}

			case 26:
				{
					unsigned char unit;
					printf("unit continue-0 second-1-127 step-128-255: ");		
					scanf("%d", &unit);				
					AM_FEND_Diseqccmd_PositionerGoW(dev_no, unit);
					break;
				}

			case 27:
				{
					unsigned char position;
					printf("position 0-255: ");		
					scanf("%d", &position);							
					AM_FEND_Diseqccmd_StorePosition(dev_no, position);
					break;
				}

			case 28:
				{
					unsigned char position;
					printf("position 0-255: ");		
					scanf("%d", &position);				
					AM_FEND_Diseqccmd_GotoPositioner(dev_no, position); 
					break;
				}

			case 29:
				{
					double local_longitude, local_latitude, satellite_longitude;
					printf("local_longitude: ");		
					scanf("%lf", &local_longitude);
					printf("local_latitude: ");		
					scanf("%lf", &local_latitude);
					printf("satellite_longitude: ");		
					scanf("%lf", &satellite_longitude);				
					AM_FEND_Diseqccmd_GotoxxAngularPositioner(dev_no, local_longitude, local_latitude, satellite_longitude);
	                            break; 
				}   

			case 30:
				{
					double local_longitude, local_latitude, satellite_longitude;
					printf("local_longitude: ");		
					scanf("%lf", &local_longitude);
					printf("local_latitude: ");		
					scanf("%lf", &local_latitude);
					printf("satellite_longitude: ");		
					scanf("%lf", &satellite_longitude);				
					AM_FEND_Diseqccmd_GotoAngularPositioner(dev_no, local_longitude, local_latitude, satellite_longitude);
	                            break; 
				}                                 

			case 31:
				{
					unsigned char ub_number;
					printf("ub_number 0-7: ");		
					scanf("%d", &ub_number);	
					unsigned char inputbank_number;
					printf("inputbank_number 0-7: ");		
					scanf("%d", &inputbank_number);	
					int transponder_freq;
					printf("transponder_freq(KHz): ");		
					scanf("%d", &transponder_freq);	
					int oscillator_freq;
					printf("oscillator_freq(KHz): ");		
					scanf("%d", &oscillator_freq);	
					int ub_freq;
					printf("ub_freq(KHz): ");		
					scanf("%d", &ub_freq);					
					AM_FEND_Diseqccmd_SetODUChannel(dev_no, ub_number, inputbank_number, transponder_freq, oscillator_freq, ub_freq);
					break;
				}

			case 32:
				{
					unsigned char ub_number;
					printf("ub_number 0-7: ");		
					scanf("%d", &ub_number);					
					AM_FEND_Diseqccmd_SetODUPowerOff(dev_no, ub_number); 
					break;
				}

			case 33:
				AM_FEND_Diseqccmd_SetODUUbxSignalOn(dev_no);
                            break;                           

			case 34:
				{
					char str[13] = {0};

					printf("diseqc cmd str: ");		
					scanf("%s", str);						
					SetDiseqcCommandString(dev_no, str);
				}
				break;

			case 35:
				sec_exit = AM_TRUE;
				break;
				
			default:
				break;
		}

		sec = -1;

		if(sec_exit == AM_TRUE)
		{
			break;
		}

		usleep(15 *1000);
		
	}

	return;
}

int main(int argc, char **argv)
{
	AM_FEND_OpenPara_t para;
	fe_status_t status;
	int fe_id=-1;	
	int blind_scan = 0;
	struct dvb_frontend_parameters blindscan_para[128];
	unsigned int count = 128;
	
	while(1)
	{
		struct dvb_frontend_parameters p;
		int mode, current_mode;
		int freq, srate, qam;
		int bw;
		char buf[64], name[64];
		
		memset(&para, 0, sizeof(para));
		
		printf("Input fontend id, id=-1 quit\n");
		printf("id: ");
		scanf("%d", &fe_id);
		if(fe_id<0)
			break;

		printf("Input fontend mode: (0-DVBC, 1-DVBT, 2-DVBS)\n");
		printf("mode(0/1/2): ");
		scanf("%d", &mode);
		para.mode = (mode==0)?FE_QAM : 
				(mode==1)? FE_OFDM : FE_QPSK;

		AM_TRY(AM_FEND_Open(/*FEND_DEV_NO*/fe_id, &para));

		while(1) {

			if(mode == 2) {
				printf("blindscan(0/1): ");
				scanf("%d", &blind_scan);
				if(blind_scan == 1)
				{
					AM_FEND_BlindScan(fe_id, blindscan_cb, (void *)&fe_id, 950000000, 2150000000);
					while(1){
						if(blindscan_process == 100){
							break;
						}
						//printf("wait process %u\n", blindscan_process);
						usleep(500 * 1000);
					}

					AM_FEND_BlindExit(fe_id); 

					printf("start AM_FEND_BlindGetTPInfo\n");
					
					AM_FEND_BlindGetTPInfo(fe_id, blindscan_para, &count);

					printf("dump TPInfo: %d\n", count);

					int i = 0;
					
					printf("\n\n");
					for(i=0; i < count; i++)
					{
						printf("Ch%2d: RF: %4d SR: %5d ",i+1, (blindscan_para[i].frequency/1000),(blindscan_para[i].u.qpsk.symbol_rate/1000));
						printf("\n");
					}	

					blind_scan = 0;
				}
			}
			
			AM_TRY(AM_FEND_SetCallback(/*FEND_DEV_NO*/fe_id, fend_cb, NULL));
			
			printf("input frontend parameters, frequency=0 quit\n");
			if(mode != 2) {
				printf("frequency(Hz): ");
			} else {
				sec(fe_id);
				printf("frequency(KHz): ");
			}
			
			scanf("%d", &freq);
			if(!freq)
				break;
			
			if(mode==0) {
				printf("symbol rate(kbps): ");
				scanf("%d", &srate);
				printf("QAM(16/32/64/128/256): ");
				scanf("%d", &qam);
				
				p.frequency = freq;
				p.u.qam.symbol_rate = srate*1000;
				p.u.qam.fec_inner = FEC_AUTO;
				switch(qam)
				{
					case 16:
						p.u.qam.modulation = QAM_16;
					break;
					case 32:
						p.u.qam.modulation = QAM_32;
					break;
					case 64:
					default:
						p.u.qam.modulation = QAM_64;
					break;
					case 128:
						p.u.qam.modulation = QAM_128;
					break;
					case 256:
						p.u.qam.modulation = QAM_256;
					break;
				}
			}else if(mode==1){
				printf("BW[8/7/6/5(AUTO) MHz]: ");
				scanf("%d", &bw);

				p.frequency = freq;
				switch(bw)
				{
					case 8:
					default:
						p.u.ofdm.bandwidth = BANDWIDTH_8_MHZ;
					break;
					case 7:
						p.u.ofdm.bandwidth = BANDWIDTH_7_MHZ;
					break;
					case 6:
						p.u.ofdm.bandwidth = BANDWIDTH_6_MHZ;
					break;
					case 5:
						p.u.ofdm.bandwidth = BANDWIDTH_AUTO;
					break;
				}

				p.u.ofdm.code_rate_HP = FEC_AUTO;
				p.u.ofdm.code_rate_LP = FEC_AUTO;
				p.u.ofdm.constellation = QAM_AUTO;
				p.u.ofdm.guard_interval = GUARD_INTERVAL_AUTO;
				p.u.ofdm.hierarchy_information = HIERARCHY_AUTO;
				p.u.ofdm.transmission_mode = TRANSMISSION_MODE_AUTO;
			}else{
				printf("dvb sx test\n");

				p.frequency = freq;

				printf("symbol rate: ");
				scanf("%d", &(p.u.qpsk.symbol_rate));
			}
#if 0
			AM_TRY(AM_FEND_SetPara(/*FEND_DEV_NO*/fe_id, &p));
#else
			AM_TRY(AM_FEND_Lock(/*FEND_DEV_NO*/fe_id, &p, &status));
			printf("lock status: 0x%x\n", status);
#endif
		}
		AM_TRY(AM_FEND_Close(/*FEND_DEV_NO*/fe_id));
	}
	
	
	return 0;
}

