#include <barelib.h>
#include <fs.h>

/* fs_write - Takes a file descriptor index  into the 'oft' and an  integer  *
 *            offset.  'fs_seek' moves the current head to match the given   *
 *            offset, bounded by the size of the file.                       *
 *                                                                           *
 *  returns - 'fs_seek' should return the new position of the file head      */
uint32_t seek(uint32_t fd, uint32_t offset, uint32_t relative) {
  return 0;
}
