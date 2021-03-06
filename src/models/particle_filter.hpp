#ifndef PARTICLE_FILTER
#define PARTICLE_FILTER


#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <Eigen/Dense>
#include <opencv2/core/eigen.hpp>
#include <time.h>
#include <float.h>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <fftw3.h>

#include "../features/haar.hpp"
#include "../likelihood/gaussian.hpp"
#include "../features/mb_lbp.hpp"
#include "../likelihood/logistic_regression.hpp"
#include "../likelihood/hamiltonian_monte_carlo.hpp"
//#include "../likelihood/gaussian_naivebayes.hpp"
#include "../likelihood/multinomialnaivebayes.hpp"
#include "../likelihood/incremental_gaussiannaivebayes.hpp"
//#include "../likelihood/weighted_gaussiannaivebayes.hpp"
#include "../features/local_binary_pattern.hpp"
#include "../features/hog.hpp"

extern const float POS_STD; 
extern const float VEL_STD; 
extern const float SCALE_STD; 
extern const float  DT; 
extern const float  THRESHOLD; 

using namespace cv;
using namespace std;
using namespace Eigen;

typedef struct particle {
    float x; /** current x coordinate */
    float y; /** current y coordinate */
    float width; /** current width coordinate */
    float height; /** current height coordinate */
    float scale; /** current velocity bounding box scale */
    float x_p; /** current x coordinate */
    float y_p; /** current y coordinate */
    float width_p; /** current width coordinate */
    float height_p; /** current height coordinate */
    float scale_p; /** current velocity bounding box scale */
} particle;

class particle_filter {
public:
    int n_particles;
    vector<particle> states;
    vector<float>  weights;
    ~particle_filter();
    particle_filter(int _n_particles);
    particle_filter();
    int time_stamp;
    bool is_initialized();
    void reinitialize();
    void initialize(Mat& current_frame, Rect ground_truth);
    void draw_particles(Mat& image, Scalar color);
    Rect estimate(Mat& image,bool draw);
    void predict();
    void update(Mat& image);
    void smoother(int fixed_lag);
    void update_model(vector<VectorXd> theta_x);
    void update_model(Mat& image,vector<Rect> positive_examples,vector<Rect> negative_examples);
    vector<VectorXd> get_dynamic_model();
    vector<VectorXd> get_observation_model();
    float getESS();
    float getMarginalLikelihood();
    float resample();
    vector<Rect> estimates;
    particle update_state(particle state, Mat& image);
    Haar haar;

protected:
    float marginal_likelihood;
    vector<VectorXd> theta_x;
    vector<VectorXd> theta_y;
    vector<Gaussian> positive_likelihood,negative_likelihood;
    float ESS;
    bool initialized;
    mt19937 generator;
    Rect reference_roi;
    Size im_size;
    Mat reference_hist;
    normal_distribution<double> position_random_walk,velocity_random_walk,scale_random_walk;
    double eps;
    vector<Rect > sampleBox;
    LocalBinaryPattern local_binary_pattern;
    MultiScaleBlockLBP multiblock_local_binary_patterns;
    LogisticRegression logistic_regression;
    MultinomialNaiveBayes multinomial_naivebayes;
    GaussianNaiveBayes gaussian_naivebayes;
    Hamiltonian_MC hamiltonian_monte_carlo;
    //IncrementalGaussianNaiveBayes incremental_gaussian_naivebayes;
};

#endif
