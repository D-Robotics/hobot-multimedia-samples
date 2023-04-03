# get_sif_data 使用说明

## 构建步骤
1. 确保SDK目录下已经生成了`deploy*/appsdk`目录，若不存在这个目录，可以重新全量编译一次镜像来生成`appsdk`
2. 输入`make`，即可完成构建

## 参数说明
- `c`: sensor的配置文件所在位置
- `v`: vio配置文件所在位置
- `i`: 待执行的sensor在sensor配置文件的索引
## 配置文件说明
可以参考[VIO配置说明](https://developer.horizon.ai/api/v1/fileData/documents/mpp_develop/10-ISP_Tuning_Guide/Sensor_Bring_Up.html#vio)
## 使用说明
1. 准备好`json`配置文件和`sensor`**驱动**，可以参考`cfg`目录下面的`IMX219`配置文件进行准备，为了获取`RAW`数据，`SIF`到`ISP`这一部分必须**离线**。
2. 将交叉编译生成的`get_sif_data`和相关配置文件移动到开发板上
3. 在开发板上运行`get_sif_data`，以`IMX219`为例:
```bash
./get_sif_data -c cfg/hb_x3player.json -v cfg/sdb_imx219_raw_10bit_1920x1080_offline_Pipeline.json -i 0
```
4. 根据打印信息选择对应的操作