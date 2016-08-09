#include "../extstore.h"

static char store_root[MAXPATHLEN];

static int build_extstore_path(kvsns_ino_t object,
			       char *extstore_path,
			       size_t pathlen)
{
	if (!extstore_path)
		return -1;

	return snprintf(extstore_path, pathlen, "%s/inum=%llu",
			store_root, (unsigned long long)object);
}

int extstore_init(char *rootpath)
{
	strncpy(store_root, rootpath, MAXPATHLEN);
	return 0;
}

int extstore_del(kvsns_ino_t *ino)
{
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = unlink(storepath);
	if (rc)
		return -errno;

	return 0;
}

int extstore_read(kvsns_ino_t *ino, 
		  off_t offset,
		  size_t buffer_size,
		  void *buffer,
		  size_t *read_amount,
		  bool *end_of_file)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t read_bytes;


	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	printf("READ: I got external path=%s\n", storepath);

	fd = open(storepath, O_CREAT|O_RDONLY|O_SYNC);
	if (fd < 0) {
		printf("READ: error %u while open %s\n",
			errno, storepath);
		return -errno;
	}

	read_bytes = pread(fd, buffer, buffer_size, offset);
	if (read_bytes < 0) {
		close(fd);
		return -errno;
	}

	rc = close(fd);
	if (rc < 0)
		return -errno;

	*read_amount = read_bytes;
	return read_bytes;
}

int extstore_write(kvsns_ino_t *ino,
		   off_t offset,
		   size_t buffer_size,
		   void *buffer,
		   size_t *write_amount,
		   bool *fsal_stable,
		   struct stat *stat)
{
	char storepath[MAXPATHLEN];
	int rc;
	int fd;
	ssize_t written_bytes;
	struct stat storestat;


	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	printf("WRITE: I got external path=%s\n", storepath);

	fd = open(storepath, O_CREAT|O_WRONLY|O_SYNC, 0755);
	if (fd < 0)
		return -errno;

	written_bytes = pwrite(fd, buffer, buffer_size, offset);
	if (written_bytes < 0) {
		close(fd);
		return -errno;
	}

	*write_amount = written_bytes;

	rc = fstat(fd, &storestat);
	if (rc < 0)
		return -errno;

	stat->st_mtime = storestat.st_mtime;
	stat->st_size = storestat.st_size;
	stat->st_blocks = storestat.st_blocks;
	stat->st_blksize = storestat.st_blksize;

	rc = close(fd);
	if (rc < 0)
		return -errno;

	*fsal_stable = true;
	return written_bytes;
}

int extstore_consolidate_attrs(kvsns_ino_t *ino, struct stat *filestat)
{
	struct stat extstat;
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = stat(storepath, &extstat);
	if (rc < 0) {
		printf("===> extstore_stat: errno=%u\n", errno);
		if (errno == ENOENT)
			return 0; /* No data written yet */
		else
			return rc;
	}

	filestat->st_mtime = extstat.st_mtime;
	filestat->st_atime = extstat.st_atime;
	filestat->st_size = extstat.st_size;
	filestat->st_blksize = extstat.st_blksize;
	filestat->st_blocks = extstat.st_blocks;

	printf("=======> extstore_stat: %s size=%lld\n",
		storepath, 
		(long long int)filestat->st_size);

	return 0;
}

int extstore_truncate(kvsns_ino_t *ino,
		      off_t filesize)
{
	int rc;
	char storepath[MAXPATHLEN];

	rc = build_extstore_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = truncate(storepath, filesize);
	if (rc == -1) {
		if (errno == ENOENT)
			return 0;
		else
			return -errno;
	}

	return 0;
}

