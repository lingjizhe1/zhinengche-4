/*********************************************************************************************************************
* TC264 Opensourec Library ����TC264 ��Դ�⣩��һ�����ڹٷ� SDK �ӿڵĵ�������Դ��
* Copyright (c) 2022 SEEKFREE ��ɿƼ�
*
* ���ļ��� TC264 ��Դ���һ����
*
* TC264 ��Դ�� ��������
* �����Ը��������������ᷢ���� GPL��GNU General Public License���� GNUͨ�ù������֤��������
* �� GPL �ĵ�3�棨�� GPL3.0������ѡ��ģ��κκ����İ汾�����·�����/���޸���
*
* ����Դ��ķ�����ϣ�����ܷ������ã�����δ�������κεı�֤
* ����û�������������Ի��ʺ��ض���;�ı�֤
* ����ϸ����μ� GPL
*
* ��Ӧ�����յ�����Դ���ͬʱ�յ�һ�� GPL �ĸ���
* ���û�У������<https://www.gnu.org/licenses/>
*
* ����ע����
* ����Դ��ʹ�� GPL3.0 ��Դ���֤Э�� �����������Ϊ���İ汾
* �������Ӣ�İ��� libraries/doc �ļ����µ� GPL3_permission_statement.txt �ļ���
* ���֤������ libraries �ļ����� �����ļ����µ� LICENSE �ļ�
* ��ӭ��λʹ�ò����������� ���޸�����ʱ���뱣����ɿƼ��İ�Ȩ����������������
*
* �ļ�����          zf_device_camera
* ��˾����          �ɶ���ɿƼ����޹�˾
* �汾��Ϣ          �鿴 libraries/doc �ļ����� version �ļ� �汾˵��
* ��������          ADS v1.9.20
* ����ƽ̨          TC264D
* ��������          https://seekfree.taobao.com/
*
* �޸ļ�¼
* ����              ����                ��ע
* 2022-09-15       pudding           first version
* 2023-04-25       pudding           ��������ע��˵��
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


fifo_struct camera_receiver_fifo;                                           // ��������ͷ��������fifo�ṹ��
uint8 camera_receiver_buffer[CAMERA_RECEIVER_BUFFER_SIZE];                  // ��������ͷ�������ݻ�����
uint8 camera_send_image_frame_header[4] = {0x00, 0xFF, 0x01, 0x01};         // ��������ͷ���ݷ��͵���λ����֡ͷ
#define ROAD_MAIN_ROW 44

//-------------------------------------------------------------------------------------------------------------------
// �������       ����ͷ������ͼ�����ݽ�ѹΪʮ�����ư�λ���� С�����
// ����˵��       *data1          ����ͷͼ������
// ����˵��       *data2          ��Ž�ѹ���ݵĵ�ַ
// ����˵��       image_size      ͼ��Ĵ�С
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
// �������       ����ͷͼ��������λ���鿴ͼ��
// ����˵��       uartn           ʹ�õĴ��ں�
// ����˵��       *image_addr     ��Ҫ���͵�ͼ���ַ
// ����˵��       image_size      ͼ��Ĵ�С
// @return      void
// Sample usage:                camera_send_image(DEBUG_UART_INDEX, &mt9v03x_image[0][0], MT9V03X_IMAGE_SIZE);
//-------------------------------------------------------------------------------------------------------------------
void camera_send_image (uart_index_enum uartn, const uint8 *image_addr, uint32 image_size)
{
    zf_assert(NULL != image_addr);
    // ��������
    uart_write_buffer(uartn, camera_send_image_frame_header, 4);

    // ����ͼ��
    uart_write_buffer(uartn, (uint8 *)image_addr, image_size);
}

//-------------------------------------------------------------------------------------------------------------------
// �������     ����ͷ���� FIFO ��ʼ��
// ����˵��     void
// ���ز���     void
// ʹ��ʾ��     camera_fifo_init();
// ��ע��Ϣ
//-------------------------------------------------------------------------------------------------------------------
void camera_fifo_init (void)
{
    fifo_init(&camera_receiver_fifo, FIFO_DATA_8BIT, camera_receiver_buffer, CAMERA_RECEIVER_BUFFER_SIZE);
}


//-------------------------------------------------------------------------------------------------------------------
// �������       ����ͷ�ɼ���ʼ��
// ����˵��       image_size      ͼ��Ĵ�С
// @return      void
// ����˵��       image_size      ͼ��Ĵ�С
// ����˵��       data_addr       ������Դ�����ַ
// ����˵��       buffer_addr     ͼ�񻺳�����ַ
// @return      void
// Sample usage:                camera_init();
//-------------------------------------------------------------------------------------------------------------------
uint8 camera_init (uint8 *source_addr, uint8 *destination_addr, uint16 image_size)
{
    uint8 num;
    uint8 link_list_num;
    switch(camera_type)
    {
        case CAMERA_BIN_IIC:                                                    // IIC С���
        case CAMERA_BIN_UART:                                                   // UART С���
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
            exti_init(OV7725_VSYNC_PIN, EXTI_TRIGGER_FALLING);                  //��ʼ�����жϣ�������Ϊ�½��ش����ж�
            break;
        case CAMERA_GRAYSCALE:                                                  // �����
            for(num = 0; num < 8; num ++)
            {
                gpio_init((gpio_pin_enum)(MT9V03X_DATA_PIN + num), GPI, GPIO_LOW, GPI_FLOATING_IN);
            }
            link_list_num = dma_init(MT9V03X_DMA_CH,
                                     source_addr,
                                     destination_addr,
                                     MT9V03X_PCLK_PIN,
                                     EXTI_TRIGGER_RISING,
                                     image_size);                               // �����Ƶ��300M �����ڶ�������������ΪFALLING

            exti_init(MT9V03X_VSYNC_PIN, EXTI_TRIGGER_FALLING);                 // ��ʼ�����жϣ�������Ϊ�½��ش����ж�
            break;
        case CAMERA_COLOR:                                                      // ��ͫ
            for(num=0; num<8; num++)
            {
                gpio_init((gpio_pin_enum)(SCC8660_DATA_PIN + num), GPI, GPIO_LOW, GPI_FLOATING_IN);
            }

            link_list_num = dma_init(SCC8660_DMA_CH,
                                     source_addr,
                                     destination_addr,
                                     SCC8660_PCLK_PIN,
                                     EXTI_TRIGGER_RISING,
                                     image_size);                               // �����Ƶ��300M �����ڶ�������������ΪFALLING

            exti_init(SCC8660_VSYNC_PIN, EXTI_TRIGGER_FALLING);                 // ��ʼ�����жϣ�������Ϊ�½��ش����ж�
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
void Get_Bin_Image(uint8 mode) // ��ͼ���ֵ��
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
            if (Image_Use[i][j] > Threshold) // ��ֵԽ����ʾ������Խ�࣬��ǳ��ͼ��Ҳ����ʾ����
                Bin_Image[i][j] = 1;
            else
                Bin_Image[i][j] = 0;
        }
    }   ��ֵ���㷨
    */
}
int otsuThreshold(unsigned char image[LCDH][LCDW])//���
{
    int histogram[HISTOGRAM_SIZE] = {0};
    computeHistogram(image, histogram);

    int totalPixels = LCDH * LCDW;
    double sum = 0.0, sumB = 0.0;
    double varMax = 0.0;
    int threshold = 0;

    // ����Ҷ�ƽ��ֵ
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
    /** ����˴�С */
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
            /* ���㲻ͬ�����ݶȷ�ֵ  */
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

            /* �ҳ��ݶȷ�ֵ���ֵ  */
            for (k = 1; k < 4; k++)
            {
                if (temp[0] < temp[k])
                {
                    temp[0] = temp[k];
                }
            }

            /* ʹ�����ص����������ص�֮�͵�һ������    ��Ϊ��ֵ  */
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
void Bin_Image_Filter(void) // ��ͼ������˲�
{
    sint16 nr; // ��
    sint16 nc; // ��
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
uint8_t ImageGetSide(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t imageOut_last[LCDH][2]) // ��ȡ���ұ���
{
    uint8_t i = 0, j = 0;

    RoadIsNoSide(imageInput, imageOut, ROAD_START_ROW); // ������
    for (i = 0; i <= LCDH; i++)
    {
        imageOut_last[i][0] = imageOut[i][0];
        imageOut_last[i][1] = imageOut[i][1];
    }

    /* �복ͷ����40�� Ѱ�ұ��� */
    for (i = ROAD_START_ROW - 1; i > ROAD_END_ROW; i--)
    {
        imageOut[i][0] = 0;
        imageOut[i][1] = 159;

        /* ���ݱ߽��������� Ѱ�ұ߽� */
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

        /* �����߽� ������������ �����Ƿ��Ҷ��� */

        if (imageOut[i][0] > (LCDW / 2 - 10) && imageOut[i][1] > (LCDW - 5))
        {
            // �Ҷ��ߴ���
            RoadNoSideProcess(imageInput, imageOut, 2, i); // ������

            if (i > 70)
            {
                imageOut[i][0] += 50;
            }
            return 1;
        }

        /* ����ұ߽� ������������ �����Ƿ����� */

        if (imageOut[i][1] < (LCDW / 2 + 10) && imageOut[i][0] < (5))
        {
            // ���ߴ���
            RoadNoSideProcess(imageInput, imageOut, 1, i);

            if (i > 70)
            {
                imageOut[i][1] -= 50;
            }
            return 2;
        }

        // �������鷢��ͻ��
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
    /* �þ���С���ȽϽ����� �ж��Ƿ��� */
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
        /* ��߽綪�� */
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
        /* ���ұ߽綪�� */
        if (state == 1)
        {
            state = 3;
        }
        /* �ұ߽綪�� */
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
uint8_t UpdownSideGet(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[2][LCDW]) // ��ȡ���±���
{
    uint8_t i = 0, j = 0;
    uint8_t last = 60;

    imageOut[0][159] = 0;
    imageOut[1][159] = 119;
    /* �����߱ȽϽ����� �ж��Ƿ��� */
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

    /* �������� Ѱ�ұ��� */
    for (i = 80 - 1; i > 0; i--)
    {
        imageOut[0][i] = 0;
        imageOut[1][i] = 119;

        /* ���ݱ߽��������� Ѱ�ұ߽� */
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
    /*�������� Ѱ�ұ���*/
    for (i = 80 + 1; i < 159; i++)
    {
        imageOut[0][i] = 0;
        imageOut[1][i] = 119;

        /* ���ݱ߽��������� Ѱ�ұ߽� */
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

    // ƫ��Ŵ�
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
    ServoDuty = g_sSteeringError * Servo_P / 10; // serveoduty��δ����
    // ƫ���޷�
    if (ServoDuty > 100)
        ServoDuty = 100;
    if (ServoDuty < -100)
        ServoDuty = -100;
}
void cameracar(void) // ���ж��ﴦ��
{

    Get_Errand();
}