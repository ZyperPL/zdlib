#include <cassert>
#include <chrono>
#include <cstring>
#include <memory>
#include <thread>

#include "ZD/File.hpp"
#include "ZD/FileWatch.hpp"

int file_test_main(int, char **)
{
  {
    ZD::File f("test", ZD::File::Read);
    assert(!f.is_open());
    assert(f.get_size() == 0);
  }

  {
    ZD::File f("images/Crate1.mtl", ZD::File::Read);
    assert(f.is_open());
    if (f.is_open())
    {
      printf("File '%s' opened.\n", f.get_name().data());
    }
    assert(f.get_size() == 272);
    assert(f.get_name() == "images/Crate1.mtl");

    auto bytes = f.read_bytes(10);
    assert(strncmp((char *)bytes.data(), "# Blender ", 10) == 0);
    assert(bytes[0] == '#');
    assert(bytes[9] == ' ');

    auto all = f.read_all_bytes();
    assert(all.size() == 272);
    assert(all.size() == f.get_size());
    assert(strncmp((char *)all.data(), "# Ble", 5) == 0);
    assert(all[270] == 'g');
    assert(all[271] == '\n');

    auto all_chars = f.read_all_chars();
    printf("Read %lu chars.\n", all_chars.size());
    assert(all_chars.size() == 272);
  }

  {
    printf("Reading file... ");
    ZD::File f("images/Crate1.mtl", ZD::File::Read);
    assert(f.is_open() && f.get_size() <= 272);

    auto b = f.read_bytes(370);
    assert(b.size() == f.get_size());
    assert(b.size() == 272);
    assert(f.read_all_bytes().size() == 272);
    if (b.size() == 272 && f.read_all_bytes().size() == b.size())
    {
      printf("DONE\n");
    }
    else
    {
      printf("ERROR!\n");
      assert(false);
    }
  }

  {
    printf("Reading line by line... ");
    ZD::File f("images/Crate1.mtl", ZD::File::Read);
    assert(f.is_open() && f.get_size() > 0);
    auto line = f.read_line();
    assert(line != std::nullopt);
    f.rewind();
    auto l2 = f.read_line();
    assert(line == l2);
    //printf("line=%s\n", line->data());
    assert(strcmp((*line).c_str(), "# Blender MTL File: 'Crate1.blend'") == 0);

    f.rewind();
    int lines = 0;
    while (auto l = f.read_line())
    {
      lines++;
    }
    printf("Read %d lines.\n", lines);
    assert(lines == 12);

    if (line)
    {
      printf("DONE\n");
    }
    else
    {
      printf("ERROR!\n");
      assert(false);
    }
  }

  {
    printf("Reading all lines... ");
    ZD::File f("images/Crate1.mtl", ZD::File::Read);
    assert(f.is_open() && f.get_size() > 0);
    auto lines = f.read_lines();
    printf("Read %lu. lines.\n", lines.size());
    size_t lines_num = 0;
    for (const auto &l : lines)
    {
      lines_num++;
      //printf("line = '%s'\n", l.data());
      (void)(l);
    }
    assert(lines_num == 12);
    assert(lines.size() == lines_num);
    if (lines.size() == 12)
    {
      printf("DONE\n");
    }
    else
    {
      printf("ERROR!\n");
      assert(false);
    }
  }

  {
    ZD::File f("images/test.txt", ZD::File::Write, ZD::File::CreateFile::Yes);
    assert(f.is_open());
    std::vector<char> data = { 't', 'e', 's', 't', '\n' };
    ssize_t ret = f.write(data);
    if (ret <= 0) {
      printf("Write error!");
      assert(false);
    }
    assert(ret == 5);
    ret = f.write("Hello, World!");
    assert(ret == 13);

    ZD::File f2("images/test.txt", ZD::File::Read);
    assert(f2.is_open());
    auto line = f2.read_line();
    assert(line == "test");
  }

  {
    ZD::File f("images/not-existing.txt", ZD::File::Write, ZD::File::CreateFile::No);
    assert(!f.is_open());
  }

  if (ZD::FileWatcher::supported)
  {
    {
      ZD::File f0("images/Crate1.obj", ZD::File::Read);
      f0.set_watch([](const ZD::File &file, std::unordered_set<ZD::FileEvent> events) {
        printf(
          "File watcher 3 '%s' %zu events.\n",
          file.get_name().data(),
          events.size());
      });
    }
    ZD::File f1("images/Crate1.mtl", ZD::File::Read);
    f1.set_watch([](const ZD::File &file, std::unordered_set<ZD::FileEvent> events) {
      printf(
        "ZD::File watch 1 '%s' %zu events.\n",
        file.get_name().data(),
        events.size());
    });
    ZD::File f2("images/Crate1.mtl", ZD::File::Read);
    f2.set_watch([](const ZD::File &file, std::unordered_set<ZD::FileEvent> events) {
      printf(
        "ZD::File watcher 2 '%s' %zu events.\n",
        file.get_name().data(),
        events.size());
    });
    ZD::File f3("images/Crate1.obj", ZD::File::Read);
    f3.set_watch([](const ZD::File &file, std::unordered_set<ZD::FileEvent> events) {
      printf(
        "ZD::File watcher 3 '%s' %zu events.\n",
        file.get_name().data(),
        events.size());
    });

    int sec = 1;
    printf("Sleeping for %d second(s)...\n", sec);
    std::this_thread::sleep_for(std::chrono::seconds(sec));
  }
  else
  {
    fprintf(stderr, "FileWatcher not supported\n");
  }
  return 0;
}
