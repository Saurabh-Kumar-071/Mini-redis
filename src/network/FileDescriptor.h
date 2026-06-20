#pragma once

#include <unistd.h>

class FileDescriptor{
private:
    int fd;

public:
    explicit FileDescriptor(int fd = -1);

    ~FileDescriptor();

    FileDescriptor(const FileDescriptor&) = delete;

    FileDescriptor& operator=(const FileDescriptor&) = delete;

    FileDescriptor(FileDescriptor&& other) noexcept;

    FileDescriptor& operator=(FileDescriptor&& other) noexcept;

    int get() const;
};
