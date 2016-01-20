/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "SM.h"
//#include "AG.h"

using namespace android;
//using namespace std;

/********************************
*	/proc   manager             *
*********************************/
#include <dirent.h>
#include <ctype.h>

#define PROC_NAME_LEN 64
#define THREAD_NAME_LEN 32
#define POLICY_NAME_LEN 4

#define Game_num 7
const char* Game_List[]={	"com.gameloft.android.ANMP.GloftA8HM",
							"jp.co.translimit.braindots",
							"com.zingmagic.bridgevfree",
							"com.raptor.furious7",
							"com.x3m.tx3",
							"com.phosphorgames.DarkMeadow",
							"com.myplay1.g013g.prod2"};

static int read_cmdline(char *filename, char *name) {
    FILE *file;
    char line[256];
    line[0] = '\0';
    file = fopen(filename, "r");
    if (!file) return 1;
    fgets(line, 256, file);
    fclose(file);
    if (strlen(line) > 0) {
        strncpy(name, line, PROC_NAME_LEN);
        name[PROC_NAME_LEN-1] = 0;
    } else
        name[0] = 0;
    return 0;
}

int check_app_fg(pid_t pid){
	SchedPolicy p;
	get_sched_policy(pid, &p);	
	if(strcmp("fg",get_sched_policy_name(p))==0){
		return 1;
	}else if(strcmp("bg",get_sched_policy_name(p))==0){
		return 2;
	}else{
		return 0;
	}
}

void check_list_apps(int* pid_o, int* app_index){	
	DIR *proc_dir;
	struct dirent *pid_dir;
	char filename[64];
    FILE *file;    
    
	pid_t pid;
	char name[PROC_NAME_LEN];
	
	proc_dir = opendir("/proc");   
	if (!proc_dir) printf("Could not open /proc.\n");
	
	while ((pid_dir = readdir(proc_dir))) {		
		if (!isdigit(pid_dir->d_name[0])){			
			continue;
		}
		pid = atoi(pid_dir->d_name);		
		pid = pid;		
		sprintf(filename, "/proc/%d/cmdline", pid);
        read_cmdline(filename, name);//proc->name
		
		for(int g=0;g<Game_num;g++){
			if(strcmp(name,Game_List[g])==0){
				if(check_app_fg(pid)==1){
					*pid_o = pid;
					*app_index = g;
					return;
				}
			}
		}		
	}
	*pid_o = -1;
	*app_index = -1;
}

/********************************
*	FPS				            *
*********************************/
sp<IBinder> get_surfaceflinger(){
	sp<IServiceManager> sm(defaultServiceManager());
	sp<IBinder> binder = sm->getService(String16("SurfaceFlinger"));
	return (binder == 0)? 0 : binder;	
}

int get_fps(sp<IBinder>& binder){	
	Parcel data,reply;	
	binder->transact(BnSurfaceComposer::GET_FPS,data,&reply);
	return (int)reply.readInt32();
}

/********************************
*	GPU				            *
*********************************/
int get_gpu_freq(){
	char s[100];
	FILE *gpu_f;
	
	sprintf(s,"%s/gpu_current_speed",gpu_fs);	
	gpu_f = fopen(s,"r");
	if(gpu_f){
		fgets(s,100,gpu_f);
		int f = atoi(s);
		fclose(gpu_f);
		return f;
	}else{
		return 0;
	}
	
}

void set_gpu_freq(int freq){
	//return;
	char s[100];
	FILE *gpu_f;	
	sprintf(s,"%s/set_speed",gpu_fs);	
	gpu_f = fopen(s,"w");
	if(gpu_f){
		fprintf(gpu_f,"%d",freq*1000);
		fclose(gpu_f);
		return;
	}else{
		return;
	}	
}
void get_gpu_time(unsigned long long* idle,unsigned long long* total){
	char s[100];
	FILE *gpu_u;
	
	sprintf(s,"%s/Time",gpu_fs);	
	gpu_u = fopen(s,"r");
	if(gpu_u){
		fgets(s,100,gpu_u);
		sscanf(s,"%llu",total);
		fclose(gpu_u);		
	}else{
		*total = 0;
	}	
	sprintf(s,"%s/Idle",gpu_fs);	
	gpu_u = fopen(s,"r");
	if(gpu_u){
		fgets(s,100,gpu_u);
		sscanf(s,"%llu",idle);
		fclose(gpu_u);		
	}else{
		*idle = 0;
	}
	
}
int get_gpu_util(unsigned long long* idle_p,unsigned long long* total_p){
	
	unsigned long long idle,time;
	get_gpu_time(&idle,&time);
	
	if(time == *total_p)
		return 0;
	int u = (int)(100 - (idle - *idle_p)*100 / (time - *total_p));
	*idle_p = idle;
	*total_p = time;
	return (u < 0)? 0 : u;	
}
int get_gpu_util(){
	return 0;
}

/********************************
*	CPU			            	*
*********************************/
int get_cpu_freq(int c){
	char s[100];
	FILE *cpu_f;
	
	sprintf(s,"%s/cpu%d/cpufreq/cpuinfo_cur_freq",cpu_fs,c);	
	cpu_f = fopen(s,"r");
	if(cpu_f){
		fgets(s,100,cpu_f);
		int f = atoi(s);
		fclose(cpu_f);
		return f;
	}else{
		return 0;
	}	
}

void set_cpu_freq(int cpu,int freq){
	//return;
	char s[100];
	FILE *cpu_f;
	//printf("Set CPU %d\n",freq);
	sprintf(s,"%s/cpu%d/cpufreq/scaling_setspeed",cpu_fs,cpu);	
	cpu_f = fopen(s,"w");
	if(cpu_f){
		fprintf(cpu_f,"%d",freq);
		fclose(cpu_f);
		return;
	}else{
		return;
	}	
}

void set_cpu_on(int cpu,int on){
	char s[100];
	FILE *cpu_onoff;
	sprintf(s,"%s/cpu%d/online",cpu_fs,cpu);	
	cpu_onoff = fopen(s,"w");
	if(cpu_onoff){
		fprintf(cpu_onoff,"%d",on);
		fclose(cpu_onoff);
		(on) ? cpu_on += 1 : cpu_on-=1;
		return;
	}else{
		return;
	}
}

void get_cpu_time(int cpu,unsigned long long* busy,unsigned long long* total){
	char s[100],t[100];
	//unsigned long long usage,time;
	FILE *cpu_u;
	
	sprintf(s,"/proc/utilize");
	if(cpu == core_num){
		sprintf(t,"cpu");
	}else{
		sprintf(t,"cpu%d",cpu);		
	}
	//printf("%s\n",t);
	cpu_u = fopen(s,"r");
	if(cpu_u){		
		while(fgets(s,100,cpu_u)){
			if(strstr(s,t)){
				sscanf(s,"%s %llu %llu",t,busy,total);
				//printf("%s %llu / %llu = %d\n",t,usage,time,(int)(((double)usage/time)*100.0));
				fclose(cpu_u);				
			}
		}		
	}else{
		fclose(cpu_u);
		*busy = 0;
		*total = 0;		
	}	
}

int get_cpu_util(int cpu,unsigned long long* usage_p,unsigned long long* time_p){
		
	unsigned long long usage,time;
	get_cpu_time(cpu,&usage,&time);
	int u;
	if(cpu == core_num){
		u = (int)(((double)(usage-*usage_p)/(time-*time_p))*400.0); 
	}else{
		u = (int)(((double)(usage-*usage_p)/(time-*time_p))*100.0); 
	}
	
	
	*usage_p= usage;
	*time_p= time;	
	return u; 
	
}

int get_cpu_util(int c){
	return 0;
}

/********************************
*	Battery		            	*
*********************************/

int get_battery(char c){
	char s[100];
	FILE *bat;
	
	switch (c){
		case 'V':
			sprintf(s,"%s/voltage_now",battery);			
			break;
		case 'I':
			sprintf(s,"%s/current_now",battery);			
			break;
		case 'C':
			sprintf(s,"%s/capacity",battery);			
			break;
		case 'T':
			sprintf(s,"%s/temp",battery);			
			break;
		default:
			sprintf(s,"%s/temp",battery);		
	}	
	bat = fopen(s,"r");
	if(bat){
		fgets(s,100,bat);
		int f = atoi(s);
		fclose(bat);
		return f;
	}else{
		return -1;
	}	
}
/********************************
*	Governor	            	*
*********************************/
void platform_init(){
	
	cpu_on = 1;
	char s[100];
	//conservative ondemand userspace powersave interactive performance
	for(int c = 0;c < core_num;c++){
		set_cpu_on(c,1);		
#if DEFAULT_GOV == 0	
		sprintf(s,"su -c echo userspace >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
#elif DEFAULT_GOV == 1		
		sprintf(s,"su -c echo ondemand >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
		printf("Initial to Ondemand\n");
#elif DEFAULT_GOV == 2
		sprintf(s,"su -c echo interactive >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
		printf("Initial to Interactive\n");
#elif DEFAULT_GOV == 3 
		sprintf(s,"su -c echo performance >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
		printf("Initial to Performance\n");
#elif DEFAULT_GOV == 4 
		sprintf(s,"su -c echo powersave >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
		printf("Initial to Powersave\n");
#elif DEFAULT_GOV == 5 
		sprintf(s,"su -c echo conservative >> %s/cpu%d/cpufreq/scaling_governor",cpu_fs,c);
		printf("Initial to Conservative\n");
#endif	
		system(s);
		//get_cpu_util(c);
	}
	system("su -c echo 0 > /sys/module/cpu_tegra3/parameters/auto_hotplug");
	set_cpu_on(1,0);
	set_cpu_on(2,0);
	set_cpu_on(3,0);
	set_cpu_freq(0,FL[0]);
	set_cpu_on(1,1);
	set_cpu_on(2,1);
	set_cpu_on(3,1);

#if DEFAULT_GOV == 0
	//set 1 , GPU_F Userspace
	system("su -c echo 1 > /sys/kernel/debug/tegra_host/scaling/manual");
#else
	system("su -c echo 0 > /sys/kernel/debug/tegra_host/scaling/manual");
#endif
	//set 1 , GPU always on
	system("su -c echo 1 > /sys/kernel/debug/tegra_host/scaling/online");
	
	int fin;
	FILE *para  = fopen("/data/SM_Para","r");	
	fscanf(para,"%d",&fin);
	set_cpu_freq(0,FL[fin]);
	fscanf(para,"%d",&fin);
	set_gpu_freq(GFL[fin]);	
	fscanf(para,"%d",&fin);
	set_cpu_on(1,fin);
	fscanf(para,"%d",&fin);
	set_cpu_on(2,fin);
	fscanf(para,"%d",&fin);
	set_cpu_on(3,fin);
	fscanf(para,"%d",&manual);	
	fscanf(para,"%d",&CPU_Sensitive);
	fclose(para);
}
void update_Gvalue(){
	g_cu0=get_cpu_util(0);
	g_cu1=get_cpu_util(1);
	g_cu2=get_cpu_util(2);
	g_cu3=get_cpu_util(3);
	g_cu=g_cu0+g_cu1+g_cu2+g_cu3;
	
	g_gu = get_gpu_util();
	g_fps = get_fps(binder);
	
	//printf("%d\t%d\t%d\t%d\t%d\t%d\n",g_cu0,g_cu1,g_cu2,g_cu3,g_cu,g_gu);
}
void* show_result(void*){
	static unsigned long long usage_p[5],time_p[5];
	static unsigned long long g_idle_p,g_time_p;
	
	for(int c=0;c < 4;c++){
		get_cpu_time(c,&usage_p[c],&time_p[c]);
	}
	get_gpu_time(&g_idle_p,&g_time_p);
	
	int sample;
	float p_result,p_cur;
	int cpu_ru,cpu_cu;
	int gpu_ru,gpu_cu;
	int fps_r,fps_c,fps_t;
	End_Test_Tick = 0;
	while (1){	
		
		sample=0;
		p_result=0;p_cur=0;
		cpu_ru=0;cpu_cu=0;
		fps_r=0;fps_c=0;fps_t=0;
		gpu_ru=0;gpu_cu=0;
		
		while(pid == -1 && game == -1){
			sleep(1);
			check_list_apps(&pid,&game);			
		}
		int s_time = (int)float(ns2ms(systemTime()))/1000;
		printf("Pid=%d game=%d\n",pid,game);
		
		while(start_sampling != 5){sleep(1);}		
		//printf("Time\tFPS\tGPU_F\tGPU_busy\tCPU0_F\tCPU0_U\tCPU1_F\tCPU1_U\tCPU2_F\tCPU2_U\tCPU3_F\tCPU3_U\tPower\n");		
		printf("Time\tFPS\tGPU_F\tGPU_busy\tCPU_F\tCPU0_U\tCPU1_U\tCPU2_U\tCPU3_U\tCU\tPower\n");		
		while(check_app_fg(pid) == 1){			
			sample++;	
			
			int cu[4];
			for(int c=0;c < 4;c++){
				cu[c]=get_cpu_util(c,&usage_p[c],&time_p[c]);
			}
			
			p_cur = (get_battery('V')/1000*get_battery('I')/1000);
			cpu_cu = cu[0]+cu[1]+cu[2]+cu[3];		
			fps_c = get_fps(binder);
			gpu_cu = get_gpu_util(&g_idle_p,&g_time_p);
			/*
			if(sample < num_sample){
				printf("Sampling...\n");
				p_result = p_result - (p_result/sample) + (p_cur/sample);
				cpu_ru = cpu_ru - (cpu_ru/sample) + (cpu_cu/sample);
				gpu_ru = gpu_ru - (gpu_ru/sample) + (gpu_cu/sample);
				fps_r = fps_r - (fps_r/sample) + (fps_c/sample);
				sleep(verbal_period);
				continue;
			}else if(sample == num_sample){
				fps_t = fps_r;
				printf("Sampling finish at FPS:%d\n",fps_t);
			}
			*/
			printf("%f\t"
				"%d\t%d\t%d\t"
				"%d\t%d\t%d\t%d\t%d\t%d\t"
				"%d\n",
			float(ns2ms(systemTime()))/1000,
			fps_c,get_gpu_freq()/1000000,gpu_cu,
			get_cpu_freq(0),cu[0],cu[1],cu[2],cu[3],cpu_cu,
			-(int)p_cur);
			
			p_result = p_result - (p_result/sample) + (p_cur/sample);
			cpu_ru = cpu_ru - (cpu_ru/sample) + (cpu_cu/sample);
			gpu_ru = gpu_ru - (gpu_ru/sample) + (gpu_cu/sample);
			fps_r = fps_r - (fps_r/sample) + (fps_c/sample);
			/*
			if(cpu_cu > cpu_ru)cpu_ru=cpu_cu;
			if(gpu_cu > gpu_ru)gpu_ru=gpu_cu;
			if(fps_c > fps_r)fps_r=fps_c;
			*/
			/*
			printf("%f\t"
				"%d\t%d\t"
				"%d\t"
				"%d\n",
			float(ns2ms(systemTime()))/1000,
			fps_c,get_gpu_freq()/1000000,
			get_cpu_freq(0),
			-(int)p_cur);
			*/
			End_Test_Tick++;
			sleep(verbal_period);
		}
		printf("Pid=%d game=%d Not Working!!\n",pid,game);
		pid = -1;game = -1;
		int e_time = (int)float(ns2ms(systemTime()))/1000;
		printf("Time\tCPU\tGPU\tFPS\tPower\n");
		printf("%d\t%d\t%d\t%d\t%d\n",e_time-s_time,cpu_ru,gpu_ru,fps_r,-(int)p_result);
		/*					
		int i;
		printf("\n===========CF_CU==========\n");
		for(i = 0;i<freq_level;i++)printf("%d\t",CF_CU[i]);printf("\n===========CF_GU==========\n");
		for(i = 0;i<freq_level;i++)printf("%d\t",CF_GU[i]);printf("\n===========CF_FPS==========\n");
		for(i = 0;i<freq_level;i++)printf("%d\t",CF_FPS[i]);printf("\n===========GF_CU==========\n");
		
		for(i = 0;i<gpu_freq_level;i++)printf("%d\t",GF_CU[i]);printf("\n============GF_GU=========\n");
		for(i = 0;i<gpu_freq_level;i++)printf("%d\t",GF_GU[i]);printf("\n===========GF_FPS=========\n");
		for(i = 0;i<gpu_freq_level;i++)printf("%d\t",GF_FPS[i]);printf("\n===========HP_CU=========\n");
		
		for(i = 0;i<core_num;i++)printf("%d\t",HP_CU[i]);printf("\n===========HP_GU=========\n");
		for(i = 0;i<core_num;i++)printf("%d\t",HP_GU[i]);printf("\n==========HP_FPS===========\n");
		for(i = 0;i<core_num;i++)printf("%d\t",HP_FPS[i]);printf("\n========================\n");
		*/
		
		//get_sched_policy(pid, &p);
		//sched_setaffinity(pid,sizeof(cpu_set_t), &set);
		//printf("Pid=%d SP=%s\n",pid,get_sched_policy_name(p));			
		
	}	
	return 0;
}

void *gov_cpu_hp(void*){	
	static int pcn = 4;
	while(1){
		if(manual)continue;
		int cu0=get_cpu_util(0);
		int cu1=get_cpu_util(1);
		int cu2=get_cpu_util(2);
		int cu3=get_cpu_util(3);
		int fps = get_fps(binder);
		int gu = get_gpu_util();
		
		int cu = cu0+cu1+cu2+cu3;
		int compe;
		
		//if(FPS_CU[fps] > cu || FPS_CU[fps] == 0)FPS_CU[fps]=cu;
		//if(FPS_HP[fps] > pcn || FPS_HP[fps] == 0)FPS_HP[fps]=pcn;
		//FPS_CU[fps] += update_adjust(FPS_CU[fps],cu);
		//FPS_HP[fps] += update_adjust(FPS_HP[fps],pcn);
		printf("HP_GU %d\n",get_gpu_util());
		HP_CU[pcn] += update_adjust(HP_CU[pcn],get_cpu_util(core_num));
		HP_GU[pcn] += update_adjust(HP_GU[pcn],get_gpu_util());
		HP_FPS[pcn] += update_adjust(HP_FPS[pcn],fps);
		
		(cu%100>80) ? compe=2 : compe=1;
		int set_on_num  = (cu / 100)+compe;		
		pcn = set_on_num;
		for(int i=0;i<set_on_num;i++)
			set_cpu_on(i,1);
		for(int i=set_on_num;i<core_num;i++)
			set_cpu_on(i,0);
		
		usleep(CHP_period);
	}
	
	return 0;
}

void *gov_cpu_f(void*){
	static int FL_point = freq_level-2;
	static int pre_FL = FL_point;
	
	if(CF_CU[pre_FL]!=0 && CF_GU[pre_FL]!=0){
		FL_point += ((g_cu - CF_CU[pre_FL])/CF_CU[pre_FL])*in_cw; //cf by cu dev 
		w_c2g_l = ((g_gu - CF_GU[pre_FL])/CF_GU[pre_FL]) * w_c2g; //cf2gu relation		
		if(w_c2g_l < 0)w_c2g_l*=-1;
		
	}else{
		CF_CU[pre_FL] += update_adjust(CF_CU[pre_FL],g_cu);
		CF_GU[pre_FL] += update_adjust(CF_GU[pre_FL],g_gu);
		CF_FPS[pre_FL] += update_adjust(CF_FPS[pre_FL],g_fps);
		return 0;
	}
	
	
	if(FL_point != pre_FL){
		
		if(FL_point > freq_level-2){
			FL_point = freq_level-2;
		}else if(FL_point < 2){
			FL_point = 2;
		}		
		pre_FL = FL_point;
		set_cpu_freq(0,FL[FL_point]);			
	}
	
	CF_CU[pre_FL] += update_adjust(CF_CU[pre_FL],g_cu);
	CF_GU[pre_FL] += update_adjust(CF_GU[pre_FL],g_gu);
	CF_FPS[pre_FL] += update_adjust(CF_FPS[pre_FL],g_fps);
	return 0;
}

void *gov_gpu_f(void*){
	static int FL_point = gpu_freq_level-1;
	static int pre_FL = FL_point;
 	
	if(GF_GU[pre_FL]!=0 && GF_CU[pre_FL]!=0){
		FL_point += ((g_gu - GF_GU[pre_FL])/GF_GU[pre_FL])*in_gw; //gf by gu dev 
		w_g2c_l = ((g_cu - GF_CU[pre_FL])/GF_CU[pre_FL]) * w_g2c;  //gf2cu relation
		if(w_g2c_l < 0)w_g2c_l*=-1;	
	}else{
		GF_CU[pre_FL] += update_adjust(GF_CU[pre_FL],g_cu);
		GF_GU[pre_FL] += update_adjust(GF_GU[pre_FL],g_gu);
		GF_FPS[pre_FL] += update_adjust(GF_FPS[pre_FL],g_fps);
		return 0;
	}	
	
	//printf("FPS = %d FP = %d  %d\n",fps,FL_point,FPS_GF[pre_FL]);	
	
	if(FL_point != pre_FL){			
		if(FL_point > gpu_freq_level-1){
			FL_point = gpu_freq_level-1;
		}else if(FL_point < 0){
			FL_point = 0;
		}		
		pre_FL = FL_point;
		set_gpu_freq(GFL[FL_point]);			
	}	
	
	GF_CU[pre_FL] += update_adjust(GF_CU[pre_FL],g_cu);
	GF_GU[pre_FL] += update_adjust(GF_GU[pre_FL],g_gu);
	GF_FPS[pre_FL] += update_adjust(GF_FPS[pre_FL],g_fps);
	return 0;
}
#if ALG_SEL == 0
void *ref1(void*){		
	
	if(CPU_Sensitive)	
		printf("Ref 1\tCPU_Sensitive\n");
	else
		printf("Ref 1\tGPU_Sensitive\n");
	int target_fps = 0;
	int target_Ug = 0;
	int target_Uc = 0;
	
	start_sampling = 1;
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Fc,Fg;
	
	while(true){
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			if(target_fps < fps)target_fps = fps;
			if(target_Uc < Uc)target_Uc = Uc;
			if(target_Ug < Ug)target_Ug = Ug;		
			
			set_cpu_freq(0,FL[3]);
			set_gpu_freq(GFL[4]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			if(target_fps < fps)target_fps = fps;
			if(target_Uc < Uc)target_Uc = Uc;
			if(target_Ug < Ug)target_Ug = Ug;
			
			set_cpu_freq(0,FL[10]);
			set_gpu_freq(GFL[0]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			if(target_fps < fps)target_fps = fps;
			if(target_Uc < Uc)target_Uc = Uc;
			if(target_Ug < Ug)target_Ug = Ug;
			
			set_cpu_freq(0,FL[10]);
			set_gpu_freq(GFL[4]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Ug : %d , Uc : %d\n",target_fps,target_Ug,target_Uc);			
			start_sampling=5;
		}
#if QT_FIX == 1
		target_fps = Q_target_set;
#endif		
		fps = get_fps(binder);
		int fps_dev = (target_fps - fps)*100/target_fps;
		
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int Fc_adj = 0;
		int Fg_adj = 0;
		int Ug_adj = 0;
		if(fps_dev > 10){
			//printf("UP! %d %d %d\n",fps,target_fps,fps_dev);
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);
			
			int fps_dif = (target_fps - fps);
			int Uc_dev = (target_Uc - Uc)*100/target_Uc;
			int Ug_dev = (target_Ug - Ug)*100/target_Ug;
			
			
			if(CPU_Sensitive){
				//CPU Sensitive			
				Fc_adj = (double)fps_dif * reciprocal(Coef_Fc2Q[game]) + FL[FL_point];
				Ug_adj = Ug + (double)fps_dif * reciprocal(Coef_Ug2Q[game]);
				if(Ug_adj > target_Ug){
					Fg_adj = (double)(Ug_adj - target_Ug) * -reciprocal(Coef_Fg2Ug[game]) + GFL[GFL_point];
				}
			}else{
				//GPU Sensitive
				Fg_adj = (double)fps_dif * reciprocal(Coef_Fg2Q[game]) + GFL[GFL_point];
				Ug_adj = Ug + (double)fps_dif * reciprocal(Coef_Uc2Q[game]);
				if(Ug_adj > target_Ug){
					Fc_adj = (double)(Ug_adj - target_Ug) * -reciprocal(Coef_Fc2Uc[game]) + FL[FL_point];
				}
			}

			while(FL_point < freq_level && FL[FL_point] < Fc_adj)FL_point++;
			while(GFL_point < gpu_freq_level && GFL[GFL_point] < Fg_adj)GFL_point++;	
				
			
			
			if(FL_point != pre_FL){		
				if(FL_point > freq_level-2){
					FL_point = freq_level-2;
				}else if(FL_point < 2){
					FL_point = 2;
				}		
				pre_FL = FL_point;
				set_cpu_freq(0,FL[FL_point]);			
			}
			if(GFL_point != pre_GFL){			
				if(GFL_point > gpu_freq_level-1){
					GFL_point = gpu_freq_level-1;
				}else if(GFL_point < 0){
					GFL_point = 0;
				}		
				pre_GFL = GFL_point;
				set_gpu_freq(GFL[GFL_point]);			
			}
			
			
		}else if(fps_dev <= 10){			
			//printf("DOWN! POWER SAVE %d %d %d\n",fps,target_fps,fps_dev);
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);
			
			int Uc_dev = target_Uc - Uc;
			int Ug_dev = target_Ug - Ug;
			if(Uc_dev*100/Uc > 10)
				Fc_adj = (double)(Uc_dev) * reciprocal(Coef_Fc2Uc[game]) + FL[FL_point];
			if(Ug_dev*100/Ug > 10)
				Fg_adj = (double)(Ug_dev) * reciprocal(Coef_Fg2Ug[game]) + GFL[GFL_point];
			
			while(FL_point > 0 && FL[FL_point] > Fc_adj)FL_point--;
			while(GFL_point > 0 && GFL[GFL_point] > Fg_adj)GFL_point--;
			
			if(FL_point != pre_FL){		
				if(FL_point > freq_level-2){
					FL_point = freq_level-2;
				}else if(FL_point < 2){
					FL_point = 2;
				}		
				pre_FL = FL_point;
				set_cpu_freq(0,FL[FL_point]);			
			}
			
			
			if(GFL_point != pre_GFL){			
				if(GFL_point > gpu_freq_level-1){
					GFL_point = gpu_freq_level-1;
				}else if(GFL_point < 0){
					GFL_point = 0;
				}		
				pre_GFL = GFL_point;
				set_gpu_freq(GFL[GFL_point]);			
			}	
			
		}		
		sleep(1);
	}	
	return 0;
}
#elif ALG_SEL == 1
void *alg1(void*){	
	if(CPU_Sensitive)	
		printf("Alg 1\tCPU_Sensitive\n");
	else
		printf("Alg 1\tGPU_Sensitive\n");
	//constant
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	const int GFL_MAX = 4;	
	const int GFL_MIN = 0;
	const int AVG_INTERVAL = 3; // for average recording
	//scaling weight
	/*
	double Coef_Fc2Uc = 0;
	double Coef_Fc2Q = 0;
	double Coef_Fg2Ug = 0;
	double Coef_Fg2Q = 0;
	double Coef_Uc2Q = 0;
	double Coef_Ug2Q = 0;
	*/
	double Coef_Uc2Fc = 0;
	double Coef_Q2Fc = 0;
	double Coef_Ug2Fg = 0;
	double Coef_Q2Fg = 0;
	double Coef_Q2Uc = 0;
	double Coef_Q2Ug = 0;
	
	//target
	int Q_target[AVG_INTERVAL];  //for delta and average 
	int UC_target[AVG_INTERVAL];
	int UG_target[AVG_INTERVAL];
	int FC_record[AVG_INTERVAL]; //for delta 
	int FG_record[AVG_INTERVAL];	
	
	start_sampling = 1;
	
	//current info
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Rec_interval = 0;
	//for debugging
	int Fc,Fg;
	
	while(true){
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){//first sample and set min CF			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);
			//initial samples
			Q_target[start_sampling-1] = fps;//Coef_Fg2Q = Coef_Uc2Q =	Coef_Ug2Q = Coef_Fc2Q = 
			UC_target[start_sampling-1] = Uc;//Coef_Fc2Uc = 
			UG_target[start_sampling-1] = Ug;//Coef_Fg2Ug = 		
			FC_record[start_sampling-1] = FL[CFL_MIN]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MIN]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			
			//Fc vary, Fc-Q , Fc-Uc , Ug-Q
			Coef_Q2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Uc2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - fps);
			
			Q_target[start_sampling-1] = fps;				
			UC_target[start_sampling-1] = Uc;
			UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MIN];			
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MIN]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);

			//Fg vary, Fg-Q , Fg-Ug , Uc-Q
			
			Coef_Q2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Ug2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(UG_target[0] - Ug);
			//printf("Coef_Q2Fg = (%d - %d) / %d = %f\n",Q_target[0],fps,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Q2Fg);
			//printf("Coef_Ug2Fg = (%d - %d) / %d = %f\n",UG_target[0],Ug,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Ug2Fg);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - fps);
			
			Q_target[start_sampling-1] = fps;
			UC_target[start_sampling-1] = Uc;
			UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Uc : %d , Ug : %d\n",AVG(Q_target,AVG_INTERVAL),AVG(UC_target,AVG_INTERVAL),AVG(UG_target,AVG_INTERVAL));			
			printf("Coef_Q2Fc : %f\nCoef_Uc2Fc : %f\nCoef_Q2Ug : %f\n",Coef_Q2Fc,Coef_Uc2Fc,Coef_Q2Ug);			
			printf("Coef_Q2Fg : %f\nCoef_Ug2Fg : %f\nCoef_Q2Uc : %f\n",Coef_Q2Fg,Coef_Ug2Fg,Coef_Q2Uc);
#if SEQUENTIAL == 0
			start_sampling=5;
#else
			start_sampling=6;
#endif
			//return 0;
		}
		
		//Start Scaling
		//calculate pointers
		Rec_interval %= AVG_INTERVAL;
		int pre0 = Rec_interval; //current
		int pre1 = (Rec_interval+AVG_INTERVAL-1) % AVG_INTERVAL; //t-1
		int pre2 = (Rec_interval+AVG_INTERVAL-2) % AVG_INTERVAL; //t-2		
		//record current status
		Q_target[pre0] = get_fps(binder);
		UC_target[pre0] = get_cpu_util(4,&Uc_bp,&Uc_tp);
		UG_target[pre0] = get_gpu_util(&Ug_bp,&Ug_tp);
		//Compute New target
		static int QT = AVG(Q_target,AVG_INTERVAL); //Compute target Q
#if QT_FIX == 1
		QT = Q_target_set;
#endif
		static int UC_T = AVG(UC_target,AVG_INTERVAL); //Compute target UC
		static int UG_T = AVG(UG_target,AVG_INTERVAL); //Compute target UG
		
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int FC_Next = 0;
		int FG_Next = 0;
		int UC_Next = 0;
		int UG_Next = 0;
		
		double delta_Q = 0.0;
		double delta_FC = 0.0;
		double delta_Coef_Q2Fc = 0.0;
		double delta_Coef_Uc2Fc = 0.0;
		
		double delta_UG = 0.0; 
		double delta_Coef_Q2Ug = 0.0;
		
		double delta_FG = 0.0;
		double delta_Coef_Ug2Fg = 0.0;
		double delta_Coef_Q2Fg = 0.0;
		
		double delta_UC = 0.0;
		double delta_Coef_Q2Uc = 0.0;
		
		
		//Q(t)-Q(t-1)
		delta_Q = Q_target[pre0] - Q_target[pre1];
		//SCALE_Q_Based
		//if(delta_Q < -4){//dropping more than 4
			//calculate FC_Next				
			if(CPU_Sensitive){
				//FC(t-1)-FC(t-2)
				delta_FC = FC_record[pre1] - FC_record[pre2];
				//printf("Qt=%d , d_FC=%f , d_Q=%f\n",QT,delta_FC,delta_Q);
				//if(delta_FC == 0)delta_FC=1.0;
				//delta_A = (A_new-A) * Learn
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//learn parameter
				Coef_Q2Fc += delta_Coef_Q2Fc;
				//printf("Coef_Q2Fc=%f , delta_Coef_Q2Fc=%f\n",Coef_Q2Fc,delta_Coef_Q2Fc);
				//printf("F_Next = %d + %d*%f = %d + %f\n",(int)FC_record[pre1],(QT - Q_target[pre0]),reciprocal(Coef_Q2Fc),(int)FC_record[pre1],(QT - Q_target[pre0])*reciprocal(Coef_Q2Fc));
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(QT - Q_target[pre0])*Coef_Q2Fc);			
				//calculate UG_Next
				//UG(t)-UG(t-1)
				delta_UG = UG_target[pre0] - UG_target[pre1];
				//printf("d_UG=%f\n",delta_UG);
				//if(delta_UG == 0)delta_UG=1.0;
				//delta_A = (A_new-A) * Learn		
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				//learn parameter
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				//printf("Coef_Q2Ug=%f , delta_Coef_Q2Ug=%f\n",Coef_Q2Ug,delta_Coef_Q2Ug);
				//printf("UG_Next = %d + %d*%f = %d + %f\n",(int)UG_target[pre0],(QT - Q_target[pre0]),reciprocal(Coef_Q2Ug),(int)UG_target[pre0],(QT - Q_target[pre0])*reciprocal(Coef_Q2Ug));
				//Compute UG
				UG_Next = UG_target[pre0] + (QT - Q_target[pre0]) * Coef_Q2Ug;			
				//Learn UG-FG Coef Coef_Ug2Fg
				//FG(t-1)-FG(t-2)
				delta_FG = FG_record[pre1] - FG_record[pre2];
				//if(delta_FG == 0)delta_FG=1.0;		
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;
				//printf("d_FG=%f\nCoef_Ug2Fg=%f , delta_Coef_Ug2Fg=%f\n",delta_FG,Coef_Ug2Fg,delta_Coef_Ug2Fg);
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_target[pre0])*Coef_Ug2Fg);	
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				//if(delta_FG == 0)delta_FG=1.0;
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(QT - Q_target[pre0])*Coef_Q2Fg);			
				//Learn UC-Q			
				delta_UC = UC_target[pre0] - UC_target[pre1];
				//if(delta_UC == 0)delta_UC=1.0;
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;			
				//Compute prediction UC
				UC_Next = UC_target[pre0] + (QT - Q_target[pre0]) * Coef_Q2Uc;			
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				//if(delta_FC == 0)delta_FC=1.0;		
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_target[pre0])*Coef_Uc2Fc);	
			}	
			
			//find the fitness FL
			FL_point = 0;		
			while(FL_point < freq_level && FL[FL_point] < FC_Next)FL_point++;
			//printf("F_Next=%d , F_set=%d\n",FC_Next,FL[FL_point]);
			//find the fitness GFL
			GFL_point = 0;		
			while(GFL_point < gpu_freq_level && GFL[GFL_point] < FG_Next)GFL_point++;
		//}else{//SCALE_U_Based
			
		
		//}	
		
		if(FL_point != pre_FL){		
			if(FL_point > freq_level-2){
				FL_point = freq_level-2;
			}else if(FL_point < 3){
				FL_point = 3;
			}		
			pre_FL = FL_point;
			set_cpu_freq(0,FL[FL_point]);			
		}
		if(GFL_point != pre_GFL){			
			if(GFL_point > gpu_freq_level-1){
				GFL_point = gpu_freq_level-1;
			}else if(GFL_point < 0){
				GFL_point = 0;
			}		
			pre_GFL = GFL_point;
			set_gpu_freq(GFL[GFL_point]);			
		}
		
		//Restore current
		FC_record[pre0] = FL[FL_point];
		FG_record[pre0] = GFL[GFL_point];	
	
		if(start_sampling == 6){
#if SEQUENTIAL == 1	
			printf("%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%f\t%f\t%f\t%f\t%f\t%f\n"
					,AVG(Q_target,AVG_INTERVAL),Q_target[0],Q_target[1],Q_target[2]
					,AVG(UC_target,AVG_INTERVAL),UC_target[0],UC_target[1],UC_target[2]
					,AVG(UG_target,AVG_INTERVAL),UG_target[0],UG_target[1],UG_target[2]
					,FC_Next,FC_record[0],FC_record[1],FC_record[2]
					,FG_Next,FG_record[0],FG_record[1],FG_record[2]
					,Coef_Uc2Fc,Coef_Q2Fc,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Q2Ug);
#elif SEQUENTIAL == 2
			printf("===========================\n");			
			printf("Q_target(%d):\t",AVG(Q_target,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",Q_target[i]);
			printf("\nUC_target(%d):\t",AVG(UC_target,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UC_target[i]);
			printf("\nUG_target(%d):\t",AVG(UG_target,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UG_target[i]);
			printf("\nFC_record(%d):\t",FC_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FC_record[i]);
			printf("\nFG_record(%d):\t",FG_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FG_record[i]);
			printf("\nCoef_Q2Fc:%f\nCoef_Q2Ug:%f\nCoef_Ug2Fg:%f\nCoef_Q2Fg:%f\nCoef_Q2Uc:%f\nCoef_Uc2Fc:%f\n"
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);		
			printf("===========================\n");
#endif			
		}
		Rec_interval++;
		
		

		sleep(1);
	}	
	return 0;
}
#elif ALG_SEL == 2
void *alg2(void*){	
	if(CPU_Sensitive)	
		printf("Alg 2\tCPU_Sensitive\n");
	else
		printf("Alg 2\tGPU_Sensitive\n");
	//constant
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	const int GFL_MAX = 4;	
	const int GFL_MIN = 0;
	const int AVG_INTERVAL = 3; // for average recording
	//scaling weight
	/*
	double Coef_Fc2Uc = 0;
	double Coef_Fc2Q = 0;
	double Coef_Fg2Ug = 0;
	double Coef_Fg2Q = 0;
	double Coef_Uc2Q = 0;
	double Coef_Ug2Q = 0;
	*/
	double Coef_Uc2Fc = 0;
	double Coef_Q2Fc = 0;
	double Coef_Ug2Fg = 0;
	double Coef_Q2Fg = 0;
	double Coef_Q2Uc = 0;
	double Coef_Q2Ug = 0;
	
	//target
	int Q_target[AVG_INTERVAL];  //for average 
	int UC_target[AVG_INTERVAL];
	int UG_target[AVG_INTERVAL];
	
	int FC_record[AVG_INTERVAL]; //for delta 
	int FG_record[AVG_INTERVAL];	
	int Q_record[AVG_INTERVAL];  
	int UC_record[AVG_INTERVAL];
	int UG_record[AVG_INTERVAL];
	
	start_sampling = 1;
	
	//current info
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Rec_interval = 0;
	//for debugging
	int Fc,Fg;
	
	while(true){
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){//first sample and set min CF			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);
			//initial samples
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;//Coef_Fg2Q = Coef_Uc2Q =	Coef_Ug2Q = Coef_Fc2Q = 
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;//Coef_Fc2Uc = 
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;//Coef_Fg2Ug = 		
			FC_record[start_sampling-1] = FL[CFL_MIN]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MIN]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			
			//Fc vary, Fc-Q , Fc-Uc , Ug-Q
			Coef_Q2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Uc2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;				
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MIN];			
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MIN]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);

			//Fg vary, Fg-Q , Fg-Ug , Uc-Q
			
			Coef_Q2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Ug2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(UG_target[0] - Ug);
			//printf("Coef_Q2Fg = (%d - %d) / %d = %f\n",Q_target[0],fps,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Q2Fg);
			//printf("Coef_Ug2Fg = (%d - %d) / %d = %f\n",UG_target[0],Ug,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Ug2Fg);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Uc : %d , Ug : %d\n",AVG(Q_target,AVG_INTERVAL),AVG(UC_target,AVG_INTERVAL),AVG(UG_target,AVG_INTERVAL));			
			printf("Coef_Q2Fc : %f\nCoef_Uc2Fc : %f\nCoef_Q2Ug : %f\n",Coef_Q2Fc,Coef_Uc2Fc,Coef_Q2Ug);			
			printf("Coef_Q2Fg : %f\nCoef_Ug2Fg : %f\nCoef_Q2Uc : %f\n",Coef_Q2Fg,Coef_Ug2Fg,Coef_Q2Uc);
#if SEQUENTIAL == 0
			start_sampling=5;
#else
			start_sampling=6;
#endif
			//return 0;
		}
		
		//Start Scaling
		//calculate pointers
		Rec_interval %= AVG_INTERVAL;
		int pre0 = Rec_interval; //current
		int pre1 = (Rec_interval+AVG_INTERVAL-1) % AVG_INTERVAL; //t-1
		int pre2 = (Rec_interval+AVG_INTERVAL-2) % AVG_INTERVAL; //t-2		
		//record current status
		Q_record[pre0] = get_fps(binder);
		UC_record[pre0] = get_cpu_util(4,&Uc_bp,&Uc_tp);
		UG_record[pre0] = get_gpu_util(&Ug_bp,&Ug_tp);
		//Compute New target
		static int QT = AVG(Q_target,AVG_INTERVAL); //Compute target Q
#if QT_FIX == 1
		QT = Q_target_set;
#endif
		static int UC_T = AVG(UC_target,AVG_INTERVAL); //Compute target UC
		static int UG_T = AVG(UG_target,AVG_INTERVAL); //Compute target UG
		
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int FC_Next = 0;
		int FG_Next = 0;
		int UC_Next = 0;
		int UG_Next = 0;
		
		double delta_Q = 0.0;
		double delta_FC = 0.0;
		double delta_Coef_Q2Fc = 0.0;
		double delta_Coef_Uc2Fc = 0.0;
		
		double delta_UG = 0.0; 
		double delta_Coef_Q2Ug = 0.0;
		
		double delta_FG = 0.0;
		double delta_Coef_Ug2Fg = 0.0;
		double delta_Coef_Q2Fg = 0.0;
		
		double delta_UC = 0.0;
		double delta_Coef_Q2Uc = 0.0;
		
		
		//Q(t)-Q(t-1)
		delta_Q = Q_record[pre0] - Q_record[pre1];
		//SCALE_Q_Based
		if(Q_record[pre0] - QT < -4 || Q_record[pre0] - QT > 4){//dropping or lifting more than 4
			//printf("Scale\n");
			if(CPU_Sensitive){
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);
								
				//System tuning				
				FC_Next = (int)(FC_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fc);			
				//printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fc,FC_Next);
				UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
				//printf("UG_Next = %d + %d * %f = %d\n",UG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Ug,UG_Next);				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);	
				//printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
				
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FC_Next = FL[pre_FL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FC_Next = FL[pre_FL-1];						
					}
				}
				/*
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL) && Coef_Ug2Fg == 0){
					if(UG_record[pre0] > UG_T){
						//Fg is too low
						//GFL+1
						printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						FG_Next = GFL[pre_GFL+1];						
					}else if(UG_record[pre0] < UG_T){
						//Fg is too high
						//GFL-1
						printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						FG_Next = GFL[pre_GFL-1];						
					}
				}
				*/
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);
				
				
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fg);	
				//printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fg,FG_Next);
				//Compute prediction UC
				UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
				//printf("UC_Next = %d + %d * %f = %d\n",UC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Uc,UC_Next);
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
				//printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						FG_Next = GFL[pre_GFL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						FG_Next = GFL[pre_GFL-1];						
					}
				}
				/*
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL) && Coef_Uc2Fc == 0){
					if(UC_record[pre0] > UC_T){
						//Fg is too low
						//GFL+1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FC_Next = FL[pre_FL+1];						
					}else if(UC_record[pre0] < UC_T){
						//Fg is too high
						//GFL-1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FC_Next = FL[pre_FL-1];						
					}
				}
				*/
			}	
			
			//find the fitness FL
			FL_point = 0;		
			while(FL_point < freq_level && FL[FL_point] < FC_Next)FL_point++;
			
			//find the fitness GFL
			GFL_point = 0;		
			while(GFL_point < gpu_freq_level && GFL[GFL_point] < FG_Next)GFL_point++;
		}else{//SCALE_U_Based
			//printf("Balance\n");
			//record current status
			for(int mv = 0; mv < AVG_INTERVAL-1; mv++){
				Q_target[mv] = Q_target[mv+1];
				UC_target[mv] = UC_target[mv+1];
				UG_target[mv] = UG_target[mv+1];
			}
			Q_target[AVG_INTERVAL-1] = Q_record[pre0];
			UC_target[AVG_INTERVAL-1] = UC_record[pre0];
			UG_target[AVG_INTERVAL-1] = UG_record[pre0];
			
			//Compute New target
			QT = AVG(Q_target,AVG_INTERVAL); //Compute target Q
			UC_T = AVG(UC_target,AVG_INTERVAL); //Compute target UC
			UG_T = AVG(UG_target,AVG_INTERVAL); //Compute target UG
			
			if(CPU_Sensitive){//Learning Only
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);		

				////check if GPU need to scale
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
					if(UG_record[pre0] > UG_T){
						//Fg is too low
						//GFL+1
						//printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						GFL_point = pre_GFL+1;						
					}else if(UG_record[pre0] < UG_T){
						//Fg is too high
						//GFL-1
						//printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						GFL_point = pre_GFL-1;
					}
				}
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);	

				////check if GPU need to scale
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
					if(UC_record[pre0] > UC_T){
						//Fg is too low
						//GFL+1
						//printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FL_point = pre_FL+1;						
					}else if(UC_record[pre0] < UC_T){
						//Fg is too high
						//GFL-1
						//printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FL_point = pre_FL-1;
					}
				}
			}
		
		}	
		
		if(FL_point != pre_FL){		
			if(FL_point > freq_level-2){
				FL_point = freq_level-2;
			}else if(FL_point < 3){
				FL_point = 3;
			}		
			pre_FL = FL_point;
			set_cpu_freq(0,FL[FL_point]);			
		}
		if(GFL_point != pre_GFL){			
			if(GFL_point > gpu_freq_level-1){
				GFL_point = gpu_freq_level-1;
			}else if(GFL_point < 0){
				GFL_point = 0;
			}		
			pre_GFL = GFL_point;
			set_gpu_freq(GFL[GFL_point]);			
		}
		
		//Restore current
		FC_record[pre0] = FL[FL_point];
		FG_record[pre0] = GFL[GFL_point];	
	
		if(start_sampling == 6){
#if SEQUENTIAL == 1			
			printf("%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%f\t%f\t%f\t%f\t%f\t%f\n"
					,AVG(Q_target,AVG_INTERVAL),Q_target[0],Q_target[1],Q_target[2]
					,AVG(UC_target,AVG_INTERVAL),UC_target[0],UC_target[1],UC_target[2]
					,AVG(UG_target,AVG_INTERVAL),UG_target[0],UG_target[1],UG_target[2]
					,FC_Next,FC_record[0],FC_record[1],FC_record[2]
					,FG_Next,FG_record[0],FG_record[1],FG_record[2]
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);
#elif SEQUENTIAL == 2			
			printf("===========================\n");			
			printf("Q_target(%d):\t",AVG(Q_target,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",Q_record[i]);
			printf("\nUC_target(%d):\t",AVG(UC_target,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UC_record[i]);
			printf("\nUG_record(%d):\t",AVG(UG_record,AVG_INTERVAL));
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UG_record[i]);
			printf("\nFC_record(%d):\t",FC_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FC_record[i]);
			printf("\nFG_record(%d):\t",FG_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FG_record[i]);
			printf("\nCoef_Q2Fc:%f\nCoef_Q2Ug:%f\nCoef_Ug2Fg:%f\nCoef_Q2Fg:%f\nCoef_Q2Uc:%f\nCoef_Uc2Fc:%f\n"
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);		
			printf("===========================\n");
#endif			
		}
		Rec_interval++;
		
		

		sleep(1);
	}	
	return 0;
}
#elif ALG_SEL == 3
void *alg3(void*){
	
	if(CPU_Sensitive)	
		printf("Alg 3\tCPU_Sensitive\n");
	else
		printf("Alg 3\tGPU_Sensitive\n");

	//constant
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	const int GFL_MAX = 4;	
	const int GFL_MIN = 0;
	const int AVG_INTERVAL = 5; // for average recording
	const int AVG_TAKE = 3;
	//scaling weight
	/*
	double Coef_Fc2Uc = 0;
	double Coef_Fc2Q = 0;
	double Coef_Fg2Ug = 0;
	double Coef_Fg2Q = 0;
	double Coef_Uc2Q = 0;
	double Coef_Ug2Q = 0;
	*/
	double Coef_Uc2Fc = 0;
	double Coef_Q2Fc = 0;
	double Coef_Ug2Fg = 0;
	double Coef_Q2Fg = 0;
	double Coef_Q2Uc = 0;
	double Coef_Q2Ug = 0;
	
	//target
	int Q_target[AVG_INTERVAL];  //for average 
	int UC_target[AVG_INTERVAL];
	int UG_target[AVG_INTERVAL];
	
	int FC_record[AVG_INTERVAL]; //for delta 
	int FG_record[AVG_INTERVAL];	
	int Q_record[AVG_INTERVAL];  
	int UC_record[AVG_INTERVAL];
	int UG_record[AVG_INTERVAL];
	
	start_sampling = 1;
	
	//current info
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Rec_interval = 3;
	//for debugging
	int Fc,Fg;
	
	while(true){
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){//first sample and set min CF			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);
			//initial samples
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;//Coef_Fg2Q = Coef_Uc2Q =	Coef_Ug2Q = Coef_Fc2Q = 
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;//Coef_Fc2Uc = 
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;//Coef_Fg2Ug = 		
			FC_record[start_sampling-1] = FL[CFL_MIN]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MIN]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			
			//Fc vary, Fc-Q , Fc-Uc , Ug-Q
			Coef_Q2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Uc2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;				
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MIN];			
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MIN]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);

			//Fg vary, Fg-Q , Fg-Ug , Uc-Q
			
			Coef_Q2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Ug2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(UG_target[0] - Ug);
			//printf("Coef_Q2Fg = (%d - %d) / %d = %f\n",Q_target[0],fps,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Q2Fg);
			//printf("Coef_Ug2Fg = (%d - %d) / %d = %f\n",UG_target[0],Ug,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Ug2Fg);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Uc : %d , Ug : %d\n",AVG(Q_target,AVG_INTERVAL),AVG(UC_target,AVG_INTERVAL),AVG(UG_target,AVG_INTERVAL));			
			printf("Coef_Q2Fc : %f\nCoef_Uc2Fc : %f\nCoef_Q2Ug : %f\n",Coef_Q2Fc,Coef_Uc2Fc,Coef_Q2Ug);			
			printf("Coef_Q2Fg : %f\nCoef_Ug2Fg : %f\nCoef_Q2Uc : %f\n",Coef_Q2Fg,Coef_Ug2Fg,Coef_Q2Uc);
#if SEQUENTIAL == 0
			start_sampling=5;
#else
			start_sampling=6;
#endif
			//return 0;
		}
		
		//Start Scaling
		//calculate pointers
		Rec_interval %= AVG_INTERVAL;
		int pre0 = Rec_interval; //current
		int pre1 = (Rec_interval+AVG_INTERVAL-1) % AVG_INTERVAL; //t-1
		int pre2 = (Rec_interval+AVG_INTERVAL-2) % AVG_INTERVAL; //t-2		
		//record current status
		Q_record[pre0] = get_fps(binder);
		UC_record[pre0] = get_cpu_util(4,&Uc_bp,&Uc_tp);
		UG_record[pre0] = get_gpu_util(&Ug_bp,&Ug_tp);
		
		//current status copy
		for(int mv = 0; mv < AVG_INTERVAL; mv++){
			Q_target[mv] = Q_record[mv];
			UC_target[mv] = UC_record[mv];
			UG_target[mv] = UG_record[mv];
		}		
		sort(Q_target,Q_target+AVG_INTERVAL-1);
		sort(UC_target,UC_target+AVG_INTERVAL-1);
		sort(UG_target,UG_target+AVG_INTERVAL-1);
		//Compute New target : take the middle 3
		int QT = AVG(Q_target+1,AVG_TAKE); //Compute target Q
#if QT_FIX == 1
		QT = Q_target_set;
#endif		
		int UC_T = AVG(UC_target+1,AVG_TAKE); //Compute target UC
		int UG_T = AVG(UG_target+1,AVG_TAKE); //Compute target UG

		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int FC_Next = 0;
		int FG_Next = 0;
		int UC_Next = 0;
		int UG_Next = 0;
		
		double delta_Q = 0.0;
		double delta_FC = 0.0;
		double delta_Coef_Q2Fc = 0.0;
		double delta_Coef_Uc2Fc = 0.0;
		
		double delta_UG = 0.0; 
		double delta_Coef_Q2Ug = 0.0;
		
		double delta_FG = 0.0;
		double delta_Coef_Ug2Fg = 0.0;
		double delta_Coef_Q2Fg = 0.0;
		
		double delta_UC = 0.0;
		double delta_Coef_Q2Uc = 0.0;
	
		//Q(t)-Q(t-1)
		delta_Q = Q_record[pre0] - Q_record[pre1];
		//SCALE_Q_Based
		if(Q_record[pre0] - QT < -4 || Q_record[pre0] - QT > 4){//dropping or lifting more than 4
			//printf("Scale\n");
			if(CPU_Sensitive){
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);
								
				//System tuning				
				FC_Next = (int)(FC_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fc);			
				//printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fc,FC_Next);
				UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
				//printf("UG_Next = %d + %d * %f = %d\n",UG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Ug,UG_Next);				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);	
				//printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
				
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FC_Next = FL[pre_FL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FC_Next = FL[pre_FL-1];						
					}
				}
				/*
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL) && Coef_Ug2Fg == 0){
					if(UG_record[pre0] > UG_T){
						//Fg is too low
						//GFL+1
						printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						FG_Next = GFL[pre_GFL+1];						
					}else if(UG_record[pre0] < UG_T){
						//Fg is too high
						//GFL-1
						printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						FG_Next = GFL[pre_GFL-1];						
					}
				}
				*/
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);
				
				
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fg);	
				//printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fg,FG_Next);
				//Compute prediction UC
				UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
				//printf("UC_Next = %d + %d * %f = %d\n",UC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Uc,UC_Next);
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
				//printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						FG_Next = GFL[pre_GFL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						FG_Next = GFL[pre_GFL-1];						
					}
				}
				/*
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL) && Coef_Uc2Fc == 0){
					if(UC_record[pre0] > UC_T){
						//Fg is too low
						//GFL+1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FC_Next = FL[pre_FL+1];						
					}else if(UC_record[pre0] < UC_T){
						//Fg is too high
						//GFL-1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FC_Next = FL[pre_FL-1];						
					}
				}
				*/
			}	
			
			//find the fitness FL
			FL_point = 0;		
			while(FL_point < freq_level && FL[FL_point] < FC_Next)FL_point++;
			
			//find the fitness GFL
			GFL_point = 0;		
			while(GFL_point < gpu_freq_level && GFL[GFL_point] < FG_Next)GFL_point++;
		}else{//SCALE_U_Based
			//printf("Balance\n");
			
			if(CPU_Sensitive){//Learning Only
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);		
				
				////check if GPU need to scale
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
					if(UG_record[pre0] > UG_T){
						//Fg is too low
						//GFL+1
						//printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
						GFL_point = pre_GFL+1;						
					}else if(UG_record[pre0] < UG_T){
						//Fg is too high
						//GFL-1
						//printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
						GFL_point = pre_GFL-1;
					}
				}
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);	
				
				////check if GPU need to scale
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
					if(UC_record[pre0] > UC_T){
						//Fg is too low
						//GFL+1
						//printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
						FL_point = pre_FL+1;						
					}else if(UC_record[pre0] < UC_T){
						//Fg is too high
						//GFL-1
						//printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
						FL_point = pre_FL-1;
					}
				}
			}
		
		}	
		
		if(FL_point != pre_FL){		
			if(FL_point > freq_level-2){
				FL_point = freq_level-2;
			}else if(FL_point < 3){
				FL_point = 3;
			}		
			pre_FL = FL_point;
			set_cpu_freq(0,FL[FL_point]);			
		}
		if(GFL_point != pre_GFL){			
			if(GFL_point > gpu_freq_level-1){
				GFL_point = gpu_freq_level-1;
			}else if(GFL_point < 0){
				GFL_point = 0;
			}		
			pre_GFL = GFL_point;
			set_gpu_freq(GFL[GFL_point]);			
		}
		
		//Restore current
		FC_record[pre0] = FL[FL_point];
		FG_record[pre0] = GFL[GFL_point];	
	
		if(start_sampling == 6){
#if SEQUENTIAL == 1			
			printf("%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%f\t%f\t%f\t%f\t%f\t%f\n"
					,AVG(Q_target,AVG_INTERVAL),Q_target[0],Q_target[1],Q_target[2],Q_target[3],Q_target[4]
					,AVG(UC_target,AVG_INTERVAL),UC_target[0],UC_target[1],UC_target[2],UC_target[3],UC_target[4]
					,AVG(UG_target,AVG_INTERVAL),UG_target[0],UG_target[1],UG_target[2],UG_target[3],UG_target[4]
					,FC_Next,FC_record[0],FC_record[1],FC_record[2],FC_record[3],FC_record[4]
					,FG_Next,FG_record[0],FG_record[1],FG_record[2],FG_record[3],FG_record[4]
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);
#elif SEQUENTIAL == 2			
			printf("===========================\n");			
			printf("Q_record(%d):\t",QT);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",Q_record[i]);
			printf("\nUC_target(%d):\t",UC_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UC_record[i]);
			printf("\nUG_record(%d):\t",UG_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UG_record[i]);
			printf("\nFC_record(%d):\t",FC_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FC_record[i]);
			printf("\nFG_record(%d):\t",FG_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FG_record[i]);
			printf("\nCoef_Q2Fc:%f\nCoef_Q2Ug:%f\nCoef_Ug2Fg:%f\nCoef_Q2Fg:%f\nCoef_Q2Uc:%f\nCoef_Uc2Fc:%f\n"
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);		
			printf("===========================\n");
#endif			
		}
		Rec_interval++;
		
		

		sleep(1);
	}	
	return 0;
}
#elif ALG_SEL == 4
void *alg4(void*){
	printf("Alg 4_%d AUTO_Sensitive\n",QT_ADJUST);
	printf("Learn:%f Period:%dms\n",learn_rate,(scale_period/1000));
	int CPU_Sensitive = 1;
	
	//constant
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	const int GFL_MAX = 4;	
	const int GFL_MIN = 0;
	const int AVG_INTERVAL = 5; // for average recording
	const int AVG_TAKE = 3;
	//scaling weight
	double Coef_Uc2Fc = 0;
	double Coef_Q2Fc = 0;
	double Coef_Ug2Fg = 0;
	double Coef_Q2Fg = 0;
	double Coef_Q2Uc = 0;
	double Coef_Q2Ug = 0;
	
	//target
	int Q_target[AVG_INTERVAL];  //for average 
	int UC_target[AVG_INTERVAL];
	int UG_target[AVG_INTERVAL];
	
	int FC_record[AVG_INTERVAL]; //for delta 
	int FG_record[AVG_INTERVAL];	
	int Q_record[AVG_INTERVAL];  
	int UC_record[AVG_INTERVAL];
	int UG_record[AVG_INTERVAL];
	
	start_sampling = 1;
	
	//current info
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Rec_interval = 3;
	//for debugging
	int Fc,Fg;
	
	while(true){
		int s_time = (int)ns2ms(systemTime());
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){//first sample and set min CF			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);
			//initial samples
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;//Coef_Fg2Q = Coef_Uc2Q =	Coef_Ug2Q = Coef_Fc2Q = 
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;//Coef_Fc2Uc = 
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;//Coef_Fg2Ug = 		
			FC_record[start_sampling-1] = FL[CFL_MIN]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			//set_cpu_freq(0,FL[CFL_MIN]);
			set_cpu_freq(0,FL[CFL_MIN]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			
			//Fc vary, Fc-Q , Fc-Uc , Ug-Q
			Coef_Q2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Uc2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;				
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MIN];			
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MIN]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);

			//Fg vary, Fg-Q , Fg-Ug , Uc-Q
			
			Coef_Q2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Ug2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(UG_target[0] - Ug);
			//printf("Coef_Q2Fg = (%d - %d) / %d = %f\n",Q_target[0],fps,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Q2Fg);
			//printf("Coef_Ug2Fg = (%d - %d) / %d = %f\n",UG_target[0],Ug,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Ug2Fg);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Uc : %d , Ug : %d\n",AVG(Q_target,AVG_TAKE),AVG(UC_target,AVG_TAKE),AVG(UG_target,AVG_TAKE));			
			printf("Coef_Q2Fc : %f\nCoef_Uc2Fc : %f\nCoef_Q2Ug : %f\n",Coef_Q2Fc,Coef_Uc2Fc,Coef_Q2Ug);			
			printf("Coef_Q2Fg : %f\nCoef_Ug2Fg : %f\nCoef_Q2Uc : %f\n",Coef_Q2Fg,Coef_Ug2Fg,Coef_Q2Uc);
#if SEQUENTIAL == 0
			start_sampling=5;
#else
			start_sampling=6;
			printf("Sense\tScale\tQT\tUgT\tUcT\tQC\tUg\tUc\tFg\tFc\tPower\tCT\n");
#endif
			//return 0;
		}
		
		//Start Scaling
#if SEQUENTIAL != 0
		float p_cur = (get_battery('V')/1000*get_battery('I')/1000);
#endif
		//calculate pointers
		Rec_interval %= AVG_INTERVAL;
		int pre0 = Rec_interval; //current
		int pre1 = (Rec_interval+AVG_INTERVAL-1) % AVG_INTERVAL; //t-1
		int pre2 = (Rec_interval+AVG_INTERVAL-2) % AVG_INTERVAL; //t-2		
		//record current status
		scale_state state = SCALE_CHECK;
		static int s_period = 0;
		s_period += scale_period;
		if(s_period >= 1000000){
			Q_record[pre0] = get_fps(binder);
			s_period = 0;
		}else{
			int temp = Q_record[AVG_INTERVAL-1];
			int mv=AVG_INTERVAL-1;
			for( ; mv > 0 ;mv--){
				Q_record[mv] = Q_record[mv-1];
			}
			Q_record[mv] = temp;
			state = SCALE_U_Based;
		}
		
		UC_record[pre0] = get_cpu_util(4,&Uc_bp,&Uc_tp);
		UG_record[pre0] = get_gpu_util(&Ug_bp,&Ug_tp);
		
		//current status copy
		for(int mv = 0; mv < AVG_INTERVAL; mv++){
		#if QT_ADJUST == 0
			Q_target[mv] = Q_record[mv];
		#elif QT_ADJUST == 1
			Q_target[mv] = Q_record[mv]-2;
		#elif QT_ADJUST == 2	
			Q_target[mv] = Q_record[mv]-4;
		#endif
			UC_target[mv] = UC_record[mv];
			UG_target[mv] = UG_record[mv];
		}		
		sort(Q_target,Q_target+AVG_INTERVAL-1);
		sort(UC_target,UC_target+AVG_INTERVAL-1);
		sort(UG_target,UG_target+AVG_INTERVAL-1);
		//Compute New target : take the middle 3
		//int QT = AVG(Q_target+1,AVG_TAKE); //Compute target Q
		int QT = AVG(Q_target+2,AVG_TAKE); //Compute target Q
#if QT_FIX == 1
		if(QT > Q_target_set)QT = Q_target_set;
#endif	
/*	
#if QT_ADJUST == 1
		QT -= 2;
#elif QT_ADJUST == 2
		QT -= 4;
#elif QT_ADJUST == 3
		QT = QT * 95 / 100;
#elif QT_ADJUST == 4
		QT = QT * 90 / 100;
#endif		
*/
#if U_TARGETS == 0		
		int UC_T = AVG(UC_target+1,AVG_TAKE); //Compute target UC
		int UG_T = AVG(UG_target+1,AVG_TAKE); //Compute target UG
#elif U_TARGETS == 1				
		int UC_T = AVG(UC_target,AVG_INTERVAL); //Compute target UC
		int UG_T = AVG(UG_target,AVG_INTERVAL); //Compute target UG		
#elif U_TARGETS == 2						
		static int UC_T = UC_target[4]; //target MAX UC
		static int UG_T = UG_target[4]; //target MAX UG
#elif U_TARGETS == 3	
		int UC_T = AVG(UC_target+2,AVG_TAKE); //Compute target UC Max 3
 		int UG_T = AVG(UG_target+2,AVG_TAKE); //Compute target UG Max 3
#endif 
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int FC_Next = FC_record[pre1];
		int FG_Next = FG_record[pre1];
		int UC_Next = 0;
		int UG_Next = 0;
		
		double delta_Q = 0.0;
		double delta_FC = 0.0;
		double delta_Coef_Q2Fc = 0.0;
		double delta_Coef_Uc2Fc = 0.0;
		
		double delta_UG = 0.0; 
		double delta_Coef_Q2Ug = 0.0;
		
		double delta_FG = 0.0;
		double delta_Coef_Ug2Fg = 0.0;
		double delta_Coef_Q2Fg = 0.0;
		
		double delta_UC = 0.0;
		double delta_Coef_Q2Uc = 0.0;

		if(AVG(UG_record,AVG_INTERVAL) > 80)
			CPU_Sensitive = 0;//GPU_Sense
		else
			CPU_Sensitive = 1;//CPU_Sense
		int scale = 0;
		//Q(t)-Q(t-1)
		delta_Q = Q_record[pre0] - Q_record[pre1];
		if(state == SCALE_CHECK){
			if(Q_record[pre0] - QT < -4 || Q_record[pre0] - QT > 4){
				state = SCALE_Q_Based;				
			}else{
				state = SCALE_U_Based;
			}
		}		
		//SCALE_Q_Based
		if(state == SCALE_Q_Based){//dropping or lifting more than 4
#if FL_DEBUG == 1
			printf("Scale\n");
#endif
			scale = 1;
			if(CPU_Sensitive){
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);				
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//System tuning				
				FC_Next = (int)(FC_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fc);			
				//
				UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
				//printf("UG_Next = %d + %d * %f = %d\n",UG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Ug,UG_Next);				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);	
				//
#if FL_DEBUG == 1
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fc,FC_Next);
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
#endif				
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//
#if FL_DEBUG == 1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
#endif
						FC_Next = FL[pre_FL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//
#if FL_DEBUG == 1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
#endif
						FC_Next = FL[pre_FL-1];						
					}
				}				
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);
				
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 				
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fg);	
				//
				//Compute prediction UC
				UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
				//printf("UC_Next = %d + %d * %f = %d\n",UC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Uc,UC_Next);
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
				//
#if FL_DEBUG == 1
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fg,FG_Next);
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
		
#endif				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < -4){
						//Current Q is too low
						//FL+1
						//
#if FL_DEBUG == 1
						printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
#endif
						FG_Next = GFL[pre_GFL+1];						
					}else if(Q_record[pre0] - QT > 4){
						//Current Q is too high
						//FL-1
						//
#if FL_DEBUG == 1
						printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
#endif
						FG_Next = GFL[pre_GFL-1];						
					}
				}				
			}				
		}else{//SCALE_U_Based
#if FL_DEBUG == 1
			printf("Balance\n");	
#endif					
			if(CPU_Sensitive){//Learning Only
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);		
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);
				int pd_Q = (FG_Next - FG_record[pre1]) * reciprocal(Coef_Q2Fg);
				if(pd_Q <= -4){
					FG_Next = FG_record[pre1];
				}else if(-4 <= pd_Q && pd_Q <= 4){
					if((FG_Next - FG_record[pre1]) > 0)
						FG_Next = FG_record[pre1];
				}
				
#if FL_DEBUG == 1
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
				printf("pd_Q = (%d - %d) * %f = %d\n",FG_Next,FG_record[pre1],reciprocal(Coef_Q2Fg),pd_Q);
#endif				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(UG_T < UG_record[pre0]){
						//Current UG is too high
#if FL_DEBUG == 1
					printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
#endif
						//FG_Next = GFL[pre_GFL+1];						
					}else if(UG_T > UG_record[pre0]){
						//Current UG is too low
#if FL_DEBUG == 1
					printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
#endif
						FG_Next = GFL[pre_GFL-1];						
					}
				}
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);	
				
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);
				int pd_Q = (FC_Next - FC_record[pre1]) * reciprocal(Coef_Q2Fc);
				if(pd_Q <= -4){
					FC_Next = FC_record[pre1];
				}else if(-4 <= pd_Q && pd_Q <= 4){
					if((FC_Next - FC_record[pre1]) > 0)
						FC_Next = FC_record[pre1];
				}
#if FL_DEBUG == 1
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
				printf("pd_Q = (%d - %d) * %f = %d\n",FC_Next,FC_record[pre1],reciprocal(Coef_Q2Fc),pd_Q);
#endif
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(UC_T < UC_record[pre0]){
						//Current UC is too high
#if FL_DEBUG == 1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
#endif
						//FC_Next = FL[pre_FL+1];						
					}else if(UC_T > UC_record[pre0]){
						//Current UC is too low
#if FL_DEBUG == 1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
#endif
						FC_Next = FL[pre_FL-1];						
					}
				}
			}
		
		}	
		
		
		//find the fitness FL
		FL_point = 0;		
		while(FL_point < freq_level && FL[FL_point] < FC_Next)FL_point++;		
		//find the fitness GFL
		GFL_point = 0;		
		while(GFL_point < gpu_freq_level && GFL[GFL_point] < FG_Next)GFL_point++;
		
		if(FL_point != pre_FL){		
			if(FL_point > freq_level-2){
				FL_point = freq_level-2;
			}else if(FL_point < 3){
				FL_point = 3;
			}		
			pre_FL = FL_point;
			set_cpu_freq(0,FL[FL_point]);			
		}
		if(GFL_point != pre_GFL){			
			if(GFL_point > gpu_freq_level-1){
				GFL_point = gpu_freq_level-1;
			}else if(GFL_point < 0){
				GFL_point = 0;
			}		
			pre_GFL = GFL_point;
			set_gpu_freq(GFL[GFL_point]);			
		}
		
		//Restore current
		FC_record[pre0] = FL[FL_point];
		FG_record[pre0] = GFL[GFL_point];	
	
		if(start_sampling == 6){
#if SEQUENTIAL == 1			
			//printf("Sense\tScale\tQT\tUgT\tUcT\tQC\tUg\tUc\tFg\tFc\tCT\n");
			printf(	"%d\t%d\t"
					"%d\t%d\t%d\t"
					"%d\t%d\t%d\t"
					"%d\t%d\t"
					"%d\t"
					"%d\n"
					,CPU_Sensitive,scale
					,QT,UG_T,UC_T
					,Q_record[pre0],UG_record[pre0],UC_record[pre0]
					,FG_record[pre0],FC_record[pre0]
					,-(int)p_cur
					,(int)ns2ms(systemTime())-s_time);
			/*
			printf("%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%f\t%f\t%f\t%f\t%f\t%f\n"
					,AVG(Q_target,AVG_INTERVAL),Q_target[0],Q_target[1],Q_target[2],Q_target[3],Q_target[4]
					,AVG(UC_target,AVG_INTERVAL),UC_target[0],UC_target[1],UC_target[2],UC_target[3],UC_target[4]
					,AVG(UG_target,AVG_INTERVAL),UG_target[0],UG_target[1],UG_target[2],UG_target[3],UG_target[4]
					,FC_Next,FC_record[0],FC_record[1],FC_record[2],FC_record[3],FC_record[4]
					,FG_Next,FG_record[0],FG_record[1],FG_record[2],FG_record[3],FG_record[4]
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);
			*/
			End_Test_Tick++;
#elif SEQUENTIAL == 2			
			printf("===========================\n");
			printf("CPU_Sensitive : %d\n",CPU_Sensitive);
			printf("Q_record(%d):\t",QT);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",Q_record[i]);
			printf("\nUC_target(%d):\t",UC_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UC_record[i]);
			printf("\nUG_record(%d):\t",UG_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UG_record[i]);
			printf("\nFC_record(%d):\t",FC_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FC_record[i]);
			printf("\nFG_record(%d):\t",FG_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FG_record[i]);
			printf("\nCoef_Q2Fc:%f\nCoef_Q2Ug:%f\nCoef_Ug2Fg:%f\nCoef_Q2Fg:%f\nCoef_Q2Uc:%f\nCoef_Uc2Fc:%f\n"
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);		
			printf("===========================\n");
#endif			
		}
		Rec_interval++;
		
		
		//printf("Time Complex %dms\n",(int)ns2ms(systemTime())-s_time);
		usleep(scale_period);
	}	
	return 0;
}
#elif ALG_SEL == 5
void *alg5(void*){
	printf("Alg 5_%d AUTO_Sensitive\n",QT_ADJUST);
	printf("Learn:%f Period:%dms\n",learn_rate,(scale_period/1000));
	int CPU_Sensitive = 1;
	
	//constant
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	const int GFL_MAX = 4;	
	const int GFL_MIN = 0;
	const int AVG_INTERVAL = 5; // for average recording
	const int AVG_TAKE = 3;
	//scaling weight
	double Coef_Uc2Fc = 0;
	double Coef_Q2Fc = 0;
	double Coef_Ug2Fg = 0;
	double Coef_Q2Fg = 0;
	double Coef_Q2Uc = 0;
	double Coef_Q2Ug = 0;
	
	//target
	int Q_target[AVG_INTERVAL];  //for average 
	int UC_target[AVG_INTERVAL];
	int UG_target[AVG_INTERVAL];
	
	int FC_record[AVG_INTERVAL]; //for delta 
	int FG_record[AVG_INTERVAL];	
	int Q_record[AVG_INTERVAL];  
	int UC_record[AVG_INTERVAL];
	int UG_record[AVG_INTERVAL];
	
	start_sampling = 1;
	
	//current info
	int fps;
	
	int Ug;
	static unsigned long long Ug_bp,Ug_tp;
	get_gpu_time(&Ug_bp,&Ug_tp);
	
	int Uc;
	static unsigned long long Uc_bp,Uc_tp;
	get_cpu_time(4,&Uc_bp,&Uc_tp);
	
	int Rec_interval = 3;
	//for debugging
	int Fc,Fg;
	
	while(true){
		int s_time = (int)ns2ms(systemTime());
		if(game == -1){sleep(1);start_sampling=1;continue;}
		Fc=get_cpu_freq(0);
		Fg=get_gpu_freq();
		//printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq(),Uc,Ug);
		if(start_sampling == 1){//first sample and set min CF			
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);
			//initial samples
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;//Coef_Fg2Q = Coef_Uc2Q =	Coef_Ug2Q = Coef_Fc2Q = 
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;//Coef_Fc2Uc = 
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;//Coef_Fg2Ug = 		
			FC_record[start_sampling-1] = FL[CFL_MIN]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			//set_cpu_freq(0,FL[CFL_MIN]);
			set_cpu_freq(0,FL[CFL_MIN]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=2;
			sleep(1);continue;
		}else if(start_sampling == 2){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);			
			
			//Fc vary, Fc-Q , Fc-Uc , Ug-Q
			Coef_Q2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Uc2Fc = (double)(FL[CFL_MAX] - FL[CFL_MIN]) * reciprocal(UC_target[0] - Uc);
			Coef_Q2Ug = (double)(UG_target[0] - Ug) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;				
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MIN];			
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MIN]);
			start_sampling=3;
			sleep(1);continue;
		}else if(start_sampling == 3){
			fps = get_fps(binder);			
			Uc = get_cpu_util(4,&Uc_bp,&Uc_tp);
			Ug = get_gpu_util(&Ug_bp,&Ug_tp);			
			printf("Sample%d Fc=%d,Fg=%d,Uc=%d,Ug=%d\n",start_sampling,Fc,Fg,Uc,Ug);

			//Fg vary, Fg-Q , Fg-Ug , Uc-Q
			
			Coef_Q2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(Q_target[0] - fps);
			Coef_Ug2Fg = (double)(GFL[GFL_MAX] - GFL[GFL_MIN]) * reciprocal(UG_target[0] - Ug);
			//printf("Coef_Q2Fg = (%d - %d) / %d = %f\n",Q_target[0],fps,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Q2Fg);
			//printf("Coef_Ug2Fg = (%d - %d) / %d = %f\n",UG_target[0],Ug,(GFL[GFL_MAX] - GFL[GFL_MIN]),Coef_Ug2Fg);
			Coef_Q2Uc = (double)(UC_target[0] - Uc) * reciprocal(Q_target[0] - fps);
			
			Q_record[start_sampling-1]=Q_target[start_sampling-1] = fps;
			UC_record[start_sampling-1]=UC_target[start_sampling-1] = Uc;
			UG_record[start_sampling-1]=UG_target[start_sampling-1] = Ug;
			FC_record[start_sampling-1] = FL[CFL_MAX]; //for delta 
			FG_record[start_sampling-1] = GFL[GFL_MAX];
			
			set_cpu_freq(0,FL[CFL_MAX]);
			set_gpu_freq(GFL[GFL_MAX]);
			start_sampling=4;
			sleep(1);continue;
		}else if(start_sampling == 4){
			printf("Sample%d Fc=%d,Fg=%d\n",start_sampling,get_cpu_freq(0),get_gpu_freq());
			printf("Q : %d , Uc : %d , Ug : %d\n",AVG(Q_target,AVG_TAKE),AVG(UC_target,AVG_TAKE),AVG(UG_target,AVG_TAKE));			
			printf("Coef_Q2Fc : %f\nCoef_Uc2Fc : %f\nCoef_Q2Ug : %f\n",Coef_Q2Fc,Coef_Uc2Fc,Coef_Q2Ug);			
			printf("Coef_Q2Fg : %f\nCoef_Ug2Fg : %f\nCoef_Q2Uc : %f\n",Coef_Q2Fg,Coef_Ug2Fg,Coef_Q2Uc);
#if SEQUENTIAL == 0
			start_sampling=5;
#else
			start_sampling=6;
			printf("Sense\tScale\tQT\tUgT\tUcT\tQC\tUg\tUc\tFg\tFc\tPower\tCT\n");
#endif
			//return 0;
		}
		
		//Start Scaling
#if SEQUENTIAL != 0
		float p_cur = (get_battery('V')/1000*get_battery('I')/1000);
#endif
		//calculate pointers
		Rec_interval %= AVG_INTERVAL;
		int pre0 = Rec_interval; //current
		int pre1 = (Rec_interval+AVG_INTERVAL-1) % AVG_INTERVAL; //t-1
		int pre2 = (Rec_interval+AVG_INTERVAL-2) % AVG_INTERVAL; //t-2		
		//record current status
		scale_state state = SCALE_CHECK;
		static int s_period = 0;
		s_period += scale_period;
		if(s_period >= 1000000){
			Q_record[pre0] = get_fps(binder);
			s_period = 0;
		}else{
			int temp = Q_record[AVG_INTERVAL-1];
			int mv=AVG_INTERVAL-1;
			for( ; mv > 0 ;mv--){
				Q_record[mv] = Q_record[mv-1];
			}
			Q_record[mv] = temp;
			state = SCALE_U_Based;
		}
		
		UC_record[pre0] = get_cpu_util(4,&Uc_bp,&Uc_tp);
		UG_record[pre0] = get_gpu_util(&Ug_bp,&Ug_tp);
		
		//current status copy
		for(int mv = 0; mv < AVG_INTERVAL; mv++){
		#if QT_ADJUST == 0
			Q_target[mv] = Q_record[mv];
		#elif QT_ADJUST == 1
			Q_target[mv] = Q_record[mv]-2;
		#elif QT_ADJUST == 2	
			Q_target[mv] = Q_record[mv]-4;
		#endif
			UC_target[mv] = UC_record[mv];
			UG_target[mv] = UG_record[mv];
		}		
		sort(Q_target,Q_target+AVG_INTERVAL-1);
		sort(UC_target,UC_target+AVG_INTERVAL-1);
		sort(UG_target,UG_target+AVG_INTERVAL-1);
		//Compute New target : take the middle 3
		//int QT = AVG(Q_target+1,AVG_TAKE); //Compute target Q
		int QT = AVG(Q_target+2,AVG_TAKE); //Compute target Q
#if QT_FIX == 1
		int L3 = AVG(Q_target+2,AVG_TAKE);
		int M3 = AVG(Q_target+1,AVG_TAKE);
		int S3 = AVG(Q_target,AVG_TAKE);
		
		if(S3 >= Q_target_set)
			QT = S3;
		else if(M3 >= Q_target_set)
			QT = M3;
		else
			QT = L3;		
#endif	
/*	
#if QT_ADJUST == 1
		QT -= 2;
#elif QT_ADJUST == 2
		QT -= 4;
#elif QT_ADJUST == 3
		QT = QT * 95 / 100;
#elif QT_ADJUST == 4
		QT = QT * 90 / 100;
#endif		
*/
#if U_TARGETS == 0		
		int UC_T = AVG(UC_target+1,AVG_TAKE); //Compute target UC
		int UG_T = AVG(UG_target+1,AVG_TAKE); //Compute target UG
#elif U_TARGETS == 1				
		int UC_T = AVG(UC_target,AVG_INTERVAL); //Compute target UC
		int UG_T = AVG(UG_target,AVG_INTERVAL); //Compute target UG		
#elif U_TARGETS == 2						
		static int UC_T = UC_target[4]; //target MAX UC
		static int UG_T = UG_target[4]; //target MAX UG
#elif U_TARGETS == 3	
		int UC_T = AVG(UC_target+2,AVG_TAKE); //Compute target UC Max 3
 		int UG_T = AVG(UG_target+2,AVG_TAKE); //Compute target UG Max 3
#endif 
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		
		
		int FC_Next = FC_record[pre1];
		int FG_Next = FG_record[pre1];
		int UC_Next = 0;
		int UG_Next = 0;
		
		double delta_Q = 0.0;
		double delta_FC = 0.0;
		double delta_Coef_Q2Fc = 0.0;
		double delta_Coef_Uc2Fc = 0.0;
		
		double delta_UG = 0.0; 
		double delta_Coef_Q2Ug = 0.0;
		
		double delta_FG = 0.0;
		double delta_Coef_Ug2Fg = 0.0;
		double delta_Coef_Q2Fg = 0.0;
		
		double delta_UC = 0.0;
		double delta_Coef_Q2Uc = 0.0;

		if(AVG(UG_record,AVG_INTERVAL) > 80)
			CPU_Sensitive = 0;//GPU_Sense
		else
			CPU_Sensitive = 1;//CPU_Sense
		int scale = 0;
		//Q(t)-Q(t-1)
		delta_Q = Q_record[pre0] - Q_record[pre1];
		if(state == SCALE_CHECK){
			if(Q_record[pre0] - QT < QLB || Q_record[pre0] - QT > QUB){
				state = SCALE_Q_Based;
				if(Q_record[pre0] == AVG(Q_record,AVG_INTERVAL)){
					state = SCALE_U_Based;
				}
			}else{
				state = SCALE_U_Based;
			}
		}		
		//SCALE_Q_Based
		if(state == SCALE_Q_Based){//dropping or lifting more than 4
#if FL_DEBUG == 1
			printf("Scale\n");
#endif
			scale = 1;
			if(CPU_Sensitive){
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);				
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//System tuning				
				FC_Next = (int)(FC_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fc);			
				//
				UG_Next = UG_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Ug;
				//printf("UG_Next = %d + %d * %f = %d\n",UG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Ug,UG_Next);				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);	
				//
#if FL_DEBUG == 1
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fc,FC_Next);
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
#endif				
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < QLB){
						//Current Q is too low
						//FL+1
						//
#if FL_DEBUG == 1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
#endif
						FC_Next = FL[pre_FL+1];						
					}else if(Q_record[pre0] - QT > QUB){
						//Current Q is too high
						//FL-1
						//
#if FL_DEBUG == 1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
#endif
						FC_Next = FL[pre_FL-1];						
					}
				}				
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);
				
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 				
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				//calculate FG_Next
				FG_Next = (int)(FG_record[pre1]+(QT - Q_record[pre0])*Coef_Q2Fg);	
				//
				//Compute prediction UC
				UC_Next = UC_record[pre0] + (QT - Q_record[pre0]) * Coef_Q2Uc;
				//printf("UC_Next = %d + %d * %f = %d\n",UC_record[pre1],(QT - Q_record[pre0]),Coef_Q2Uc,UC_Next);
				//calculate FC_Next
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);	
				//
#if FL_DEBUG == 1
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(QT - Q_record[pre0]),Coef_Q2Fg,FG_Next);
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
		
#endif				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(Q_record[pre0] - QT < QLB){
						//Current Q is too low
						//FL+1
						//
#if FL_DEBUG == 1
						printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
#endif
						FG_Next = GFL[pre_GFL+1];						
					}else if(Q_record[pre0] - QT > QUB){
						//Current Q is too high
						//FL-1
						//
#if FL_DEBUG == 1
						printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
#endif
						FG_Next = GFL[pre_GFL-1];						
					}
				}				
			}				
		}else{//SCALE_U_Based
#if FL_DEBUG == 1
			printf("Balance\n");	
#endif					
			if(CPU_Sensitive){//Learning Only
				//Coef Learning
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				//printf("delta_Coef_Q2Fc = ((%f * 1/%f) - %f) * %f = %f\n",delta_FC,delta_Q,Coef_Q2Fc,learn_rate,delta_Coef_Q2Fc);
				//printf("Coef_Q2Fc = %f + %f = %f\n",Coef_Q2Fc,delta_Coef_Q2Fc,(Coef_Q2Fc+delta_Coef_Q2Fc));
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				delta_UG = UG_record[pre0] - UG_record[pre1];
				delta_Coef_Q2Ug = ((delta_UG * reciprocal(delta_Q))-Coef_Q2Ug)*learn_rate;
				Coef_Q2Ug += delta_Coef_Q2Ug;	
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Ug2Fg = ((delta_FG * reciprocal(delta_UG))-Coef_Ug2Fg)*learn_rate;
				Coef_Ug2Fg += delta_Coef_Ug2Fg;Coef_Ug2Fg = -abs(Coef_Ug2Fg);		
				
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				FG_Next = (int)(FG_record[pre1]+(UG_T - UG_record[pre0])* Coef_Ug2Fg);
				int pd_Q = (FG_Next - FG_record[pre1]) * reciprocal(Coef_Q2Fg);
				if(pd_Q <= -4){
					FG_Next = FG_record[pre1];
				}else if(-4 <= pd_Q && pd_Q <= 4){
					if((FG_Next - FG_record[pre1]) > 0)
						FG_Next = FG_record[pre1];
				}
				
#if FL_DEBUG == 1
				printf("FG_Next = %d + %d * %f = %d\n",FG_record[pre1],(UG_T - UG_record[pre0]),Coef_Ug2Fg,FG_Next);
				printf("pd_Q = (%d - %d) * %f = %d\n",FG_Next,FG_record[pre1],reciprocal(Coef_Q2Fg),pd_Q);
#endif				
				//Check if Coef is converged
				if(FG_record[0] == AVG(FG_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(UG_T < UG_record[pre0]){
						//Current UG is too high
#if FL_DEBUG == 1
					printf("Forcing GFL up: %d -> %d\n",FG_Next,GFL[pre_GFL+1]);
#endif
						//FG_Next = GFL[pre_GFL+1];						
					}else if(UG_T > UG_record[pre0]){
						//Current UG is too low
#if FL_DEBUG == 1
					printf("Forcing GFL down: %d -> %d\n",FG_Next,GFL[pre_GFL-1]);
#endif
						FG_Next = GFL[pre_GFL-1];						
					}
				}
			}else{
				//Learn FG-Q
				delta_FG = FG_record[pre1] - FG_record[pre2];
				delta_Coef_Q2Fg = ((delta_FG * reciprocal(delta_Q))-Coef_Q2Fg)*learn_rate; 
				Coef_Q2Fg += delta_Coef_Q2Fg;Coef_Q2Fg = abs(Coef_Q2Fg);
				
				//Learn UC-Q			
				delta_UC = UC_record[pre0] - UC_record[pre1];
				delta_Coef_Q2Uc = ((delta_UC * reciprocal(delta_Q))-Coef_Q2Uc)*learn_rate;
				Coef_Q2Uc += delta_Coef_Q2Uc;		
				
				//Learn UC-FC Coef Coef_Ug2Fg
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Uc2Fc = ((delta_FC * reciprocal(delta_UC))-Coef_Uc2Fc)*learn_rate;
				Coef_Uc2Fc += delta_Coef_Uc2Fc;Coef_Uc2Fc = -abs(Coef_Uc2Fc);	
				
				delta_FC = FC_record[pre1] - FC_record[pre2];
				delta_Coef_Q2Fc = ((delta_FC * reciprocal(delta_Q))-Coef_Q2Fc)*learn_rate; 
				Coef_Q2Fc += delta_Coef_Q2Fc;Coef_Q2Fc = abs(Coef_Q2Fc);
				
				FC_Next = (int)(FC_record[pre1]+(UC_T - UC_record[pre0])*Coef_Uc2Fc);
				int pd_Q = (FC_Next - FC_record[pre1]) * reciprocal(Coef_Q2Fc);
				if(pd_Q <= -4){
					FC_Next = FC_record[pre1];
				}else if(-4 <= pd_Q && pd_Q <= 4){
					if((FC_Next - FC_record[pre1]) > 0)
						FC_Next = FC_record[pre1];
				}
#if FL_DEBUG == 1
				printf("FC_Next = %d + %d * %f = %d\n",FC_record[pre1],(UC_T - UC_record[pre0]),Coef_Uc2Fc,FC_Next);
				printf("pd_Q = (%d - %d) * %f = %d\n",FC_Next,FC_record[pre1],reciprocal(Coef_Q2Fc),pd_Q);
#endif
				//Check if Coef is converged
				if(FC_record[0] == AVG(FC_record,AVG_INTERVAL)){
				//if(AVG(FC_record,AVG_INTERVAL)==FL[CFL_MAX]){
					if(UC_T < UC_record[pre0]){
						//Current UC is too high
#if FL_DEBUG == 1
						printf("Forcing FL up: %d -> %d\n",FC_Next,FL[pre_FL+1]);
#endif
						//FC_Next = FL[pre_FL+1];						
					}else if(UC_T > UC_record[pre0]){
						//Current UC is too low
#if FL_DEBUG == 1
						printf("Forcing FL down: %d -> %d\n",FC_Next,FL[pre_FL-1]);
#endif
						FC_Next = FL[pre_FL-1];						
					}
				}
			}
		
		}	
		
		
		//find the fitness FL
		FL_point = 0;		
		while(FL_point < freq_level && FL[FL_point] < FC_Next)FL_point++;		
		//find the fitness GFL
		GFL_point = 0;		
		while(GFL_point < gpu_freq_level && GFL[GFL_point] < FG_Next)GFL_point++;
		
		if(FL_point != pre_FL){		
			if(FL_point > freq_level-2){
				FL_point = freq_level-2;
			}else if(FL_point < 3){
				FL_point = 3;
			}		
			pre_FL = FL_point;
			set_cpu_freq(0,FL[FL_point]);			
		}
		if(GFL_point != pre_GFL){			
			if(GFL_point > gpu_freq_level-1){
				GFL_point = gpu_freq_level-1;
			}else if(GFL_point < 0){
				GFL_point = 0;
			}		
			pre_GFL = GFL_point;
			set_gpu_freq(GFL[GFL_point]);			
		}
		
		//Restore current
		FC_record[pre0] = FL[FL_point];
		FG_record[pre0] = GFL[GFL_point];	
	
		if(start_sampling == 6){
#if SEQUENTIAL == 1			
			//printf("Sense\tScale\tQT\tUgT\tUcT\tQC\tUg\tUc\tFg\tFc\tCT\n");
			printf(	"%d\t%d\t"
					"%d\t%d\t%d\t"
					"%d\t%d\t%d\t"
					"%d\t%d\t"
					"%d\t"
					"%d\n"
					,CPU_Sensitive,scale
					,QT,UG_T,UC_T
					,Q_record[pre0],UG_record[pre0],UC_record[pre0]
					,FG_record[pre0],FC_record[pre0]
					,-(int)p_cur
					,(int)ns2ms(systemTime())-s_time);
			/*
			printf("%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%d\t%d\t%d\t%d\t"
					"%f\t%f\t%f\t%f\t%f\t%f\n"
					,AVG(Q_target,AVG_INTERVAL),Q_target[0],Q_target[1],Q_target[2],Q_target[3],Q_target[4]
					,AVG(UC_target,AVG_INTERVAL),UC_target[0],UC_target[1],UC_target[2],UC_target[3],UC_target[4]
					,AVG(UG_target,AVG_INTERVAL),UG_target[0],UG_target[1],UG_target[2],UG_target[3],UG_target[4]
					,FC_Next,FC_record[0],FC_record[1],FC_record[2],FC_record[3],FC_record[4]
					,FG_Next,FG_record[0],FG_record[1],FG_record[2],FG_record[3],FG_record[4]
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);
			*/
			End_Test_Tick++;
#elif SEQUENTIAL == 2			
			printf("===========================\n");
			printf("CPU_Sensitive : %d\n",CPU_Sensitive);
			printf("Q_record(%d):\t",QT);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",Q_record[i]);
			printf("\nUC_target(%d):\t",UC_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UC_record[i]);
			printf("\nUG_record(%d):\t",UG_T);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",UG_record[i]);
			printf("\nFC_record(%d):\t",FC_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FC_record[i]);
			printf("\nFG_record(%d):\t",FG_Next);
			for(int i=0;i<AVG_INTERVAL;i++)
				printf("%d\t",FG_record[i]);
			printf("\nCoef_Q2Fc:%f\nCoef_Q2Ug:%f\nCoef_Ug2Fg:%f\nCoef_Q2Fg:%f\nCoef_Q2Uc:%f\nCoef_Uc2Fc:%f\n"
					,Coef_Q2Fc,Coef_Q2Ug,Coef_Ug2Fg,Coef_Q2Fg,Coef_Q2Uc,Coef_Uc2Fc);		
			printf("===========================\n");
#endif			
		}
		Rec_interval++;
		
		
		//printf("Time Complex %dms\n",(int)ns2ms(systemTime())-s_time);
		usleep(scale_period);
	}	
	return 0;
}
#elif ALG_SEL == 11	
void *odm(void*){
	const int up_threshold = 95;
	const int down_threshold = 20;
	const int sample_rate = 750000;
	printf("Odm\n");
	printf("up_threshold:%d\n"
			"down_threshold:%d\n"
			"sample_rate:%d\n"
			,up_threshold,down_threshold,sample_rate);
	
	static unsigned long long usage_p[5],time_p[5];
	for(int c=0;c < 4;c++){
		get_cpu_time(c,&usage_p[c],&time_p[c]);
	}
	
	const int CFL_MAX = 10;
	const int CFL_MIN = 3;
	scale_state state = SCALE_CHECK;
	
	int cu[4];
	set_gpu_freq(GFL[4]);
	set_cpu_freq(0,FL[CFL_MIN]);
	static int p_FL = CFL_MIN;
	printf("Sample 1\n");sleep(1);
	printf("Sample 2\n");sleep(1);
	printf("Sample 3\n");sleep(1);
	start_sampling=5;
	while(true){
		state = SCALE_CHECK;
		//scale up check
		for(int cpu = 0;cpu < core_num ;cpu++){
			cu[cpu] = get_cpu_util(cpu,&usage_p[cpu],&time_p[cpu]); 
			//printf("%d ",cu[cpu]);
		}
		//printf("\n");
		if(state == SCALE_CHECK){
			for(int cpu = 0;cpu < core_num ;cpu++){
				if(cu[cpu] >= up_threshold){
					state = SCALE_UP;
					break;
				}
			}
		}
		//scale down check
		if(state == SCALE_CHECK){
			state = SCALE_BALANCE;			
			for(int cpu = 0;cpu < core_num ;cpu++){
				if(cu[cpu] <= down_threshold){
					state = SCALE_DOWN;
					break;
				}
			}
		}

		if(state == SCALE_UP){
			//printf("SCALE_UP\n");			
			p_FL = CFL_MAX;
		}else if(state == SCALE_DOWN){
			//printf("SCALE_DOWN\n");			
			p_FL -= 1;
			if(p_FL < CFL_MIN)
				p_FL = CFL_MIN;			
		}
		if(state != SCALE_BALANCE)
			set_cpu_freq(0,FL[p_FL]);	
		usleep(sample_rate);
	}
	return 0;
}
#endif
void *test(void*){//find parameters
	start_sampling = 5;
	set_cpu_freq(0,FL[10]);
	set_gpu_freq(GFL[0]);
	return 0;
	while(1){
#if CPU_TRAIN == 1
		set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[9]);sleep(5);
		//set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[8]);sleep(5);
		//set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[7]);sleep(5);
		//set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[6]);sleep(5);
		//set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[5]);sleep(5);
		//set_cpu_freq(0,FL[10]);sleep(5);
		set_cpu_freq(0,FL[4]);sleep(5);
#else
		set_gpu_freq(GFL[4]);sleep(5);
		set_gpu_freq(GFL[3]);sleep(5);
		set_gpu_freq(GFL[2]);sleep(5);
		set_gpu_freq(GFL[1]);sleep(5);
		set_gpu_freq(GFL[0]);sleep(5);
#endif	
	}
	
}

int main(int argc, char** argv) {
	printf("System Monitor Start!!\n");
	platform_init();
	binder = get_surfaceflinger();
	if(binder == 0){
		printf("Cannot get SurfaceFlinger service\n");
		return 0;
	}else{
		printf("Binder SurfaceFlinger\n");		
	}	
			
	
	if(!manual){ //not manual
		set_cpu_freq(0,FL[freq_level-2]);
		for(int i=0;i<core_num;i++)
			set_cpu_on(i,1);
		set_gpu_freq(GFL[gpu_freq_level-1]);		
	}else{
		printf("Using manual setting\n");
		//set_cpu_freq(0,FL[freq_level-2]);
		set_cpu_freq(0,FL[3]);
		for(int i=0;i<core_num;i++)
			set_cpu_on(i,1);
		//set_gpu_freq(GFL[gpu_freq_level-1]);
		set_gpu_freq(GFL[0]);
		start_sampling=5;
	}
	sleep(2);
	// framework initial done
	
	pthread_t thread,c_hp,cf,gf,for_test;
	pthread_t algs;
	
	//scale_state state = SCALE_CHECK;
	
	pthread_create(&thread,NULL,show_result,NULL);

#if ALG_SEL == -1
	printf("Training Coefs\n");
	pthread_create(&for_test,NULL,test,NULL);
	while(End_Test == 0 || End_Test_Tick <= End_Test){
		sleep(1);
	}
#endif
	
#if DEFAULT_GOV == 0
	if(!manual){
#if ALG_SEL == 0
		pthread_create(&algs,NULL,ref1,NULL);
#elif ALG_SEL == 1		
		pthread_create(&algs,NULL,alg1,NULL);
#elif ALG_SEL == 2
		pthread_create(&algs,NULL,alg2,NULL);
#elif ALG_SEL == 3
		pthread_create(&algs,NULL,alg3,NULL);
#elif ALG_SEL == 4
		pthread_create(&algs,NULL,alg4,NULL);
#elif ALG_SEL == 5
		pthread_create(&algs,NULL,alg5,NULL);
#elif ALG_SEL == 11		
		pthread_create(&algs,NULL,odm,NULL);		
#endif
	}
#else
		while(true){
			if(game == -1){
				sleep(1);
				continue;
			}else{
				break;
			}
		}		
		printf("Sample 1\n");sleep(1);
		printf("Sample 2\n");sleep(1);
		printf("Sample 3\n");sleep(1);
		start_sampling=5;
#endif

	while(End_Test == 0 || End_Test_Tick <= End_Test){
		sleep(1);
	}
	

    return 0;
}
