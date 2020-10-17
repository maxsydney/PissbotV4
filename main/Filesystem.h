#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

// Simple RAII class for reading and writing to an embedded spiffs filesystem

#include "esp_spiffs.h"

class Filesystem
{
    public:
        Filesystem(const char* base_path, const char* partition_label, 
                   size_t max_files, bool format_if_mount_failed);

        ~Filesystem(void);

    private:
        esp_vfs_spiffs_conf_t _conf {};
};

#endif // FILE_SYSTEM_H