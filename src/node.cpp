
/*The MIT License (MIT)
 *
 * Copyright (c) 2017, Scanse, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ros/ros.h"
#include <iostream>
#include <pcl/point_types.h>
#include "sensor_msgs/PointCloud2.h"
#include <pcl/point_cloud.h>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/io/pcd_io.h>
#include <neo/neo.hpp>

void publish_scan(ros::Publisher *pub,
                  const neo::scan *scan, std::string frame_id)
{
    pcl::PointCloud <pcl::PointXYZ> cloud, cloud1;
    sensor_msgs::PointCloud2 cloud_msg;
    static int pcd_index = 0;

    float angle;
    int32_t range;
    float x;
    float y;
    int i = 0;

    cloud.height = 1;
    cloud.width = scan->samples.size();
    cloud.points.resize(cloud.width * cloud.height);

	cloud1.height = 1;
	cloud1.width = scan->samples.size();
	cloud1.points.resize(cloud.width * cloud.height);

    for (const neo::sample& sample : scan->samples)
    {
        range = sample.distance;
        angle = ((float)sample.angle / 1000); //millidegrees to degrees

        //Polar to Cartesian Conversion
        x = (range * cos(DEG2RAD(angle))) / 100;
        y = (range * sin(DEG2RAD(angle))) / 100;

        cloud.points[i].x = x;
        cloud.points[i].y = y;
		//cloud1.points[i].x = range;
		//cloud1.points[i].y = angle;
        i++;
    }
	std::ostringstream s1_polar, s1;
    s1_polar << "/home/tyu/pcl/pcd/test_polar_" << pcd_index << ".pcd";
    s1 << "/home/tyu/pcl/pcd/test_" << pcd_index << ".pcd";
	pcd_index++;
	// std::string s2 = s1.str();
    //pcl::io::savePCDFileASCII(s1_polar.str(), cloud1);
    //pcl::io::savePCDFileASCII(s1.str(), cloud);

    //Convert pcl PC to ROS PC2
    pcl::toROSMsg(cloud, cloud_msg);
    cloud_msg.header.frame_id = frame_id;

    ROS_DEBUG("Publishing a full scan");
    pub->publish(cloud_msg);
}


int main(int argc, char *argv[]) try
{
    //Initialize Node and handles
    ros::init(argc, argv, "neo_node");
    ros::NodeHandle nh;
    ros::NodeHandle nh_private("~");

    //Get Serial Parameters
    std::string serial_port;
    nh_private.param<std::string>("serial_port", serial_port, "/dev/ttyUSB0");
    int serial_baudrate;
    nh_private.param<int>("serial_baudrate", serial_baudrate, 115200);

    //Get Scanner Parameters
    int rotation_speed;
    nh_private.param<int>("rotation_speed", rotation_speed, 5);

    //Get frame id Parameters
    std::string frame_id;
    nh_private.param<std::string>("frame_id", frame_id, "laser_frame");

    //Setup Publisher
    ros::Publisher scan_pub = nh.advertise<sensor_msgs::PointCloud2>("pc2", 1000);

    //Create Neo Driver Object
    neo::neo device{serial_port.c_str()};
    ROS_INFO("Device connect successful!");

    //Send Rotation Speed
    device.set_motor_speed(rotation_speed);

    ROS_INFO("expected rotation frequency: %d (Hz)", rotation_speed);

    //Start Scan
    device.start_scanning();

    while (ros::ok())
    {
        //Grab Full Scan
        const neo::scan scan = device.get_scan();

        publish_scan(&scan_pub, &scan, frame_id);

        ros::spinOnce();
    }

    //Stop Scanning & Destroy Driver
    device.stop_scanning();
    device.set_motor_speed(0);
} catch (const neo::device_error& e) {
      ROS_ERROR_STREAM("Error: " << e.what() << std::endl);
}