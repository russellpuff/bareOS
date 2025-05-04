#include <barelib.h>
#include <fs.h>
//#include <string.h>


int16 strcmp0(const char* str1, const char* str2) {
  for(; *str1 == *str2; str1++, str2++) {
    if(*str1 == '\0') { return 0; }
  }
  return (unsigned char)*str1 - (unsigned char)*str2;
}


extern fsystem_t* fsd;

/*  Search for 'filename' in the root directory.  If the  *
 *  file exists,  returns an error.  Otherwise, create a  *
 *  new file  entry in the  root directory, a llocate an  *
 *  unused  block in the block  device and  assign it to  *
 *  the new file.                                         */
int32 create(char* filename) {
	if(fsd->root_dir.numentries == DIR_SIZE) return -1;

	/* Try to find a slot. */
	for (uint32 i = 0; i < fsd->root_dir.numentries; ++i) {
        if (!strcmp0(filename, fsd->root_dir.entry[i].name))
            return -1;
    }
    /* Use next available slot (no way to delete files so whatever). */
    int32 slot = fsd->root_dir.numentries;

	/* Find bit to use. */
	uint32 b = 0;
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
	inode.size = 0;
	for(uint16 i = 0; i < INODE_BLOCKS; ++i)
		inode.blocks[i] = EMPTY;
	if(write_bs(b, 0, &inode, sizeof(inode_t)) == -1) return -1;

	
	++fsd->root_dir.numentries;
	setmaskbit_fs(b);

	if (write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz) == -1) return -1; /* write back */
    if (write_bs(SB_BIT, 0, fsd, sizeof(fsystem_t)) == -1) return -1; /* write super */
	return 0;
}
