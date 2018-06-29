#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<error.h>
#include<sys/ioctl.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<scsi/scsi_ioctl.h>
#include<scsi/sg.h>

#include "util_sgio.h"
#include "util.h"


int scsi_readcmd(int block, unsigned char *buffer, int size) {
	int i;
	unsigned char r6CmdBlk[READ6_CMD_LEN] = {
		SCSI_READ6,
		0,
		0,
		block,
		LBA_LEN,
		0};
	sg_io_hdr_t io_hdr;
	unsigned char sense_buffer[32];
	memset(&io_hdr,0,sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = sizeof(r6CmdBlk);
	/* io_hdr.iovec_count = 0; */  /* memset takes care of this */
	io_hdr.mx_sb_len = sizeof(sense_buffer);
	io_hdr.dxfer_direction = SG_DXFER_FROM_DEV;
	io_hdr.dxfer_len = size;
	io_hdr.dxferp = buffer;
	io_hdr.cmdp = r6CmdBlk;
	io_hdr.sbp = sense_buffer;
	io_hdr.timeout = 20000;
	if(ioctl(fd,SG_IO,&io_hdr)<0){
		printf("ioctl failed\n");
		for(i = 0;i<32;i++){
			printf("%hx ",sense_buffer[i]);
		}
		printf("\n");
		return 1;
	}
	return 0;
}

int scsi_writecmd(int block, unsigned char* buffer, int size) {
	int i;
	unsigned char w6CmdBlk[WRITE6_CMD_LEN] = {
		SCSI_WRITE6,
		0,
		0,
		block,
		LBA_LEN,
		0};  //cdb
	sg_io_hdr_t io_hdr;
	unsigned char sense_buffer[32];
	/////////// data buffer ///////////
	memset(&io_hdr,0,sizeof(sg_io_hdr_t));
	io_hdr.interface_id = 'S';
	io_hdr.cmd_len = sizeof(w6CmdBlk);
	/* io_hdr.iovec_count = 0; */  /* memset takes care of this */
	io_hdr.mx_sb_len = sizeof(sense_buffer);
	io_hdr.dxfer_direction = SG_DXFER_TO_DEV;
	io_hdr.dxfer_len = size;
	io_hdr.dxferp = buffer;
	io_hdr.cmdp = w6CmdBlk;
	io_hdr.sbp = sense_buffer;
	io_hdr.timeout = 20000;
	if(ioctl(fd,SG_IO,&io_hdr)<0){
		printf("ioctl failed\n");
		for(i = 0;i<32;i++){
			printf("%hx ",sense_buffer[i]);
		}
		printf("\n");
		return 1;
	}
	return 1;
}

void init(int size){
	int i;
	for( i=0; i< size; i++ ){
		unsigned char write_buffer[RWBUFFER];
		memset( write_buffer , 0xcf , RWBUFFER );
		scsi_writecmd(i, write_buffer, RWBUFFER);
		if( i%1000 == 0) {
			printf("ok %d\n", i);
		}
	}
}

void update(struct block_data *bd, struct LBA *lba) {
	unsigned char buffer[RWBUFFER];
	scsi_readcmd(lba->lba_num, buffer, RWBUFFER);
	int cursor  = BLOCK_DATA_SIZE * lba->data_cursor;
	buffer[cursor] = bd -> dirty;
	cursor++;
	memcpy(&buffer[cursor], bd -> key, KEY_SIZE);
	cursor = cursor + KEY_SIZE;
	memcpy(&buffer[cursor], bd -> value, VALUE_SIZE);
	scsi_writecmd(lba->lba_num, buffer, RWBUFFER);
}

int insert(struct block_data *bd, unsigned char *buffer, int i){
	scsi_readcmd(i, buffer, RWBUFFER);
	int cursor = 0;
	while ( cursor < RWBUFFER){
		if (buffer[cursor] == 0) {
			buffer[cursor] = bd->dirty;
			cursor++;
			memcpy(&buffer[cursor], bd -> key, KEY_SIZE);
			cursor = cursor + KEY_SIZE;
			memcpy(&buffer[cursor], bd -> value, VALUE_SIZE);
			scsi_writecmd(i, buffer, RWBUFFER);
			return 1;
		}
		cursor = cursor + BLOCK_DATA_SIZE;
	}
	return 0;
}

void range_insert(struct block_data *bd, int range_min, int range_max) {
	unsigned char buffer[RWBUFFER];
	int i;
	for( i=range_min; i < range_max; i++ ) {
		memset(&buffer,0,RWBUFFER);
		if (insert(bd, buffer, i)) {
			return;
		}
	}
	printf("overflow\n");
	return;
}

void get_value(struct block_data *bd, struct LBA *lba) {
	unsigned char buffer[RWBUFFER];
	scsi_readcmd(lba->lba_num, buffer, RWBUFFER);
	int cursor  = BLOCK_DATA_SIZE * lba->data_cursor;
	cursor++;
	cursor = cursor + KEY_SIZE;
	memcpy(bd->value, &buffer[cursor], VALUE_SIZE);
}

int find_key(struct block_data *bd, struct LBA *lba, int range_min, int range_max) {
	int i, j;
	unsigned char rw_buffer[RWBUFFER];
	struct block_data *read_bd;
	for( i=range_min; i< range_max; i++){
		scsi_readcmd(i, rw_buffer, RWBUFFER);
		for( j=0; j< RWBUFFER/BLOCK_DATA_SIZE -1 ; j++ ) {
			read_bd = (block_data *)&rw_buffer[j*BLOCK_DATA_SIZE];
			if ( !memcmp(read_bd->key, bd -> key, KEY_SIZE) ){
				lba -> lba_num= i;
				lba -> data_cursor = j;
				return 1;
			}
		}
	}
	return 0;
}

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

void debug(int num){
	unsigned char buffer[RWBUFFER];
	scsi_readcmd(num, buffer, RWBUFFER);
	hex_dump(buffer, RWBUFFER);
}

