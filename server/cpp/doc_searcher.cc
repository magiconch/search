#include "doc_searcher.h"

DEFINE_int32(desc_max_size, 160, "描述的最大长度");

namespace doc_server {

bool DocSearcher::Search(const Request& req, Response* resp) {
  // 1. 定义上下文对象
  Context context;
  context.req = &req;
  context.resp = resp;
  // 2. 进行分词
  CutQuery(&context);
  // 3. 进行触发
  Retrieve(&context);
  // 4. 进行排序
  Rank(&context);
  // 5. 构造响应
  PackageResponse(&context);
  // 6. 记录日志
  Log(&context);
  return true;
}

bool DocSearcher::CutQuery(Context* context) {
  // 对查询词进行分词, 分词结果直接用字面值表示
  // 直接将分词结果中的暂停词就干掉了
  Index* index = Index::Instance();
  index->CutWordWithoutStopWord(context->req->query(),
      &context->words);
  return true;
}

bool DocSearcher::Retrieve(Context* context) {
  // 遍历分词结果, 一次查索引中的倒排. 获取到所有的倒排拉链
  // 再把这些倒排拉链合并到一起
  Index* index = Index::Instance();
  for (const auto& word : context->words) {
    const auto* inverted_list = index->GetInvertedList(word);
    // 此处的倒排拉链可能是 NULL
    if (inverted_list == NULL) {
      continue;
    }
    for (const auto& weight : *inverted_list) {
      context->all_query_chains.push_back(weight);
    }
  }
  return true;
}

bool DocSearcher::Rank(Context* context) {
  // 虽然之前在制作索引的阶段已经对所有的倒排拉链都排过序了. 
  // all_query_chains 其实包含了多个关键词的拉链
  // 仍然需要再重新排序. 排序规则和之前一样, 还是看权重
  std::sort(context->all_query_chains.begin(),
      context->all_query_chains.end(),
      Index::CmpWeight);
  return true;
}

bool DocSearcher::PackageResponse(Context* context) {
  // 构造响应结果本质就是查正排
  Index* index = Index::Instance();
  const Request* req = context->req;
  Response* resp = context->resp;
  resp->set_timestamp(common::TimeUtil::TimeStamp());
  for (const auto& weight : context->all_query_chains) {
    const auto* doc_info = index->GetDocInfo(weight.doc_id());
    auto* item = resp->add_item();
    item->set_title(doc_info->title());
    // TODO 后面仔细考虑如何生成描述
    item->set_desc(GenDesc(weight.first_pos(),
                    doc_info->content()));
    item->set_jump_url(doc_info->jump_url());
    item->set_show_url(doc_info->show_url());
  }
  return true;
}

std::string DocSearcher::GenDesc(int first_pos,
    const std::string& content) {
  // 1. 从 first_pos 位置往前找, 找到这个句子的开始位置
  int desc_beg = 0;
  // first_pos 这个词在正文中第一次出现的位置.
  // 如果没出现过, first_pos 就无效
  if (first_pos != -1) {
    desc_beg = FindSentenceBeg(first_pos, content);
  }
  std::string desc;  // 最终的描述结果
  if (desc_beg + fLI::FLAGS_desc_max_size
      >= (int32_t)content.size()) {
    // 2. 截取固定长度的字符串.
    desc = content.substr(desc_beg);
  } else {
    // 3. 截取之后如果文档还没结束, 就把最后的三个字符改成 '.'
    desc = content.substr(desc_beg, fLI::FLAGS_desc_max_size);
    /*
    desc[desc.size() - 1] = '.';
    desc[desc.size() - 2] = '.';
    desc[desc.size() - 3] = '.';
  */
  }
  // 需要在生成描述的时候, 进行转义字符的替换.
  // 把有些 html 中非法的字符替换为转义字符
  ReplaceEscape(&desc);
  return desc;
}

void DocSearcher::ReplaceEscape(std::string* input) {
  // 假设 文本中包含了一个 < 符号
  // 第二行代码把 < -> &lt;
  // 第四行代码把 &lt; -> &amp;lt;
  // 所以取地址符号, 必须要放到第一个位置进行替换
  boost::algorithm::replace_all(*input, "&", "&amp;");
  boost::algorithm::replace_all(*input, "\"", "&quot;");
  boost::algorithm::replace_all(*input, "<", "&lt;");
  boost::algorithm::replace_all(*input, ">", "&gt;");
}

// 此处的判定还是比较简单粗暴的方式
// 实际使用过程中会发现很多不合理的细节. 
// 不合理的细节老铁们根据实际情况自行调整里面的判定逻辑
int DocSearcher::FindSentenceBeg(int first_pos,
    const std::string& content) {
  for (int i = first_pos; i >= 0; --i) {
    if (content[i] == ','
        || content[i] == '?'
        || content[i] == '!'
        || content[i] == ';'
        || (content[i] == '.' && content[i + 1] == ' ')) {
      return i + 1; 
    }
  }
  return 0;
}

bool DocSearcher::Log(Context* context) {
  LOG(INFO) << "[Request]" << context->req->Utf8DebugString();
  LOG(INFO) << "[Response]" << context->resp->Utf8DebugString();
  return true;
}
}  // end doc_server
