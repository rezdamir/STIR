//
// $Id$
//

/*!
  \file
  \ingroup utilities 
  \brief Generates a noise realisation according to Poisson statistics for  
  some projection data

  \author Kris Thielemans
  \author Sanida Mustafovic

  $Date$
  $Revision$

  Usage:
  \code
  poisson_noise [-p | --preserve-mean] \
        output_filename input_projdata_filename \
        scaling_factor seed-unsigned-int
  \endcode
  The scaling_Factor is used to multiply the input data before generating
  the Poisson random number. This means that a scaling_factor larger than 1
  will result in less noisy data.<br>
  The seed value for the random number generator has to be strictly positive.<br>
  Without the -p option, the mean of the output data will
  be equal to\nscaling_factor*mean_of_input, otherwise it
  will be equal to mean_of_input.<br>
  The options -p and --preserve-mean are identical.
*/
/*
    Copyright (C) 2000- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/

#include "stir/ProjDataInterfile.h"
#include "stir/SegmentByView.h"
#include "stir/Succeeded.h"

/* Originally there were 2 versions of this code. One using the 
   rand() function available in the C library, and another one
   using boost/random.hpp. However, it is never recommended to 
   use rand() for any serious work, as it might be very unreliable on
   some systems. So, we now use the 2nd version all the time.
   In fact, the version using rand() was never adapted to use
   a Gaussian distributino for high mean values, so won't even compile
   at present.
   If you really can't use boost/random.hpp, you could define RAND
   and do extra coding. Not recommended...
*/
#ifdef RAND
#undef RAND
#endif

#ifndef RAND
#include <boost/random.hpp>
#endif
#include "stir/round.h"

#ifndef STIR_NO_NAMESPACES
using std::cerr;
using std::endl;
using std::cout;
#endif


START_NAMESPACE_STIR


#ifndef RAND
  // try boost::mt19937 or boost::ecuyer1988 instead of boost::minstd_rand
  typedef boost::mt19937 base_generator_type;
  // initialize by reproducible seed
  static base_generator_type generator(boost::uint32_t(42));
  boost::uniform_01<base_generator_type> random01(generator);
#else
#error can not do normal distribution

inline double random01() { return static_cast<double>(rand()) / RAND_MAX; }
#endif

// Generate a random number according to a Poisson distribution
// with mean mu
int generate_poisson_random(const float mu)
{  
  // check if mu is large. If so, use the normal distribution
  // note: the threshold must be such that exp(threshold) is still a floating point number
  if (mu > 60.F)
  {
    boost::normal_distribution<base_generator_type> randomnormal(generator, mu, sqrt(mu));
    const double random = randomnormal();
    return random<0 ? 0 : 
      //#ifndef STIR_NO_NAMESPACES
      //stir::  // needs namespace as there's a global round on gcc on Linux
      //#endif
      round(random);
  }
  else
  {
    double u = random01();
  
    // prevent problems if n growing too large (or even to infinity) 
    // when u is very close to 1
    if (u>1-1.E-6)
      u = 1-1.E-6;
  
    const double upper = exp(mu)*u;
    double accum = 1.;
    double term = 1.; 
    int n = 1;
  
    while(accum <upper)
    {
      accum += (term *= mu/n); 
      n++;
    }
    
    return (n - 1);
  }
}




void 
poisson_noise(ProjData& output_projdata, 
	    const ProjData& input_projdata, 
	    const float scaling_factor,
	    const bool preserve_mean)
{
  
  
  for (int seg= input_projdata.get_min_segment_num(); 
       seg<=input_projdata.get_max_segment_num();
       seg++)  
  {
    SegmentByView<float> seg_input= input_projdata.get_segment_by_view(seg);
    SegmentByView<float> seg_output= 
      output_projdata.get_empty_segment_by_view(seg,false);
  
    cerr << "Segment " << seg << endl;

    for(int view=seg_input.get_min_view_num();view<=seg_input.get_max_view_num();view++)    
      for(int ax_pos=seg_input.get_min_axial_pos_num();ax_pos<=seg_input.get_max_axial_pos_num();ax_pos++)
	//for(int tang_pos=-5;tang_pos<5;tang_pos++)	
      for(int tang_pos=seg_input.get_min_tangential_pos_num();tang_pos<=seg_input.get_max_tangential_pos_num();tang_pos++)
      { 
	/*	cerr << "View " << view << endl;
	cerr << "Ax_Pos" << ax_pos << endl;
	cerr << "Tang Poss" << tang_pos << endl;
	*/

	const float bin = seg_input[view][ax_pos][tang_pos];
	//cerr << "bin" << bin << endl;
	const int random_poisson = generate_poisson_random(bin*scaling_factor);
	//cerr << "done " << random_poisson << endl;
	seg_output[view][ax_pos][tang_pos] = 
	  preserve_mean ?
	  random_poisson / scaling_factor
	  :
	  static_cast<float>(random_poisson);		
      }           
      output_projdata.set_segment(seg_output);      
  }
}



END_NAMESPACE_STIR

USING_NAMESPACE_STIR


void usage()
{
    cerr <<"Usage: poisson_noise [-p | --preserve-mean] <output_filename (no extension)> <input_projdata_filename> scaling_factor seed-unsigned-int\n"
         <<"The seed value for the random number generator has to be strictly positive.\n"
         << "Without the -p option, the mean of the output data will"
	 << " be equal to\nscaling_factor*mean_of_input, otherwise it"
	 << "will be equal to mean_of_input.\n"
	 << "The options -p and --preserve-mean are identical.\n";
}

int
main (int argc,char *argv[])
{
  if(argc<5)
  {
    usage();
    return(EXIT_FAILURE);
  }  
  
  bool preserve_mean = false;

  // option processing
  if (argv[1][0] == '-')
    {
      if (strcmp(argv[1],"-p")==0 ||
	  strcmp(argv[1],"--preserve-mean")==0)
	preserve_mean = true;
      else
	{
	  usage();
	  return(EXIT_FAILURE);
	}  
      ++argv;
    }
	  
  const char *const filename = argv[1];
  const float scaling_factor = atof(argv[3]);
  shared_ptr<ProjData>  in_data = ProjData::read_from_file(argv[2]);

#ifndef RAND
  boost::uint32_t seed = atoi(argv[4]);
  // check seed!=0 as current generator used does not allow this value
  if (seed==0)
   error("Seed value has to be non-zero.\n");
  generator.seed(seed);
#else
  unsigned int seed = atoi(argv[4]);
  // check seed!=0 as Darren Hogg observed strange statistics on Linux with a zero seed
  if (seed==0)
   error("Seed value has to be non-zero.\n");
  srand(seed);
#endif


  ProjDataInterfile new_data(in_data->get_proj_data_info_ptr()->clone(), filename);

  
  poisson_noise(new_data,*in_data, scaling_factor, preserve_mean);
  
  return EXIT_SUCCESS;
}

