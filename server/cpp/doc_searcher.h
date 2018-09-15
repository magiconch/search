///////////////////////////////////////////////////////
// 这个文件实现了搜索业务相关的逻辑
///////////////////////////////////////////////////////
#pragma once

#include "server.pb.h"
#include "../../index/cpp/index.h"

namespace doc_server {

typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;
typedef doc_index::Index Index;
typedef doc_index_proto::Weight Weight;

// 上下文. 一次请求中涉及到的所有的数据的集合
struct Context {
  const Request* req;
  Response* resp;
  // 分词结果
  std::vector<std::string> words;
  // 触发结果
  std::vector<Weight> all_query_chains;
};

// 这个类包含了一次搜索过程中所涉及到的所有核心逻辑
class DocSearcher {
public:
  // 完成整个搜索功能
  bool Search(const Request& req, Response* resp);
private:
  // 输入查询词, 输出查询词的分词结果
  bool CutQuery(Context* context);
  // 输入分词结果, 输出若干个 Weight 构成的 vector
  // 合并成的一个比较长的 Weight 的 vector
  bool Retrieve(Context* context);
  // 输入触发出来的结果. 输出针对触发结果排序后的结果
  bool Rank(Context* context);
  // 输入触发结果排序后的结果, 输出是查正排后得到的 Response 结构
  bool PackageResponse(Context* context);
  // 记录日志
  bool Log(Context* context);

  // 以下函数为辅助用函数
  std::string GenDesc(int first_pos, const std::string& content);
  int FindSentenceBeg(int first_pos, const std::string& content);
  void ReplaceEscape(std::string* input);
};

}  // end doc_server
