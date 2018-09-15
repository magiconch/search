///////////////////////////////////////////////////////
// 搭建服务器框架
// 和业务不相关
///////////////////////////////////////////////////////

#include <base/base.h>
#include <sofa/pbrpc/pbrpc.h>
#include "server.pb.h"
#include "doc_searcher.h"

DEFINE_string(port, "10000", "端口号");
DEFINE_string(index_path, "../output/index_file",
              "索引文件的路径");

namespace doc_server {

class DocServerAPIImpl : public doc_server_proto::DocServerAPI {
public:
  // RpcController 对象包含了网络通信的细节. 
  // 例如客户端的ip/port.... 
  // Closure 对象 -> 闭包
  // 回调函数的更进一步
  void Search(::google::protobuf::RpcController* controller,
           const ::doc_server_proto::Request* request,
           ::doc_server_proto::Response* response,
           ::google::protobuf::Closure* done) {
    (void) controller;

    DocSearcher searcher;
    searcher.Search(*request, response);

    // 这一步是通知框架, 该次请求处理结束了
    done->Run();
  }
};
}  // end doc_server

int main(int argc, char* argv[]) {
  using namespace sofa::pbrpc;
  base::InitApp(argc, argv);
  // 0. 需要先加载索引
  doc_index::Index* index = doc_index::Index::Instance();
  CHECK(index->Load(fLS::FLAGS_index_path));
  std::cout << "Index Load Done!" << std::endl;
  
  // 1. 定义 RpcServerOptions 对象
  // 这个对象描述了RPC服务器一些相关选项
  // 包含了 RpcServer 相关选项和配置
  RpcServerOptions option;
  option.work_thread_num = 4; // 线程池中线程的个数
  // 2. 定义 RpcServer 对象
  RpcServer server(option);
  // 3. 启动服务器. 相当于 创建socket + bind + listen
  CHECK(server.Start("0.0.0.0:" + fLS::FLAGS_port));
  // 4. 指定服务器处理请求的方式, 定义一个 DocServerAPIImpl 对象
  //    并且注册到框架中
  doc_server::DocServerAPIImpl* server_impl
    = new doc_server::DocServerAPIImpl();
  server.RegisterService(server_impl);
  // 5. 进入事件循环, 相当于循环进行 accept
  server.Run();
  return 0;
}

