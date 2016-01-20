#ifndef SYSMONITOR_H
#define SYSMONITOR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
//#include <algorithm>

#include <string.h>
#include <utils/threads.h>
#include <pthread.h>

//#include <linux/cpu.h>

#include <utils/Errors.h>
#include <utils/String16.h>

#include <binder/IServiceManager.h>
#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
 
#include <binder/BinderService.h>
#include <SurfaceFlinger.h>
#include <cutils/sched_policy.h>
#include <sys/syscall.h>

/*
 *
 * 	/proc/pid/cmdline
 *	com.gameloft.android.ANMP.GloftA8HM
 *	jp.co.translimit.braindots
 *	com.zingmagic.bridgevfree
 *	com.raptor.furious7
 *  com.x3m.tx3
 *  com.phosphorgames.DarkMeadow
 *   
 */
using namespace android;
int manual;
int CPU_Sensitive; //0 for GPU_Sensitive



const double Coef_Fc2Uc[]={-0.00004,-0.00007,-0.000002,-0.00003,-0.00007,-0.00004,-0.00002};
const double Coef_Fc2Q[]={0.000003,0.00001,0.000002,0.00001,0.00001,0.00002,0.000004};
const double Coef_Fg2Ug[]={-0.0194,-0.3649,0,-0.055,-0.0346,-0.1169,-0.0654};
const double Coef_Fg2Q[]={0.0463,0.00001,0.0007,0.0343,0.0035,0.0363,0.017};
const double Coef_Uc2Q[]={0.1111,-0.0686,-0.0286,0.0709,-0.0895,0.1381,0.127};
const double Coef_Ug2Q[]={0.2643,0.1226,0.2942,0.1249,0.2006,0.3811,0.0691};
/*
//Flexibility test
const double Coef_Fc2Uc[]={-0.00004,-0.00004,-0.00004,-0.00004};
const double Coef_Fc2Q[]={0.000003,0.000003,0.000003,0.000003};
const double Coef_Fg2Ug[]={-0.0194,-0.0194,-0.0194,-0.0194};
const double Coef_Fg2Q[]={0.0463,0.0463,0.0463,0.0463};
const double Coef_Uc2Q[]={0.1111,0.1111,0.1111,0.1111};
const double Coef_Ug2Q[]={0.2643,0.2643,0.2643,0.2643};
*/
/********************************
*	          PATH              *
*********************************/
#define cpu_fs "/sys/devices/system/cpu"
#define gpu_fs "/sys/kernel/debug/tegra_host/scaling"
#define battery "/sys/devices/platform/tegra-i2c.4/i2c-4/4-0055/power_supply/battery"

/********************************
*	SM Parameters               *
*********************************/
// s
#define verbal_period 1
// us
#define GF_period 1000000
#define CF_period 1000000
#define CHP_period 100000

/********************************
*	system parameter            *
*********************************/
#define core_num 4
#define freq_level 12
#define gpu_freq_level 5
const int num_sample = 4;

/********************************
*	Governor Variable         	*
*********************************/
int FL[freq_level]={51000,102000,204000,340000,475000,640000,760000,860000,1000000,1100000,1200000,1300000};
int GFL[gpu_freq_level]={228000,275000,332000,380000,416000};//truncate by thousand 

//static unsigned long long usage_p[5],time_p[5];
//static unsigned long long g_idle_p,g_time_p;

int cpu_on;

sp<IBinder> binder;

int pid = -1;
int game = -1;
int start_sampling;
/********************************
*	/proc   manager             *
*********************************/

static int read_cmdline(char *filename, char *name);
int check_app_fg(pid_t pid);
void check_list_apps(int* pid_o, int* app_index);
/********************************
*	FPS				            *
*********************************/
sp<IBinder> get_surfaceflinger();
int get_fps(sp<IBinder>& binder);
/*
int FPS_CU[62];
int FPS_GU[62];
int FPS_CF[62];
int FPS_GF[62];
int FPS_HP[62];
*/
int CF_CU[freq_level];
int CF_GU[freq_level];
int CF_FPS[freq_level];

int GF_CU[gpu_freq_level];
int GF_GU[gpu_freq_level];
int GF_FPS[gpu_freq_level];

int HP_CU[core_num];
int HP_GU[core_num];
int HP_FPS[core_num];

/********************************
*	GPU				            *
*********************************/
int get_gpu_freq();
void set_gpu_freq(int freq);
int get_gpu_util();
void get_gpu_time(unsigned long long* idle,unsigned long long* total);
int get_gpu_util(unsigned long long* idle_p,unsigned long long* total_p);
/********************************
*	CPU			            	*
*********************************/
int get_cpu_freq(int c);
void set_cpu_freq(int cpu,int freq);
void set_cpu_on(int cpu,int on);
void get_cpu_time(int cpu,unsigned long long* busy,unsigned long long* total);
int get_cpu_util(int cpu,unsigned long long* usage_p,unsigned long long* time_p);
int get_cpu_util(int c);
/********************************
*	Battery		            	*
*********************************/

int get_battery(char c);
/********************************
*	Governor	            	*
*********************************/
void platform_init();
void* show_result(void*);

/********************************
*	TOOLS  		            	*
*********************************/
int update_adjust(int pre,int cur){
	int div = cur - pre;
	//printf("%d %d\n",pre,cur);
	if(div > 0){
		return div / 3;
	}else if(div < 0){
		return div / 3 * 2;
	}else{
		return 0;
	}
}

void swap(int* a , int* b){
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
}
void sort(int* begging , int* end){
	int *cur = begging+1;
	while(cur <= end){
		int *test = cur;		
		while(test > begging){
			int *pre = test-1;
			if(*test < *pre){
				swap(test,pre);
				test = pre;
				continue;
			}else{
				break;
			}			
		}
		cur++;
	}
}

void update_Gvalue();

int ABS(int t){
	return (t < 0)? -t : t;
}
double reciprocal(double o){
	if(o){
		return 1.0/o;
	}
	return 0;
}
int AVG(int a[],int size){ 
	int sum=0;
	for(int k = 0; k < size;k++){
		sum += a[k];	
		//printf("AVG : %d %d\n",sum,a[k]);
	}
	return sum / size;
}
/********************************
*	Global variable        	*
*********************************/
enum scale_state{
	SCALE_UP,
	SCALE_DOWN,
	SCALE_BALANCE,
	SCALE_CHECK,
	SCALE_U_Based,
	SCALE_Q_Based
};


int g_cu0,g_cu1,g_cu2,g_cu3,g_cu;
int g_gu;
int g_fps;

float in_cw;
float in_gw;

float w_gf = 0.2;	//scale_up
float w_cf = 0.2;

float w_gf_d = 0.2;	//scale_down
float w_cf_d = 0.2;


float w_g2c = 0.02; //learning rate
float w_c2g = 0.02;

float w_g2c_l ; //learn dev
float w_c2g_l ;

int scale_period = 1000000; //us
double learn_rate = 0.2;
int End_Test_Tick = 0;
const int End_Test = 90;	//End at End_Test seconds
							//set 0 for no limit
/********************************
*	Algorithms and Parameters  	*
*********************************/
#define SEQUENTIAL 	0	// 0 no debug
						// 1 Sequential info
						// 2 Blocking info
						
#define FL_DEBUG 	0	// 1 print if FL Change
						

#define CPU_TRAIN	0   // 0 vary GPU
						// 1 vary CPU
#define ALG_SEL 	5	// -1 coef training
						// 0 ref1
						// 1 alg1
						// 2 alg2 balance target
						// 3 alg3 C5->3 target
						// 4 Auto Sense, Balance, Max 3 UT
						// 5 
					
#define DEFAULT_GOV	0	// 0 userspace
						// 1 ondemand
						// 2 interactive
						// 3 performance
						// 4 powersave
						// 5 conservative


#define QT_FIX 1	// 0 variate by algorithms
					// 1 to fix Q target 
					
#if QT_FIX == 1
	int Q_target_set = 31;
#endif

#define AUTO_SENSE 	0	// 0 By setting
						// 1 AUTO C/G

//for alg4						
#define U_TARGETS	3	// 0 C 5->3
						// 1 Average
						// 2 Max
						// 3 Max 3

#define QT_ADJUST	0	// 0 for nothing
						// 1 for less by 2 frames
						// 2 for less by 4 frames
						// 3 for less by 5% frames
						// 4 for less by 10% frames

//============================						
void *odm(void*);
//============================
//============================
void *ref1(void*);
//============================
//============================
void *alg1(void*);
//============================
//============================
void *alg2(void*);
//============================
//============================
void *alg3(void*);
//============================
//============================
void *alg4(void*);
//============================
const int QLB = 0;
const int QUB = 4;
//============================
void *alg5(void*);
//============================
#endif