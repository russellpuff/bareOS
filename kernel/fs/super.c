#include <fs/fs.h>
#include <lib/string.h>
#include <system/interrupts.h>
#include <mm/malloc.h>

fsystem_t* fsd = NULL;
filetable_t oft[NUM_FD];


/*  Build the file system and save it to a block device.  *
 *  Must be called before the filesystem can be used      */
void mkfs(void) {
    fsystem_t temp_fsd;
    bdev_t device = bs_getstats();
    uint32_t masksize, i;

    masksize = device.nblocks / 8;                                /*                                             */
    masksize += (device.nblocks % 8 ? 0 : 1);                     /*  Construct the 'fsd' variable               */
    temp_fsd.device = device;                                     /*  and set to initial values                  */
    temp_fsd.freemasksz = masksize;                               /*                                             */
    temp_fsd.freemask = malloc(masksize);                         /*  Allocate the free bitmask                  */
    temp_fsd.root_dir.numentries = 0;                             /*                                             */

    for (i = 0; i < masksize; i++)                                /*                                             */
        temp_fsd.freemask[i] = 0;                                 /*  Initially clear the free bitmask           */
     
    for (i = 0; i < DIR_SIZE; i++) {                              /*  Set up the directory entries as            */
        temp_fsd.root_dir.entry[i].inode_block = EMPTY;           /*  empty.                                     */
        memset(temp_fsd.root_dir.entry[i].name, 0, FILENAME_LEN); /*                                             */
    }                                                             /*                                             */

    temp_fsd.freemask[SB_BIT / 8] |= 0x1 << (SB_BIT % 8);        /*                                             */
    temp_fsd.freemask[BM_BIT / 8] |= 0x1 << (BM_BIT % 8);        /*  Set  the  super  block  and free  bitmask  */
    write_bs(SB_BIT, 0, &temp_fsd, sizeof(fsystem_t));           /*  block  as used  and write  the 'fsd'  and  */
    write_bs(BM_BIT, 0, temp_fsd.freemask, temp_fsd.freemasksz); /*  bitmask to the 0 and 1 block respectively  */
    free(temp_fsd.freemask);                                     /*                                             */

    return;
}


/*  Take an initialized block device containing a file system *
 *  and copies it into the 'fsd' to make it the active file   *
 *  system.                                                   */
uint32_t mount_fs(void) {
    uint32_t i;

    if ((fsd = (fsystem_t*)malloc(sizeof(fsystem_t))) == (fsystem_t*)-1) {  /*  Allocate space for the fsd  */
        return 1;                                                            /*                              */
    }                                                                       /*  Read the contents of the    */
    read_bs(SB_BIT, 0, fsd, sizeof(fsystem_t));                             /*  superblock into the 'fsd'   */
    if ((fsd->freemask = malloc(fsd->freemasksz)) == NULL) {                /*                              */
        return 1;                                                            /*  Allocate space for the      */
    }                                                                       /*  free bitmask and read       */
    read_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz);                     /*  the block from the block    */
    /*  device.                     */
    for (i = 0; i < NUM_FD; i++) {                                              /*                              */
        oft[i].state = FSTATE_CLOSED;                                         /*  Initialize the open file    */
        oft[i].head = 0;                                                      /*  table                       */
        oft[i].direntry = 0;                                                  /*                              */
    }                                                                       /*                              */

    return 0;
}


/*  Write the current state of the file system to a block device and  *
 *  free the resources for the file system.                           */
uint32_t umount_fs(void) {
    write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz);     /*  Write the bitmask and super blocks to  */
    write_bs(SB_BIT, 0, fsd, sizeof(fsystem_t));             /*  their respective block device blocks   */

    free(fsd->freemask);                                     /*  Free memory used for the filesystem    */
    free((void*)fsd);                                        /*                                         */

    return 0;
}