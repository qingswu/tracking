#ifndef MULTINOMIAL_H
#define MULTINOMIAL_H

#include <stdlib.h>
#include <cmath>
#include "../Eigen/Core"
#include "../Eigen/Dense"
#include <iostream>

using namespace Eigen;

class Multinomial
{
public:
    Multinomial();
    Multinomial(MatrixXd &counts);
    Multinomial(MatrixXd &counts, double alpha);
    double log_likelihood(VectorXd test);

    VectorXd getTheta() const;
    void setTheta(const VectorXd &value);


private:
    VectorXd theta;
};

#endif // MULTINOMIAL_H
