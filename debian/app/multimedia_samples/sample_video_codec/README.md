# video codec 使用说明

## 程序功能

`sample_vdec_basic` 实现最基础解码功能，读取本地H264/H265/MJPEG文件，进行解码保存NV12结果

`sample_venc_basic` 实现最基础编码功能，读取NV12图像，编码为H264（或H265或MJPEG），并保存为本地文件

`sample_vdec_two_channel` 面向需要多通道同时解码的场景，在`sample_vdec_basic` 基础上增加一路解码通道，实现双通道解码功能。读取本地H264/H265/MJPEG文件，两路同时进行解码分别保存NV12文件。

`sample_venc_two_channel` 面向需要多通道同时编码的场景，在`sample_venc_basic` 基础上增加一路编码通道，实现双通道编码功能。读取本地NV12文件，两路同时进行解码分别保存H264（或H265或MJPEG）。

### 程序部署

#### sample_vdec_basic

把编译生成的`sample_vdec_basic` 和需要解码的文件上传到开发板后，给程序赋予可执行权限 `chmod a+x sample_vdec_basic`, 然后执行程序 `./sample_vdec_basic -w width -h height -t ecode_type -f file`

其中width为图像宽所包含像素个数

height为为图像高所包含的像素格式

encode_tyep可以为h264\h265\mjpeg

file为要解码的文件名

#### sample_venc_basic

把编译生成的`sample_venc_basic` 和需要解码的文件上传到开发板后，给程序赋予可执行权限 `chmod a+x sample_venc_basic`, 然后执行程序 `./sample_venc_basic -w width -h height -t ecode_type -f file0 -g file1`

其中width为图像宽所包含像素个数

height为为图像高所包含的像素格式

encode_tyep可以为h264\h265\mjpeg

file0为要编码的文件名需要为NV12格式

file1为要编码的文件名需要为NV12格式，其width和height需要和file0保持一样

#### sample_vdec_two_channel

把编译生成的`sample_vdec_two_channel` 和需要解码的文件上传到开发板后，给程序赋予可执行权限 `chmod a+x sample_vdec_two_channel`, 然后执行程序 `./sample_vdec_two_channel -w width -h height -t ecode_type -f file`

其中width为图像宽所包含像素个数

height为为图像高所包含的像素格式

encode_tyep可以为h264\h265\mjpeg

file为要解码的文件名

#### sample_venc_two_channel

把编译生成的`sample_venc_two_channel` 和需要解码的文件上传到开发板后，给程序赋予可执行权限 `chmod a+x sample_venc_two_channel`, 然后执行程序 `./sample_venc_two_channel -w width -h height -t ecode_type -f file0 -g file1`

其中width为图像宽所包含像素个数

height为为图像高所包含的像素格式

encode_tyep可以为h264\h265\mjpeg

file0为要编码的文件名需要为NV12格式

file1为要编码的文件名需要为NV12格式，其width和height需要和file0保持一样

### 运行效果说明

#### sample_vdec_basic

在当前运行目录下生成decode.nv12，该文件内容随着解码内容更新

#### sample_venc_basic

在当前运行目录下生成sample_venc.h264/sample_venc.h265/sample_venc.jpg。H264/H265文件内容为交替显示是file1和file2
#### sample_vdec_two_channel

在当前运行目录下生成sample_decode_ch0.nv12和sample_decode_ch1.nv12，该文件内容随着解码内容更新。

#### sample_venc_two_channel

在当前运行目录下生成sample_venc_ch0.h264（sample_venc_ch0.h265/sample_venc_ch0.jpg）和sample_venc_ch1.h264（sample_venc_ch1.h265/sample_venc_ch1.jpg）。H264/H265文件内容为交替显示是file1和file2

## 编译

在sdk下执行make命令编译

