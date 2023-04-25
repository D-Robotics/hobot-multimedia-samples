# vps_zoom 使用说明

## vps_zoom基本功能

vps_zoom程序使用ipu和pym对yuv图像中的部分区域做多倍放大处理，对处理后的yuv图像编码成h264视频流，可以直接使用MPC-BE等工具进行预览，类似电子云台中的zoom功能

## 编译

当前代码通过一个Makefile文件配置编译

依赖的多媒体头文件和库文件分别在BSP SDK的appsdk目录和system/rootfs_yocto目录下，编译时需要注意这两个依赖目录位置是否发现变化

安装交叉编译工具链后，注意是用的交叉编译工具链版本及路径，执行 make 命令直接可以编译生成 vps_zoom 程序

## 程序使用说明

把 vps_zoom 及 19201080.yuv 拷贝到开发板上之后，添加可执行权限后执行即可

```bash
chmod a+x vps_zoom