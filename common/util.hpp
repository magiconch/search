#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <boost/algorithm/string.hpp>
#include <sys/time.h>

namespace common {

  // 此处的 class 更多的是起到一个 命名空间 这样的角色
class StringUtil {
public:
  // aaa,bbb;cccc
  // split(is_any_of(",;")) => aaa bbb ccc
  // 
  // aaa,,,bbb
  // token_compress_off: 切分结果 4 个部分
  // token_compress_on: 切分结果 2 个部分
  static void Split(const std::string& input,
            std::vector<std::string>* output,
            const std::string& split_char) {
    boost::split(*output, input, 
                boost::is_any_of(split_char),
                boost::token_compress_off);// 压缩
  }
};

// 这是一个通用的词典类. 以 hash 表的方式来完成对各种需要的数据
// 进行存储和查找
class DictUtil {
public:
  // 从文件中加载数据到内存中
  bool Load(const std::string& file_path) {
    std::ifstream file(file_path.c_str());
    if (!file.is_open()) {
      return false;
    }
    std::string line;
    while (std::getline(file, line)) {
      set_.insert(line);
    }
    file.close();
    return true;
  }

  // 查找
  bool Find(const std::string& key) const {
    return set_.find(key) != set_.end();
  }
private:
  std::unordered_set<std::string> set_;
};

class FileUtil {
public:
  static bool Read(const std::string& input_path,
      std::string* data) {
    std::ifstream file(input_path.c_str());
    if (!file.is_open()) {
      return false;
    }
    // 整个文件的长度如何获取?
    file.seekg(0, file.end);
    int64_t length = file.tellg();
    file.seekg(0, file.beg);
    // 能够把指定长度的数据读到内存中
    data->resize(length);
    file.read(const_cast<char*>(data->data()), length);// 去掉const
    file.close();
    return true;
  }

  static bool Write(const std::string& output_path,
      const std::string& data) {
    std::ofstream file(output_path.c_str());
    if (!file.is_open()) {
      return false;
    }
    file.write(data.data(), data.size());
    file.close();
    return true;
  }
};

class TimeUtil {
public:
  // 秒级时间戳
  static int64_t TimeStamp() {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);   
    return tv.tv_sec;
  }

  // 毫秒级时间戳
  static int64_t TimeStampMS() {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);   
    return tv.tv_sec * 1e3 + tv.tv_usec / 1e3;
  }

  // 微秒级时间戳
  static int64_t TimeStampUS() {
    struct timeval tv;
    ::gettimeofday(&tv, NULL);   
    return tv.tv_sec * 1e6 + tv.tv_usec;
  }
};
}  // end common
