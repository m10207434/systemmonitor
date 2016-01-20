#include "SM.h"
//#include "AG.h"

void *ref1(void*){
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
		
		fps = get_fps(binder);
		int fps_dev = (target_fps - fps)*100/target_fps;
		
		static int FL_point = freq_level-2;
		static int pre_FL = FL_point;
		static int GFL_point = gpu_freq_level-1;
		static int pre_GFL = GFL_point;
		
		int CPU_Sensitive = 1;
		
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