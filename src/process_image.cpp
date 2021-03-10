#include "ros/ros.h"
#include "sensor_msgs/Image.h"
#include "cv_bridge/cv_bridge.h"
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <iostream>

#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "alexsm_ball_chaser/DriveToTarget.h"

using alexsm_ball_chaser::DriveToTarget;

const int N_PIXELS = 640000; // 800 x 800
const char* OPENCV_WINDOW = "stream";

struct Ball {
    double x;
    double y;
    double areaRatio;
};

ros::ServiceClient client;

void call_service(double linear_x, double angular_z) {
    
    DriveToTarget srv;
    srv.request.linear_x = linear_x;
    srv.request.angular_z = angular_z;

    bool ok = client.call(srv);

    if (!ok) {
        ROS_ERROR("Failed to call service /ball_chaser/command_robot");
    } 

}

cv::Mat threshold(const cv::Mat &im) {
    
    cv::Mat im_gray;
    cv::cvtColor(im, im_gray, cv::COLOR_BGR2GRAY);

    cv::Mat im_mask;
    cv::threshold(im_gray, im_mask, 250, 255, cv::THRESH_BINARY);

    return im_mask;
}

Ball find_ball(const cv::Mat &im_mask) {

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;

    cv::connectedComponentsWithStats(im_mask, labels, stats, centroids);

    Ball ball{};
    
    ball.x = centroids.row(1).at<double>(0);
    ball.y = centroids.row(1).at<double>(1);

    int area = stats.at<int>(1, cv::CC_STAT_AREA);
    ball.areaRatio = ((double) area) / (double(N_PIXELS));

    return ball;
}

void improc_callback(const sensor_msgs::Image ros_im) {

    cv_bridge::CvImagePtr im;
    im = cv_bridge::toCvCopy(ros_im, sensor_msgs::image_encodings::BGR8);

    cv::imshow(OPENCV_WINDOW, im->image);
    cv::waitKey(3);

    cv::Mat im_mask = threshold(im->image);

    int n_white = cv::countNonZero(im_mask);

    if (n_white == 0) {
        
        // do not move
        call_service(0, 0);

        return;
    }

    Ball ball = find_ball(im_mask);
    // TODO call serice to move towards the ball
}

int main(int argc, char* argv[]) {

    ros::init(argc, argv, "process_image");
    ros::NodeHandle this_node;

    cv::namedWindow(OPENCV_WINDOW, cv::WINDOW_AUTOSIZE);

    client = this_node.serviceClient<alexsm_ball_chaser::DriveToTarget>(
        "ball_chaser/command_robot"
    );

    ros::Subscriber sub = this_node.subscribe(
        "/camera/rgb/image_raw",
        10,
        improc_callback
    );

    ros::spin();

    return 0;
}