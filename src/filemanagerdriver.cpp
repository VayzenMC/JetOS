#include "filemanagerdriver.h"
#include "kernel/iso9660.h"

extern "C" int filemanager_read(const char *path, void *destination,
                                 unsigned int capacity)
{
    return iso9660_read_file(path, (unsigned char *)destination, capacity);
}

extern "C" int filemanager_list(const char *path, char *destination,
                                 unsigned int capacity)
{
    return iso9660_list_directory(path, (unsigned char *)destination, capacity);
}