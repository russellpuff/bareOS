#include <barelib.h>
#include <fs.h>
#include <string.h>

/*
int16 strcmp(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}
*/

extern fsystem_t* fsd;

/*  Search for 'filename' in the root directory.  If the  *
 *  file exists,  returns an error.  Otherwise, create a  *
 *  new file  entry in the  root directory, a llocate an  *
 *  unused  block in the block  device and  assign it to  *
 *  the new file.                                         */
int32 create(char* filename) {
	if(fsd->root_dir.numentries == DIR_SIZE) return -1;
	/* Try to find a slot. */
	int32 slot = -1;
	for(int32 i = 0; i < DIR_SIZE; ++i) {
		if(!fsd->root_dir.entry[i].name[0])  {
			if(slot == -1) slot = i;
			continue;
		}
		if(!strcmp(filename, fsd->root_dir.entry[i].name)) return -1;
	}
	if(slot == -1) return -1;
	/* Find bit to use. */
	int b = 0;
	for(; b < fsd->device.nblocks; ++b)
		if(!getmaskbit_fs(b)) break;
	if(b == fsd->device.nblocks) return -1;

	/* Copy in filename. */
	/* barelib.c memset causing issues, do it manually. */
	for(int16 m = 0; m < FILENAME_LEN; ++m)
		fsd->root_dir.entry[slot].name[m] = '\0';
	/* barelib.c memcpy also junk. */
	for (int n = 0; filename[n] && n < FILENAME_LEN - 1; ++n)
		fsd->root_dir.entry[slot].name[n] = filename[n];

	fsd->root_dir.entry[slot].inode_block = b;

	/* Construct inode. */
	inode_t inode;
	inode.id = b;
	for(uint16 i = 0; i < INODE_BLOCKS; ++i)
		inode.blocks[i] = EMPTY;
	if(write_bs(b, 0, &inode, sizeof(inode_t)) < 0) return -1;

	fsd->root_dir.entry[slot].inode_block = b;
	++fsd->root_dir.numentries;
	setmaskbit_fs(b);

	if (write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz) < 0) return -1; /* write back */
    if (write_bs(SB_BIT, 0, fsd, sizeof(fsystem_t)) < 0) return -1; /* write super */
	return 0;
}
