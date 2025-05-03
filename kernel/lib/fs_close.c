#include <barelib.h>
#include <fs.h>

extern fsystem_t* fsd;
extern filetable_t oft[NUM_FD];

/*  Modify  the state  of the open  file table to  close  *
 *  the 'fd' index and write the inode back to the block  *
    device.  If the  entry is already closed,  return an  *
 *  error.                                                */
int32 close(int32 fd) {
  return 0;
}
