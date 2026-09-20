#ifndef JETOS_FILEMANAGERDRIVER_H
#define JETOS_FILEMANAGERDRIVER_H

/* Kernel-safe file manager API. It has no hosted C/C++ runtime dependency. */
#ifdef __cplusplus
extern "C" {
#endif

int filemanager_read(const char *path, void *destination, unsigned int capacity);
int filemanager_list(const char *path, char *destination, unsigned int capacity);

#ifdef __cplusplus
}
#endif

#endif