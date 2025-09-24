#include <fs/fs.h>
#include <lib/string.h>
#include <lib/limits.h>
#include <mm/malloc.h>

fsystem_t* boot_fsd; /* Active fsd the system boots with. */

/* Initializes a blank filesystem and block device and writes it to its own super block * 
 * registers the new fs as a newly-discovered drive                                     */
bdev_t mkfs(uint32_t blocksize, uint32_t numblocks) {
    /* Set up a fresh fsd and blank block range to associate with it. */
    fsystem_t temp_fsd;
    boot_fsd = &temp_fsd; /* For functions that operate on the fsd, this avoids passing a reference to all of them. */
    temp_fsd.super.magic = FS_MAGIC;
    temp_fsd.super.version = FS_VERSION;
    temp_fsd.super.fat_head = FT_BIT;
    temp_fsd.super.intable_head = IN_BIT;
    temp_fsd.super.intable_size = blocksize / sizeof(inode_t);
    temp_fsd.super.root_inode = 0;
    temp_fsd.super.intable_blocks[0] = IN_BIT;
    temp_fsd.super.intable_numblks = 1;
    mk_ramdisk(blocksize, numblocks, &temp_fsd);

    /* Clear the super, bitmask, and FAT tables and mark them as used.*/
    bdev_zero_blocks(SB_BIT, 1);
    bdev_zero_blocks(BM_BIT, 1);
    bdev_zero_blocks(FT_BIT, FT_LEN);
    bdev_zero_blocks(IN_BIT, 1);
    bm_set(SB_BIT);
    bm_set(BM_BIT);
    bm_set(IN_BIT);
    for (uint16_t i = 0; i < FT_LEN; ++i) bm_set(FT_BIT + i);
    fat_set(SB_BIT, FAT_RSVD);
    fat_set(BM_BIT, FAT_RSVD);
    fat_set(IN_BIT, FAT_RSVD);
    for (uint16_t i = 0; i < FT_LEN; ++i) fat_set(FT_BIT + i, FAT_RSVD);

    /* The 'mk_dir' will set up an empty root directory, then we discard the dirent_t  *
     * This function expects this to be the first inode created, thus root gets index  *
     * 0 by default, for now there's no reason to validate this                        */
    dirent_t root = mk_dir("", temp_fsd.super.root_inode);

    /* Write super to the super block. It will be restored to the real fsd if this blank *
     * filesystem is mounted by the user                                                 */
    write_bdev(SB_BIT, 0, &temp_fsd.super, sizeof(fsuper_t));

    /* Secretly writes the bdev's block sz/ct to super so if it gets rediscovered we know how big it is. *
     * This will never be rewritten. */
    bdev_t temp = *temp_fsd.device;
    temp.ramdisk = NULL;
    write_bdev(SB_BIT, sizeof(fsuper_t), &temp, sizeof(bdev_t));

    boot_fsd = NULL;
    return *temp_fsd.device;
}

/* Since drives right now are stored in RAM as ramdisk, the only way to discover them is *
 * to know the pointer to the start of that ramdisk. Looks for a MAGIC it knows about to *
 * try to guess whether the pointer is valid. Only mounting it is definitive proof.      */
byte discover_drive(byte* ramdisk) {
    if (*(uint32_t*)ramdisk != FS_MAGIC) return 1; /* Not a readable drive. */
    drv_reg* iter = reg_drives;
    while (iter->next != NULL) iter = iter->next;
    if (iter->drive.id == 255) return 2; /* Too many discovered drives. */
    drv_reg* new = malloc(sizeof(drv_reg));
    if (new == NULL) return 3; /* OOM */
    iter->next = new;
    new->drive.id = iter->drive.id + 1;
    new->drive.mounted = false;

    /* Read serialized bdev size info. */
    memcpy(&new->drive, ramdisk + sizeof(fsuper_t), sizeof(bdev_t));
    new->drive.bdev.ramdisk = ramdisk; /* Set ramdisk ptr */

    return 0;
}

/* Take a drive_t with an initialized block device and attach   *
 * it to a fresh fsd. Reads the super from the bdev super block *
 * and sets the device as mounted for system use.               */
uint32_t mount_fs(drive_t* drive) {
    /* Allocate memory for the drive.fsd */
    drive->fsd = (fsystem_t*)malloc(sizeof(fsystem_t));
    if (drive->fsd == NULL) return 1; /* Not enough memory. */

    /* Copy in super from the super block. */
    read_bdev(SB_BIT, 0, &drive->fsd->super, sizeof(fsuper_t));
    if (drive->fsd->super.magic != FS_MAGIC || drive->fsd->super.version != FS_VERSION) { free(drive->fsd); return 2; } /* Filesystem mismatch */

    /* Init oft */
    for (byte i = 0; i < OFT_MAX; ++i) {
        drive->fsd->oft[i].state = CLOSED;
        drive->fsd->oft[i].mode = RD_ONLY;
        drive->fsd->oft[i].curr_block = 0xFFFF;
        drive->fsd->oft[i].curr_index = 0xFFFF;
        drive->fsd->oft[i].in_index = IN_ERR;
        drive->fsd->oft[i].in_dirty = false;
        memset(&drive->fsd->oft[i].inode, 0, sizeof(inode_t));
    }

    /* Set fsd ref to bdev. */
    drive->fsd->device = &drive->bdev;

    drive->mounted = true;
    return 0;
}


/*  Write the current state of the file system to a block device and  *
 *  free the resources for the file system.                           */
uint32_t umount_fs(void) {
    //write_bs(BM_BIT, 0, fsd->freemask, fsd->freemasksz);     /*  Write the bitmask and super blocks to  */
    //write_bs(SB_BIT, 0, fsd, sizeof(fsystem_t));             /*  their respective block device blocks   */
    //
    //free(fsd->freemask);                                     /*  Free memory used for the filesystem    */
    //free((void*)fsd);                                        /*                                         */

    return 0;
}