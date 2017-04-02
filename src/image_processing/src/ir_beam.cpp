#include "ros/ros.h"
#include <ros/console.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include "std_msgs/String.h"
#include "image_processing/jetsonGPIO.h"
#include "std_msgs/Int8.h"

using namespace std;

jetsonTX1GPIONumber T = gpio186;
unsigned int value = 0;

	
int main(int argc, char **argv)
{
	
	ros::init(argc, argv, "ir_beam");

	ros::NodeHandle nh;

	gpioExport(T);

	ros::Publisher irsensor_pub = nh.advertise<std_msgs::Int8>("irbeam", 5);

	ros::Rate loop_rate(10);
	
	// usleep(200000);

	gpioSetDirection(T, 0);

	while(ros::ok())
	{

	//std_msgs::String msg;
	
	std_msgs::Int8 msg;

	//std::stringstream ss;

	//cout << "while loop" << endl;

	gpioGetValue(T, &value);

	cout << "value is: " << value << endl;

	if(value == 0)
	{
		//ss << "empty" << cout ;
		
		msg.data = 0;
		
		cout << "message int is: " << msg.data << endl;	

		ROS_INFO("%d", msg.data);
		
		irsensor_pub.publish(msg);
	}
	else
	{	
		//ss << "ball" << cout;

		//msg.data = ss.str();
		msg.data = 1;
		cout << "message string is: " << msg.data << endl;

		//ROS_INFO("%s", msg.data.c_str());

		irsensor_pub.publish(msg);
	}

//	msg.data = ss.str();
//
//	ROS_INFO("%s", msg.data.c_str());

//	irsensor_pub.publish(msg);

	ros::spinOnce();

	loop_rate.sleep();

	}//end of while	

	gpioUnexport(T);

	return 0;
}		
