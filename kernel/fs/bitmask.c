#include <fs/fs.h>

void setmaskbit_fs(uint32_t x) {                     /*                                           */
	if (fsd == NULL) return;                         /*  Sets the block at index 'x' as used      */
	fsd->freemask[x / 8] |= 0x1 << (x % 8);          /*  in the free bitmask.                     */
}                                                  /*                                           */

void clearmaskbit_fs(uint32_t x) {                   /*                                           */
	if (fsd == NULL) return;                         /*  Sets the block at index 'x' as unused    */
	fsd->freemask[x / 8] &= ~(0x1 << (x % 8));       /*  in the free bitmask.                     */
}                                                  /*                                           */

uint32_t getmaskbit_fs(uint32_t x) {                   /*                                           */
	if (fsd == NULL) return -1;                      /*  Returns the current value of the         */
	return (fsd->freemask[x / 8] >> (x % 8)) & 0x1;  /*  'x'th block in the block device          */
}                                                  /*  0 for unused 1 for used.                 */