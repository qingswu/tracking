/**
 * @file particle_filter.cpp
 * @brief particle filter
 * @author Sergio Hernandez
 */
 #include "../include/particle_filter.hpp"

particle_filter::particle_filter(int _n_particles) {
    n_particles = _n_particles;
    time_stamp=0;
    initialized=false;
    marginal_likelihood=0.0;
    unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    generator.seed(seed1);
    theta.resize(3);
    theta << POS_STD,VEL_STD,SCALE_STD;
    //rng(0xFFFFFFFF);
}

bool particle_filter::is_initialized() {
    return initialized;
}


void particle_filter::initialize(Rect roi,Size _im_size,Mat& _reference_hist,Mat& _reference_hog) {
    weights.resize(n_particles);
    reference_hist=_reference_hist;
    reference_hog=_reference_hog;
    reference_roi=roi;
    im_size=_im_size;
    marginal_likelihood=0.0;
    normal_distribution<double> position_random_walk(0.0,theta(0));
    normal_distribution<double> velocity_random_walk(0.0,theta(1));
    normal_distribution<double> scale_random_walk(0.0,theta(2));
    for (int i=0;i<n_particles;i++){
        particle state;
        state.x=cvRound(roi.x+position_random_walk(generator));
        state.y=cvRound(roi.y+position_random_walk(generator));
        state.dx+=velocity_random_walk(generator);
        state.dy+=velocity_random_walk(generator);
        state.scale=1.0f+scale_random_walk(generator);
        states.push_back(state);
        weights.push_back(1.f/n_particles);
        ESS=0.0f;
        state.width=cvRound(reference_roi.width*abs(state.scale));
        state.height=cvRound(reference_roi.height*abs(state.scale));
    }
    color_lilekihood=Gaussian(0.0,SIGMA_COLOR);
    hog_likelihood=Gaussian(0.0,SIGMA_SHAPE);
    double eps= std::numeric_limits<double>::epsilon();
    Eigen::VectorXd alpha,alpha_hog;
    alpha.setOnes(reference_hist.total());
    for(int h=0;h<H_BINS;h++)
        for( int s = 0; s < S_BINS; s++ ){
            double val=reference_hist.at<float>(h, s);
            gamma_distribution<double> color_prior(val,1.0);
            alpha[h*S_BINS+s] = (val!=0.0) ? color_prior(generator) : eps;
        }
    alpha_hog.setOnes(reference_hog.total());
    for(unsigned int g=0;g<reference_hog.total();g++){
        double val=reference_hog.at<float>(0,g);
        gamma_distribution<double> hog_prior(val,1.0);
        alpha_hog[g] = (val!=0.0) ? hog_prior(generator) : eps;
    }
    polya = dirichlet(alpha);
    poisson = Poisson(alpha);
    alpha.normalize();
    discrete = Multinomial(alpha);
    polya_hog = dirichlet(alpha_hog);
    alpha_hog.normalize();
    discrete_hog = Multinomial(alpha_hog);
    //cout << "Particle filter initialized" << endl;
    initialized=true;
}

void particle_filter::predict(){
    if(initialized==true){
        time_stamp++;
        vector<particle> tmp_new_states;
        normal_distribution<double> position_random_walk(0.0,theta(0));
        normal_distribution<double> velocity_random_walk(0.0,theta(1));
        normal_distribution<double> scale_random_walk(0.0,theta(2));
        uniform_real_distribution<double> unif_rnd(0.0,1.0);       
        for (int i=0;i<n_particles;i++){
            particle state=states[i];
            uniform_int_distribution<int> unif_width(0,(int)(im_size.width-state.width));
            uniform_int_distribution<int> unif_height(0,(int)(im_size.height-state.height));
            float _x,_y,_dx,_dy,_width,_height;
            _dx=state.dx;
            _dy=state.dy;
            _x=cvRound(state.x+_dx+position_random_walk(generator));
            _y=cvRound(state.y+_dy+position_random_walk(generator));
            _width=cvRound(state.width);
            _height=cvRound(state.height);
            
            if((_x+_width)<im_size.width && _x>=0 && 
                (_y+_height)<im_size.height && _y>=0 && 
                isless(ESS/n_particles,(float)THRESHOLD)){
                state.x=_x;
                state.y=_y;
                state.width=_width;
                state.height=_height;
                state.dx=_dx;
                state.dy=_dy;
                //state.scale+=scale_random_walk(generator);
            }
            else{
                state.dx=velocity_random_walk(generator);
                state.dy=velocity_random_walk(generator);
                state.width=cvRound(reference_roi.width*abs(state.scale));
                state.height=cvRound(reference_roi.height*abs(state.scale));
                double u=unif_rnd(generator);
                if(u<0.5){
                    state.x=cvRound(reference_roi.x+position_random_walk(generator));
                    state.y=cvRound(reference_roi.y+position_random_walk(generator));
                }
                else{
                    double val_x=unif_width(generator);
                    state.x= (val_x>0 && val_x <(im_size.width-state.width)) ? val_x : reference_roi.x;
                    double val_y=unif_height(generator);
                    state.y= (val_y>0 && val_y <(im_size.height-state.height)) ? val_y : reference_roi.y;
                }
                state.scale=1.f+scale_random_walk(generator);
            }
            tmp_new_states.push_back(state);
        }
        states.swap(tmp_new_states);
        tmp_new_states = vector<particle>();
    }
}


void particle_filter::draw_particles(Mat& image){
    for (int i=0;i<n_particles;i++){
        particle state=states[i];
        Point pt1,pt2;
        pt1.x=cvRound(state.x);
        pt1.y=cvRound(state.y);
        pt2.x=cvRound(state.x+state.width);
        pt2.y=cvRound(state.y+state.height);
        rectangle( image, pt1,pt2, Scalar(0,255,255), 1, LINE_AA );
    }
}

Rect particle_filter::estimate(Mat& image,bool draw=false){
    float _x=0.0,_y=0.0,_width=0.0,_height=0.0;
    Rect estimate;
    for (int i=0;i<n_particles;i++){
        particle state=states[i];
        _x+=weights[i]*state.x;
        _y+=weights[i]*state.y;
        _width+=weights[i]*state.width;
        _height+=weights[i]*state.height;
    }
    Point pt1,pt2;
    pt1.x=cvRound(_x);
    pt1.y=cvRound(_y);
    pt2.x=cvRound(_x+_width);
    pt2.y=cvRound(_y+_height);
    //Sorts points
    int aux = 0;
    if (pt1.x > pt2.x){
      aux = pt1.x;
      pt1.x = pt2.x;
      pt2.x = aux;
    }
    if (pt1.y > pt2.y){
      aux = pt1.y;
      pt1.y = pt2.y;
      pt2.y = aux;
    }

    if(draw) rectangle( image, pt1,pt2, Scalar(0,0,255), 1, LINE_AA );
    if(pt2.x<im_size.width && pt1.x>=0 && pt2.y<im_size.height && pt1.y>=0){
        estimate=Rect(pt1.x,pt1.y,cvRound(pt2.x-pt1.x),cvRound(pt2.y-pt1.y));
    }
    return estimate;
}


void particle_filter::update(Mat& image,bool hog=false)
{
    vector<double> tmp_weights;
    for (int i=0;i<n_particles;i++){
        Mat part_hist,part_roi,part_hog;
        particle state=states[i];
        //cout << "state : x=" << state.x << ",y=" << state.y << ",width=" << state.width << ",height="<< state.height << endl;
        if (state.width < 0){
          state.width = reference_roi.width;
        }
        if (state.height < 0){
          state.height = reference_roi.width;
        }
        Rect boundingBox=Rect(cvRound(state.x),cvRound(state.y),cvRound(state.width),cvRound(state.height));
        part_roi=image(boundingBox);
        calc_hist_hsv(part_roi,part_hist);
        double bc_color = compareHist(reference_hist, part_hist, HISTCMP_BHATTACHARYYA);
        double prob = 0.0f;
        if(bc_color != 1.0f ){
            prob = color_lilekihood.log_likelihood(bc_color);
        }
        double weight=weights[i]+prob;
        if(hog){
            calc_hog(part_roi,part_hog);
            if(part_hog.size()==reference_hog.size()){
                double bc_hog = compareHist(reference_hog, part_hog, HISTCMP_BHATTACHARYYA);
                double prob_hog = hog_likelihood.log_likelihood(bc_hog);
                weight+=prob_hog;
            }
        }
        tmp_weights.push_back(weight);
    }
    weights.swap(tmp_weights);
    tmp_weights = vector<double>();
    resample();
}

void particle_filter::update_discrete(Mat& image,int distribution=MULTINOMIAL_LIKELIHOOD,bool hog = false){
    //double lambda=polya.getAlpha().sum();
    vector<double> tmp_weights;
    double eps= std::numeric_limits<double>::epsilon();
    double prob = 0.0f;
    for (int i=0;i<n_particles;i++){
        Mat part_hist,part_roi,part_hog;
        particle state=states[i];
        if (state.width < 0){
          state.width = reference_roi.width;
        }
        if (state.height < 0){
          state.height = reference_roi.width;
        }
        Rect boundingBox=Rect(cvRound(state.x),cvRound(state.y),cvRound(state.width),cvRound(state.height));
        part_roi=image(boundingBox);
        calc_hist_hsv(part_roi,part_hist);
        VectorXd counts;
        counts.setOnes(part_hist.total());
        //double k=0.0;
        for(int h=0;h<H_BINS;h++)
            for( int s = 0; s < S_BINS; s++ )
            {
                double val=part_hist.at<float>(h, s);
                counts[h*S_BINS+s] = (val!=0.0) ? val : eps;
            }
        //double poisson_log_prior=k * log(lambda) - lgamma(k + 1.0) - lambda;
        if(distribution==DIRICHLET_LIKELIHOOD) prob = polya.log_likelihood(counts);
        else if(distribution==MULTINOMIAL_LIKELIHOOD) prob=discrete.log_likelihood(counts);        
	    else prob=poisson.log_likelihood(counts);
        double weight=weights[i]+prob;
        if(hog){
            VectorXd hog_counts;
            calc_hog(part_roi,part_hog);
            hog_counts.setOnes(part_hog.total());
            if(part_hog.size()==reference_hog.size()){
                //for(unsigned int g=0;g<part_hog.total();g++){
                //    double val=part_hog.at<float>(0, g);
                //    hog_counts[g] = (val!=0.0) ? val : eps;
                //}
                double bc_hog = compareHist(reference_hog, part_hog, HISTCMP_BHATTACHARYYA);
                double prob_hog = hog_likelihood.log_likelihood(bc_hog);
                weight+=prob_hog;
            }
        }
        tmp_weights.push_back(weight);
    }
    weights.swap(tmp_weights);
    tmp_weights = vector<double>();
    resample();
}

void particle_filter::resample(){
    vector<double> cumulative_sum(n_particles);
    vector<double> normalized_weights(n_particles);
    vector<double> new_weights(n_particles);
    vector<double> squared_normalized_weights(n_particles);
    uniform_real_distribution<double> unif_rnd(0.0,1.0); 
    float logsumexp=0.0;
    float max_value = *max_element(weights.begin(), weights.end());
    for (unsigned int i=0; i<weights.size(); i++) {
        new_weights[i]=weights[i]-max_value;
        logsumexp+=exp(weights[i]-max_value);
    }
    float norm_const=max_value+log(logsumexp);
    for (unsigned int i=0; i<weights.size(); i++) {
        normalized_weights[i] = exp(weights[i]-norm_const);
    }
    for (unsigned int i=0; i<weights.size(); i++) {
        squared_normalized_weights[i]=pow(normalized_weights[i],2.0f);
        if (i==0) {
            cumulative_sum[i] = normalized_weights[i];
        } else {
            cumulative_sum[i] = cumulative_sum[i-1] + normalized_weights[i];
        }
    }
    Scalar sum_squared_weights=sum(squared_normalized_weights);
    marginal_likelihood+=norm_const-log(n_particles); 
    ESS=1.0f/sum_squared_weights[0];
    if(isless(ESS/n_particles,(float)THRESHOLD)){
        vector<particle> new_states;
        for (int i=0; i<n_particles; i++) {
            double uni_rand = unif_rnd(generator);
            vector<double>::iterator pos = lower_bound(cumulative_sum.begin(), cumulative_sum.end(), uni_rand);
            int ipos = distance(cumulative_sum.begin(), pos);
            particle state=states[ipos];
            new_states.push_back(state);
            weights[i]=1.0f/n_particles;
        }
        states.swap(new_states);
    }
    else{
        weights.swap(new_weights);
    }
}

float particle_filter::getESS(){
    return ESS/n_particles;
}

void particle_filter::update_model(VectorXd theta_new,VectorXd alpha_new){
    //double alpha=0.1;
    poisson.setLambda(alpha_new);
    alpha_new.normalize();
    discrete.setTheta(alpha_new);
    theta=theta_new;
}

VectorXd particle_filter::get_discrete_model(){
    //double alpha=0.1;
    return discrete.getTheta();

}

VectorXd particle_filter::get_continuous_model(){
    return theta;

}

double particle_filter::getMarginalLikelihood(){
    return marginal_likelihood;
}
