//
// $Id$
//
/*!
  \file
  \ingroup recon_buildblock
  \brief Declaration of class QuadraticPriot

  \author Kris Thielemans

  $Date$
  $Revision$
*/


#ifndef __Tomo_recon_buildblock_QuadraticPrior_H__
#define __Tomo_recon_buildblock_QuadraticPrior_H__


#include "tomo/RegisteredParsingObject.h"
#include "tomo/recon_buildblock/GeneralisedPrior.h"
#include "Array.h"

START_NAMESPACE_TOMO


template <int num_dimensions, typename elemT> class DiscretisedDensity;

/*!
  \ingroup recon_buildblock
  \brief
  A class in the GeneralisedPrior hierarchy. This implements a quadratic Gibbs prior.

  \todo make weights used flexible
*/
template <typename elemT>
class QuadraticPrior:  public 
      RegisteredParsingObject<
	      QuadraticPrior<elemT>,
              GeneralisedPrior<elemT>
	       >
{
public:
  //! Name which will be used when parsing a GeneralisedPrior object
  static const char * const registered_name; 

  //! Default constructor 
  QuadraticPrior();

  //! Constructs it explicitly
  QuadraticPrior(const bool only_2D, float penalization_factor);
  
  //! compute gradient by applying the filter
  void compute_gradient(DiscretisedDensity<3,elemT>& prior_gradient, 
			const DiscretisedDensity<3,elemT> &current_image_estimate);

  
private:
  bool only_2D;
  Array<3,float> weights;
  virtual void set_defaults();
  virtual void initialise_keymap();
  virtual bool post_processing();
  string kappa_filename;
  shared_ptr<DiscretisedDensity<3,elemT> > kappa_ptr;
};


END_NAMESPACE_TOMO

#endif

