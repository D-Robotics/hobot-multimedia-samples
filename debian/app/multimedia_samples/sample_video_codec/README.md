# sample_video_codec 使用说明

## 构建步骤
1. 确保SDK目录下已经生成了`deploy*/appsdk`目录，若不存在这个目录，可以重新全量编译一次镜像来生成`appsdk`
2. 输入`make`，即可完成构建

## 参数说明
- `d`: 是否开启双通道编/解码 1.开启 0.关闭
- `e`: 是否使用外部buffer 1.开启 0.关闭
- `--height1`: 待输入的图片/视频高（通道一）
- `--height2`: 待输入的图片/视频高（通道二）
- `--width1`: 待输入的图片/视频高宽（通道一）
- `--width2`: 待输入的图片/视频高宽（通道二）
- `--input1`：待输入的数据路径（通道一)
- `--input2`：待输入的数据路径（通道二)
- `--output1`：编码后的视频流的输出的路径（通道一）
- `--output2`：编码后的视频流的输出的路径（通道二）
- `--prefix1`：解码输出图片的前缀（通道一）
- `--prefix2`：解码输出图片的前缀（通道二）
- `-r`：编码的码率控制，目前支持0 H264CBR, 1 H264VBR, 2 H264AVBR, 3 H264FIXQP, 4 H264QPMAP, 5 H265CBR, 6 H265VBR, 7 H265AVBR, 8 H265FIXQP, 9 H265QPMAP,10 MJPEGFIXQP
- `-t`：编码/解码类型：目前支持 0.H264 1.H265 2.MJPEG
- `-m`: 选择功能 0.编码 1.解码


## 使用说明
1. 输入`make`即可编译出可执行文件`sample_vcodec`
2. 将交叉编译生成的`sample_vcodec`和相关配置文件移动到开发板上
3. 用以下几个场景举例说明`sample`的使用方法：
- 单通道编码**1080P**图片，生成**H264**码流
`./sample_vcodec -d 0 -e 0 --height1=1080 --width1=1920 --input1=1920x1080.yuv --output1=test1.264 -m 0 -r 0 -t 0`
- 双通道编码**1080P**和**2160P**图片，生成**H264**码流
`./sample_vcodec -d 1 -e 0 --height1=1080 --height2=2160 --width1=1920 --width2=3840 --input1=1920x1080.yuv --input2=3840_2160.yuv --output1=1080p.264 --output2=2160p.264 -m 0 -r 0 -t 0`
- 单通道编码**1440P**图片，生成**H265**码流
`./sample_vcodec -d 0 -e 0 --height1=1440 --width1=2560 --input1=2560x1440_s_2560_f_5791.yuv --output1=1440p.265 -m 0 -r 5 -t 1`
- 单通道编码**1440**图片，生成**MJPEG**码流
`./sample_vcodec -d 0 -e 0 --height1=1440 --width1=2560 --input1=2560x1440_s_2560_f_5791.yuv --output1=1440p.mjpeg -m 0 -t 2 -r 10`
- 单通道解码**2160P@H264**码流，生成**NV12**图片数据
`./sample_vcodec -d 1 --height1=2160 --width1=3840 --input1=2160p.264 --prefix1=decode_2160 -m 1 -t 0`
- 双通道解码**2160P@H264**和**1080P@H264**码流,生成**NV12**图片数据
`./sample_vcodec -d 1 --height1=2160 --width1=3840 --input1=2160p.264 --prefix1=decode_2160 --height2=1080 --width2=1920 --input2=1080p.264 --prefix2=decode_1080 -m 1 -t 0`
- 单通道解码**1440P@MJPEG**码流，生成**NV12**图片数据
`./sample_vcodec -d 0 -e 0 --height1=1440 --width1=2560 --input1=1440p.mjpeg --prefix1=mjpeg -m 1 -t 2`