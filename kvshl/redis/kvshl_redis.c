#include <errno.h>
#include "../kvshl.h"

int kvshl_init(void)
{
	return 0;
}

int kvshl_set_char(char *k, char *v)
{
	if (!k || !v)
		return -EINVAL;

	return 0;
}

int kvshl_get_char(char *k, char *v)
{
	if (!k || !v)
		return -EINVAL;

	return 0;
}
