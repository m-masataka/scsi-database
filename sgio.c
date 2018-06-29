#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>

#include "util_sgio.h"
int fd;

int put(unsigned char *key, float value, int k_size, int v_size) {
	struct block_data bd;
	struct LBA lba;
	bd.dirty = 0x01;
	memset( &bd.key[0] , 0x00 , KEY_SIZE );
	memcpy( &bd.key[0], key, k_size);
	memset( &bd.value[0] , 0x00 , VALUE_SIZE );
	memcpy( &bd.value[0], &value, v_size );
	if (find_key(&bd, &lba, 0, 100)) {
		update(&bd, &lba);
	} else {
		range_insert(&bd, 0, 100);
	}
	return 1;
}

float find(unsigned char *key, int k_size) {
	struct block_data bd;
	struct LBA lba;
	memset( &bd.key[0] , 0x00 , KEY_SIZE );
	memcpy(&bd.key[0], key, k_size);
	if (find_key(&bd, &lba, 0, 100)) {
		printf("lba: %d, %d \n", lba.lba_num, lba.data_cursor);
		get_value(&bd, &lba);
		float *f;
		f = (float *)(&bd.value[0]);
		return *f;
	}
	return 0.0;
}

/*int main(){
	return 0;
}*/
/*int main(int argc, char *argv[]){
	if((fd = open(DEVICE_NAME,O_RDWR))<0){
		printf("device file opening failed\n");
		return  1;
	}
	srand((unsigned) time(NULL));
	float x;
	x=(rand()%1001);
	x=x/1000;
	//init(8388608);
	//init(8388608);
	put("OK2", 0.4, 3, sizeof(float));
	put("OK8", 0.7, 3, sizeof(float));
	put("OK2", 0.9, 3, sizeof(float));
	put("OK8", 0.2, 3, sizeof(float));
	put(argv[1], x, 3, sizeof(float));
	float f = find("OK8", 3);
	printf("%f\n",f);
	debug(0);
	debug(8888800);
}
*/
