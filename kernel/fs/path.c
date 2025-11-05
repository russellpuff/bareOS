#include <fs/fs.h>
#include <system/panic.h>
#include <lib/string.h>

#define MAX_CHILDREN 64 /* Arbitrary search limit for directory children */
#define MAX_PATH_DEPTH 32 /* Arbitrary path depth limit for searching */
#define MAX_PATH_LEN ((FILENAME_LEN * MAX_PATH_DEPTH) + MAX_PATH_DEPTH + 1) /* Enough for max depth count filenames plus '/' for each */

/* Takes a dirent_t and a string buffer and populates the buffer with a full         *
 * path from the root to the entry                                                   *
 * Expects the buffer to be MAX_PATH_LEN in size, for now. Returns updated pointer   */
char* dirent_path_expand(dirent_t this, char* buffer) {
	if (buffer == NULL) return NULL;
	if (boot_fsd->super.root_dirent.inode == this.inode) {
		buffer[0] = '/';
		buffer[1] = '\0';
		return buffer;
	}
	buffer[MAX_PATH_LEN - 1] = '\0';
	/* TODO: replace this with a stack when stacks are implemented */
	uint16_t pos = MAX_PATH_LEN - 2; /* zero index position in buffer, leave space for null terminator */
	while (1) {
		dirent_t other;
		dir_iter_t iter;
		inode_t child = get_inode(this.inode);
		dir_open(child.parent, &iter);
		bool found = false;
		while (dir_next(&iter, &other) == 1) {
			if (!strcmp(other.name, ".") || !strcmp(other.name, "..")) continue;
			if (this.inode == other.inode) {
				found = true;
				uint32_t len = strlen(other.name);
				pos = pos - len + 1;
				memcpy(buffer + pos, other.name, len);
				if (boot_fsd->super.root_dirent.inode != other.inode) {
					pos -= 1;
					buffer[pos] = '/';
				}
				this.inode = child.parent;
				break;
			}
		}
		if (!found) {
			char* msg = "Filesystem corruption detected. No handler exists to recover from this.\n"
				"Reason: couldn't find a child dirent in its parent's inode blocks.\n"
				"Child: %u, Child name: %s, Parent: %u\n";
			panic(msg, this.inode, this.name, child.parent);
		}
		dir_close(&iter);
		if (boot_fsd->super.root_dirent.inode == this.inode) break;
	}
	return buffer + pos;
}

/* Scans a directory if a child by a specified name exists *
 * Pass in an optional dirent_t pointer to get a reference *
 * to the child if exists                                  */
bool dir_child_exists(dirent_t parent, const char* name, dirent_t* out) {
	if (name == NULL) return false;
	if (parent.type != DIR) return false;
	dirent_t children[MAX_CHILDREN];
	uint16_t count = dir_collect(parent, children, MAX_CHILDREN);
	for (uint16_t i = 0; i < count; ++i) {
		if (!strcmp(children[i].name, name)) {
			if (out != NULL) memcpy(out, &children[i], sizeof(dirent_t));
			return true;
		}
	}
	return false;
}

/* Function takes a path and an optional cwd (for resolving relative paths) *
 * and gets the lowest level directory dirent_t it can find. Used for       *
 * resolving the parent to a target file or directory, will not check if    *
 * the actual target at the end exists, as this may be called when creating *
 * new entries.                                                             *
 *                                                                          *
 * Paths with multiple consecutive slashes are rejected by this function by *
 * design. Users do not get the luxury of POSIX for now.                    *
 *                                                                          *
 * If the target of this path does exist and is a directory, it will be     *
 * returned by this function as a consequence of the design. This can be    *
 * useful in some cases so it's considered a feature.                       *
 *                                                                          *
 * Returns: status code as retval, 'out' is populated with requested        *
 * dirent_t if status ok. If the return value isn't 0, 'out' is unused.     *
 * 1 = bad call, 2 = part of the path doesn't exist (double slashes fault   *
 * here), 3 = a part of the path that wasn't the target was a file, 4 =     *
 * full path or component was too long                                      */
uint8_t resolve_dir(const char* path, const dirent_t* cwd, dirent_t* out) {
	if (path == NULL || *path == '\0') return 1; /* Invalid call */
	if (strlen(path) > MAX_PATH_LEN) return 4; /* Path too long */

	const char* p = path;
	dirent_t iter;

	if (path[0] == '/') {
		iter = boot_fsd->super.root_dirent;
		p++; /* Skip leading slash */
	}
	else {
		if (!cwd) return 1; /* Invalid call */
		iter = *cwd;
	}

	while (*p != '\0') {
		const char* r = p;
		while (*r != '/' && *r != '\0') ++r;

		/* Component length check */
		uint8_t l = (uint8_t)(r - p);
		if (l == 0) return 2; /* // in path */
		if (l > FILENAME_LEN) return 4; /* Part of the path was too long */

		char name[FILENAME_LEN];
		memcpy(name, p, l);
		name[l] = '\0';

		/* Keep parent before descent so we can ignore target if needed */
		dirent_t parent = iter;

		if (!dir_child_exists(iter, name, &iter)) {
			if (*r == '\0') {
				/* Component missing but we're done, this is a target that doesn't exist yet */
				iter = parent; /* Resolve nonexistent target's parent */
				break;
			}
			return 2; /* Part of the path didn't exist */
		}

		if (*r == '/') {
			if (r[1] == '/') return 2; /* // in path */
			if (iter.type != DIR) return 3; /* Part of the path was a file */
			p = r + 1;
			continue;
		}

		/* Final component reached, if it's a file, resolve its parent */
		if (iter.type != DIR) iter = parent;
	}

	memcpy(out, &iter, sizeof(dirent_t));
	return 0;
}

/* Takes a path and populates a buffer with its predicted name       *
 * Return code can be used to help guess what kind of path this is   *
 * 0 = error, 1 = directory, 2 = file (or dir without '/'), 3 = root *
 * 'buffer' will not be populated when returning 0 or 3              *
 *                                                                   *
 * This is for finding target name for API calls like open & mkdir,  *
 * so the caller must decide how to handle 2 based on context        *
 *                                                                   *
 * file/dir name policy: must be less than FILENAME_LEN with a null  *
 * term at the end, can start with '.' but cannot end with '.'       *
 *                                                                   *
 * This function cannot and will not resolve names of dot entries    *
 *                                                                   *
 * This function makes little effort to enforce the no multi-slash   *
 * policy for paths. Dir names ending in two slashes are rejected.   *
 * It is not responsible for and makes no effort to validate the     *
 * whole path. Use 'resolve_dir' to validate parent.                 */
uint8_t path_to_name(const char* path, char* buffer) {
	if (path == NULL || buffer == NULL || path == buffer) return 0;
	uint64_t len = strlen(path);

	if (len == 0) return 0;
	/* We don't populate the buffer because the caller will either treat  *
	 * this like an error, or have a unique handler for when the root is  *
	 * targeted here. Either way, everything knows the root's name.       */
	if (!strcmp(path, "/")) return 3; 

	/* og_len = original path length, immutable | len = length of target name */
	uint64_t og_len = len;
	const char* ptr = strrchr(path, '/');
	bool dir = false;
	if (ptr == NULL) {
		ptr = path; /* Is just a relative name without a trailing slash */
	}
	else {
		/* Trailing '/', need to back up until we hit the start or parent */
		if ((uint64_t)(ptr - path) == len - 1) {
			dir = true;
			while (ptr > path && ptr[-1] != '/') --ptr;
			len = (og_len - 1) - (uint64_t)(ptr - path); /* Truncate trailing '/' on memcpy */
			if (len == 0) return 0; /* This can happen if the path ends with two slashes, which isn't allowed */
		}
		else {
			/* No trailing, calculate correct length of just the name */
			ptr += 1;
			len = og_len - (uint64_t)(ptr - path);
		}
	}
	if (len > FILENAME_LEN - 1) return 0; /* Filename policy: caller required to create a buffer of FILENAME_LEN size */
	
	if (*ptr == '/' || ptr[len - 1] == '.') return 0;
	memcpy(buffer, ptr, len);
	buffer[len] = '\0';

	return dir ? 1 : 2;
}
