#pragma once 
#include <unordered_map>
#include <cppjieba/Jieba.hpp>
#include <base/base.h>
#include "index.pb.h"
#include "../../common/util.hpp"

// 构建一个命名空间
namespace doc_index {

  typedef doc_index_proto::DocInfo DocInfo;
  typedef doc_index_proto::Weight Weight;

  // 倒排拉链
  typedef std::vector<Weight> InvertedList;

  // 分词结果
  struct WordCnt {
    int title_cnt;
    int content_cnt;
    // 第一次出现位置，
    // 方便页面展示
    int first_pos;
    
    // 构造函数初始化列表
    WordCnt() : title_cnt(0), content_cnt(0), first_pos(-1)
    {}
  };
  typedef std::unordered_map<std::string, WordCnt> WordCntMap;

  class Index {
  public:
    static Index* Instance() {
      if (inst_ == NULL){
        inst_ = new Index();
      }
      return inst_;
    }

    Index();
    // 1. 构建索引
    bool Build(const std::string& input_path);
    // 2. 保存索引
    bool Save(const std::string& output_path);
    // 3. 加载索引
    bool Load(const std::string& index_path);
    // 4. 反解测试
    bool Dump(const std::string& forward_dump_path,
              const std::string& inverted_dump_path); 
    // 5. 查正排，从 doc_id 获取到文档详细内容
    const DocInfo* GetDocInfo(uint64_t doc_id) const;
    // 6. 查倒排，根据关键词，快速获取到倒排拉链
    const InvertedList* GetInvertedList(
                        const std::string& key) const;
    // 7. 对查询词分词，并删除暂停词
    bool CutWordWithoutStopWord(const std::string& query, std::vector<std::string>* word);
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

    // 辅助索引模块
    const DocInfo* BuildForward(const std::string& line);
    void BuildInverted(const DocInfo& doc_info);
    void SortInvertedList();
    void SplitTitle(const std::string& title, DocInfo* doc_info);
    void SplitContent(const std::string& content,
                      DocInfo* doc_info);
    int CalcWeight(const int title_cnt, const int content_cnt);
    bool ConvertToProto(std::string* proto_data);
    bool ConvertFromProto(const std::string& proto_data);
  }; // end class Index 

} // end doc_index
