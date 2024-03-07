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
 * 2022-09-15       pudding            first version
 * 2023-04-25       pudding            ��������ע��˵��
 ********************************************************************************************************************/

#ifndef _zf_device_camera_h_
#define _zf_device_camera_h_

#include "zf_common_fifo.h"
#include "zf_common_typedef.h"
#include "zf_driver_uart.h"
#include "zf_device_type.h"

//=================================================����ͷ������ ��������================================================
#define CAMERA_RECEIVER_BUFFER_SIZE (8)         // ��������ͷ�������ݻ�������С
extern fifo_struct camera_receiver_fifo;        // ��������ͷ��������fifo�ṹ��
extern uint8 camera_send_image_frame_header[4]; // ��������ͷ���ݷ��͵���λ����֡ͷ
//=================================================����ͷ������ ��������================================================

//=================================================����ͷ������ ��������================================================
void camera_binary_image_decompression(const uint8 *data1, uint8 *data2, uint32 image_size); // ����ͷ������ͼ�����ݽ�ѹΪʮ�����ư�λ���� С�����
void camera_send_image(uart_index_enum uartn, const uint8 *image_addr, uint32 image_size);   // ����ͷͼ��������λ���鿴ͼ��
void camera_fifo_init(void);                                                                 // ����ͷ���� FIFO ��ʼ��
uint8 camera_init(uint8 *source_addr, uint8 *destination_addr, uint16 image_size);           // ����ͷ�ɼ���ʼ��

void Get_Use_Image(void);                                                                          // ѹ��ͼ�񣬻�ûҶ�ͼ��
void Get_Bin_Image(uint8 mode);                                                                    // ��ö�ֵ��ͼ��1Ϊ��򷨣�2Ϊsobel
int otsuThreshold(unsigned char image[LCDH][LCDW]);                                                // ���
void lq_sobelAutoThreshold(unsigned char imageIn[LCDH][LCDW], unsigned char imageOut[LCDH][LCDW]); // sobel����Ӧ��ֵ

void Bin_Image_Filter(void);                                                                                        // ��ͼ������˲�
uint8_t ImageGetSide(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t imageOut_last[LCDH][2]);    // ��ȡ���ұ���
void RoadNoSideProcess(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t mode, uint8_t lineIndex); // ���ߴ���
uint8_t RoadIsNoSide(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[LCDH][2], uint8_t lineIndex);                 // ���ߴ���
uint8_t UpdownSideGet(uint8_t imageInput[LCDH][LCDW], uint8_t imageOut[2][LCDW]);                                   // ��ȡ���±���
uint8_t GetRoadWide(uint8_t imageInput[LCDH][2], uint8_t imageOut[LCDH]);                                           // ��ȡ�������
int16_t RoadGetSteeringError(uint8_t imageSide[LCDH][2], uint8_t lineIndex);                                        // ��ȡ���ƫ��
void Get_Errand(void);                                                                                              // ��ȡͼ��ƫ��
void cameracar(void);                                                                                               // ����ͷԪ���жϴ���

//=================================================����ͷ������ ��������================================================

#endif
