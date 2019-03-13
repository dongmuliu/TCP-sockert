
#include <string>
#include<vector>
#include<stdint.h>
#include <stdio.h>
#include "opencv2/imgproc/imgproc.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <math.h>
#include<vector>
#include<stdint.h>
#include <stdio.h>
#include <winsock2.h>

using namespace cv;
using namespace std;
	
#define CAM_IP					"192.168.7.63"	//���IP
#define CAM_PORT				50660			//����˿�
#define MAXLINE					253600		//���������ֵ
#define MAX_NUM_PIX				82656		//328 * 252
#define LOW_AMPLITUDE 			32500		//ǿ�ȹ���ֵ
#define MAX_PHASE				30000.0		//�¶Ƚ�����δʹ�ã�
#define MAX_DIST_VALUE 			30000		//��Զ����ֵ
#define OFFSET_PHASE_DEFAULT	0			//��Ȳ���ֵ
//�����������ڲΡ��������
#define   FX  286.6034
#define   FY  287.3857	
#define   CX  176.2039
#define   CY  126.5788	
#define   K1  -0.12346
#define   K2  0.159423


#pragma comment (lib, "ws2_32.lib")  //���� ws2_32.dll

int		drnuLut[50][252][328];				//�¶Ƚ����ñ�
char sendline[] = "getDistanceSorted";		//��ȡ����ָ��


void imageAverageEightConnectivity(ushort *depthdata);
void calculationAddOffset(ushort *img);
int calculationCorrectDRNU(ushort * img);


//socket��ȡͼ��
//������sendline[] ����ָ��
//������length �ڴ���ȡ���ĳ���
//���أ��ɹ��󷵻�����ָ��
//		ʧ�ܷ���NULL
unsigned char* socket_com(char sendline[], int length)
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	char buf[MAXLINE];		//���ջ�����
	int count = 0;					//�������ֽڼ���
	SOCKET  sockfd;
	int rec_len, Ret;				//���͡�����״̬
	struct sockaddr_in servaddr;

	//socket��ʼ��
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == INVALID_SOCKET)
	{
		cout << "Create Socket Failed::" << GetLastError() << endl;
		exit(0);
	}

	//�������IP��Ϣ
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(CAM_PORT);
	servaddr.sin_addr.s_addr = inet_addr(CAM_IP);

	//�������
	Ret = connect(sockfd, (SOCKADDR*)&servaddr, sizeof(servaddr));
	if (Ret == SOCKET_ERROR)
	{
		cout << "Connect Error::" << GetLastError() << endl;
		exit(0);
	}
	else
	{
		cout << "���ӳɹ�!" << endl;
	}

	//���Ͳɼ�����
	Ret = send(sockfd, sendline, strlen(sendline), 0);
	if (Ret == SOCKET_ERROR)
	{
		cout << "Send Info Error::" << GetLastError() << endl;
		exit(0);
	}
	else
	{
		//cout << "send�ɹ�!" << endl;
	}

	//���շ���ͼ������
	int i2 = 0;
	unsigned char* ptr_buf2 = new unsigned char[MAXLINE];
	while (count < length) //320*240*2=153600
	{
		rec_len = recv(sockfd, buf, MAXLINE, 0);
		for (int i = 0; i < rec_len; i++)
		{

			ptr_buf2[i2] = (unsigned char)buf[i];
			i2++;

		}
		if (rec_len == SOCKET_ERROR)
		{
			cout << "����Error::" << GetLastError() << endl;
			exit(0);
		}

		count = count + rec_len;

	}
	//�ر��׽���
	closesocket(sockfd);
	//��ֹʹ�� DLL
	WSACleanup();

	return ptr_buf2;

}
//�˲�
//���룺ͼ����Ϣ��ָ��
void calibrate(ushort *img)
{
	//��ֵ�˲�
	imageAverageEightConnectivity(img);
	//�¶Ƚ���
	calculationCorrectDRNU(img);
	//��Ȳ���
	calculationAddOffset(img);
	
}

//�������
//���룺 ��������ͼƬ
//����� У�����ͼƬ
Mat undistimg(Mat src)
{
	
	Mat img;

	//�ڲξ���
	Mat cameraMatrix = Mat::eye(3, 3, CV_64F);		//3*3��λ����
	cameraMatrix.at<double>(0, 0) = FX;
	cameraMatrix.at<double>(0, 1) = 0;
	cameraMatrix.at<double>(0, 2) = CX;
	cameraMatrix.at<double>(1, 1) = FY;
	cameraMatrix.at<double>(1, 2) = CY;
	cameraMatrix.at<double>(2, 2) = 1;

	//�������
	Mat distCoeffs = Mat::zeros(5, 1, CV_64F);		//5*1ȫ0����
	distCoeffs.at<double>(0, 0) = K1;
	distCoeffs.at<double>(1, 0) = K2;
	distCoeffs.at<double>(2, 0) = 0;
	distCoeffs.at<double>(3, 0) = 0;
	distCoeffs.at<double>(4, 0) = 0;

	Size imageSize = src.size();
	Mat map1, map2;
	//����1������ڲξ���
	//����2���������
	//����3����ѡ���룬��һ�͵ڶ��������֮�����ת����
	//����4��У�����3X3�������
	//����5����ʧ��ͼ��ߴ�
	//����6��map1�������ͣ�CV_32FC1��CV_16SC2
	//����7��8�����X/Y������ӳ�����
	cv::initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, imageSize, CV_32FC1, map1, map2);	//�������ӳ��
	//����1������ԭʼͼ��
	//����2�����ͼ��
	//����3��4��X\Y������ӳ��
	//����5��ͼ��Ĳ�ֵ��ʽ
	//����6���߽���䷽ʽ
	cv::remap(src, img, map1, map2, INTER_LINEAR);																	//�������
	return img.clone();
}

//��ֵ�˲�
//���룺 ���ͼ��ָ��
void imageAverageEightConnectivity(ushort *depthdata)
{
	
		int pixelCounter;
		int nCols = 320;
		int nRowsPerHalf = 120;
		int size = 320 * 240;
		ushort actualFrame[MAX_NUM_PIX];
		int i, j, index;
		int pixdata;
		memcpy(actualFrame, depthdata, size * sizeof(ushort));
		// up side
		// dowm side
		// left side and right side
		// normal part
		for (i = 1; i < 239; i++) {
			for (j = 1; j < 319; j++){
				index = i * 320 + j;
				pixelCounter = 0;
				pixdata = 0;
				if (actualFrame[index] < 30000) {
					pixelCounter++;
					pixdata += actualFrame[index];
				}
				if (actualFrame[index - 1]  < 30000) {   // left
					pixdata += actualFrame[index - 1];
					pixelCounter++;
				}
				if (actualFrame[index + 1]  < 30000) {   // right
					pixdata += actualFrame[index + 1];
					pixelCounter++;
				}
				if (actualFrame[index - 321]  < 30000) {   // left up
					pixdata += actualFrame[index - 321];
					pixelCounter++;
				}
				if (actualFrame[index - 320]  < 30000) {   // up
					pixdata += actualFrame[index - 320];
					pixelCounter++;
				}
				if (actualFrame[index - 319]  < 30000) {   // right up
					pixdata += actualFrame[index - 319];
					pixelCounter++;
				}
				if (actualFrame[index + 319]  < 30000) {   // left down
					pixdata += actualFrame[index - 321];
					pixelCounter++;
				}
				if (actualFrame[index + 320]  < 30000) {   // down
					pixdata += actualFrame[index - 320];
					pixelCounter++;
				}
				if (actualFrame[index + 321]  < 30000) {   // right down
					pixdata += actualFrame[index - 319];
					pixelCounter++;
				}
				//�����Χ��Ч����С��6��Ϊ��Ч��
				if (pixelCounter < 6) {
					*(depthdata + index) = LOW_AMPLITUDE;
				}
				else {
					*(depthdata + index) = pixdata / pixelCounter;
				}
			}
		}
}

//�¶�У��
int calculationCorrectDRNU(ushort * img)
{
	//int i, x, y, l;
	//double dist, tempDiff = 0;
	////printf("gOffsetDRNU = %d\n", offset);  //w
	//uint16_t maxDistanceMM = 150000000 / 12000;
	//double stepLSB = gStepCalMM * MAX_PHASE / maxDistanceMM;
	////printf("stepLSB = %2.4f\n", stepLSB);  //w

	//uint16_t *pMem = img;
	//int width = 320;
	//int height = 240;
	//tempDiff = realTempChip - gTempCal_Temp;
	//for (l = 0, y = 0; y< height; y++){
	//	for (x = 0; x < width; x++, l++){
	//		dist = pMem[l];

	//		if (dist < LOW_AMPLITUDE){	//correct if not saturated
	//			
	//			if (dist<0)	//if phase jump
	//				dist += MAX_DIST_VALUE;

	//			i = (int)(round(dist / stepLSB));

	//			if (i<0) i = 0;
	//			else if (i >= 50) i = 49;

	//			dist = (double)pMem[l] - drnuLut[i][y][x];
	//			/*if (l == 1300){
	//			printf(" DRNULutPixel = %d,", drnuLut[i][y][x]);
	//			}
	//			if (l == 1301){
	//			printf("  %d,", drnuLut[i][y][x]);
	//			}
	//			if (l == 1302){
	//			printf(" %d \n", drnuLut[i][y][x]);
	//			}*/
	//			// temprature cali
	//			dist -= tempDiff * 3.12262;	// 0.312262 = 15.6131 / 50

	//			pMem[l] = (uint16_t)round(dist);

	//		}	//end if LOW_AMPLITUDE
	//	}	//end width
	//}	//end height
	////printf(" pMem = %d, %d, %d\n", pMem[1300], pMem[1301], pMem[1302]);
	return 0;

}

//��Ȳ���
//���룺ͼ����Ϣָ��
void calculationAddOffset(ushort *img)
{
	int offset = 0;
	uint16_t maxDistanceCM = 0;
	int l = 0;
	offset = OFFSET_PHASE_DEFAULT;
	uint16_t val;
	uint16_t *pMem = img;
	int numPix = 320 * 240;

	for (l = 0; l<numPix; l++){
		val = pMem[l];
		if (val < 30000) { //if not low amplitude, not saturated and not  adc overflow
			pMem[l] = (val + offset) % MAX_DIST_VALUE;
		}
	}
}


int main()
{
	while (1)
	{
		unsigned char* ptr_buf_unsigned = socket_com(sendline, 153600);		//��ȡͼ������

		//�����ֽںϳ�һ������ֵ
		uint16_t fameDepthArray2[MAXLINE];
		int realindex;
		for (int j = 0; j < 76800; j++)
		{
			uint16_t raw_dep;
			raw_dep = ptr_buf_unsigned[j * 2 + 1] * 256 + ptr_buf_unsigned[2 * j];
			//cout << raw_dep << " ";
			realindex = 76800 - (j / 320 + 1) * 320 + j % 320;   //����
			fameDepthArray2[realindex] = raw_dep;
		}
		//�˲�����
		calibrate(fameDepthArray2);

		//ת��ά����
		uint16_t depth[240][320];
		for (int i = 0; i < 240; i++)
		{
			for (int j = 0; j < 320; j++)
			{
				depth[i][j] = fameDepthArray2[i * 320 + j];
			}
		}

		//תMat��ʽ
		Mat src_1(240, 320, CV_16UC1, Scalar(0));
		Mat imshowsrc(240, 320, CV_8UC1, Scalar(0));
		for (int i = 0; i < 240; i++)
		{
			for (int j = 0; j < 320; j++)
			{
				src_1.at<ushort>(i, j) = depth[i][j];
			}
		}

		//�������
		Mat src_2 =	undistimg(src_1);
		
		for (int i = 0; i < 240; i++)
		{
			for (int j = 0; j < 320; j++)
			{
				imshowsrc.at<uchar>(i, j) = 255 - src_2.at<ushort>(i, j) * 25 / 3000;
			}
		}

		//ͼƬ��ʾ
		imshow("test", imshowsrc);
		waitKey(1);
	}
	return 0;

}