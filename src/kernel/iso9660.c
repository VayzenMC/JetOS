#include "iso9660.h"

#define ISO_SECTOR_SIZE 2048
#define ATA_PRIMARY 0x1F0
#define ATA_SECONDARY 0x170
#define ATA_PRIMARY_CONTROL 0x3F6
#define ATA_SECONDARY_CONTROL 0x376

static unsigned char sector[ISO_SECTOR_SIZE];

static unsigned char inb(unsigned short port)
{
    unsigned char value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static void outb(unsigned short port, unsigned char value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void outw(unsigned short port, unsigned short value)
{
    __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static unsigned short u16le(const unsigned char *p)
{
    return (unsigned short)p[0] | ((unsigned short)p[1] << 8);
}

static unsigned int u32le(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8)
        | ((unsigned int)p[2] << 16) | ((unsigned int)p[3] << 24);
}

static void ata_delay(unsigned short control)
{
    inb(control);
    inb(control);
    inb(control);
    inb(control);
}

static int wait_status(unsigned short control, unsigned char mask, unsigned char value)
{
    unsigned int timeout = 1000000;
    unsigned char status;

    do {
        status = inb(control);
        if ((status & mask) == value) return 0;
        --timeout;
    } while (timeout != 0);
    return -1;
}

static int atapi_read_on_channel(unsigned short base, unsigned short control,
                                 unsigned int lba, unsigned char *destination)
{
    unsigned char packet[12];
    unsigned int i;

    outb(base + 6, 0xA0);
    ata_delay(control);
    if (wait_status(control, 0x80, 0) < 0) return -1;

    outb(base + 1, 0);
    outb(base + 2, 0);
    outb(base + 4, ISO_SECTOR_SIZE & 0xFF);
    outb(base + 5, ISO_SECTOR_SIZE >> 8);
    outb(base + 7, 0xA0);
    if (wait_status(control, 0x80, 0) < 0) return -1;
    if (inb(base + 7) & 0x01) return -1;

    packet[0] = 0xA8;
    packet[1] = 0;
    packet[2] = (unsigned char)(lba >> 24);
    packet[3] = (unsigned char)(lba >> 16);
    packet[4] = (unsigned char)(lba >> 8);
    packet[5] = (unsigned char)lba;
    packet[6] = 0;
    packet[7] = 0;
    packet[8] = 0;
    packet[9] = 1;
    packet[10] = 0;
    packet[11] = 0;
    for (i = 0; i < 12; i += 2)
        outw(base, (unsigned short)packet[i] | ((unsigned short)packet[i + 1] << 8));

    if (wait_status(control, 0x88, 0x08) < 0) return -1;
    for (i = 0; i < ISO_SECTOR_SIZE / 2; ++i) {
        unsigned short word;
        __asm__ volatile("inw %1, %0" : "=a"(word) : "Nd"(base));
        destination[i * 2] = (unsigned char)word;
        destination[i * 2 + 1] = (unsigned char)(word >> 8);
    }
    return (inb(base + 7) & 0x01) ? -1 : 0;
}

static int atapi_read(unsigned int lba, unsigned char *destination)
{
    if (atapi_read_on_channel(ATA_SECONDARY, ATA_SECONDARY_CONTROL, lba, destination) == 0)
        return 0;
    return atapi_read_on_channel(ATA_PRIMARY, ATA_PRIMARY_CONTROL, lba, destination);
}

static int iso_name_equal(const unsigned char *entry, unsigned int length, const char *name)
{
    unsigned int i = 0;
    unsigned int name_length = 0;
    char actual;
    char expected;

    while (name[name_length] != '\0') ++name_length;
    if (length > 2 && entry[length - 2] == ';' && entry[length - 1] == '1')
        length -= 2;
    if (length != name_length) return 0;
    while (i < length) {
        actual = (char)entry[i];
        expected = name[i];
        if (actual >= 'a' && actual <= 'z') actual -= 'a' - 'A';
        if (expected >= 'a' && expected <= 'z') expected -= 'a' - 'A';
        if (actual != expected) return 0;
        ++i;
    }
    return 1;
}

static int iso_find_in_directory(unsigned int extent, unsigned int size,
                                 const char *name, unsigned int *found_extent,
                                 unsigned int *found_size, unsigned char *found_flags)
{
    unsigned int offset;
    unsigned int lba;

    for (offset = 0; offset < size; offset += ISO_SECTOR_SIZE) {
        if (atapi_read(extent + offset / ISO_SECTOR_SIZE, sector) < 0) return -1;
        for (lba = 0; lba < ISO_SECTOR_SIZE;) {
            unsigned char length = sector[lba];
            unsigned char name_length;
            if (length == 0) break;
            if (length < 34 || lba + length > ISO_SECTOR_SIZE) return -1;
            name_length = sector[lba + 32];
            if (iso_name_equal(sector + lba + 33, name_length, name)) {
                *found_extent = u32le(sector + lba + 2);
                *found_size = u32le(sector + lba + 10);
                *found_flags = sector[lba + 25];
                return 0;
            }
            lba += length;
        }
    }
    return -1;
}

static int iso_find_path(const char *path, unsigned int *extent,
                         unsigned int *size, unsigned char *flags)
{
    unsigned int root_extent;
    unsigned int root_size;
    unsigned char root_flags;
    unsigned int name_start = 0;
    unsigned int name_end;
    char name[32];
    unsigned int length;

    if (atapi_read(16, sector) < 0 || sector[0] != 1 || sector[1] != 'C'
        || sector[2] != 'D' || sector[3] != '0' || sector[4] != '0'
        || sector[5] != '1') return -1;
    root_extent = u32le(sector + 156 + 2);
    root_size = u32le(sector + 156 + 10);
    root_flags = sector[156 + 25];
    if (path[0] == '/') name_start = 1;

    while (path[name_start] != '\0') {
        name_end = name_start;
        while (path[name_end] != '\0' && path[name_end] != '/') ++name_end;
        length = name_end - name_start;
        if (length == 0 || length >= sizeof(name)) return -1;
        {
            unsigned int i;
            for (i = 0; i < length; ++i) name[i] = path[name_start + i];
            name[length] = '\0';
        }
        if (iso_find_in_directory(root_extent, root_size, name, &root_extent,
                                  &root_size, &root_flags) < 0) return -1;
        name_start = path[name_end] == '/' ? name_end + 1 : name_end;
    }
    *extent = root_extent;
    *size = root_size;
    *flags = root_flags;
    return 0;
}

int iso9660_read_file(const char *path, unsigned char *destination, unsigned int capacity)
{
    unsigned int extent;
    unsigned int size;
    unsigned char flags;
    unsigned int copied = 0;
    unsigned int count;

    if (iso_find_path(path, &extent, &size, &flags) < 0 || (flags & 2) || size > capacity)
        return -1;
    while (copied < size) {
        if (atapi_read(extent + copied / ISO_SECTOR_SIZE, sector) < 0) return -1;
        count = size - copied;
        if (count > ISO_SECTOR_SIZE) count = ISO_SECTOR_SIZE;
        {
            unsigned int i;
            for (i = 0; i < count; ++i) destination[copied + i] = sector[i];
        }
        copied += count;
    }
    return (int)copied;
}

int iso9660_list_directory(const char *path, unsigned char *destination, unsigned int capacity)
{
    unsigned int extent;
    unsigned int size;
    unsigned char flags;
    unsigned int offset;
    unsigned int output = 0;
    unsigned int i;

    if (iso_find_path(path, &extent, &size, &flags) < 0 || !(flags & 2)) return -1;
    destination[0] = '\0';
    for (offset = 0; offset < size; offset += ISO_SECTOR_SIZE) {
        if (atapi_read(extent + offset / ISO_SECTOR_SIZE, sector) < 0) return -1;
        for (i = 0; i < ISO_SECTOR_SIZE;) {
            unsigned char length = sector[i];
            unsigned char name_length;
            if (length == 0) break;
            if (length < 34 || i + length > ISO_SECTOR_SIZE) return -1;
            name_length = sector[i + 32];
            if (name_length > 1 || sector[i + 33] != 0) {
                unsigned int name_end = name_length;
                if (name_end > 2 && sector[i + 32 + name_end - 2] == ';'
                    && sector[i + 32 + name_end - 1] == '1') name_end -= 2;
                for (unsigned int n = 0; n < name_end; ++n) {
                    if (output + 1 >= capacity) return -1;
                    destination[output++] = sector[i + 33 + n];
                }
                if (output + 1 >= capacity) return -1;
                destination[output++] = '\n';
                destination[output] = '\0';
            }
            i += length;
        }
    }
    return (int)output;
}