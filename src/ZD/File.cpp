#include "File.hpp"

#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <stdio.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
namespace ZD
{
  File::File(std::string_view file_name, OpenMode mode, CreateFile create)
  : name { file_name }
  , mode { mode }
  {
    int oflag = 0;
    switch (mode)
    {
      case Read: oflag = O_RDONLY; break;
      case Write: oflag = O_WRONLY; break;
      case ReadWrite: oflag = O_RDWR; break;
    }
    if (create == CreateFile::Yes)
    {
      oflag |= O_CREAT;
      oflag |= O_TRUNC;
    }

    fd = open(file_name.data(), oflag, 0660);

    if (fd == -1)
    {
      perror("File open");
      fprintf(stderr, "Cannot open file '%s'!\n", file_name.data());
    }
    else
    {
      obtain_size();
    }
  }

  File::~File()
  {
    if (fd != -1)
    {
      close(fd);
    }

    if (file_watcher != nullptr)
    {
      file_watcher.reset();
    }
  }

  void File::rewind() { lseek(fd, 0, SEEK_SET); }

  size_t File::obtain_size() const
  {
    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
      perror("obtain_size fstat");
      fprintf(stderr, "Cannot fstat file '%s'!\n", name.data());
    }

    size = sb.st_size;
    return sb.st_size;
  }

  std::optional<std::string> File::read_line(int max_size)
  {
    assert(mode == Read || mode == ReadWrite);

    std::unique_ptr<char[]> buf(new char[max_size]);
    const char *beg = buf.get();
    size_t readed = 0;
    while ((readed = read(fd, buf.get(), max_size)) > 0)
    {
      char *p = (char*)memchr(buf.get(), '\n', readed);
      if (p)
      {
        size_t offset = p - buf.get();
        //printf("buf=%p p=%p offset=%lu\n", buf.get(), p, offset);
        lseek(fd, -(readed - offset) + 1, SEEK_CUR);
        return std::string(buf.get(), buf.get() + offset);
      }

      beg += readed;
    }

    return {};
  }

  std::vector<std::string> File::read_lines() const
  {
    assert(mode == Read || mode == ReadWrite);
    lseek(fd, 0, SEEK_SET);
    assert(lseek(fd, 0, SEEK_CUR) == 0);

    std::vector<std::string> strings;

    std::unique_ptr<char[]> buf(new char[FILE_BUF_SIZE]);
    memset(buf.get(), 0, FILE_BUF_SIZE);
    const char *beg = buf.get();
    size_t readed = 0;
    while ((readed = read(fd, buf.get(), FILE_BUF_SIZE)) > 0)
    {
      const char *prev_p = beg;
      for (const char*p = prev_p;
           (p = (char*)memchr(p, '\n', readed + (prev_p - p)));
           ++p)
      {
        if (p)
        {
          size_t offset = p - prev_p;
          //printf("prev_p=%p p=%p offset=%lu\n", prev_p, p, offset);
          lseek(fd, -(readed - offset), SEEK_CUR);
          strings.emplace_back(prev_p, prev_p + offset);
        }
        prev_p = p + 1;
      }

      beg += readed;
    }

    return strings;
  }

  std::vector<char> File::read_bytes(int max_size)
  {
    assert(mode == Read || mode == ReadWrite);
    char *buf = new char[max_size];
    size_t readed = read(fd, buf, max_size);

    std::vector<char> data(buf, buf + readed);
    delete[] buf;
    return data;
  }

  std::vector<char> File::read_all_bytes() const
  {
    assert(mode == Read || mode == ReadWrite);
    lseek(fd, 0, SEEK_SET);
    assert(lseek(fd, 0, SEEK_CUR) == 0);
    obtain_size();
    std::vector<char> data(this->size);

    auto *ptr = data.data();
    std::unique_ptr<char[]> buf(new char[FILE_BUF_SIZE]);
    size_t readed = 0;
    size_t saved = 0;
    while ((readed = read(fd, buf.get(), FILE_BUF_SIZE)) > 0)
    {
      std::memcpy(&ptr[0], buf.get(), readed);
      ptr += readed;
      saved += readed;
    }

    assert(saved == this->size);
    return data;
  }

  std::string File::read_all_chars() const
  {
    assert(mode == Read || mode == ReadWrite);
    lseek(fd, 0, SEEK_SET);
    assert(lseek(fd, 0, SEEK_CUR) == 0);
    obtain_size();
    std::string str;
    str.resize(this->size);

    auto *ptr = str.data();
    std::unique_ptr<char[]> buf(new char[FILE_BUF_SIZE]);
    size_t readed = 0;
    size_t saved = 0;
    while ((readed = read(fd, buf.get(), FILE_BUF_SIZE)) > 0)
    {
      std::memcpy(&ptr[0], buf.get(), readed);
      ptr += readed;
      saved += readed;
    }
    if (*ptr != '\0')
    {
      str.resize(this->size + 1);
      *ptr = '\0';
    }
    str.shrink_to_fit();

    assert(saved == this->size);
    return str;
  }

  ssize_t File::write(std::vector<char> data)
  {
    if (mode != Write && mode != ReadWrite)
    {
      return 0;
    }
    assert(mode == Write || mode == ReadWrite);

    ssize_t written = 0;
    written += ::write(fd, data.data(), data.size());
    obtain_size();
    return written;
  }

  ssize_t File::write(std::string_view str)
  {
    if (mode != Write && mode != ReadWrite)
    {
      return 0;
    }
    assert(mode == Write || mode == ReadWrite);
    ssize_t written = 0;
    written += ::write(fd, str.data(), str.size());
    obtain_size();
    return written;
  }

  void File::set_watch(FileCallback callback)
  {
    file_watcher = FileWatcher::add(*this, callback);
  }

  void File::remove_watch() { file_watcher.reset(); }
} // namespace ZD
