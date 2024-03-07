/*********************************************************************************************************************
* TC264 Opensourec Library 即（TC264 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 TC264 开源库的一部分
*
* TC264 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 本开源库的发布是希望它能发挥作用，但并未对其作任何的保证
* 甚至没有隐含的适销性或适合特定用途的保证
* 更多细节请参见 GPL
*
* 您应该在收到本开源库的同时收到一份 GPL 的副本
* 如果没有，请参阅<https://www.gnu.org/licenses/>
*
* 额外注明：
* 本开源库使用 GPL3.0 开源许可证协议 以上许可申明为译文版本
* 许可申明英文版在 libraries/doc 文件夹下的 GPL3_permission_statement.txt 文件中
* 许可证副本在 libraries 文件夹下 即该文件夹下的 LICENSE 文件
* 欢迎各位使用并传播本程序 但修改内容时必须保留逐飞科技的版权声明（即本声明）
*
* 文件名称          zf_device_camera
* 公司名称          成都逐飞科技有限公司
* 版本信息          查看 libraries/doc 文件夹内 version 文件 版本说明
* 开发环境          ADS v1.9.20
* 适用平台          TC264D
* 店铺链接          https://seekfree.taobao.com/
*
* 修改记录
* 日期              作者                备注
* 2022-09-15       pudding           first version
* 2023-04-25       pudding           增加中文注释说明
********************************************************************************************************************/

#include "zf_common_debug.h"
#include "zf_common_interrupt.h"
#include "zf_driver_gpio.h"
#include "zf_driver_dma.h"
#include "zf_driver_exti.h"
#include "zf_device_mt9v03x.h"
#include "zf_device_ov7725.h"
#include "zf_device_scc8660.h"
#include "zf_device_camera.h"


fifo_struct camera_receiver_fifo;                                           // 定义摄像头接收数据fifo结构体
uint8 camera_receiver_buffer[CAMERA_RECEIVER_BUFFER_SIZE];                  // 定义摄像头接收数据缓冲区
uint8 camera_send_image_frame_header[4] = {0x00, 0xFF, 0x01, 0x01};         // 定义摄像头数据发送到上位机的帧头
#define ROAD_MAIN_ROW 44

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       摄像头二进制图像数据解压为十六进制八位数据 小钻风用
// 参数说明       *data1          摄像头图像数组
// 参数说明       *data2          存放解压数据的地址
// 参数说明       image_size      图像的大小
// @return      void
// Sample usage:   camera_binary_image_decompression(&ov7725_image_binary[0][0], &data_buffer[0][0], OV7725_SIZE);
//-------------------------------------------------------------------------------------------------------------------
void camera_binary_image_decompression (const uint8 *data1, uint8 *data2, uint32 image_size)
{
    zf_assert(NULL != data1);
    zf_assert(NULL != data2);
    uint8  i = 8;

    while(image_size --)
    {
        i = 8;
        while(i --)
        {
            *data2 ++ = (((*data1 >> i) & 0x01) ? 255 : 0);
        }
        data1 ++;
    }
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介       摄像头图像发送至上位机查看图像
// 参数说明       uartn           使用的串口号
// 参数说明       *image_addr     需要发送的图像地址
// 参数说明       image_size      图像的大小
// @return      void
// Sample usage:                camera_send_image(DEBUG_UART_INDEX, &mt9v03x_image[0][0], MT9V03X_IMAGE_SIZE);
//-------------------------------------------------------------------------------------------------------------------
void camera_send_image (uart_index_enum uartn, const uint8 *image_addr, uint32 image_size)
{
    zf_assert(NULL != image_addr);
    // 发送命令
    uart_write_buffer(uartn, camera_send_image_frame_header, 4);

    // 发送图像
    uart_write_buffer(uartn, (uint8 *)image_addr, image_size);
}

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     摄像头串口 FIFO 初始化
// 参数说明     void
// 返回参数     void
// 使用示例     camera_fifo_init();
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
void camera_fifo_init (void)
{
    fifo_init(&camera_receiver_fifo, FIFO_DATA_8BIT, camera_receiver_buffer, CAMERA_RECEIVER_BUFFER_SIZE);
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介       摄像头采集初始化
// 参数说明       image_size      图像的大小
// @return      void
// 参数说明       image_size      图像的大小
// 参数说明       data_addr       数据来源外设地址
// 参数说明       buffer_addr     图像缓冲区地址
// @return      void
// Sample usage:                camera_init();
//-------------------------------------------------------------------------------------------------------------------
uint8 camera_init (uint8 *source_addr, uint8 *destination_addr, uint16 image_size)
{
    uint8 num;
    uint8 link_list_num;
    switch(camera_type)
    {
        case CAMERA_BIN_IIC:                                                    // IIC 小钻风
        case CAMERA_BIN_UART:                                                   // UART 小钻风
            for(num = 0; num < 8; num ++)
            {
                gpio_init((gpio_pin_enum)(OV7725_DATA_PIN + num), GPI, GPIO_LOW, GPI_FLOATING_IN);
            }
            link_list_num = dma_init(OV7725_DMA_CH,
                                     source_addr,
                                     destination_addr,
                                     OV7725_PCLK_PIN,
                                     EXTI_TRIGGER_FALLING,
                                     image_size);
            exti_init(OV7725_VSYNC_PIN, EXTI_TRIGGER_FALLING);                  //初始化场中断，并设置为下降沿触发中断
            break;
        case CAMERA_GRAYSCALE:                                                  // 总钻风
            for(num = 0; num < 8; num ++)
            {
                gpio_init((gpio_pin_enum)(MT9V03X_DATA_PIN + num), GPI, GPIO_LOW, GPI_FLOATING_IN);
            }
            link_list_num = dma_init(MT9V03X_DMA_CH,
                                     source_addr,
                                     destination_addr,
                                     MT9V03X_PCLK_PIN,
                                     EXTI_TRIGGER_RISING,
                                     image_size);                               // 如果超频到300M 倒数第二个参数请设置为FALLING

            exti_init(MT9V03X_VSYNC_PIN, EXTI_TRIGGER_FALLING);                 // 初始化场中断，并设置为下降沿触发中断
            break;
        case CAMERA_COLOR:                                                      // 凌瞳
            for(num=0; num<8; num++)
            {
                gpio_init((gpio_pin_enum)(SCC8660_DATA_PIN + num), GPI, GPIO_LOW, GPI_FLOATING_IN);
            }

            link_list_num = dma_init(SCC8660_DMA_CH,
                                     source_addr,
                                     destination_addr,
                                     SCC8660_PCLK_PIN,
                                     EXTI_TRIGGER_RISING,
                                     image_size);                               // 如果超频到300M 倒数第二个参数请设置为FALLING

            exti_init(SCC8660_VSYNC_PIN, EXTI_TRIGGER_FALLING);                 // 初始化场中断，并设置为下降沿触发中断
            break;
        default:
            break;
    }
    return link_list_num;
}

void Get_Use_Image(void)
{
    short i = 0, j = 0, row = 0, line = 0;
    for (i = 0; i < LCDH; i++)
    {
        for (j = 0; j <= LCDW; j++)
        {
            Image_Use[row][line] = mt9v03x_image[i][j + 14];
            line++;
        }
        line = 0;
        row++;
    }
}
void Get_Bin_Image(uint8 mode) // 对图像二值化
{

    
    switch (mode)
    {
    case 1: 
        Threshold = (unsigned char)otsuThreshold(Image_Use);
        break;
    case 2:
        lq_sobelAutoThreshold(Image_Use, Bin_Image);
        break;
    default:
        break;
    }

    /*
    for (i = 0; i < LCDH; i++)
    {
        for (j = 0; j < LCDW; j++)
        {
            if (Image_Use[i][j] > Threshold) // 数值越大，显示的内容越多，较浅的图像也能显示出来
                Bin_Image[i][j] = 1;
            else
                Bin_Image[i][j] = 0;
        }
    }   二值化算法
    */
}
int otsuThreshold(unsigned char image[LCDH][LCDW])//大津法
{
    int histogram[HISTOGRAM_SIZE] = {0};
    computeHistogram(image, histogram);

    int totalPixels = LCDH * LCDW;
    double sum = 0.0, sumB = 0.0;
    double varMax = 0.0;
    int threshold = 0;

    // 计算灰度平均值
    for (int t = 0; t < HISTOGRAM_SIZE; t++)
    {
        sum += t * histogram[t];
    }

    for (int t = 0; t < HISTOGRAM_SIZE; t++)
    {
        double wB = 0, wF = 0;
        double varBetween, meanB, meanF;

        wB += histogram[t];
        wF = totalPixels - wB;
        if (wF == 0)
            break;

        sumB += (double)(t * histogram[t]);

        meanB = sumB / wB;
        meanF = (sum - sumB) / wF;

        varBetween = (double)wB * (double)wF * (meanB - meanF) * (meanB - meanF);

        if (varBetween > varMax)
        {
            varMax = varBetween;
            threshold = t;
        }
    }

    return threshold;
}
void lq_sobelAutoThreshold(unsigned char imageIn[LCDH][LCDW], unsigned char imageOut[LCDH][LCDW])
{
    /** 卷积核大小 */
    short KERNEL_SIZE = 3;
    short xStart = KERNEL_SIZE / 2;
    short xEnd = LCDW - KERNEL_SIZE / 2;
    short yStart = KERNEL_SIZE / 2;
    short yEnd = LCDH - KERNEL_SIZE / 2;
    short i, j, k;
    short temp[4];
    for (i = yStart; i < yEnd; i++)
    {
        for (j = xStart; j < xEnd; j++)
        {
            /* 计算不同方向梯度幅值  */
            temp[0] = -(short)imageIn[i - 1][j - 1] + (short)imageIn[i - 1][j + 1]   //{{-1, 0, 1},
                      - (short)imageIn[i][j - 1] + (short)imageIn[i][j + 1]          // {-1, 0, 1},
                      - (short)imageIn[i + 1][j - 1] + (short)imageIn[i + 1][j + 1]; // {-1, 0, 1}};

            temp[1] = -(short)imageIn[i - 1][j - 1] + (short)imageIn[i + 1][j - 1]   //{{-1, -1, -1},
                      - (short)imageIn[i - 1][j] + (short)imageIn[i + 1][j]          // { 0,  0,  0},
                      - (short)imageIn[i - 1][j + 1] + (short)imageIn[i + 1][j + 1]; // { 1,  1,  1}};

            temp[2] = -(short)imageIn[i - 1][j] + (short)imageIn[i][j - 1]           //  0, -1, -1
                      - (short)imageIn[i][j + 1] + (short)imageIn[i + 1][j]          //  1,  0, -1
                      - (short)imageIn[i - 1][j + 1] + (short)imageIn[i + 1][j - 1]; //  1,  1,  0

            temp[3] = -(short)imageIn[i - 1][j] + (short)imageIn[i][j + 1]           // -1, -1,  0
                      - (short)imageIn[i][j - 1] + (short)imageIn[i + 1][j]          // -1,  0,  1
                      - (short)imageIn[i - 1][j - 1] + (short)imageIn[i + 1][j + 1]; //  0,  1,  1

            temp[0] = abs(temp[0]);
            temp[1] = abs(temp[1]);
            temp[2] = abs(temp[2]);
            temp[3] = abs(temp[3]);

            /* 找出梯度幅值最大值  */
            for (k = 1; k < 4; k++)
            {
                if (temp[0] < temp[k])
                {
                    temp[0] = temp[k];
                }
            }

            /* 使用像素点邻域内像素点之和的一定比例    作为阈值  */
            temp[3] = (short)imageIn[i - 1][j - 1] + (short)imageIn[i - 1][j] + (short)imageIn[i - 1][j + 1] + (short)imageIn[i][j - 1] + (short)imageIn[i][j] + (short)imageIn[i][j + 1] + (short)imageIn[i + 1][j - 1] + (short)imageIn[i + 1][j] + (short)imageIn[i + 1][j + 1];

            if (temp[0] > temp[3] / 12.0f)
            {
                imageOut[i][j] = 0;
            }
            else
            {
                imageOut[i][j] = 1;
            }
        }
    }
}
void Bin_Image_Filter(void) // 对图像进行滤波
{
    sint16 nr; // 行
    sint16 nc; // 列
    for (nr = 1; nr < LCDH - 1; nr++)
    {
        for (nc = 1; nc < LCDW - 1; nc = nc + 1)
        {
            if ((Bin_Image[nr][nc] == 0) && (Bin_Image[nr - 1][nc] + Bin_Image[nr + 1][nc] + Bin_Image[nr][nc + 1] + Bin_Image[nr][nc - 1] > 2))
            {
                Bin_Image[nr][nc] = 1;
            }
            else if ((Bin_Image[nr][nc] == 1) && (Bin_Image[nr - 1][nc] + Bin_Image[nr + 1][nc] + Bin_Image[nr][nc + 1] + Bin_Image[nr][nc - 1] < 2))
            {
                Bin_Image[nr][nc] = 0;
            }
        }
    }
}
uint8_t ImageGetSide(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t imageOut_last[LCDH][2]) // 获取左右边线
{
    uint8_t i = 0, j = 0;

    RoadIsNoSide(imageInput, imageOut, ROAD_START_ROW); // ？存疑
    for (i = 0; i <= LCDH; i++)
    {
        imageOut_last[i][0] = imageOut[i][0];
        imageOut_last[i][1] = imageOut[i][1];
    }

    /* 离车头近的40行 寻找边线 */
    for (i = ROAD_START_ROW - 1; i > ROAD_END_ROW; i--)
    {
        imageOut[i][0] = 0;
        imageOut[i][1] = 159;

        /* 根据边界连续特性 寻找边界 */
        for (j = imageOut[i + 1][0] + 10; j > 0; j--)
        {
            if (!imageInput[i][j])
            {
                imageOut[i][0] = j;
                break;
            }
        }
        for (j = imageOut[i + 1][1] - 10; j < 160; j++)
        {
            if (!imageInput[i][j])
            {
                imageOut[i][1] = j;
                break;
            }
        }

        /* 如果左边界 即将超出中线 则检查是否右丢线 */

        if (imageOut[i][0] > (LCDW / 2 - 10) && imageOut[i][1] > (LCDW - 5))
        {
            // 右丢线处理
            RoadNoSideProcess(imageInput, imageOut, 2, i); // ？存疑

            if (i > 70)
            {
                imageOut[i][0] += 50;
            }
            return 1;
        }

        /* 如果右边界 即将超出中线 则检查是否左丢线 */

        if (imageOut[i][1] < (LCDW / 2 + 10) && imageOut[i][0] < (5))
        {
            // 左丢线处理
            RoadNoSideProcess(imageInput, imageOut, 1, i);

            if (i > 70)
            {
                imageOut[i][1] -= 50;
            }
            return 2;
        }

        // 边线数组发生突变
        if ((imageOut_last[110][0] - imageOut[110][0]) > 20 && (imageOut_last[110][0] - imageOut[110][0]) < -20)
        {
            for (i = 0; i < LCDH; i++)
            {
                imageOut[i][0] = imageOut_last[i][0];
                imageOut[i][1] = 158;
            }
        }
        if (imageOut_last[110][1] - imageOut[110][1] > 20 && imageOut_last[110][1] - imageOut[110][1] < -20)
        {
            for (i = 0; i <= LCDH; i++)
            {
                imageOut[i][1] = imageOut_last[i][1];
                imageOut[i][0] = 1;
            }
        }
    }
    return 0;
}
uint8_t RoadIsNoSide(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t lineIndex)
{
    uint8_t state = 0;
    uint8_t i = 0;
    static uint8_t last = 78;
    imageOut[lineIndex][0] = 0;
    imageOut[lineIndex][1] = 159;
    /* 用距离小车比较近的行 判断是否丢线 */
    for (i = last; i > 1; i--)
    {
        if (!imageInput[lineIndex][i])
        {
            imageOut[lineIndex][0] = i;
            break;
        }
    }
    if (i == 1)
    {
        /* 左边界丢线 */
        state = 1;
    }

    for (i = last; i < 159; i++)
    {
        if (!imageInput[lineIndex][i])
        {
            imageOut[lineIndex][1] = i;
            break;
        }
    }
    if (i == 159)
    {
        /* 左右边界丢线 */
        if (state == 1)
        {
            state = 3;
        }
        /* 右边界丢线 */
        else
        {
            state = 2;
        }
    }
    if (imageOut[lineIndex][1] <= imageOut[lineIndex][0])
    {
        state = 4;
    }
    return state;
}
void RoadNoSideProcess(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t mode, uint8_t lineIndex)
{
    uint8_t i = 0, j = 0, count = 0;
    switch (mode)
    {
    case 1:
        for (i = imageOut[lineIndex][1]; i > 1; i--)
        {
            count++;
            for (j = lineIndex; j > ROAD_END_ROW && lineIndex > count; j--)
            {
                if (imageInput[j][i])
                {
                    imageOut[lineIndex - count][0] = 0;
                    imageOut[lineIndex - count][1] = i;
                    break;
                }
            }
        }
        break;
    case 2:
        for (i = imageOut[lineIndex][0]; i < 159; i++)
        {
            count++;
            for (j = lineIndex; j > ROAD_END_ROW && lineIndex > count; j--)
            {
                if (imageInput[j][i])
                {
                    imageOut[lineIndex - count][0] = i;
                    imageOut[lineIndex - count][1] = 159;
                    break;
                }
            }
        }
        break;
    }
}
uint8_t UpdownSideGet(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[2][LCDW]) // 获取上下边线
{
    uint8_t i = 0, j = 0;
    uint8_t last = 60;

    imageOut[0][159] = 0;
    imageOut[1][159] = 119;
    /* 用中线比较近的行 判断是否丢线 */
    for (i = last; i >= 0; i--)
    {
        if (!imageInput[i][80])
        {
            imageOut[0][80] = i;
            break;
        }
    }

    for (i = last; i < 120; i++)
    {
        if (!imageInput[i][80])
        {
            imageOut[1][80] = i;
            break;
        }
    }

    /* 中线往左 寻找边线 */
    for (i = 80 - 1; i > 0; i--)
    {
        imageOut[0][i] = 0;
        imageOut[1][i] = 119;

        /* 根据边界连续特性 寻找边界 */
        for (j = imageOut[0][i + 1] + 10; j > 0; j--)
        {
            if (!imageInput[j][i])
            {
                imageOut[0][i] = j;
                break;
            }
        }
        for (j = imageOut[1][i + 1] - 10; j < 120; j++)
        {
            if (!imageInput[j][i])
            {
                imageOut[1][i] = j;
                break;
            }
        }
    }
    /*中线往右 寻找边线*/
    for (i = 80 + 1; i < 159; i++)
    {
        imageOut[0][i] = 0;
        imageOut[1][i] = 119;

        /* 根据边界连续特性 寻找边界 */
        for (j = imageOut[0][i - 1] + 10; j > 0; j--)
        {
            if (!imageInput[j][i])
            {
                imageOut[0][i] = j;
                break;
            }
        }
        for (j = imageOut[1][i - 1] - 10; j < 120; j++)
        {
            if (!imageInput[j][i])
            {
                imageOut[1][i] = j;
                break;
            }
        }
    }
    return 0;
}
uint8_t GetRoadWide(uint8_t imageInput[LCDH][2], uint8_t imageOut[LCDH])
{
    uint8_t i = 0;
    for (i = 10; i <= LCDH - 5; i++)
    {
        imageOut[i] = 0;
        if (imageInput[i][1] > imageInput[i][0])
        {
            imageOut[i] = imageInput[i][1] - imageInput[i][0];
        }
        else
        {
            imageOut[i] = 160;
        }
    }
    return 0;
}
int16_t RoadGetSteeringError(uint8_t imageSide[LCDH][2], uint8_t lineIndex)
{
    int16_t sum = 0;
    sum = (imageSide[lineIndex][0] + imageSide[lineIndex][1] - 159) +
          (imageSide[lineIndex - 6][0] + imageSide[lineIndex - 6][1] - 159) +
          (imageSide[lineIndex - 12][0] + imageSide[lineIndex - 12][1] - 159) +
          (imageSide[lineIndex - 18][0] + imageSide[lineIndex - 18][1] - 159) +
          (imageSide[lineIndex - 24][0] + imageSide[lineIndex - 24][1] - 159);
    sum = sum / 5;
    return sum;
}
void Get_Errand(void)
{

    g_sSteeringError = RoadGetSteeringError(ImageSide, ROAD_MAIN_ROW);

    // 偏差放大
    if ((g_sSteeringError < 60) && (g_sSteeringError > -60))
    {
        if ((g_sSteeringError < 20) && (g_sSteeringError > -20))
        {
            Servo_P = 11;
        }
        else
        {
            Servo_P = 14;
        }
    }
    else
    {
        Servo_P = 11;
    }
    ServoDuty = g_sSteeringError * Servo_P / 10; // serveoduty尚未定义
    // 偏差限幅
    if (ServoDuty > 100)
        ServoDuty = 100;
    if (ServoDuty < -100)
        ServoDuty = -100;
}
void cameracar(void) // 在中断里处理
{

    Get_Errand();
}