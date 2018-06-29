
#define SENSE_BUFFER_SIZE 32

#define SCSI_WRITE6 0x0a
#define WRITE6_CMD_LEN 6
#define WRITE_BUFFER_SIZE  512

#define SCSI_READ6 0x08
#define READ6_CMD_LEN 6
#define READ_BUFFER_SIZE 512

#define LBA_LEN 0x01
#define DEVICE_NAME "/dev/sdc"

#define RWBUFFER 512

#define KEY_SIZE 23
#define VALUE_SIZE 8
#define META_SIZE 1
#define BLOCK_DATA_SIZE (KEY_SIZE + VALUE_SIZE + META_SIZE)
/*
|     meta and flag 1byte            |
|     key 4byte        | value 2byte |
  * 128
*/

extern int fd;

typedef struct block_data {
	unsigned char dirty;
	unsigned char key[KEY_SIZE];
	unsigned char value[VALUE_SIZE];
}block_data;

typedef struct LBA {
	int lba_num;
	int data_cursor;
}LBA;


int scsi_readcmd(int block, unsigned char *buffer, int size);

int scsi_writecmd(int block, unsigned char* buffer, int size);

void init(int size);

void update(struct block_data *bd, struct LBA *lba);

int insert(struct block_data *bd, unsigned char *buffer, int i);

void range_insert(struct block_data *bd, int range_min, int range_max);

void get_value(struct block_data *bd, struct LBA *lba);

int find_key(struct block_data *bd, struct LBA *lba, int range_min, int range_max);

int put(unsigned char *key, float value, int k_size, int v_size);

float find(unsigned char *key, int k_size);

void debug(int num);
