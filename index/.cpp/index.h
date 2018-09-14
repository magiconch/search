#pragma once
#include <unordered_map>
#include <cppjieba/Jieba.hpp>
#include <base/base.h>
#include "index.pb.h"
#include "../../common/util.hpp"

// 索引模块的命名空间
namespace doc_index {

typedef doc_index_proto::DocInfo DocInfo;
typedef doc_index_proto::Weight Weight;
// 表示一个倒排拉链
typedef std::vector<Weight> InvertedList;

// 描述分词结果的统计结果
struct WordCnt {
  int title_cnt;
  int content_cnt;
  int first_pos;  // 该词在该文档正文中第一次出现的位置
                  // 该字段是为了方便我们后面来构造描述信息
  // 使用构造函数将他们初始化
  WordCnt() : title_cnt(0), content_cnt(0), first_pos(-1) {}
};

typedef std::unordered_map<std::string, WordCnt> WordCntMap;

// Index 类描述两个方面内容:
// 1. 索引在**内存**中的表现形式
// 2. 索引这个类应该对外提供出哪些 API
class Index {
public:
  static Index* Instance() {
    if (inst_ == NULL) {
      inst_ = new Index();
    }
    return inst_;
  }

  Index();

  // 1. 构建. 把raw_input文件进行分析, 在内存中构建索引结构
  //    为了实现索引构建程序
  bool Build(const std::string& input_path);
  // 2. 保存. 把内存中的结构基于 protobuf 进行序列化,
  //    保存到磁盘文件中
  //    为了实现索引构建程序
  bool Save(const std::string& output_path);
  // 3. 加载. 把索引文件中的内容读取出来,
  //    并在内存中还原出索引结构
  //    为了索引反解程序, 搜索服务器
  bool Load(const std::string& index_path);
  // 4. 反解. 为了实现索引反解工具, 方便测试.
  //    内存中的索引结构, 直接按照方便肉眼观察的格式输出到文件中
  //    只是为了搜索反解程序
  bool Dump(const std::string& forward_dump_path,
            const std::string& inverted_dump_path);
  // 5. 查正排. 根据 doc_id 快速的获取到 文档详细内容
  //    为了实现搜索服务器
  const DocInfo* GetDocInfo(uint64_t doc_id) const;
  // 6. 查倒排. 根据关键词, 快速获取到倒排拉链
  //    为了实现搜索服务器
  const InvertedList* GetInvertedList(
          const std::string& key) const;
  // 7. 对查询词分词, 同时干掉暂停词
  bool CutWordWithoutStopWord(const std::string& query,
        std::vector<std::string>* words);
  // 8. 设置权重
  static bool CmpWeight(const Weight& w1, const Weight& w2);
private:
  // 正排索引
  std::vector<DocInfo> forward_index_;
  // 倒排索引
  std::unordered_map<std::string, InvertedList> inverted_index_;
  cppjieba::Jieba jieba_;
  common::DictUtil stop_word_dict_;

  static Index* inst_;

  // 以下函数是实现索引模块中辅助的函数
  const DocInfo* BuildForward(const std::string& line);
  void BuildInverted(const DocInfo& doc_info);
  void SortInvertedList();
  void SplitTitle(const std::string& title, DocInfo* doc_info);
  void SplitContent(const std::string& content, 
          DocInfo* doc_info);
  int CalcWeight(int title_cnt, int content_cnt);
  bool ConvertToProto(std::string* proto_data);
  bool ConvertFromProto(const std::string& proto_data);
};
}  // end doc_index
