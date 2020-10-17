#include "Filesystem.h"

Filesystem::Filesystem(const char* base_path, const char* partition_label, 
                       size_t max_files, bool format_if_mount_failed))
{
    _conf = {
        base_path,
        partition_label,
        max_files,
        format_if_mount_failed
    };

    
}