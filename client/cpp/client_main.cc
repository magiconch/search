// 
// Created by magiconch on 2018.
// 
#include <base/base.h>
#include <sofa/pbrpc/pbrpc.h>
#include <ctemplate/template.h>
#include "../../common/util.hpp"
#include "server.pb.h"

DEFINE_string(server_addr, "127.0.0.1:10000", "服务器地址");
DEFINE_string(template_path,
              "./front/template/search_page.html",
              // "../template/search_page.html",
              "模板文件的路径");

namespace doc_client {

typedef doc_server_proto::Request Request;
typedef doc_server_proto::Response Response;

int GetQueryString(char output[]) {
  // 1. 先从环境变量中获取到方法
  char* method = getenv("REQUEST_METHOD");
  if (method == NULL) {
    fprintf(stderr, "REQUEST_METHOD failed\n");
    return -1;
  }
  // 2. 如果是 GET 方法, 就是直接从环境变量中
  //    获取到 QUERY_STRING
  if (strcasecmp(method, "GET") == 0) {
    char* query_string = getenv("QUERY_STRING");
    if (query_string == NULL) {
      fprintf(stderr, "QUERY_STRING failed\n");
      return -1;
    }
    strcpy(output, query_string);
  } else {
    // 3. 如果是 POST 方法, 先通过环境变量获取到 CONTENT_LENGTH
    //    再从标准输入中读取 body
    char* content_length_str = getenv("CONTENT_LENGTH");
    if (content_length_str == NULL) {
      fprintf(stderr, "CONTENT_LENGTH failed\n");
      return -1;
    }
    int content_length = atoi(content_length_str);
    int i = 0;  // 表示当前已经往  output 中写了多少个字符了
    for (; i < content_length; ++i) {
      read(0, &output[i], 1);
    }
    output[content_length] = '\0';
  }
  return 0;
}

void PackageRequest(Request* req) {
  req->set_timestamp(common::TimeUtil::TimeStamp());
  // 此处的查询词不能直接写死, 而是要通过 CGI 的方式从
  // HTTP 请求中解析得到
  // 请求形如: /client?query=filesystem
  char query_string[1024 * 10] = {0};
  GetQueryString(query_string);
  char query[1024 * 10] = {0};
  // sscanf 容错能力比较差.
  // 把汉字切错了
  // 更严谨的方式, 使用字符串切分的方法来解析.
  sscanf(query_string, "query=%s", query);
  // req->set_query("分享");
  req->set_query(query);
}

void ParseResponse(const Response& resp) {
  // CGI程序返回的信息呢
  // 1. 部分header(带有空行的)
  // 2. body
  // std::cout << resp.Utf8DebugString() << std::endl;
  // 需要构造出 HTML 的结果
  // 借助 ctemplate 库, 完成模板的渲染过程
	ctemplate::TemplateDictionary dict("search_page");

	for (int i = 0; i < resp.item_size(); ++i) {
    const auto& item = resp.item(i);

    // 构造字典的核心目的是为了知道模板中的变量应该替换成具体
    // 哪个值
		ctemplate::TemplateDictionary* item_dict;
    // 此处的 item 名字需要和模板文件中的 {{#item}} 对应的上
		item_dict = dict.AddSectionDictionary("item");
		item_dict->SetValue("jump_url", item.jump_url());
		item_dict->SetValue("title", item.title());
		item_dict->SetValue("desc", item.desc());
		item_dict->SetValue("show_url", item.show_url());
	}

	std::string output;
	ctemplate::Template* tpl;
	tpl = ctemplate::Template::GetTemplate(
        FLAGS_template_path, ctemplate::DO_NOT_STRIP);
	tpl->Expand(&output, &dict);

  std::cout << "\n"
            << output;
}

void Search(const Request& req, Response* resp) {
  using namespace sofa::pbrpc;
  // 1. RPC 概念1: 定义 RpcClient 对象
  RpcClient client;
  // 2. RPC 概念2: 定义 RpcChannel 对象. 描述了一个连接
  RpcChannel channel(&client, fLS::FLAGS_server_addr);
  // 3. RPC 概念3: 定义一个 DocServerAPI_Stub 对象
  //    描述了应该调用哪个远端函数
  doc_server_proto::DocServerAPI_Stub stub(&channel);
  RpcController ctrl;
  // ctrl.SetTimeout(2000);
  // NULL表示按照同步的方式来调用服务器.
  // 不为 NULL 表示异步的方式来调用服务器. 此时这个闭包就
  // 用来处理服务器的响应结构
  stub.Search(&ctrl, &req, resp, NULL);
  if (ctrl.Failed()) {
    std::cerr << "RPC failed" << std::endl;
  } else {
    std::cerr << "RPC OK" << std::endl;
  }
}

// 这个函数完成了整个调用服务器代码的逻辑
void CallServer() {
  // 1. 构造请求
  Request req;
  Response resp;
  PackageRequest(&req);
  // 2. 远程调用
  Search(req, &resp);
  // 3. 获取响应并打印结果
  ParseResponse(resp);
}
}  // end doc_client

int main(int argc, char* argv[]) {
  base::InitApp(argc, argv);
  doc_client::CallServer();
  return 0;
}
