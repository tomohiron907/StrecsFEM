cmake_minimum_required(VERSION 3.12)
project(FrdToVtuConverter)

# find_packageをコンポーネント指定方式に変更
# これにより、必要なライブラリのみがリンクされ、Pythonへの依存を回避できる
find_package(VTK REQUIRED
  COMPONENTS
    CommonCore
    CommonDataModel
    IOXML
)
include(${VTK_USE_FILE})

add_executable(frd2vtu frd2vtu.cpp)
target_link_libraries(frd2vtu PRIVATE ${VTK_LIBRARIES})