#include "CalculateCollisionProbabilityFast.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include <math/mvn.hpp>

CalculateCollisionProbabilityFast::CalculateCollisionProbabilityFast(std::string funName, double costWeight, std::map<int, PredictedObject> predictions, double vehicleLength, double vehicleWidth)
    : CostStrategy(funName, costWeight)
    , m_predictions(predictions)
    , m_vehicleLength(vehicleLength)
    , m_vehicleWidth(vehicleWidth)
{
    std::cerr << "WARNING: This version of CalculateCollisionProbabilityFast is not yet complete! Check results carefully." << std::endl;
}

void CalculateCollisionProbabilityFast::evaluateTrajectory(TrajectorySample& trajectory)
{
    double cost = 0.0;

    Eigen::Vector2d offset(m_vehicleLength / 2.0, m_vehicleWidth / 2.0);

    for (const auto& [obstacle_id, prediction] : m_predictions) {

        std::vector<double> inv_dist;

        for (int i = 1; i < trajectory.m_cartesianSample.x.size(); ++i)
        {
            if (i >= prediction.predictedPath.size()) { break; }

            Eigen::Vector2d u(trajectory.m_cartesianSample.x[i], trajectory.m_cartesianSample.y[i]);
            Eigen::AlignedBox2d box { u - offset, u + offset };

            const auto& pose = prediction.predictedPath.at(i-1);
            Eigen::Vector2d v = pose.position.head<2>();

            // Check if the distance between the vehicles is larger than ~7 meters
            // If true, skip calculating the probability since it will be very low
            //
            // NOTE: Adapted from Python code, but with a large threshold to be safe
            // since the compared points aren't exactly the same
            // (exterior distance vs center distance, 3 box vs 1 box)
            if (box.squaredExteriorDistance(v) > 50.0) {
                continue;
            }

            // Rotate covariance matrix to account for ego vehicle orientation
            Eigen::Rotation2D cov_rot(-trajectory.m_cartesianSample.theta[i]);
            Eigen::Matrix2d cov_rot_mat { cov_rot.toRotationMatrix() };
            Eigen::Matrix2d cov = cov_rot_mat * pose.covariance.topLeftCorner<2,2>() * cov_rot_mat.transpose();
            // Eigen::Matrix2d cov = pose.covariance.topLeftCorner<2,2>();


            double bvcost = bvn_prob(box, v, cov);
            cost += bvcost;
            assert(!std::isnan(cost));
        }
    }

    assert(!std::isnan(cost));

    trajectory.addCostValueToList(m_functionName, cost, cost*m_costWeight);
}

void CalculateCollisionProbabilityFast::printPredictions()
{
    // std::cout << "Predictions: " << std::endl;
}

