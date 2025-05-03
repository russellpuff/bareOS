#include <barelib.h>
#include <fs.h>
#include <malloc.h>
#include "testing.h"

static tests_t tests[] = {
  {
    .category = "General",
    .prompts = {
      "Program Compiles",
      "Creates blockstore",
      "Creates filesystem",
      "Mounts filesystem"
    }
  },
  {
    .category = "Create",
    .prompts = {
      "Returns error if directory is full",
      "Returns error if filename exists",
      "Returns error if no blocks remain",
      "Sets filename to a directory entry",
      "Sets inode index to a directory entry",
      "Marks inode as used in bitmask",
      "Saves changes back to blockstore"
    }
  },
  {
    .category = "Open",
    .prompts = {
      "Returns error if file is already open",
      "Returns error if file does not exist",
      "Returns error if oft is full",
      "Adds file to open file table",
      "Reads file control block into inode",
      "Returns index of the open file table entry"
    }
  },
  {
    .category = "Close",
    .prompts = {
      "Returns error if file is not open",
      "Writes inode back to blockstore",
      "Removes file from open file table"
    }
  }
};

void* memcpy(void*, void*, int);
void* memset(void*, int, int);

static int32 strcmp(char* a, char* b) {
  int i;
  for (i=0; a[i] == b[i] && a[i] != '\0'; i++);
  return a[i] != b[i];
}


static uint32 syscall_disabled(int32* result) {
  *result = 0;
  return 1;
}

static uint32 disabled(void) {
  return 1;
}

static uint32 phase = 1, test = 3;
static byte ramdisk[512][512];
static char blk_seed[16] = { 81, 42, 39, 100, 32, 88, 19, 22, 95, 201, 222, 23, 87, 48, 28, 11 };
static char size_seed[16] = { 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115 };
static void validate_create_error(char* filename, byte validate_name, byte* bitmap_default) {
  int32 result;
  for (int i=0; i<fsd->freemasksz; i++)
    fsd->freemask[i] = bitmap_default[i];

  result = create(filename);

  assert(result == -1, tests[phase].results[test], "s", "FAIL - Returned non-error value");
  for (int i=0; i<DIR_SIZE; i++)
    assert(strcmp(fsd->root_dir.entry[i].name, filename) != 0 || !validate_name, tests[phase].results[test],
	   "s", "FAIL - Filename found in directory after failure");
  for (int i=0; i<fsd->freemasksz; i++)
    assert(fsd->freemask[i] == bitmap_default[i], tests[phase].results[test], "s", "FAIL - Unneeded block was reserved for inode");

}

static void validate_create_success(int32 idx) {
  char filename[FILENAME_LEN] = {0};
  char cmpname[FILENAME_LEN] = {0};
  uint32 expect_blk = 1;
  int32 result;
  memcpy(filename, "testfile_", 9);
  memcpy(cmpname, "testfile_", 9);
  filename[9] = '0' + idx;

  for (int i=0; i<512; i++)
    for (int j=0; j<512; j++)
      ramdisk[i][j] = 0;

  // Find allocated block
  while (getmaskbit_fs(++expect_blk));
  
  result = create(filename);
  filename[9] = 'x';  // Purposely corrupt filename to check for real filname copy
  
  // Check fsd->root_dir.numentries
  assert(result != -1, tests[1].results[3], "s", "FAIL - 'create' returned with an error");
  assert(fsd->root_dir.numentries == idx+1, tests[1].results[4], "s", "FAIL - Directory entry count was not correct");
  for (int i=0; i<idx; i++) {
    cmpname[9] = '0' + i;
    assert(strcmp(cmpname, fsd->root_dir.entry[i].name) == 0, tests[1].results[3],
	   "s", "FAIL - Previous file name unexpectedly changed when new file was created");
  }

  // Check bitmap allocation
  assert(getmaskbit_fs(expect_blk) == 1, tests[1].results[5], "s", "FAIL - expected bit was not set after creating file");
  
  // Check fsd->root_dir.entry[i].name
  cmpname[9] = '0' + idx;
  assert(strcmp(cmpname, fsd->root_dir.entry[idx].name) == 0, tests[1].results[3], "s", "FAIL - Filename was not written to directory");

  // Check fsd->root_dir.entry[i].inode_block
  assert(expect_blk == fsd->root_dir.entry[idx].inode_block, tests[1].results[5], "s", "FAIL - expected block was not reserved for file");
  assert(getmaskbit_fs(fsd->root_dir.entry[idx].inode_block) == 1, tests[1].results[5],
	 "s", "FAIL - inode block was not marked as allocated");

  // Validate writeback
  uint32 blk = fsd->root_dir.entry[idx].inode_block;
  inode_t inode;

  // Validate inode block
  memcpy(&inode, ramdisk[blk], sizeof(inode_t));
  assert(inode.id == blk, tests[1].results[6], "s", "FAIL - Data in the inode block did not match expected value");
  assert(inode.size == 0, tests[1].results[6], "s", "FAIL - Data in the inode block did not match expeced value");
  
  // Validate bitmap
  char testmap[fsd->freemasksz];
  memcpy(testmap, &ramdisk[1], fsd->freemasksz);
  for (int i=0; i<fsd->freemasksz; i++)
    assert(testmap[i] == fsd->freemask[i], tests[1].results[6], "s", "FAIL - freemask block did not match freemask in mounted filesystem");

  // Validate superblock
  byte tmpfs[sizeof(fsystem_t)];
  byte* ptr = (byte*)fsd;

  memcpy(tmpfs, &ramdisk[0], sizeof(fsystem_t));
  for (int i=0; i<sizeof(fsystem_t); i++)
    assert(tmpfs[i] == ptr[i], tests[1].results[6], "s", "FAIL - superblock does not match current state of fsd");  
}

static void validate_open(char* filename, int32 expect_idx, int32 dir_idx) {
  uint32 blk = fsd->root_dir.entry[dir_idx].inode_block;
  int32 result = open(filename);
  assert(result == expect_idx, tests[2].results[5], "s", "FAIL - Returned unexpected oft index");
  assert(oft[result].state == FSTATE_OPEN, tests[2].results[3], "s", "FAIL - Returned oft index was not set to OPEN");
  assert(oft[result].head == 0, tests[2].results[3], "s", "FAIL - Returned oft index head was not set to the start of file");
  assert(oft[result].direntry == dir_idx, tests[2].results[3], "s", "FAIL - Returned oft index direntry did not match the file's index");
  assert(oft[result].inode.id == ramdisk[blk][0], tests[2].results[4],
	 "s", "FAIL - inode id does not match the value stored in the ramdisk");
  assert(oft[result].inode.size == ramdisk[blk][4], tests[2].results[4],
	 "s", "FAIL - inode size does not match the value stored in the ramdisk");
}

static uint32 pre_write_bs(uint32* result, uint32 blk, uint32 offset, void* buffer, uint32 len) {
  if (assert(blk < 512, tests[phase].results[test], "s", "FAIL - write_bs outside of block device range") &&
      assert(offset < 512, tests[phase].results[test], "s", "FAIL - write_bs offset outside of block size") &&
      assert((offset + len) <= 512, tests[phase].results[test], "s", "FAIL - write_bs writes outside of block size")) {
    memcpy(&(ramdisk[blk][offset]), buffer, len);
  }
  *result = 0;
  return 1;
}

static uint32 pre_read_bs(uint32* result, uint32 blk, uint32 offset, void* buffer, uint32 len) {
  if (assert(blk < 512, tests[phase].results[test], "s", "FAIL - read_bs outside of block device range") &&
      assert(offset < 512, tests[phase].results[test], "s", "FAIL - read_bs offset outside of block size") &&
      assert((offset + len) <= 512, tests[phase].results[test], "s", "FAIL - read_bs reads outside of block size")) {
    memcpy(buffer, &(ramdisk[blk][offset]), len);
  }
  *result = 0;
  return 1;
}


static void setup_general(void) {
  bdev_t dev = bs_getstats();
  assert(1, tests[0].results[0], "");
  assert(dev.blocksz == MDEV_BLOCK_SIZE, tests[0].results[1], "s", "FAIL - Block Device size was not set to MDEV_BLOCK_SIZE");
  assert(dev.nblocks == MDEV_NUM_BLOCKS, tests[0].results[1], "s", "FAIL - Block Device count was not set to MDEV_NUM_BLOCKS");

  assert(fsd->freemask != 0, tests[0].results[2], "s", "FAIL - Freemask was not successfully malloc'd during 'mkfs'");
  assert(((alloc_t*)fsd->freemask - 1)->state == M_USED, tests[0].results[2],
	 "s", "FAIL - Freemask was not successfully malloc'd during 'mkfs'");
  assert(((alloc_t*)fsd->freemask - 1)->size == fsd->freemasksz, tests[0].results[2],
	 "s", "FAIL - Freemaks was not succesfully malloc'd durring 'mkfs'");

  assert(t__analytics[BS_MK_RAMDISK_REF].numcalls > 0, tests[0].results[1], "s", "FAIL - Ramdisk was not created");
  assert(t__analytics[FS_MKFS_REF].numcalls > 0, tests[0].results[2], "s", "FAIL - Filesystem was not created");
  assert(t__analytics[FS_MOUNT_REF].numcalls > 0, tests[0].results[3], "s", "FAIL - Filesystem was not mounted");
  t__reset();
}

static void setup_create(void) {
  phase = 1;
  byte bitmap_default[fsd->freemasksz];
  
  // Directory Full Test
  test = 0;
  fsd->root_dir.numentries = DIR_SIZE;
  for (int i=0; i<fsd->freemasksz; i++)
    bitmap_default[i] = 0;
  bitmap_default[0] = 0x3;

  validate_create_error("hello.txt", true, bitmap_default);

  // File Exists Test
  test = 1;
  fsd->root_dir.numentries = 1;
  for (int i=0; i<fsd->freemasksz; i++)
    bitmap_default[i] = 0;
  bitmap_default[0] = 0x7;

  memcpy(fsd->root_dir.entry[0].name, "goodbye.txt", 12);
  fsd->root_dir.entry[0].inode_block = 2;

  validate_create_error("goodbye.txt", false, bitmap_default);

  // No Blocks Test
  test = 2;
  fsd->root_dir.numentries = 0;
  for (int i=0; i<fsd->freemasksz; i++)
    bitmap_default[i] = 0xFF;

  validate_create_error("toobig.txt", true, bitmap_default);
  
  // Create File 0-DIR_SIZE
  test = 6;
  for (int i=0; i<fsd->freemasksz; i++)
    fsd->freemask[i] = 0x0;
  fsd->freemask[0] = 0x3;

  for (int i=0; i<DIR_SIZE; i++)
    validate_create_success(i);
  
  t__reset();
}

static void setup_open(void) {
  int32 result;

  // Initialize fsd
  fsd->root_dir.numentries = DIR_SIZE;
  for (int i=0; i<DIR_SIZE; i++) {
    memcpy(fsd->root_dir.entry[i].name, "testfile_##", 10);
    fsd->root_dir.entry[i].name[9] = '0' + (i / 10);
    fsd->root_dir.entry[i].name[10] = '0' + (i % 10);
    fsd->root_dir.entry[i].inode_block = i + 2;
    setmaskbit_fs(i+2);
  }

  for (int i=0; i<DIR_SIZE; i++) {
    ramdisk[i+2][0] = blk_seed[i];
    ramdisk[i+2][4] = size_seed[i];
  }
  
  // Test error on filename doesn't exist
  result = open("hello.txt");
  assert(result == -1, tests[2].results[1], "s", "FAIL - Returned success value");
  for (int i=0; i<NUM_FD; i++)
    assert(oft[i].state != FSTATE_OPEN, tests[2].results[1], "FAIL - Entry opened in oft");

  // Test error on already open

  // Test file open
  validate_open("testfile_00", 0, 0);

  // Test file already open
  result = open("testfile_00");
  assert(result == -1, tests[2].results[0], "s", "FAIL - Returned success value");


  int i;
  char* filename = "testfile_##";
  for (i=0; i<NUM_FD-1; i++) {
    int idx = NUM_FD - i;
    filename[9]  = '0' + (idx / 10);
    filename[10] = '0' + (idx % 10);
    validate_open(filename, i+1, idx);
  }
  
  // Test error on no fd available
  filename[9] = '1';
  filename[10] = '3';
  result = open(filename);
  assert(result == -1, tests[2].results[2], "s", "FAIL - Returned successful value");
  
  t__reset();
}

static void setup_close(void) {
  inode_t inode;
  int32 result;
  // Try closing unopened file
  result = close(2);
  assert(result == -1, tests[3].results[0], "s", "FAIL - Returned non-error value");

  // Close normal file
  oft[2].state = FSTATE_OPEN;
  oft[2].head = 0;
  oft[2].direntry = 0;
  oft[2].inode.id = 2;
  oft[2].inode.blocks[0] = 3;
  oft[2].inode.size = 30;
  fsd->root_dir.entry[0].inode_block = 2;
  memcpy(fsd->root_dir.entry[0].name, "testfile", 9);
  memset(ramdisk[2], 0, 512);
  
  result = close(2);
  assert(result != -1, tests[3].results[2], "s", "FAIL - Returned error value from function call");
  assert(oft[2].state != FSTATE_OPEN, tests[3].results[2], "s", "FAIL - oft state was not change");
  memcpy(&inode, ramdisk[2], sizeof(inode_t));
  assert(inode.id == 2, tests[3].results[1], "s", "FAIL - inode in ramdisk does not match closed file");
  assert(inode.size == 30, tests[3].results[1], "s", "FAIL - inode in ramdisk does not match closed file");
  assert(inode.blocks[0] == 3, tests[3].results[1], "s", "FAIL - inode in ramdisk does not match closed file");

  t__reset();
}

void t__ms9(uint32 run) {
  switch (run) {
  case 0:
    t__initialize_tests(tests, 4);
    t__analytics[RESCHED_REF].precall = (uint32 (*)())disabled;
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_general;
    t__analytics[SEM_POST_REF].precall = (uint32 (*)())syscall_disabled;
    t__analytics[SEM_WAIT_REF].precall = (uint32 (*)())syscall_disabled;
    t__analytics[BS_WRITE_REF].precall = (uint32 (*)())pre_write_bs;
    t__analytics[BS_READ_REF].precall = (uint32 (*)())pre_read_bs;
    break;
  case 1:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_create; break;
  case 2:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_open; break;
  case 3:
    t__analytics[DISP_KERN_REF].precall = (uint32 (*)())setup_close; break;
  }
}
