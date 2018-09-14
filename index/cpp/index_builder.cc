#include <base/base.h>
#include "index.h"

DEFINE_string(input_path, "../data/input/nmbdata",
              "索引输入文件");
DEFINE_string(output_path, "../data/output/index_file",
              "索引输出文件");

int main(int argc, char* argv[]) {
  base::InitApp(argc, argv);
  doc_index::Index* index = doc_index::Index::Instance();
  CHECK(index->Build(fLS::FLAGS_input_path));
  CHECK(index->Save(fLS::FLAGS_output_path));
  return 0;
}
