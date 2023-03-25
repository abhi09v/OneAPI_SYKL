#include <CL/sycl.hpp>                    //added header     
#include <iostream>
#include <numeric>
#include <cmath>

using namespace cl::sycl;                        //namespace into program

int main(int argc, char** argv) {

  constexpr size_t NX = 10;                  //define constant parameters
  constexpr size_t NY = 10;
  constexpr size_t NZ = 10;

  std::vector<float> vel_x(NX*NY*NZ);           //create three vectors, vel_x, vel_y, and vel_z, each with a length of NX*NY*NZ elements                         
  std::vector<float> vel_y(NX*NY*NZ);
  std::vector<float> vel_z(NX*NY*NZ);
  
  std::iota(vel_x.begin(), vel_x.end(), 1.0f); // populate with values 1, 2, 3, ..., NX*NY*NZ
  std::iota(vel_y.begin(), vel_y.end(), 2.0f);
  std::iota(vel_z.begin(), vel_z.end(), 3.0f);

  float flux = 0.0f;                   //initializing the flux

  try {                                          //q obejct to select device
    queue q{gpu_selector{}};
    std::cout << "Running on "
              << q.get_device().get_info<info::device::name>()
              << std::endl;

    buffer<float, 1> vel_x_buf(vel_x.data(), range<1>(NX*NY*NZ));
    buffer<float, 1> vel_y_buf(vel_y.data(), range<1>(NX*NY*NZ));     //buffer data between host and device for 1 index range
    buffer<float, 1> vel_z_buf(vel_z.data(), range<1>(NX*NY*NZ));
    buffer<float, 1> flux_buf(&flux, range<1>(1));

    q.submit([&](handler& cgh) {                                   //handle instruction and memory transfer from kernel
      auto vel_x_acc = vel_x_buf.get_access<access::mode::read>(cgh);           
      auto vel_y_acc = vel_y_buf.get_access<access::mode::read>(cgh);             
      auto vel_z_acc = vel_z_buf.get_access<access::mode::read>(cgh);             //kernel will only read data from the buffers
      auto flux_acc = flux_buf.get_access<access::mode::write>(cgh);

      cgh.parallel_for<class stokes_theorem>(                                  //multiple instances of an operation to execute in parallel
        range<1>(NX*NY*NZ),                                                   //index id useful for iteration and offlading
        [=](id<1> idx) {                                        
          const float x = static_cast<float>(idx % NX) + 0.5f;                     
          const float y = static_cast<float>((idx / NX) % NY) + 0.5f;
          const float z = static_cast<float>(idx / (NX*NY)) + 0.5f;             //shift negative cooardinate

          const float vx = vel_x_acc[idx];
          const float vy = vel_y_acc[idx];                                     // calculate the flux value for given index
          const float vz = vel_z_acc[idx];

          const float r = std::sqrt(x*x + y*y + z*z);                         // define r 

          flux_acc[0] += (2.0f * M_PI * r * vz) / 3.0f;      //Taking r radius and taking cone height as r/3
        }
      );
    });

    std::cout << "Flux: " << flux << std::endl;                 //output

  } catch (sycl::exception& e) {                                   // catch exception 
    std::cerr << "SYCL exception caught: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}
