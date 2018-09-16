#include <fstream>
#include "index.h"

namespace doc_index {

DEFINE_string(dict_path,
    "/root/third_part/data/jieba_dict/jieba.dict.utf8",
    "词典路径");
DEFINE_string(hmm_path,
    "/root/third_part/data/jieba_dict/hmm_model.utf8",
    "hmm 词典路径");
DEFINE_string(user_dict_path,
    "/root/third_part/data/jieba_dict/user.dict.utf8",
    "用户词典路径");
DEFINE_string(idf_path,
    "/root/third_part/data/jieba_dict/idf.utf8",
    "idf 词典路径");
DEFINE_string(stop_word_path,
    "/root/third_part/data/jieba_dict/stop_words.utf8",
    "暂停词词典路径");

Index* Index::inst_ = NULL;

Index::Index() : jieba_(FLAGS_dict_path,
    FLAGS_hmm_path,
    FLAGS_user_dict_path,
    FLAGS_idf_path,
    FLAGS_stop_word_path) {
  CHECK(stop_word_dict_.Load(FLAGS_stop_word_path));
}
// 查正排
const DocInfo* Index::GetDocInfo(uint64_t doc_id) const {
  if (doc_id >= forward_index_.size()) {
    return NULL;
  }
  return &forward_index_[doc_id];
}

const InvertedList* Index::GetInvertedList(
    const std::string& key) const {
  // std::unordered_map<std::string,
  //    std::vector<Weight> >::iterator
  auto it = inverted_index_.find(key);
  if (it == inverted_index_.end()) {
    return NULL;
  }
  return &(it->second);
}

bool Index::Build(const std::string& input_path) {
  LOG(INFO) << "Index Build";
  // 1. 先按行读取文件
  std::ifstream file(input_path.c_str());
  //if (!file.is_open()) {
  //  return false;
  //}
  // 出错检验
  CHECK(file.is_open()) << "input_path " << input_path;
  std::string line;
  while (std::getline(file, line)) {
    // 2. 针对读出的一行, 构造出一个 DocInfo 结构,
    //    并更新正排索引
    // LOG(INFO) << line;
    const DocInfo* doc_info = BuildForward(line);
    CHECK(doc_info != NULL);
    // 3. 根据当前的 DocInfo 构造倒排索引中的键值对,
    //    并更新倒排索引
    //    BuildForward 更新了正排索引, 返回新构造好的
    //    doc_info 对象. 后面倒排使用
    //    用到这里的 doc_info
    //    直接放到Index::inverted_index_
    BuildInverted(*doc_info);
  }
  // 4. 对所有的倒排拉链进行排序
  SortInvertedList();
  file.close();
  LOG(INFO) << "Index Build Done";
  return true;
}

const DocInfo* Index::BuildForward(const std::string& line) {
  // 1. 先对 line 进行按照 \3 字符串切分
  std::vector<std::string> tokens;
  common::StringUtil::Split(line, &tokens, "\3");
  // LOG(INFO) << tokens[0] << tokens[1] << tokens[2]; 
  if (tokens.size() != 3){
    LOG(FATAL) << "Line Split is /%/d  tokens! ";//<< tokens.size();
    return NULL;
  }
  // 2. 构造一个 DocInfo 对象.
  DocInfo doc_info;
  doc_info.set_doc_id(forward_index_.size());
  doc_info.set_title(tokens[0]);
  doc_info.set_content(tokens[2]);
  doc_info.set_jump_url(tokens[1]);
  doc_info.set_show_url(doc_info.jump_url());
  // 3. 需要对标题和正文进行分词,
  //    分词结构是为了后续制作倒排奠定基础
  //    分词结构也需要保存到 DocInfo
  SplitTitle(tokens[0], &doc_info);
  SplitContent(tokens[2], &doc_info);
  // 4. 把 DocInfo 对象更新到正排索引中
  forward_index_.push_back(doc_info);
  return &forward_index_.back();
}

void Index::SplitTitle(const std::string& title,
              DocInfo* doc_info) {
  // 1. 调用 jieba 分词的接口进行分词
  std::vector<cppjieba::Word> words;
  jieba_.CutForSearch(title, words);
  // 2. 需要把 jieba 分词的分词结果转换成我们期望的存储的格式
  if (words.size() <= 1){
    //错误处理
    LOG(FATAL) << "SplitTitle Failed!";
    return;
  }
  for (size_t i = 0; i < words.size(); ++i) {
    auto* token = doc_info->add_title_token();
    // 当前分词结果的 offset 就是前闭后开区间中的前闭
    token->set_beg(words[i].offset);
    // 当前分词结果的下一个元素的 offset 就是前闭后开区间中的后开
    if (i + 1 < words.size()) {
      // i + 1 没越界
      token->set_end(words[i + 1]. offset);
    } else {
      token->set_end(title.size());
    }
  }
  return;
}

void Index::SplitContent(const std::string& content, 
    DocInfo* doc_info) {
  // 1. 调用 jieba 分词的接口进行分词
  std::vector<cppjieba::Word> words;
  jieba_.CutForSearch(content, words);
  if (words.size() <= 1){
    //如果size小于1，说明分词错误
    LOG(FATAL) << "SplitContent Failed!";
    return;
  }
  // 2. 需要把 jieba 分词的分词结果转换成我们期望的存储的格式
  for (size_t i = 0; i < words.size(); ++i) {
    auto* token = doc_info->add_content_token();
    // 当前分词结果的 offset 就是前闭后开区间中的前闭
    token->set_beg(words[i].offset);
    // 当前分词结果的下一个元素的 offset 就是前闭后开区间中的后开
    if (i + 1 < words.size()) {
      // i + 1 没越界
      token->set_end(words[i + 1]. offset);
    } else {
      token->set_end(content.size());
    }
  }
  return;
}

void Index::BuildInverted(const DocInfo& doc_info) {
  // 1. 先统计出分词结果中每个分词结果出现的次数
  WordCntMap word_cnt_map;
  // 统计标题
  for (int i = 0; i < doc_info.title_token_size(); ++i) {
    const auto& token = doc_info.title_token(i);
    std::string word = doc_info.title().substr(
          token.beg(), token.end() - token.beg()
        );
    // 1. 如果搜索英文的话，进行大小写弱处理
    boost::to_lower(word);
    //    把暂停词表加载到一个hash表中. 依次取分词结果, 判定
    //    是否是暂停词. 如果是就过滤掉, 不参与次数统计
    if (stop_word_dict_.Find(word)) {
      continue;
    }
    ++word_cnt_map[word].title_cnt;
  }
  // 统计正文
  for (int i = 0; i < doc_info.content_token_size(); ++i) {
    const auto& token = doc_info.content_token(i);
    std::string word = doc_info.content().substr(
          token.beg(), token.end() - token.beg()
        );
    boost::to_lower(word);
    if (stop_word_dict_.Find(word)) {
      continue;
    }
    ++word_cnt_map[word].content_cnt;
    if (word_cnt_map[word].content_cnt == 1) {
      word_cnt_map[word].first_pos = token.beg();
    }
  }
  // 2. 遍历分词结果, 取分词结果中去倒排索引中进行查找
  //    此时遍历只需要遍历刚才的 word_cnt_map 即可. 遍历
  //    这个结构, 能够直接的进行计算权重. 
  for (const auto& word_pair : word_cnt_map) {
    // 3. 如果当前分词结果在倒排索引中不存在, 就新建一个键值对.
    //    如果已经存在, 就取出之前的value(倒排拉链)
    InvertedList& inverted_list
            = inverted_index_[word_pair.first];
    // 4. 构造一个 Weight 对象, 把 Weight 更新到倒排拉链中
    Weight weight;
    weight.set_doc_id(doc_info.doc_id());
    weight.set_weight(CalcWeight(word_pair.second.title_cnt,
          word_pair.second.content_cnt));
    weight.set_first_pos(word_pair.second.first_pos);
    inverted_list.push_back(weight);
  }
  return;
}

int Index::CalcWeight(const int title_cnt, const int content_cnt) {
  return 10 * title_cnt + content_cnt;
}

void Index::SortInvertedList() {
  for (auto& inverted_pair : inverted_index_) {
    InvertedList& inverted_list = inverted_pair.second;
    std::sort(inverted_list.begin(), inverted_list.end(),
        CmpWeight);
  }
}

bool Index::CmpWeight(const Weight& w1, const Weight& w2) {
  return w1.weight() > w2.weight();
}

bool Index::Save(const std::string& output_path) {
  LOG(INFO) << "Index Save";
  // 1. 将内存中的索引进行序列化
  std::string proto_data;  // 存放索引序列化之后的结果
  CHECK(ConvertToProto(&proto_data));
  // 2. 把序列化的结构存到文件中
  CHECK(common::FileUtil::Write(output_path, proto_data));
  return true;
}

bool Index::Load(const std::string& index_path) {
  LOG(INFO) << "Index Load";
  // 1. 从文件中读到内存中, 并反序列化
  std::string proto_data;
  CHECK(common::FileUtil::Read(index_path, &proto_data));
  // 2. 把 protobuf 结构的数据整理成 内存中的索引数据
  CHECK(ConvertFromProto(proto_data));
  return true;
}

// 遍历内存中的索引结构, 根据内存索引结构构建出一个对应的
// proto_buf 中的索引结构就可以了
bool Index::ConvertToProto(std::string* proto_data) {
  doc_index_proto::Index index;
  // 1. 先将正排放到 protobuf 中
  for (const auto& doc_info : forward_index_) {
    // add 操作只是在 proto 中新增了一个元素. 
    // 元素的内容还没有进行设置
    auto* proto_doc_info = index.add_forward_index();
    *proto_doc_info = doc_info;
  }
  // 2. 再将倒排放到 protobuf 中
  for (const auto& inverted_pair : inverted_index_) {
    auto* kwd_info = index.add_inverted_index();
    kwd_info->set_key(inverted_pair.first);
    for (const auto& weight : inverted_pair.second) {
      auto* proto_weight = kwd_info->add_value();
      *proto_weight = weight;
    }
  }
  index.SerializeToString(proto_data);
  return true;
}

bool Index::ConvertFromProto(const std::string& proto_data) {
  // 对索引文件进行反序列化
  doc_index_proto::Index index;
  index.ParseFromString(proto_data);
  // 1. 先处理正排
  for (int i = 0; i < index.forward_index_size(); ++i) {
    const auto& doc_info = index.forward_index(i);
    forward_index_.push_back(doc_info);
  }
  // 2. 再处理倒排
  for (int i = 0; i < index.inverted_index_size(); ++i) {
    const auto& kwd_info = index.inverted_index(i);
    InvertedList& inverted_list
        = inverted_index_[kwd_info.key()];
    for (int j = 0; j < kwd_info.value_size(); ++j) {
      const auto& weight = kwd_info.value(j);
      inverted_list.push_back(weight);
    }
  }
  return true;
}

bool Index::Dump(const std::string& forward_dump_path,
    const std::string& inverted_dump_path) {
  LOG(INFO) << "Index Dump";
  // 1. 处理正排
  std::ofstream forward_dump_file(forward_dump_path.c_str());
  CHECK(forward_dump_file.is_open());
  for (const auto& doc_info : forward_index_) {
    forward_dump_file << doc_info.Utf8DebugString()
                      << "======================" << std::endl;
  }
  forward_dump_file.close();
  // 2. 处理倒排
  std::ofstream inverted_dump_file(inverted_dump_path.c_str());
  CHECK(inverted_dump_file.is_open());
  for (const auto& inverted_pair : inverted_index_) {
    inverted_dump_file << inverted_pair.first << std::endl;
    for (const auto& weight : inverted_pair.second) {
      inverted_dump_file << weight.Utf8DebugString();
    }
    inverted_dump_file << "=====================" << std::endl;
  }
  inverted_dump_file.close();
  LOG(INFO) << "Index Dump Done";
  return true;
}

bool Index::CutWordWithoutStopWord(const std::string& query,
    std::vector<std::string>* words) {
  words->clear();
  // 为了后面实现过滤暂停词的行为 
  std::vector<std::string> tmp;
  jieba_.CutForSearch(query, tmp);
  for (std::string word : tmp) {
    boost::to_lower(word);
    if (stop_word_dict_.Find(word)) {
      continue;
    }
    words->push_back(word);
  }
  return true;
}
}  // end doc_index
