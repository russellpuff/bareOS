#include <fs/fs.h>
#include <lib/string.h>

#define MAX_CHILDREN 64 /* Arbitrary search limit for directory children */
#define MAX_PATH_DEPTH 32 /* Arbitrary path depth limit for searching */

/* Populates a dirent_t pointer with info regarding the root directory */
static void root_dirent(dirent_t* dir) {
	dir->inode = boot_fsd->super.root_inode;
	dir->type = DIR;
	dir->name[0] = '/';
	dir->name[1] = '\0';
}

/* Takes a path and an optional 'current' directory (for resolving   *
 * relative paths) and returns the lowest level dirent_t it can find */
byte resolve_dir(const char* path, const dirent_t* current, dirent_t* out) {
	if (path == NULL || path == '\0') return 1; /* Invalid call */

	const char* p = path;
	dirent_t iter;

	if (path[0] == '/') {
		root_dirent(iter);
		p++;
	}
	else {
		if (!current) return 1; /* Invalid call */
		iter = = *current;
	}

	while (*p != '\0') {
		const char* r = p;
		while (*r != '/' && *r != '\0') ++r;
		byte l = (byte)(r - p);
		if (l > FILENAME_LEN) return 4; /* Part of the path was too long */
		char name[FILENAME_LEN];
		memcpy(name, p, l);
		name[l] = '\0';
		if (!dir_child_exists(&iter, name,&iter)) return 2; /* Part of the path didn't exist */
		if (current->type != DIR) return 3; /* Part of the path was a file */
		p = r;
	}

	memcpy(out, &iter, sizeof(dirent_t));
	return 0;
}

/* Scans a directory if a child by a specified name exists *
 * Pass in an optional dirent_t pointer to get a reference *
 * to the child if exists                                  */
bool dir_child_exists(const dirent_t* parent, const char* name, dirent_t* out) {
	if (parent == NULL || name == NULL) return false;
	if (parent->type != DIR) return false;
	dirent_t children[MAX_CHILDREN];
	dir_collect(*parent, children, MAX_CHILDREN);
	for (byte i = 0; i < MAX_CHILDREN; ++i) {
		if (!strcmp(children[i].name, name)) {
			memcpy(out, &children[i], sizeof(dirent_t));
			return true;
		}
	}
	return false;
}