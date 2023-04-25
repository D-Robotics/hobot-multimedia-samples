# vot_lcd_test 使用说明

## 编译

当前代码通过一个Makefile文件配置编译

依赖的多媒体头文件和库文件分别在BSP SDK的appsdk目录和system/rootfs_yocto目录下，编译时需要注意这两个依赖目录位置是否发现变化

安装交叉编译工具链后，执行 make 命令直接可以编译生成 vot_lcd_test 程序



## 程序使用说明

把 vot_lcd_test和 720x1280.yuv 拷贝到开发板

```bash
chmod +x vot_lcd_test
# ./vot_lcd_test
（注意lcd屏幕是sdb生态板子默认调过的一款720*1280分辨率屏幕，如果是使用者调试另外的屏幕，
 则需要修改对应分辨率和屏幕对应的时序参数）

