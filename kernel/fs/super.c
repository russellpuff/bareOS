#include <fs/fs.h>
#include <lib/string.h>
#include <lib/limits.h>
#include <mm/malloc.h>

#include <lib/bareio.h>
/* TODO: put these somewhere better */
fsystem_t* boot_fsd; /* Active fsd the system boots with. */
drv_reg* reg_drives = NULL;
mount_t* mounted = NULL;

/* Initializes a blank filesystem and block device and writes it to its own super block * 
 * registers the new fs as a newly-discovered drive                                     */
uint8_t mkfs(uint32_t blocksize, uint32_t numblocks) {
	/* Set up a fresh fsd and blank block range to associate with it. */
	fsystem_t temp_fsd;
	boot_fsd = &temp_fsd; /* For functions that operate on the fsd, this avoids passing a reference to all of them. */
	temp_fsd.super.magic = FS_MAGIC;
	temp_fsd.super.version = FS_VERSION;
	temp_fsd.super.fat_head = FT_BIT;
	temp_fsd.super.fat_size = FT_LEN;
	temp_fsd.super.intable_head = IN_BIT;
	temp_fsd.super.intable_size = blocksize / sizeof(inode_t); /* times numblks, which is just 1 for a new fs */
	temp_fsd.super.intable_blocks[0] = IN_BIT;
	temp_fsd.super.intable_numblks = 1;
	temp_fsd.device = malloc(sizeof(bdev_t));
	if (mk_ramdisk(blocksize, numblocks, &temp_fsd) == -1) {
		// todo: handle error based on whether it's critical or not
		// critical = there's no other fs on the system and we're trying to create a blank one to run on
		// panic() doesn't exist on this branch
		kprintf("couldn't malloc space for the ramdisk");
		while (1);
	}

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

	/* mk_dir relies on the root existing already, so this has to be created by hand */
	/* todo: find some better way to do this */
	dirent_t* root = &boot_fsd->super.root_dirent;
	root->name[0] = '/';
	root->name[1] = '\0';
	root->type = EN_DIR;
	root->inode = in_find_free();
	inode_t rino;
	memset(&rino, 0, sizeof(rino));
	rino.parent = root->inode;
	rino.type = EN_DIR;
	rino.head = allocate_block();
	write_inode(rino, root->inode);
	dirent_t self_dot = get_dot_entry(root->inode, ".");
	dirent_t parent_dot = get_dot_entry(root->inode, "..");
	dir_write_entry(*root, self_dot);
	dir_write_entry(*root, parent_dot);

	/* Make some default subdirectories for the root */
	dirent_t dir;
	mk_dir("bin", boot_fsd->super.root_dirent, &dir); /* Where binaries are stored */
	mk_dir("home", boot_fsd->super.root_dirent, &dir); /* Home directory (for the only user) */
	mk_dir("etc", boot_fsd->super.root_dirent, &dir); /* Where logs and config files are stored */

	

	/* Set default tz. I go with EST because I want to */
	create_write("timezone", "est", dir);

	/* Make timezone subdir */
	mk_dir("tzinfo", dir, &dir);

	/* Make some default zone info files */
	create_write("utc", "UTC0", dir); /* Default UTC */
	create_write("est", "EST5EDT,M3.2.0/2,M11.1.0/2", dir); /* EST because that's where I live :D */
	create_write("jst", "JST-9", dir); /* JST as one across the world with a negative offset */

	/* Write super to the super block. It will be restored to the real fsd if this blank *
	 * filesystem is mounted by the user                                                 */
	write_bdev(SB_BIT, 0, &temp_fsd.super, sizeof(fsuper_t));

	/* Secretly writes the bdev's block sz/ct to super so if it gets rediscovered we know how big it is. *
	 * This will never be rewritten. */
	bdev_t temp = *temp_fsd.device;
	temp.ramdisk = NULL;
	write_bdev(SB_BIT, sizeof(fsuper_t), &temp, sizeof(bdev_t));

	boot_fsd = NULL;

	uint8_t status = discover_drive(temp_fsd.device->ramdisk);
	if (status != 0) {
		// todo: same error conditions as above
		const char* str = "couldn't discover new fs:";
		switch (status) {
		case 1: kprintf("%s not readable", str); break;
		case 2: kprintf("%s too many discovered drives", str); break; /* TODO: prompt user to destroy or preserve drive */
		case 3: kprintf("%s out of memory", str); break;
		}
		while (1);
	}
	
	return 0;
}

/* Since drives right now are stored in RAM as ramdisk, the only way to discover them is *
 * to know the pointer to the start of that ramdisk. Looks for a MAGIC it knows about to *
 * try to guess whether the pointer is valid. Only mounting it is definitive proof.      */
uint8_t discover_drive(byte* ramdisk) {
	if (*(uint32_t*)ramdisk != FS_MAGIC) return 1; /* Not a readable drive. */
   
	drv_reg* new = malloc(sizeof(drv_reg));
	if (new == NULL) return 3; /* OOM */
	new->next = NULL;

	if (reg_drives == NULL) {
		reg_drives = new;
		new->drive.id = 0;
	}
	else {
		drv_reg* iter = reg_drives;
		while (iter->next != NULL) iter = iter->next;
		if (iter->drive.id == 255) return 2; /* Too many discovered drives. */
		iter->next = new;
		new->drive.id = iter->drive.id + 1;
	}
	new->drive.mounted = false;

	/* Read serialized bdev size info. */
	memset(&new->drive.bdev, 0, sizeof(bdev_t));
	memcpy(&new->drive.bdev, ramdisk + sizeof(fsuper_t), sizeof(bdev_t));
	new->drive.bdev.ramdisk = ramdisk; /* Set ramdisk ptr */
	new->drive.fsd = NULL;

	return 0;
}

/* Take a drive_t with an initialized block device and attach   *
 * it to a fresh fsd. Reads the super from the bdev super block *
 * and sets the device as mounted for system use.               */
uint32_t mount_fs(drive_t* drive, const char* mount_point) {
	/* Allocate memory for the drive.fsd */
	drive->fsd = (fsystem_t*)malloc(sizeof(fsystem_t));
	if (drive->fsd == NULL) return 1; /* Not enough memory. */
	drive->fsd->device = &drive->bdev;

	fsystem_t* old_fsd = boot_fsd;
	boot_fsd = drive->fsd; /* temp while functions still only use this */

	/* Copy in super from the super block. */
	read_bdev(SB_BIT, 0, &drive->fsd->super, sizeof(fsuper_t));
	if (drive->fsd->super.magic != FS_MAGIC || drive->fsd->super.version != FS_VERSION) { free(drive->fsd); return 2; } /* Filesystem mismatch */

	boot_fsd = old_fsd;

	/* Init oft */
	for (uint8_t i = 0; i < OFT_MAX; ++i) {
		drive->fsd->oft[i].state = CLOSED;
		drive->fsd->oft[i].mode = RD_ONLY;
		drive->fsd->oft[i].curr_index = 0;
		drive->fsd->oft[i].in_index = IN_ERR;
		drive->fsd->oft[i].in_dirty = false;
		memset(&drive->fsd->oft[i].inode, 0, sizeof(inode_t));
	}

	/* Add to mounted list */
	/* malloc space for the entry*/
	mount_t* mnt = (mount_t*)malloc(sizeof(mount_t));
	if ((byte*)mnt == NULL) return 1;

	/* copy in mount point, TODO: validate this somewhere */
	uint32_t n = strlen(mount_point);
	mnt->mp = malloc(n + 1);
	if (mnt->mp == NULL) return 1;
	memcpy(mnt->mp, mount_point, n);
	mnt->mp[n] = '\0';

	mnt->fsd = drive->fsd;
	mnt->next = NULL;

	if (mounted == NULL) mounted = mnt;
	else {
		mount_t* iter = mounted;
		while (iter->next != NULL) iter = iter->next;
		iter->next = mnt;
	}

	drive->mounted = true;

	return 0;
}

uint32_t umount_fs(void) {
	return 0;
}
