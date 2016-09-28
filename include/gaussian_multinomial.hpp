#ifndef GAUSSIAN_MULTINOMIAL_H
#define GAUSSIAN_MULTINOMIAL_H

#include <iostream>
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include "../include/gaussian.hpp"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/eigen.hpp>

using namespace std;
using namespace Eigen;
using namespace cv;

class GaussianMultinomial{
	public:
		GaussianMultinomial();
		GaussianMultinomial(Mat& _positiveFeatureValue, Mat& _negativeFeatureValue);
		void fit();
		float test(int index_particle);
		void setSampleFeatureValue(Mat& _sampleFeatureValue);
		vector<Gaussian> positive_likelihood,negative_likelihood;
		VectorXd theta_y_mu, theta_y_sigma;
	protected:
		bool initialized;
    	Mat *sampleFeatureValue;
    	Mat *negativeFeatureValue;
};

#endif