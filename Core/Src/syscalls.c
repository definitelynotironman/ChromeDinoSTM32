/* No console hardware is configured. Newlib I/O calls fail explicitly instead
 * of implying that UART output or keyboard input has been implemented. These
 * functions are normally discarded because gameplay uses no stdio calls. */
#include <errno.h>
#include <sys/types.h>

int _close(int descriptor)
{
    (void)descriptor;
    errno = ENOSYS;
    return -1;
}
int _read(int descriptor, char *buffer, int count)
{
    (void)descriptor;
    (void)buffer;
    (void)count;
    errno = ENOSYS;
    return -1;
}
int _write(int descriptor, char *buffer, int count)
{
    (void)descriptor;
    (void)buffer;
    (void)count;
    errno = ENOSYS;
    return -1;
}
off_t _lseek(int descriptor, off_t offset, int origin)
{
    (void)descriptor;
    (void)offset;
    (void)origin;
    errno = ENOSYS;
    return (off_t)-1;
}
