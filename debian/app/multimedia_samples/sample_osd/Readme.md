# sample_osd 使用说明

## sample_osd

sample_osd用于给vps的YUV数据叠加osd。

## 编译

当前代码通过一个Makefile文件配置编译

依赖的多媒体头文件和库文件分别在BSP SDK的appsdk目录和system/rootfs_yocto目录下，编译时需要注意这两个依赖目录位置是否发现变化

安装交叉编译工具链后，注意是用的交叉编译工具链版本及路径，执行 make 命令直接可以编译生成 sample_osd 程序

## 程序使用说明

把 sample_osd 及 1280720.yuv 拷贝到开发板上之后，添加可执行权限后按照如下命令执行即可

```bash
chmod a+x sample_osd
./sample_osd