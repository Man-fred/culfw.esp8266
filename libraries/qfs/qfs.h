#ifndef __QFS_H
#define __QFS_H

#include "df.h"

#define QFS_PAGESIZE      528
#define QFS_BLOCKSIZE     512
#define QFS_MAGIC_OFFSET  QFS_BLOCKSIZE
#define QFS_MAGIC_LEN     4
#define QFS_MAGIC         "QFS1"
#define QFS_FILENAMESIZE  14
#define QFS_ROOT_PAGE     0
#define QFS_MAX_INODE     (QFS_BLOCKSIZE/sizeof(inode_t))       // 32

#define QFS_LASTPAGE_MODIFIED  (1<<0)
#define QFS_ROOTPAGE_MODIFIED  (1<<1)

#define FS_FILENAME QFS_FILENAMESIZE

typedef uint16_t fs_inode_t;
typedef uint16_t fs_index_t;
typedef int32_t fs_size_t;

typedef struct {
  uint16_t       len;
  char           name[QFS_FILENAMESIZE];
} inode_t;

typedef struct {
  df_chip_t chip;
  df_page_t lastpage;
  uint8_t   flags;
  uint8_t   last_inode_index;
  inode_t   last_inode;
} fs_t;

typedef enum {
    FS_OK         = 0,
    FS_NOSUCHFILE = 1,
    FS_EOF        = 2,
    FS_BADPAGE    = 3,
    FS_BADINODE   = 4,
    FS_DUPLICATE  = 5,
    FS_MEM        = 6,
    FS_BADSEEK    = 7,
} fs_status_t;

class FsClass {
	public:
		fs_status_t init(fs_t *fs, df_chip_t chip, uint8_t force);
		fs_status_t create(fs_t *fs, char *name);
		fs_status_t rename(fs_t *fs, char *from, char *to);
		fs_status_t remove(fs_t *fs, char *name);
		fs_status_t list(fs_t *fs, char *dir, char *buf, fs_index_t index);
		fs_inode_t  get_inode(fs_t *fs, char *file);
		fs_size_t   read(fs_t *fs, fs_inode_t inode,
														void *buf, fs_size_t offset, fs_size_t length);
		fs_status_t write(fs_t *fs, fs_inode_t inode,
														void *buf, fs_size_t offset, fs_size_t length);
		fs_size_t   size(fs_t *fs, fs_inode_t inode);
		fs_status_t sync(fs_t *fs);
  private:
		void cache_last_inode(fs_t *fs, fs_inode_t inode);
		void cache_page(fs_t *fs, uint16_t page, uint8_t doload);
		uint16_t offset2page(uint16_t inode, uint16_t offset);
};

extern FsClass Fs;

#endif
