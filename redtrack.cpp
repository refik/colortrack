/* Author: Refik Turkeli (alintilar var)*/
#include <math.h>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <opencv/cv.h>
#include <opencv/highgui.h>
#include <cvblob.h>


int n;
int speed1, speed2;
int center;
int pickcenter;
int key;
int maxratio2;
float ratio;
float maxratio;


int main()
{	
	int i,j,k,z,ave;
	int x[1], hsv[1600];
	int temp=0, check=0;
	int heighthsv,widthhsv,stephsv,channelshsv;
	int heightmono,widthmono,stepmono,channelsmono;
	int sthreshold=140;
	uchar *datahsv, *datamono, *datahsv2;
	double hlower=175,hupper=5,sum=0,aved;		//kirmizi rengin araligi burda tanımlanıyor 175 - 5		
	CvCapture* capture = 0;
	i=j=k=0;
	x[0] = 8;
	capture = cvCaptureFromCAM( 0 );		//webcam initialize ediliyor
	
	while ( key != 'q' ) {
		IplImage* frame = 0;
		frame = cvQueryFrame( capture );	//kameradan gelen görüntüler "frame" içine alınıyor
		IplImage *colimgbot = cvCreateImage( cvGetSize(frame), 8, 3 );
		IplImage *monoimgbot = cvCreateImage( cvGetSize(frame), 8, 1 );	//sadece kirmizi renklerin beyaza çevrildiği monochrome image yaratıldı
		cvNamedWindow( "Raw", 1 );

//		cvRectangle(frame,cvPoint(300, 220),cvPoint(340, 260),cvScalar(0, 0, 255, 0),3, 8, 0);

//		while ( key != 'a') {
//			cvShowImage("Raw",frame);
//			key = cvWaitKey(10);
//		}

		heighthsv = colimgbot->height;
		widthhsv = colimgbot->width;
		stephsv =colimgbot->widthStep;
		channelshsv = colimgbot->nChannels;
		datahsv = (uchar *)colimgbot->imageData;

		heightmono = monoimgbot ->height;
		widthmono = monoimgbot->width;
		stepmono = monoimgbot->widthStep;
		channelsmono = monoimgbot->nChannels;
		datamono = (uchar *)monoimgbot->imageData;
		cvCvtColor(frame,colimgbot,CV_RGB2HSV);

		/* Bu alandaki loop lar resimdeki kirmizi renkleri beyaz alıp diger pixelleri siyah birakıyor */
		for (i=0;i< (heighthsv);i++) {
			for (j=0;j<(widthhsv);j++) {
				if ((datahsv[(i)*stephsv+j*channelshsv]<=hlower) && (datahsv[(i)*stephsv+j*channelshsv]>=hupper)) { 
					if ((datahsv[(i)*stephsv+j*(channelshsv)+1])>sthreshold) {
						datamono[i*stepmono+j*channelsmono]=255;
					} else {
						datamono[i*stepmono+j*channelsmono]=0;
					}
				} else {

				}
			}
		}

		for (i=0;i< (heighthsv);i++) {
			for (j=0;j<(widthhsv);j++) {
				if (!(datamono[i*stepmono+j*channelsmono]==0 || datamono[i*stepmono+j*channelsmono]==255)) {
					datamono[i*stepmono+j*channelsmono]=0;
				}
			}
		}
		/*-------------------------------------------------------------------------------------------*/

		cvErode(monoimgbot,monoimgbot,0,6);
		cvDilate(monoimgbot,monoimgbot,0,10);

		int q[1];
		q[0] = 1;
		cvSaveImage("spotted.jpg",monoimgbot,q);
		IplImage *img = cvLoadImage("spotted.jpg", 1);

		/* Burada kirmizi renklerin beyaza cevrildigi image uzerinde blob detection yapiliyor */
		cvThreshold(img, img, 100, 200, CV_THRESH_BINARY);
		cvSetImageROI(img, cvRect(0, 0, 640, 480));
		IplImage *chB=cvCreateImage(cvGetSize(img),8,1);
		cvSplit(img,chB,NULL,NULL,NULL);
		IplImage *labelImg = cvCreateImage(cvGetSize(chB),IPL_DEPTH_LABEL,1);
		CvBlobs blobs;
		unsigned int result = cvLabel(chB, labelImg, blobs);
		cvRenderBlobs(labelImg, blobs, img, img);

		for (CvBlobs::const_iterator it=blobs.begin(); it!=blobs.end(); ++it) {
			CvContourChainCode *contour = cvGetContour(it->second, labelImg);
			cvRenderContourChainCode(contour, img);

			CvContourPolygon *polygon = cvConvertChainCodesToPolygon(contour);
			CvContourPolygon *sPolygon = cvSimplifyPolygon(polygon, 10.);
			cvRenderContourPolygon(sPolygon, frame, CV_RGB(0, 0, 255));

			delete sPolygon;
			delete polygon;
			delete contour;

			center = (((it->second->maxx - it->second->minx)/2) + it->second->minx) - 320;
			ratio = it->second->area * 100 / 307200;	//ratio resimdeki kirmizi alanin butun resme orani

			if (ratio > maxratio) {			
				maxratio = ratio;			//maxratio resimdeki en buyuk kirmizi alanin resme orani
				pickcenter = center;			//pickcenter resimdeki en buyuk kirmizinin resmin merkezine uzakligi
			}
		}
		/*--------------------------------------------------------------------------------------*/

		cvShowImage( "Raw", frame );
		maxratio2 = (int) maxratio;

		std::ofstream fout("p.txt");
		speed1 = 1000;	//speed1 ve speed2 motora verilecek degerler. 0 ile 1200 arasi. text dosyasina yaziliyor
		speed2 = 1000;

		/* Burada eger en buyuk kirmizinin merkez noktasi -40 ve 40 pixel arasinda değilse yön veriliyor */
		if (!(pickcenter < 40 && pickcenter > -40)) {
			if(pickcenter < 0) {
				speed2 = speed2 + (pickcenter*3);
			} else {
				speed1 = speed1 - (pickcenter*3);
			}
		} 

		/* Eger yon vermek icin hesaplanan motorun degeri negatif cikarsa 0 olsun */
		if(speed2 < 0) { speed2 = 0; }
		if(speed1 < 0) { speed1 = 0; }

		/* burasi kontrol icin. Kirmizi alanin resme orani 2'den kucuk cikarsa check degerini increment ediyor */
		if(ratio<2) {
			check++;
		} else {
			check = 0;
		}

		/* burada eger oran 2 den kucukse ve ustte hesaplanan check degeri 5 ten buyuksa motor hizlari sifirlanıyor */
		/* check diye bir degerin olmasinin sebebi eger sadece 1 frame de kirmizi algilayamazsa motoru tutukluk yaptirmasin */
		if(ratio<2 && check > 5) {
			speed1 = 0;
			speed2 = 0;
		}

		fout << speed1 << " " << speed2;
		fout.close();
		maxratio = 0;
		pickcenter = 0;
		cvReleaseImage(&chB);
		cvReleaseImage(&labelImg);
		cvReleaseImage(&img);
		cvReleaseBlobs(blobs);
		key = cvWaitKey(10);
	}

	cvDestroyWindow("Raw");
	cvReleaseCapture( &capture );
	return 0;
}
