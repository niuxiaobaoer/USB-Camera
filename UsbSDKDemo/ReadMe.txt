本sdk开发包可以直接运行bin中的文件进行测试运行，在输入框中输入相机的分辨率，默认为1280×1024，点击VideoCapture即开始采集。SnapShot保存截图，在目录中的snap.jpg.
另外在软件运行时还会保存录像，为h.264编码。

软件采用vs2010编写，使用了OpenCV库中的显示、保存图片、保存视频文件的功能。

API函数在CCTAPI.h中。

CCT_API int startCap(int height,int width,LPMV_CALLBACK2 CallBackFunc);
开始采集。其中：
int height：采集图像的高度
int width:采集图像的宽度
LPMV_CALLBACK2 CallBackFunc：回调函数（具体使用方法在代码中）

CCT_API int stopCap();
停止图像采集。

CCT_API int setMirrorType(DataProcessType mirrortype);
设置图像的镜像方式。其中
DataProcessType为自定义枚举类型
enum DataProcessType
{
	Normal_Proc,Xmirror_Proc,Ymirror_Proc,XYmirror_Proc
};