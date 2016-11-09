#include <iostream>
#include <stdlib.h>
#include <cfloat>
#include <cmath>
#include <algorithm>    // std::max
#include <vector>
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Cholesky>
#include "multivariate_gaussian.hpp"
#include "../libs/cppoptlib/meta.h"
#include "../libs/cppoptlib/problem.h"
#include "../libs/cppoptlib/solver/bfgssolver.h"
#include "../libs/cppoptlib/solver/gradientdescentsolver.h"

using namespace Eigen;
using namespace std;

class LogisticRegression
{
 public:
	LogisticRegression();
	LogisticRegression(MatrixXd &_X,VectorXd &_Y,double lambda=1.0);
 	VectorXd Train(int n_iter,double alpha=0.01,double tol=0.001);
 	VectorXd Predict(MatrixXd &_X);
 	double foo(RowVectorXd& _weights);
 	VectorXd foo2(RowVectorXd& _weights);
 	void setWeights(VectorXd &_W);
 	VectorXd getWeights();


 private:
 	RowVectorXd weights;
 	MatrixXd *X_train;
 	VectorXd *Y_train;
 	int rows,dim;
 	double lambda;
 	VectorXd featureMeans;
 	VectorXd Sigmoid(VectorXd &_eta);
 	VectorXd LogSigmoid(VectorXd &_eta);
 	MatrixXd ComputeHessian(MatrixXd &_X,  VectorXd &_Y,RowVectorXd &_W);
 	VectorXd ComputeGradient(MatrixXd &_X, VectorXd &_Y,RowVectorXd &_W);
 	double LogPrior(RowVectorXd &_W);
 	double LogLikelihood(MatrixXd &_X,VectorXd &_Y,RowVectorXd &_W);
 	MatrixXd Hessian;
};


template<typename T>
class LogisticRegressionWrapper : public cppoptlib::Problem<T> {
  public:
    using typename cppoptlib::Problem<T>::TVector;
    LogisticRegression *logistic;
    Eigen::MatrixXd x_train;
    Eigen::VectorXd y_train;

    LogisticRegressionWrapper(const MatrixXd &X_, const VectorXd &y_,double _lambda) {
      x_train=X_;
      y_train=y_;
      logistic=new LogisticRegression(x_train,y_train,_lambda);
    }

    T value(const TVector &beta) {
        Eigen::RowVectorXd w=beta.transpose();
        return logistic->foo(w);
    }

    void gradient(const TVector &beta, TVector &grad) {
        Eigen::RowVectorXd w=beta.transpose();
        grad = logistic->foo2(w);
    }

    int getDim(){
    	return logistic->getWeights().size();
    }
};
