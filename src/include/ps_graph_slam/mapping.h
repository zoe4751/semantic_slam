#ifndef MAPPING_H
#define MAPPING_H

#include <Eigen/Dense>

#include <ros/ros.h>
#include <tf_conversions/tf_eigen.h>
#include "ps_graph_slam/keyframe.hpp"

//PCL ROS
#include <pcl_conversions/pcl_conversions.h>
#include <pcl_ros/point_cloud.h>

#include "pcl/point_types.h"
#include <pcl/filters/project_inliers.h>
#include <pcl/octree/octree_search.h>

#include "ps_graph_slam/ros_utils.hpp"
#include "ps_graph_slam/map_cloud.h"
#include "tools.h"

class mapping
{

public:
    mapping(float cam_angle)
    {

        std::cout << "Initialized mapping thread " << std::endl;
        this->init();
    }
    ~mapping()
    {

    }

private:
    float cam_angle_;
    std::vector<pcl::PointCloud<pcl::PointXYZRGB>::Ptr> out_cloud_;
    std::vector<map_cloud> map_cloud_vec_;

    std::deque<ps_graph_slam::KeyFrame::Ptr> new_keyframes_;
    std::vector<ps_graph_slam::KeyFrame::Ptr> keyframes_optimized_;
    std::mutex keyframe_lock_, cloud_lock_, opt_keyframe_lock_;


private:
    bool add_new_keyframe_;

private:
    void init()
    {
        out_cloud_.clear();
        map_cloud_vec_.clear();
        //out_cloud_ .reset(new pcl::PointCloud<pcl::PointXYZRGB>);
        //out_cloud_->points.clear();
        new_keyframes_.clear();
        add_new_keyframe_ = false;
    }


public:
    void setKeyframes(std::deque<ps_graph_slam::KeyFrame::Ptr> keyframes)
    {
        keyframe_lock_.lock();
        for(int i = 0; i< keyframes.size(); ++i)
        {
            new_keyframes_.push_back(keyframes[i]);
            keyframe_lock_.unlock();
        }
        keyframe_lock_.unlock();
    }

    void optimizedKeyframes(std::vector<ps_graph_slam::KeyFrame::Ptr> keyframes_optimized)
    {
        opt_keyframe_lock_.lock();
        keyframes_optimized_ = keyframes_optimized;
        opt_keyframe_lock_.unlock();
    }

    void generateMap()
    {
        while(1)
        {
            keyframe_lock_.lock();
            std::deque<ps_graph_slam::KeyFrame::Ptr> keyframes;
            keyframes = new_keyframes_;
            new_keyframes_.clear();
            keyframe_lock_.unlock();

            for(int i=0; i< keyframes.size(); ++i)
            {
                pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud (new pcl::PointCloud<pcl::PointXYZRGB>());

                //pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_cloud (new pcl::PointCloud<pcl::PointXYZRGB>());
                Eigen::MatrixXf pose =  ps_graph_slam::matrix2vector(keyframes[i]->robot_pose.matrix().cast<float>());

                Eigen::Matrix4f transformation_mat;
                semantic_tools sem_tool_obj;
                sem_tool_obj.transformNormalsToWorld(pose,
                                                     transformation_mat,
                                                     cam_angle_);

                for(int j = 0; j< keyframes[i]->cloud->points.size(); ++j)
                {
                    Eigen::Matrix4f cam_points, map_points;
                    cam_points.setOnes(), map_points.setOnes();

                    cam_points(0) = keyframes[i]->cloud->points[j].x;
                    cam_points(1) = keyframes[i]->cloud->points[j].y;
                    cam_points(2) = keyframes[i]->cloud->points[j].z;

                    map_points = transformation_mat * cam_points;

                    pcl::PointXYZRGB dst_pt;
                    dst_pt.x = map_points(0) + pose(0);
                    dst_pt.y = map_points(1) + pose(1);
                    dst_pt.z = map_points(2) + pose(2);
                    dst_pt.rgb = keyframes[i]->cloud->points[j].rgb;

                    cloud->push_back(dst_pt);
                }

                cloud->width = cloud->size();
                cloud->height = 1;
                cloud->is_dense = false;

                map_cloud current_map_cloud;
                current_map_cloud.keyframe_pose = pose;
                current_map_cloud.out_cloud     = cloud;

                cloud_lock_.lock();
                map_cloud_vec_.push_back(current_map_cloud);
                cloud_lock_.unlock();

                //global_cloud_.push_back(out_cloud);
            }

        }
    }

    void opitmizeMap()
    {
        //        while(1)
        //        {
        //            std::vector<ps_graph_slam::KeyFrame::Ptr> opt_keyframe;
        //            opt_keyframe_lock_.lock();
        //            opt_keyframe = keyframes_optimized_;
        //            keyframes_optimized_.clear();
        //            opt_keyframe_lock_.unlock();

        //            cloud_lock_.lock();
        //            for(int i=0; i< map_cloud_vec_.size(); ++i)
        //            {
        //                //pcl::PointCloud<pcl::PointXYZRGB>::Ptr out_cloud (new pcl::PointCloud<pcl::PointXYZRGB>());
        //                Eigen::MatrixXf pose =  ps_graph_slam::matrix2vector(opt_keyframe[i]->robot_pose.matrix().cast<float>());

        //                Eigen::Matrix4f transformation_mat;
        //                semantic_tools sem_tool_obj;
        //                sem_tool_obj.transformRobotToWorld(pose,
        //                                                   transformation_mat);

        //                for(int j = 0; j< map_cloud_vec_[i].out_cloud->cloud->points.size(); ++j)
        //                {
        //                    Eigen::Matrix4f cam_points, map_points;
        //                    cam_points.setOnes(), map_points.setOnes();

        //                    cam_points(0) = keyframes[i]->cloud->points[j].x;
        //                    cam_points(1) = keyframes[i]->cloud->points[j].y;
        //                    cam_points(2) = keyframes[i]->cloud->points[j].z;

        //                    map_points = transformation_mat * cam_points;
        //                }

        //            }
        //            cloud_lock_.unlock();

        //    }

    }

    std::vector<map_cloud> getOutputMap()
    {
        return map_cloud_vec_;
    }



};

#endif
