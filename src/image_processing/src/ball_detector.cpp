#include "ros/ros.h"
#include <ros/console.h>
#include "std_msgs/String.h"
#include <image_transport/image_transport.h>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <math.h>
#include <cstdlib>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <cv_bridge/cv_bridge.h>
#include "image_processing/jetsonGPIO.h"

#define LEFTLIMIT 0.35
#define RIGHTLIMIT 0.75
#define BOTTOMLIMIT 0.9

#define TENNIS_BALL = 1;
#define LAUNCHER = 1;
#define NONE = 2;

ros::Publisher pub;
image_transport::Subscriber sub;
using namespace cv;
using namespace std;
jetsonTX1GPIONumber C = gpio37;
jetsonTX1GPIONumber D = gpio38;
jetsonTX1GPIONumber En = gpio187;
jetsonTX1GPIONumber S = gpio63;
//jetsonTX1GPIONumber T = gpio186;
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
const int MAX_NUM_OBJECTS = 50;
const int MIN_OBJECT_AREA = 20*20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH/1.5;
const int CannyThreshInitialValue = 200;
const int AccumulatorThreshInitialValue = 50;
const int MaxAccumulatorThresh = 200;
const int MaxCannyThresh = 255;
int H_MIN_B = 29;
int H_MAX_B = 64;
int H_MIN_L = 148;
int H_MAX_L = 255;
int S_MIN = 47;
int S_MAX = 255;
int V_MIN = 87;
int V_MAX = 218;
int erode_MIN = 4;
int erode_MAX = 10;
int dialate_MIN = 8;
int dialate_MAX = 25;
int cannyThresh = 200;
int accumulatorThresh = 50;
int bottomLimit = 0;
int max_radius = 0;
int x_location = 0;
int y_location = 0;
double blob_area = 0;

int TARGET_FLAG = 1;
 
int NO_TENNIS_BALL = 1;

int SWEEPER_ON = 0;

cv::Point current_center;

const string trackbarWindowName = "Trackbars";

// callback for ir beam break sensor 
void irBeamSensorCallback(const std_msgs::String::ConstPtr& msg)
{
	ROS_INFO("I heard: [%s]", msg->data.c_str());

	// test to see if msg contains just the string message 
	cout<< "message recieved is: " << msg << endl;

	// if so ... set TENNIS_BALL and LAUNCHER as flags to signal GPIO directions

}


void on_trackbar( int, void* )
{
    //std::cout << " erd_Min: " << erode_MIN << " erd_Max: " << erode_MAX << " dlatd_Min: " << dialate_MIN << " dlated_Max: " << dialate_MAX << " H_Min: " << H_MIN << " H_Max: " << H_MAX << " S_Min: " << S_MIN << " S_Max: " << S_MAX << " V_Min: " << V_MIN << " V_Max: " << V_MAX <<std::endl;	
}

void createTrackbars()
{
    //create window for trackbars
    
    
    namedWindow(trackbarWindowName,0);
    //create memory to store trackbar name on window
    char TrackbarName[50];
//    sprintf( TrackbarName, "H_MIN", H_MIN);
//    sprintf( TrackbarName, "H_MAX", H_MAX);
//    sprintf( TrackbarName, "S_MIN", S_MIN);
//    sprintf( TrackbarName, "S_MAX", S_MAX);
//    sprintf( TrackbarName, "V_MIN", V_MIN);
//    sprintf( TrackbarName, "V_MAX", V_MAX);
    //create trackbars and insert them into window
    //3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
    //the max value the trackbar can move (eg. H_HIGH),
    //and the function that is called whenever the trackbar is moved(eg. on_trackbar)
    //                                  ---->    ---->     ---->
    
    createTrackbar( "Erode_Min", trackbarWindowName, &erode_MIN, erode_MAX, on_trackbar );
    createTrackbar( "HEIGHT LIMIT" , trackbarWindowName, &bottomLimit, 100, on_trackbar);
    createTrackbar( "Dialate_Min", trackbarWindowName, &dialate_MIN, dialate_MAX, on_trackbar );
    createTrackbar( "Dialate_Max", trackbarWindowName, &dialate_MAX, 25, on_trackbar);
    createTrackbar( "H_Min", trackbarWindowName, &H_MIN_B, 180, on_trackbar );
    createTrackbar( "H_Max", trackbarWindowName, &H_MAX_B, 180, on_trackbar );
    createTrackbar( "S_Min", trackbarWindowName, &S_MIN, 255, on_trackbar );
    createTrackbar( "S_Max", trackbarWindowName, &S_MAX, 255, on_trackbar );
    createTrackbar( "V_Min", trackbarWindowName, &V_MIN, 255, on_trackbar );
    createTrackbar( "V_Max", trackbarWindowName, &V_MAX, 255, on_trackbar ); 
    createTrackbar( "Canny_Thresh", trackbarWindowName, &cannyThresh, MaxCannyThresh, on_trackbar);
    createTrackbar( "Accumulator_Thresh",  trackbarWindowName, &accumulatorThresh, MaxAccumulatorThresh, on_trackbar);
}


void imageCallback(const sensor_msgs::ImageConstPtr& msg)
{
    cv::Mat image;
  try
  {
	  image = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8)->image;
  //imagePtr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
  }
  catch (cv_bridge::Exception& e)
  {
    ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
  }

  
   cv::Mat test = image.clone(); 

  cvtColor(image, image, CV_BGR2HSV);
  cv::Size size = image.size();
  cv::inRange(image, cv::Scalar(29,47,87,0),
                    cv::Scalar(64,255,218,0), image);
  cv::Mat erodeElement = getStructuringElement( cv::MORPH_ELLIPSE,cv::Size(3,3));
  
     //dilate with larger element so make sure object is nicely visible
  cv::Mat dilateElement = getStructuringElement( cv::MORPH_ELLIPSE,cv::Size(8,8));

  cv::erode(image,image,erodeElement);
  cv::erode(image,image,erodeElement);

  cv::dilate(image,image,dilateElement);
  cv::dilate(image,image,dilateElement);
  const cv::Mat img2(size.height,size.width,CV_8UC1);
  cv::addWeighted(image,1.0,NULL,1.0,0,img2);
  cv::GaussianBlur(img2, img2, cv::Size(11,11), 0, 0);
  std::vector<cv::Vec3f> circles;
  cv::HoughCircles(img2,circles,CV_HOUGH_GRADIENT,4,84,200,50,10,42);
  for (int i = 0; i < circles.size(); i++)
  {

  	cv::Point center(round(circles[i][0]),round(circles[i][1]));
        int radius = round(circles[i][2]);

       cv::circle(test,  center, 3,    CV_RGB(0,255,0), -1, CV_AA, 0);
                    cv::circle(test,  center, radius, CV_RGB(255,0,0),  3, CV_AA, 0);

//	resizeWindow("test",1000,1000);
//	imshow("test",test);
	
	ROS_INFO("radius: , %d x = %d y = %d width %d height %d  ", radius,center.x,center.y,size.width,size.height);		

if(radius > 25 && (center.x/size.width) > LEFTLIMIT && (center.x/size.width) < RIGHTLIMIT && (center.y/size.height)>BOTTOMLIMIT)
	{

	ROS_ERROR("SPIN");		
	}                    

  }
 
 imshow("test",test);
 cv::waitKey(10);
}

void ball_detection(Mat &frame)
{
	cout << "entering ball detection" << endl;	
	
	Mat mask_ball,edges;

	cvtColor(frame, edges, CV_BGR2HSV); // edges -- HSV frame
	
        Size size = frame.size();  // size of camera frame

	

	// HSV search for ball
        cv::inRange(edges, cv::Scalar(H_MIN_B,S_MIN,V_MIN,0),
                    cv::Scalar(H_MAX_B,S_MAX,V_MAX,0), mask_ball);

	// erode and dilate elements for mask_ball
	//
        Mat erodeElement_b = getStructuringElement( MORPH_ELLIPSE,Size(erode_MIN,erode_MIN));
        //dilate with larger element so make sure object is nicely visible
        Mat dilateElement_b = getStructuringElement( MORPH_ELLIPSE,Size(dialate_MIN,dialate_MIN));
        
        erode(mask_ball,mask_ball,erodeElement_b);
        erode(mask_ball,mask_ball,erodeElement_b);
        
        dilate(mask_ball,mask_ball,dilateElement_b);
        dilate(mask_ball,mask_ball,dilateElement_b);

	
	

	// new frame img2 compatible with Hough Circles -- for ball detection only
	Mat img2(size.height,size.width,CV_8UC1);
	
        cv::addWeighted(mask_ball,1.0,NULL,1.0,0,img2);
        GaussianBlur(img2, img2, Size(11,11), 0, 0);
        std::vector<cv::Vec3f> circles;
        HoughCircles(img2,circles,CV_HOUGH_GRADIENT,4,84,cannyThresh,accumulatorThresh,15,42);

	

	 	for (int i = 0; i < circles.size(); i++) 
		{
                    cv::Point center(round(circles[i][0]),round(circles[i][1]));
                    int radius = round(circles[i][2]);
        	    cv::circle(img2,  center, 3,    CV_RGB(0,255,0), -1, CV_AA, 0);
                    cv::circle(img2,  center, radius, CV_RGB(255,0,0),  3, CV_AA, 0);
		
		    if(radius > max_radius)
		    {
			max_radius = radius;
			
			x_location = center.x;
			y_location = center.y;
			current_center = center;
		    }	

         	}

//		cv::circle(testFrame,  current_center, 3,    CV_RGB(0,255,0), -1, CV_AA, 0);
 //       	cv::circle(testFrame, current_center, max_radius, CV_RGB(255,0,0),  3, CV_AA, 0);
		float bottomLimits = (float)(bottomLimit)/100;

//		cout<< "size of the frame is:  "<< (current_center.y/size.height) << "bottomLimits " << bottomLimits<<endl;

		// if the ball is close enough to the robot to collect
		if(max_radius > 25 && ((float)current_center.x/size.width) > LEFTLIMIT && ((float)current_center.x/size.width) < RIGHTLIMIT && ((float)current_center.y/size.height)>0.71)
		{
		// turn on sweeper 
	
		cout << "turing on sweeper" << endl;		
		SWEEPER_ON = 1;		

		ROS_ERROR("SPIN");
		gpioSetValue(En, 1);
		gpioSetValue(C, 1);
		gpioSetValue(D, 0);		
		
			
		}// end of if  

	max_radius = 0;
	
	cout << "leaving ball detection" << endl;
        
}

void ball_collected()
{
	cout << "turning off sweeper" << endl;	
	SWEEPER_ON = 0;	
		
	// in callback, when "ball" message recieved set NO_TENNIS_BALL = 0;	

	// tennis ball has been collected so turn off sweeper
	gpioSetValue(C, 0);
		
	// if ball was collected dont try to collect until its released
	TARGET_FLAG = 0;
}




//void launcher_detection(Mat threshold, Mat &cameraFeed)
void launcher_detection(Mat &frame)
{
	cout << "entering launcher detection" << endl;
	
	Mat mask_launcher,edges;	

	cvtColor(frame, edges, CV_BGR2HSV); // edges -- HSV frame
	
        Size size = frame.size();  // size of camera frame

	// HSV search for launcher
	cv::inRange(edges, cv::Scalar(H_MIN_L,S_MIN,V_MIN,0),
                    cv::Scalar(H_MAX_L,S_MAX,V_MAX,0), mask_launcher);

	// erode and dilate elements for mask_launcher
	//
        Mat erodeElement_l = getStructuringElement( MORPH_RECT,Size(erode_MIN,erode_MIN));
        //dilate with larger element so make sure object is nicely visible
        Mat dilateElement_l = getStructuringElement( MORPH_RECT,Size(dialate_MIN,dialate_MIN));
        
        erode(mask_launcher,mask_launcher,erodeElement_l);
        erode(mask_launcher,mask_launcher,erodeElement_l);
        
        dilate(mask_launcher,mask_launcher,dilateElement_l);
        dilate(mask_launcher,mask_launcher,dilateElement_l);

	
        


	Mat temp;
	int x = 0, y=0;

    	mask_launcher.copyTo(temp);

    	//these two vectors needed for output of findContours
    	vector< vector<Point> > contours;
    	vector<Vec4i> hierarchy;
    	//find contours of filtered image using openCV findContours function
    	findContours(temp,contours,hierarchy,CV_RETR_CCOMP,CV_CHAIN_APPROX_SIMPLE );
    	//use moments method to find our filtered object
    	double refArea = 0;
    	bool objectFound = false;
    	if (hierarchy.size() > 0) 
	{
        	int numObjects = hierarchy.size();
        	//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
        	if(numObjects<MAX_NUM_OBJECTS)
		{
           	 for (int index = 0; index >= 0; index = hierarchy[index][0]) 
		 {
                
                    Moments moment = moments((cv::Mat)contours[index]);
                    double area = moment.m00;
                
                	//if the area is less than 20 px by 20px then it is probably just noise
                	//if the area is the same as the 3/2 of the image size, probably just a bad filter
                	//we only want the object with the largest area so we safe a reference area each
                	//iteration and compare it to the area in the next iteration.
                	if(area>MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea)
			{
                    	x = moment.m10/area;
                    	y = moment.m01/area;
                    	objectFound = true;
                    	refArea = area;
                	}
			else objectFound = false;
                
                    cout << "blob area is: " << area << endl;
		    blob_area = area;
            	}
            	//let user know you found an object
            	//if(objectFound ==true)
		//
                	//putText(cameraFeed,"Tracking Object",Point(0,50),2,1,Scalar(0,255,0),2);
                	//draw object location on screen
                	//drawObject(x,y,cameraFeed);}
            
        	//}
		//else putText(cameraFeed,"TOO MUCH NOISE! ADJUST FILTER",Point(0,50),1,2,Scalar(0,0,255),2);
		} // end of if

	}


	// if close enough to launcher marker release the ball
		if(objectFound && blob_area >= 14500.0 )
		{
			cout << "turning on sweeper backward" << endl;			

			ROS_ERROR("RELEASE");
			gpioSetValue(En, 1);
			gpioSetValue(C, 0);
			gpioSetValue(D, 1);
			
			// usleep(2000000);
			gpioSetValue(S, 1);
			
			for(int a = 0; a < 2; a++)
			{
				cout << "looping" ;
	
				usleep(2000000);
			}

			gpioSetValue(D, 0);
                        gpioSetValue(S,0);
                        gpioSetValue(En,0);
			cout << "turning off backward sweeper" << endl;			

			// if ball was released then get ready to collect the next ball
			TARGET_FLAG = 1;
		}
		
		cout << "leaving launcher detection" << endl;
	
}




int main(int argc, char **argv)
{
	// if testing, set to true to display trackbars
	bool testing = false;
	
	ros::init(argc, argv, "ball_detector");

	ros::NodeHandle nh;

	ros::Subscriber sub = nh.subscribe("irbeam",15, irBeamSensorCallback);
	
        image_transport::ImageTransport transport(nh);

	gpioExport(C);
	gpioExport(D);
	gpioExport(En);
	gpioExport(S);


	gpioSetValue(En, 0);
	gpioSetValue(C, 0);
	gpioSetValue(D, 0);
	gpioSetValue(S, 0);
	

 	VideoCapture cap; // open the usb webcam, device 0

    	cap.open(1); 

    	if(!cap.isOpened())  // check if we succeeded
    	{
        	std::cout <<"errror";
        	return -1;
    	}
	
    	if(testing) // if testing show trackbars
    	{
     		createTrackbars();
    	}


	// setting directions for GPIO's
	gpioSetDirection(C, 1);
	gpioSetDirection(D, 1);
	gpioSetDirection(En, 1);
	gpioSetDirection(S, 1); 


    while(ros::ok())
    {
	
	Mat frame;
	
        cap >> frame; // get a new frame from camera

	// call functions for ball detection and launcher detection here
	if(TARGET_FLAG)
	{
		// call ball_detection
		ball_detection(frame);
		
		// if sweeper was turned on
		if(SWEEPER_ON)
		{
			// while no tennis ball is detected
			while(NO_TENNIS_BALL)
			{
				ros::spinOnce();
			}
			
			// once there is a ball call the sweeper to turn off
			ball_collected();	
		}
		
	}
	else
	{
		// call launcher_detection
		launcher_detection(frame);
	}	


    } // end of while loop


	gpioUnexport(C);
	gpioUnexport(D);
	gpioUnexport(En);
	gpioUnexport(S);

	return 0;
}





