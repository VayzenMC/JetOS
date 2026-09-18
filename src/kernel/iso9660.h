#ifndef JETOS_ISO9660_H
#define JETOS_ISO9660_H

int iso9660_read_file(const char *path, unsigned char *destination, unsigned int capacity);
int iso9660_list_directory(const char *path, unsigned char *destination, unsigned int capacity);

#endif