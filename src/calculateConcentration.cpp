/*
  This code transforms the particles position to concentration field
  defined in a square lattice. This field is passed to the code
  HydroGrid.

  The concentration in a cell of volume (dx*dy)  is defined as 
  c = number_of_particles_inside_cell / (dx*dy);

  Therefore the average concentration in the system is
  c_avg = total_number_of_particles / (lx*ly)

  where (lx*ly) is the (2D-) volume of the system.
*/

#include <stdlib.h> 
#include <sstream>
#include <iostream>
#include <fstream>
#include "visit_writer.h"
#include "visit_writer.c"

#include <boost/python.hpp>
#include <math.h>
#include <stdio.h>
#include <vector>

namespace bp = boost::python;
using namespace std;

bool callHydroGrid(const int option,
                   const string outputname,
                   double *c,
                   double *density,
                   const int mx,
                   const int my,
                   const double lx,
                   const double ly,
                   const double dt,
                   const int step);


void calculateConcentration(string outputname,
                            double lx, // Domain x length
                            double ly, // Domain y length
                            int green_start, // Start of "green" particles
                            int green_end, // End of "green" particles
                            int mx, // Grid size x
                            int my, // Grid size y
                            int step, // Step of simulation
                            double dt, // Time interval between successive snapshots (calls to updateHydroGrid)
                            int np, // Number of particles
                            int option, // option = 0 (initialize), 1 (update), 2 (save), 3 (finalize)
                            double *x_array, double *y_array){
  
  static double dx, dy, inverse_volume_cell;
  static double *c, *density;

  dx = lx / mx;
  dy = ly / my;
  inverse_volume_cell = 1.0 / (dx * dy);
  
  c = new double [mx*my*2];
  density = new double [mx*my];
      
  if(option == 0) { // Initialize HydroGrid
    callHydroGrid(0,
                  outputname,
                  c,
                  density,
                  mx,
                  my,
                  lx,
                  ly,
                  dt,
                  step);  
  }
  else if(option == 1){ // Update hydrogrid data
    
    // Set concentration to zero
    for(int i=0; i < mx*my; i++){
      c[i] = 0;  
      c[mx*my+i] = 0;
      density[i] = 0;
    }
  
    // Loop over particles and save as concentration
    for(int i=0;i<np;i++){
      // Extract data
      
      double x = x_array[i];     
      double y = y_array[i];     
      
      // Use PBC
      x = x - (int(x / lx + 0.5*((x>0)-(x<0)))) * lx;
      y = y - (int(y / ly + 0.5*((y>0)-(y<0)))) * ly;
	  
      // Find cell
      int jx   = int(x / dx + 0.5*mx) % mx;
      int jy   = int(y / dy + 0.5*my) % my;
      int icel = jx + jy * mx;

      // Is particle green or red
      if((i>=green_start)&&(i<=green_end)) { // Particle is green
        c[icel] += 1.0;
      }
      else{ // Particle is red
        c[mx*my+icel] += 1.0;
      }
      density[icel] += 1.0;
    }

    // Scale concentration and density fields
    for(int i=0; i < mx*my; i++){
      density[i] = inverse_volume_cell*density[i];
      c[i]       = inverse_volume_cell*c[i];
      c[mx*my+i] = inverse_volume_cell*c[mx*my+i];
    }

    // Call HydroGrid to update data
    callHydroGrid(1,
                  outputname,
                  c,
                  density,
                  mx,
                  my,
                  lx,
                  ly,
                  dt,
                  step);  
  }
  else if(option == 2){ // Call HydroGrid to print data
    callHydroGrid(3,
                  outputname,
                  c,
                  density,
                  mx,
                  my,
                  lx,
                  ly,
                  dt,
                  step);  
  }  
  else if(option == 3){ // Call HydroGrid to print final data and free memory
    // Free HydroGrid
    callHydroGrid(2,
                  outputname,
                  c,
                  density,
                  mx,
                  my,
                  lx,
                  ly,
                  dt,
                  step);

  }

  // Free memory
  delete[] c;
  delete[] density;

}

/*
  Wrapper to call calculateConcentration from python
 */
void calculateConcentrationPython(string outputname,
                            double lx, 
                            double ly, 
                            int green_start,
                            int green_end,
                            int mx,
                            int my,
                            int step,
                            double dt,
                            int np,
                            int option, // option = 0 (initialize), 1 (update), 2 (save), 3 (finalize)
                            bp::numeric::array r_vectors) // Should be an Nx3 numpy array
{

   double* x = new double [np];
   double* y = new double [np];
   
   for(int i=0;i<np;i++){
      // Extract data
      bp::numeric::array r_vector_1 = bp::extract<bp::numeric::array>(r_vectors[i]);
      x[i] = bp::extract<double>(r_vector_1[0]);     
      y[i] = bp::extract<double>(r_vector_1[1]);     
  
  }    
         
   calculateConcentration(outputname, lx, ly, 
                            green_start, green_end, mx, my, step,
                            dt, np, option, x, y);
                            
   delete[] x;
   delete[] y;                            
                            
}

BOOST_PYTHON_MODULE(calculateConcentration)
{
  using namespace boost::python;
  boost::python::numeric::array::set_module_and_type("numpy", "ndarray");
  def("calculate_concentration", calculateConcentrationPython);
}
