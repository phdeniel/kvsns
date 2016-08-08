#include "extstore.h"

static char store_root[MAXPATHLEN];

static int build_external_path(kvsns_ino_t object,
			       char *external_path,
			       size_t pathlen)
{
	if (!external_path)
		return -1;

	return snprintf(external_path, pathlen, "%s/inum=%llu",
			(unsigned long long)object);
}

int external_init(char *rootpath)
{
	strncpy(store_root, rootpath, MAXPATHLEN);
	return 0;
}

int external_read(kvsns_ino_t *ino, 
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


	rc = build_external_path(*ino, storepath, MAXPATHLEN);
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

int external_write(kvsns_ino_t *ino,
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


	rc = build_external_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	printf("WRITE: I got external path=%s\n", storepath);

	fd = open(storepath, O_CREAT|O_WRONLY|O_SYNC);
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

int external_consolidate_attrs(kvsns_ino_t *ino, struct stat *filestat)
{
	struct stat extstat;
	char storepath[MAXPATHLEN];
	int rc;

	rc = build_external_path(*ino, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	rc = stat(storepath, &extstat);
	if (rc < 0) {
		printf("===> external_stat: errno=%u\n", errno);
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

	printf("=======> external_stat: %s size=%lld\n",
		storepath, 
		(long long int)filestat->st_size);

	return 0;
}

#if 0
int external_unlink(struct fsal_obj_handle *dir_hdl,
		    const char *name)
{
	int rc;
	char storepath[MAXPATHLEN];
	creden_t cred;
	kvsns_ino_t object;
	int type = 0;
	struct stat stat;

	cred.uid = op_ctx->creds->caller_uid;
	cred.gid = op_ctx->creds->caller_gid;

	rc = libzfswrap_lookup(clovis_get_root_pvfs(op_ctx->fsal_export),
			       &cred,
			       myself->handle->clovis_handle,
			       name, &object, &type);
	if (rc)
		return errno;

	if (type == S_IFDIR)
		return 0;

	rc = libzfswrap_getattr(clovis_get_root_pvfs(op_ctx->fsal_export),
				&cred,
				object,
				&stat,
				&type);
	if (rc)
		return errno;

	if (stat.st_nlink > 1)
		return 0;

	rc = build_external_path(object, storepath, MAXPATHLEN);
	if (rc < 0)
		return rc;

	/* file may not exist */
	if (unlink(storepath) < 0) {
		/* Store obj may not exist if file was
		 * never written */
		if (errno == ENOENT)
			return 0;
		else
			return errno;
	}

	/* Should not be reach, but needed for compiler's happiness */
	return 0;
}
#endif

int external_truncate(kvsns_ino_t *ino,
		      off_t filesize)
{
	int rc;
	char storepath[MAXPATHLEN];

	rc = build_external_path(*ino, storepath, MAXPATHLEN);
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

