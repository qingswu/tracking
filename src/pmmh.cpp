#include "../include/pmmh.hpp"

pmmh::pmmh(int _num_particles,int _fixed_lag,int _mcmc_steps){
    unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed1);
    num_particles=_num_particles;
    fixed_lag=_fixed_lag;
    mcmc_steps=_mcmc_steps;
    alpha.setOnes((int)H_BINS*S_BINS);
    alpha.normalize();
    color_prior=dirichlet(alpha);
    pos_prior=Gaussian(0.0,1.0);
    vel_prior=Gaussian(0.0,1.0);
    scale_prior=Gaussian(0.0,1.0);
    initialized=false;
}

pmmh::~pmmh(){
    //delete filter;
    images = vector<Mat>();
}

void pmmh::initialize(Mat& current_frame, Rect ground_truth){
    images = vector<Mat>();
    filter=new particle_filter(num_particles);
    filter->initialize(current_frame,ground_truth);
    images.push_back(current_frame);
    reference_roi=ground_truth;
    theta_y=filter->get_discrete_model();
    theta_x=filter->get_continuous_model();
    initialized=true;
}

bool pmmh::is_initialized(){
    return initialized;
}

void pmmh::reinitialize(){
    delete filter;
    initialized=false;
}

double pmmh::marginal_likelihood(VectorXd theta_x,VectorXd theta_y){
    particle_filter pmmh(num_particles);
    Mat current_frame = images[0].clone(); 
    pmmh.initialize(current_frame,reference_roi);
    pmmh.update_model(theta_x,theta_y);
    int time_step=MIN((int)images.size(),fixed_lag);
    //int start_time;
    //(fixed_lag==0) || (time_step<fixed_lag) ? start_time=0 : start_time=time_step-fixed_lag;
    //cout << "------------------------"  << endl;
    for(int k=1;k<time_step;k++){
        //cout << "time:" << k << endl;
        Mat current_frame = images[k].clone();
        pmmh.predict();
        pmmh.update_discrete(current_frame);
    }
    double res=pmmh.getMarginalLikelihood();
    return res;
}

VectorXd pmmh::discrete_proposal(VectorXd alpha){
    VectorXd proposal(alpha.size());
    double eps= std::numeric_limits<double>::epsilon();
    for(int i=0;i<alpha.size();i++){
        gamma_distribution<double> color_prior(alpha[i],1.0);
        double val=color_prior(generator);
        proposal[i] = (val>0.0) ? val : eps;
    }
    proposal.normalize();
    return proposal;
}

VectorXd pmmh::continuous_proposal(VectorXd alpha){
    VectorXd proposal(alpha.size());
    double eps= std::numeric_limits<double>::epsilon();
    for(int i=0;i<alpha.size();i++){
        normal_distribution<double> random_walk(alpha(i),0.1);
        double val=MAX(random_walk(generator),eps);
        proposal[i] = val;
    }
    return proposal;
}

double pmmh::gamma_prior(VectorXd x, VectorXd a, double b)
{
    double loglike=0.0;
    for(int i=0;i<a.size();i++){
        if (x(i) >= 0 || a(i) >= 0 || b >= 0){
            loglike+=-x(i)*b+(a(i)-1.0)*log(x(i))+a(i)*log(b)-lgamma(a(i));
        }
    }
    return loglike;
}

void pmmh::update(Mat& current_frame){
    uniform_real_distribution<double> unif_rnd(0.0,1.0);
    filter->predict();
    filter->update_discrete(current_frame);
    //double forward_filter = filter->getMarginalLikelihood();
    images.push_back(current_frame);
    double forward_filter = marginal_likelihood(theta_x,theta_y);
    for(int n=0;n<mcmc_steps;n++){
        theta_y_prop=discrete_proposal(theta_y);
        theta_x_prop=continuous_proposal(theta_x);
        double proposal_filter = marginal_likelihood(theta_x_prop,theta_y_prop);
        //
        double acceptprob = proposal_filter - forward_filter;
        acceptprob+=color_prior.log_likelihood(theta_y_prop)-color_prior.log_likelihood(theta_y);
        acceptprob+=pos_prior.log_likelihood(theta_x_prop(0))-pos_prior.log_likelihood(theta_x(0));
        acceptprob+=vel_prior.log_likelihood(theta_x_prop(1))-vel_prior.log_likelihood(theta_x(1));
        acceptprob+=scale_prior.log_likelihood(theta_x_prop(2))-scale_prior.log_likelihood(theta_x(2));
        //cout << "forward_filter:" << forward_filter << ",proposal_filter:" << proposal_filter << endl;
        double u=unif_rnd(generator);
        if( log(u) < acceptprob){
            //cout << "accept!" << endl;
            theta_y=theta_y_prop;
            theta_x=theta_x_prop;
            filter->update_model(theta_x_prop,theta_y_prop);
            forward_filter=proposal_filter;
            }
    }
    
}

Rect pmmh::estimate(Mat& image,bool draw){
    return filter->estimate(image,draw);
}