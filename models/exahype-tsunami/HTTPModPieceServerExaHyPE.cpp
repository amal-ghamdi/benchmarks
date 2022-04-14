#include <iostream>

#include <string>

#include "umbridge.h"

#include <chrono>
#include <thread>
#include <iomanip>
#include <stdlib.h>

int test_delay = 0;

class ExampleModel : public umbridge::Model {
public:

  ExampleModel()
   : umbridge::Model(Eigen::VectorXi::Ones(1)*2, Eigen::VectorXi::Ones(1)*4)
  {
    outputs.push_back(Eigen::VectorXd::Ones(4));

    char const* shared_dir_cstr = std::getenv("SHARED_DIR");
    if ( shared_dir_cstr == NULL ) {
      std::cerr << "Environment variable SHARED_DIR not set!" << std::endl;
      exit(-1);
    }
    shared_dir = std::string(shared_dir_cstr);
  }

  void Evaluate(std::vector<std::reference_wrapper<const Eigen::VectorXd>> const& inputs, json config) override {
    int level = config.value("level", 0);
    bool verbose = config.value("verbosity", false);
    bool vtk_output = config.value("vtk_output", false);

    std::cout << "Entered for level " << level << std::endl;

    std::ofstream inputsfile (shared_dir + "/inputs.txt");
    typedef std::numeric_limits<double> dl;
    inputsfile << std::fixed << std::setprecision(dl::digits10);
    for (int i = 0; i < inputs[0].get().rows(); i++) {
      inputsfile << inputs[0](i) << std::endl;
    }
    inputsfile.close();

    int status;
    if(verbose) {
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l0 && cp exahype_debug.log-filter exahype.log-filter");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l1 && cp exahype_debug.log-filter exahype.log-filter");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l2 && cp exahype_debug.log-filter exahype.log-filter");
    } else{
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l0 && cp exahype_release.log-filter exahype.log-filter");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l1 && cp exahype_release.log-filter exahype.log-filter");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l2 && cp exahype_release.log-filter exahype.log-filter");
    }
    if(vtk_output){
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 10000.0,/\"time\": 1.0,/g' SWE_asagi_limited_l0.exahype2");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 10000.0,/\"time\": 1.0,/g' SWE_asagi_limited_l1.exahype2");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 10000.0,/\"time\": 1.0,/g' SWE_asagi_limited_l2.exahype2");
    } else{
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 1.0,/\"time\": 10000.0,/g' SWE_asagi_limited_l0.exahype2");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 1.0,/\"time\": 10000.0,/g' SWE_asagi_limited_l1.exahype2");
        system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE && sed -i 's/\"time\": 1.0,/\"time\": 10000.0,/g' SWE_asagi_limited_l2.exahype2");
    }
    if(level == 0) {
      status = system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l0 && ./ExaHyPE-SWE ../SWE_asagi_limited_l0.exahype2");
    } else if(level == 1) {
      status = system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l1 && ./ExaHyPE-SWE ../SWE_asagi_limited_l1.exahype2");
    } else if(level == 2) {
      status = system("cd /ExaHyPE-Tsunami/ApplicationExamples/SWE/SWE_asagi_limited_l2 && ./ExaHyPE-SWE ../SWE_asagi_limited_l2.exahype2");
    } else {
      std::cerr << "Unknown model requested by client!" << std::endl;
      exit(-1);
    }
    std::cout << "Exahype exit status " << status << std::endl;

    std::ifstream outputsfile(shared_dir + "/outputs.txt");
    for (int i = 0; i < outputs[0].rows(); i++) {
      outputsfile >> outputs[0](i);
    }
    outputsfile.close();
    std::cout << "Read outputs from exahype:" << outputs[0] << std::endl;

    std::cout << "Left" << std::endl;
  }

  bool SupportsEvaluate() override {
    return true;
  }
private:
  std::string shared_dir;
};

int main(){

  char const* port_cstr = std::getenv("PORT");
  if ( port_cstr == NULL ) {
    std::cerr << "Environment variable PORT not set!" << std::endl;
    exit(-1);
  }

  char const* delay_cstr = std::getenv("TEST_DELAY");
  if ( delay_cstr != NULL ) {
    test_delay = atoi(delay_cstr);
  }


  const int port = atoi(port_cstr);
  ExampleModel model;

  umbridge::serveModel(model, "0.0.0.0", port);

  return 0;
}
