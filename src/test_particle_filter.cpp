#include "models/particle_filter.hpp"
#include "utils/utils.hpp"
#include "utils/image_generator.hpp"

#include <time.h>
#include <iostream>
#include <cstdlib>

using namespace std;
using namespace cv;

class TestParticleFilter{
public:
  TestParticleFilter(string _firstFrameFilename, string _gtFilename, int _num_particles);
  void run();
private:
  int num_particles,num_frames;
  imageGenerator generator;
  double reinit_rate;
  //discrete_particle_filter filter;
  //particle_filter filter;
  vector<Mat> images;
  vector<string> gt_vec;
};

TestParticleFilter::TestParticleFilter(string _firstFrameFilename, string _gtFilename, int _num_particles){
  imageGenerator generator(_firstFrameFilename,_gtFilename);
  num_particles = _num_particles;
  num_frames = generator.getDatasetSize();
  gt_vec = generator.ground_truth;
  images = generator.images;
}

void TestParticleFilter::run(){
  particle_filter filter(num_particles);
  Rect ground_truth;
  Mat current_frame; 
  string current_gt;
  reinit_rate = 0.0;
  time_t start, end;
  time(&start);
  Performance performance;
  namedWindow("Tracker");
  for(int k=0;k <num_frames;++k){
    current_gt=gt_vec[k];
    ground_truth=generator.stringToRect(current_gt);
    current_frame = images[k].clone();
    if(!filter.is_initialized()){
        filter.initialize(current_frame,ground_truth);
    }else{
        filter.predict();
        filter.update(current_frame);
        filter.draw_particles(current_frame,Scalar(0,255,255));
        rectangle( current_frame, ground_truth, Scalar(0,255,0), 1, LINE_AA );
        Rect estimate = filter.estimate(current_frame,true);
        //cout << "--------------------------------------------" << endl;
        //cout << "GT, "<< "x:" << ground_truth.x << ",y:" << ground_truth.y << ",w:" << ground_truth.width << ",h:" << ground_truth.height << endl;
        double r1 = performance.calc(ground_truth, estimate);
        //cout  << "ESS : " << filter.getESS() << "ratio : " << r1 << endl;
        if(r1<0.1) {
          filter.reinitialize();
          reinit_rate+=1.0;
      }
    }
    imshow("Tracker",current_frame);
    waitKey(1);
  }
  time(&end);
  double sec = difftime (end, start);
  // print precision,recall,fps,rate,num_frames
  //cout << "ML:" << filter.getMarginalLikelihood() << endl;
  cout  << performance.get_avg_precision()/(num_frames-reinit_rate);
  cout << "," << performance.get_avg_recall()/(num_frames-reinit_rate);
  cout << "," << num_frames/sec << "," << reinit_rate <<  "," << num_frames << endl;
};

int main(int argc, char* argv[]){
    
    
    if(argc != 7) {
        cerr <<"Incorrect input list" << endl;
        cerr <<"exiting..." << endl;
        return EXIT_FAILURE;
    }
    else{
        string _firstFrameFilename,_gtFilename;
        int _num_particles;
        if(strcmp(argv[1], "-img") == 0) {
            _firstFrameFilename=argv[2];
        }
        else{
            cerr <<"No images given" << endl;
            cerr <<"exiting..." << endl;
            return EXIT_FAILURE;
        }
        if(strcmp(argv[3], "-gt") == 0) {
            _gtFilename=argv[4];
        }
        else{
            cerr <<"No ground truth given" << endl;
            cerr <<"exiting..." << endl;
            return EXIT_FAILURE;
        }
        if(strcmp(argv[5], "-npart") == 0) {
            _num_particles=atoi(argv[6]);
        }
        else{
            _num_particles=300;
        }
        TestParticleFilter tracker(_firstFrameFilename,_gtFilename,_num_particles);
        tracker.run();
    }
}

