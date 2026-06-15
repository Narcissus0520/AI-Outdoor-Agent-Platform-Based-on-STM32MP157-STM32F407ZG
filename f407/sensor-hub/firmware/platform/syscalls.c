#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

extern uint8_t _end;
extern uint8_t _heap_end;

int _close(int file)
{
    (void)file;
    errno = EBADF;
    return -1;
}

int _fstat(int file, struct stat* status)
{
    (void)file;
    status->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

off_t _lseek(int file, off_t offset, int direction)
{
    (void)file;
    (void)offset;
    (void)direction;
    return 0;
}

ssize_t _read(int file, void* buffer, size_t length)
{
    (void)file;
    (void)buffer;
    (void)length;
    errno = ENOSYS;
    return -1;
}

ssize_t _write(int file, const void* buffer, size_t length)
{
    (void)file;
    (void)buffer;
    errno = ENOSYS;
    return length == 0U ? 0 : -1;
}

void* _sbrk(ptrdiff_t increment)
{
    static uint8_t* heap_end;

    if (heap_end == NULL) {
        heap_end = &_end;
    }

    uint8_t* previous_heap_end = heap_end;
    if (increment < 0 || heap_end + increment > &_heap_end) {
        errno = ENOMEM;
        return (void*)-1;
    }

    heap_end += increment;
    return previous_heap_end;
}

int _getpid(void)
{
    return 1;
}

int _kill(int process_id, int signal)
{
    (void)process_id;
    (void)signal;
    errno = EINVAL;
    return -1;
}

void _exit(int status)
{
    (void)status;
    while (1) {
    }
}
