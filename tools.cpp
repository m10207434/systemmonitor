#include "SM.h"

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
	printf("1\n");
	printf("3\n");
}