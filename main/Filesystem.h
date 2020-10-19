#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

// Simple RAII class for reading and writing to an embedded spiffs filesystem

#include "esp_spiffs.h"
#include "PBCommon.h"

class Filesystem
{
    static constexpr const char* Name = "Filesystem";
    public:
        Filesystem(const char* base_path, const char* partition_label, 
                   size_t max_files, bool format_if_mount_failed);

        ~Filesystem(void);

        PBRet getInfo(size_t& total, size_t& used);
        bool isOpen(void) const { return _isOpen; }

    private:
        esp_vfs_spiffs_conf_t _conf {};
        bool _isOpen = false;
};

#endif // FILE_SYSTEM_H