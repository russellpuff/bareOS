#include <barelib.h>
#include <fs.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles"
    }
  },
  {
    .category = "Read Start",
    .prompts = {
      "Read returns error when file is not open",
      "Read file from start <partial-block>",
      "Read file from start <complete-block>",
      "Read file from start <multi-block>",
      "Read file from start <multi-block-partial>",
    }
  },
  {
    .category = "Read Offset",
    .prompts = {
      "Read block from offset <partial-block>",
      "Read block from offset <rest-of-block>",
      "Read block from offset <multi-block>",
      "Read block from offset <multi-block-partial>",
    }
  },
  {
    .category = "Read Nth Block",
    .prompts = {
      "Read nth block <partial>",
      "Read nth block <complete-block>",
      "Read nth block <multi-block>",
      "Read more than filesize"
    }
  },
  {
    .category = "Write Start",
    .prompts = {
      "Write returns error when file is not open",
      "Write start of file <partial-block>",
      "Write start of file <complete-block>",
      "Write start of file <multi-block>",
    }
  },
  {
    .category = "Write Append",
    .prompts = {
      "Overwrite existing data",
      "Append to end of file <partial>",
      "Append to end of file <offset-partial-block>",
      "Append to end of file <multi-block>"
    }
  },
};


uint32 read(uint32 fd, char* buff, uint32 len);
uint32 write(uint32 fd, char* buf, uint32 len);

void* memcpy(void*, void*, int);
void* memset(void*, int, int);

static uint32 dlen = 80, stride = 3;
static char* dictionary = "aaghdsfkjqweriurtyiubbxcm,bnasdfkhgiourtyqweuryta;a.,,mn3241o7608uaghlkjbnvcx5ba";
static uint32 phase = 0, test = 0;
static byte ramdisk[512][512];
static void build_file(uint32 size, uint32* blks) {
  uint32 sel = 0;

  for (uint32 offset=0; offset < size; offset++) {
    ramdisk[blks[offset / MDEV_BLOCK_SIZE]][offset % MDEV_BLOCK_SIZE] = dictionary[sel % dlen];
    sel += stride;
  }
}

static void validate_read(int32 test_idx, int32 fd, int32 offset, int32 size) {
  char buffer[2000];
  int32 expect_head = (size + offset <= oft[fd].inode.size ? size + offset : oft[fd].inode.size);
  test = test_idx;
  oft[fd].head = offset;
  memset(buffer, 0, 2000);
  read(fd, buffer, size);

  
  assert(oft[fd].head == expect_head, tests[phase].results[test], "s", "FAIL - Head was not correct after read");
  assert(oft[fd].inode.size == 2000, tests[phase].results[test], "s", "FAIL - File size changed during read");
  for (int i=0; i<size; i++) {
    uint16 blk = oft[fd].inode.blocks[(i + offset) / MDEV_BLOCK_SIZE];
    uint16 idx = (i + offset) % MDEV_BLOCK_SIZE;
    if (i + offset < oft[fd].inode.size)
      assert(buffer[i] == ramdisk[blk][idx], tests[phase].results[test], "s", "FAIL - Buffer data does not match contents of ramdisk");
    else
      assert(buffer[i] == 0, tests[phase].results[test], "s", "FAIL - Buffer was written past the filesize");
  }
}

static void validate_write(int32 test_idx, int32 fd, int32 offset, int32 size) {
  char target[4000];
  char buffer[size];
  int32 i = 0;
  inode_t* inode = &oft[fd].inode;
  int32 expect_size = (offset + size > inode->size ? offset + size : inode->size);
  
  do {
    uint16 blk = i / MDEV_BLOCK_SIZE;
    uint16 idx = i % MDEV_BLOCK_SIZE;
    target[i] = ramdisk[inode->blocks[blk]][idx];
  } while (++i < 4000 && i < inode->size);
  
  for (int j=0; j<size; j++) {
    buffer[j] = j % 20;
    target[j + offset] = j % 20;
  }

  oft[fd].head = offset;
  write(fd, buffer, size);

  for (int i=0; i<inode->size; i++) {
    uint16 blk = i / MDEV_BLOCK_SIZE;
    uint16 idx = i % MDEV_BLOCK_SIZE;
    assert(target[i] == ramdisk[inode->blocks[blk]][idx], tests[phase].results[test_idx],
	   "s", "FAIL - Data in file does not match expected values after write");
  }
  // Assert Size
  assert(inode->size == expect_size, tests[phase].results[test_idx], "s", "FAIL - File size does not match expected new file size");
  
  // Assert Head
  assert(oft[fd].head == offset + size, tests[phase].results[test_idx],
	 "s", "FAIL - File head does not point to the correct index after write");
}

static uint32 syscall_disabled(int32* result) {
  *result = 0;
  return 1;
}

static uint32 disabled(void) {
  return 1;
}

static byte skip_bs_validate = true;
static uint32 pre_write_bs(uint32* result, uint32 blk, uint32 offset, void* buffer, uint32 len) {
  if (skip_bs_validate ||
      (assert(blk < 512, tests[phase].results[test], "s", "FAIL - write_bs outside of block device range") &&
       assert(offset < 512, tests[phase].results[test], "s", "FAIL - write_bs offset outside of block size") &&
       assert((offset + len) <= 512, tests[phase].results[test], "s", "FAIL - write_bs writes outside of block size"))) {
    memcpy(&(ramdisk[blk][offset]), buffer, len);
  }
  *result = 0;
  return 1;
}

static uint32 pre_read_bs(uint32* result, uint32 blk, uint32 offset, void* buffer, uint32 len) {
  if (skip_bs_validate ||
      (assert(blk < 512, tests[phase].results[test], "s", "FAIL - read_bs outside of block device range") &&
       assert(offset < 512, tests[phase].results[test], "s", "FAIL - read_bs offset outside of block size") &&
       assert((offset + len) <= 512, tests[phase].results[test], "s", "FAIL - read_bs reads outside of block size"))) {
    memcpy(buffer, &(ramdisk[blk][offset]), len);
  }
  *result = 0;
  return 1;
}


static void setup_general(void) {
  assert(1, tests[0].results[0], "");
  t__reset();
}

static void run_read_1(int32 fd) {
  // Test Read [block: 0, offset: 0, size: 10]
  validate_read(1, fd, 0, 10);

  // Test Read [block: 0, offset: 0, size: MDEV_BLOCK_SIZE]
  validate_read(2, fd, 0, MDEV_BLOCK_SIZE);

  // Test Read [Block: 0, offset: 0, size: MDEV_BLOCK_SIZE + 40]
  validate_read(3, fd, 0, MDEV_BLOCK_SIZE + 40);

  // Test Read [Block: 0, offset: 0, size: MDEV_BLOCK_SZE * 2]
  validate_read(4, fd, 0, MDEV_BLOCK_SIZE * 2);
}

static void run_read_2(int32 fd) {
  // Test Read [Block: 0, offset: 30, size: 100]
  validate_read(0, fd, 30, 100);

  // Test Read [Block: 0, offset: 82, size: MDEV_BLOCK_SIZE - 82]
  validate_read(1, fd, 82, MDEV_BLOCK_SIZE - 82);

  // Test Read [Block: 0, offset: 11, size: (MDEV_BLOCK_SIZE - 11) + MDEV_BLOCK_SIZE]
  validate_read(2, fd, 11, (MDEV_BLOCK_SIZE - 11) + MDEV_BLOCK_SIZE);

  // Test Read [Block: 0, offset: 48, size: MDEV_BLOCK_SZE]
  validate_read(3, fd, 48, MDEV_BLOCK_SIZE);
}

static void run_read_3(int32 fd) {
  int32 blk_offset = 2 * MDEV_BLOCK_SIZE;
  // Test Read [Block: 2, offset: 0, size: 383]
  validate_read(0, fd, blk_offset, 383);
  
  // Test Read [Block: 2, offset: 0, MDEV_BLOCK_SIZE]
  validate_read(1, fd, blk_offset, MDEV_BLOCK_SIZE);

  // Test Read [Block: 2, offset: 40, MDEV_BLOCK_SIZE + 40]
  validate_read(2, fd, blk_offset + 40, MDEV_BLOCK_SIZE + 40);
  
  // Read off of file
  validate_read(3, fd, 1990, 30);
}

static void run_write_1(int32 fd) {
  // Test Write [Block: 0, offset: 0, size: 50]
  validate_write(1, fd, 0, 50);

  // Test Write [Block: 0, offset: 0, size: MDEV_BLOCK_SIZE]
  validate_write(2, fd, 0, MDEV_BLOCK_SIZE);

  // Test Write [Block: 0, offset: 0, size: MDEV_BLOCK_SIZE + 80]
  validate_write(3, fd, 0, MDEV_BLOCK_SIZE + 80);
}

static void run_write_2(int32 fd) {
  // Test Write [offset: oft[fd].inode.size, size: 43]
  validate_write(1, fd, oft[fd].inode.size, 43);
  
  // Test Write [offset: oft[fd].inode.size, size: MDEV_BLOCK_SIZE - (oft[fd].inode.size % MDEV_BLOCK_SIZE)]
  validate_write(2, fd, oft[fd].inode.size, MDEV_BLOCK_SIZE - (oft[fd].inode.size % MDEV_BLOCK_SIZE));

  // Test Write [offset: oft[fd].inode.size, size: MDEV_BLOCK_SIZE * 2]
  validate_write(3, fd, oft[fd].inode.size, MDEV_BLOCK_SIZE * 2);

  // Test Write [offset: 200, size: 80]
  validate_write(0, fd, 200, 80);
}

static int32 setup_test_file(int32 size, inode_t* inode) {
  fsd->root_dir.numentries = 1;
  fsd->root_dir.entry[0].inode_block = 2;
  memcpy(fsd->root_dir.entry[0].name, "foo", 4);
  setmaskbit_fs(2);

  inode->id = 2;
  inode->size = size;
  inode->blocks[0] = 8;
  inode->blocks[1] = 4;
  inode->blocks[2] = 32;
  inode->blocks[3] = 19;
  inode->blocks[4] = 100;
  inode->blocks[5] = 101;
  inode->blocks[6] = 102;
  inode->blocks[7] = 103;

  build_file(size, inode->blocks);
  setmaskbit_fs(8);
  setmaskbit_fs(4);
  setmaskbit_fs(32);
  setmaskbit_fs(19);

  return 2;
}

static void setup_read(void) {
  inode_t inode;
  int32 result, fd=setup_test_file(2000, &inode);
  char buffer[1000];
  skip_bs_validate = false;
  
  // Test Error on Not Open
  result = read(fd, buffer, 100);
  assert(result == -1, tests[1].results[0], "s", "FAIL - Function returned non-error value");
  
  // Open file [oft_idx: 2]
  oft[fd].state = FSTATE_OPEN;
  oft[fd].head = 0;
  oft[fd].direntry = 0;
  memcpy(&(oft[fd].inode), &inode, sizeof(inode));


  switch (phase) {
  case 1:
    run_read_1(fd); break;
  case 2:
    run_read_2(fd); break;
  case 3:
    run_read_3(fd); break;
  }

  

  t__reset();
}

static void setup_write(void) {
  inode_t inode;
  int32 result, fd=setup_test_file(phase == 4 ? 0 : 2000, &inode);
  char buffer[100];
  skip_bs_validate = false;

  for (int i=0; i<100; i++) {
    buffer[i] = i % 20;
  }
  // Test Error on Not Open
  result = write(fd, buffer, 100);
  assert(result == -1, tests[4].results[0], "s", "FAIL - Function returned non-error value");
  
  // Open file [oft_idx: 2]
  oft[fd].state = FSTATE_OPEN;
  oft[fd].head = 0;
  oft[fd].direntry = 0;
  memcpy(&(oft[fd].inode), &inode, sizeof(inode));

  switch (phase) {
  case 4:
    run_write_1(fd); break;
  case 5:
    run_write_2(fd); break;
  }
  
  t__reset();
}
  
void t__ms10(uint32 run) {
  phase = run;
  skip_bs_validate = true;
  switch (run) {
  case 0:
    t__initialize_tests(tests, 6);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general;
    t__analytics[SEM_POST_REF].precall = (uint32 (*)())syscall_disabled;
    t__analytics[SEM_WAIT_REF].precall = (uint32 (*)())syscall_disabled;
    t__analytics[BS_WRITE_REF].precall = (uint32 (*)())pre_write_bs;
    t__analytics[BS_READ_REF].precall = (uint32 (*)())pre_read_bs;
    break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_read; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_read; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_read; break;
  case 4:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_write; break;
  case 5:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_write; break;
  }
}
