//
// $Id$
//
/*!
  \file
  \ingroup recon_buildblock

  \brief Implementation for class BinNormalisationFromAttenuationImage

  \author Kris Thielemans
  $Date$
  $Revision$
*/
/*
    Copyright (C) 2003- $Date$, IRSL
    See STIR/LICENSE.txt for details
*/


#include "stir/recon_buildblock/BinNormalisationFromAttenuationImage.h"
#include "stir/recon_buildblock/ForwardProjectorByBinUsingRayTracing.h"
#include "stir/DiscretisedDensityOnCartesianGrid.h" // used for rescaling attenuation image
#include "stir/RelatedViewgrams.h"
#include "stir/ArrayFunction.h"
#include "stir/Succeeded.h"
#include "stir/is_null_ptr.h"

START_NAMESPACE_STIR

const char * const 
BinNormalisationFromAttenuationImage::registered_name = 
  "From Attenuation Image"; 


void 
BinNormalisationFromAttenuationImage::set_defaults()
{
  attenuation_image_ptr = 0;
  forward_projector_ptr = 0;
  attenuation_image_filename = "";
}

void 
BinNormalisationFromAttenuationImage::
initialise_keymap()
{
  parser.add_start_key("Bin Normalisation From Attenuation Image");
  parser.add_key("attenuation_image_filename", &attenuation_image_filename);
  parser.add_parsing_key("forward projector type", &forward_projector_ptr);
  parser.add_stop_key("End Bin Normalisation From Attenuation Image");
}

bool 
BinNormalisationFromAttenuationImage::
post_processing()
{
  // read attenuation_image
  // we do this only when it isn't initialised yet, as this function can be called from a constructor
  if (is_null_ptr(attenuation_image_ptr))
    attenuation_image_ptr = 
      DiscretisedDensity<3,float>::read_from_file(attenuation_image_filename);
  if (is_null_ptr(attenuation_image_ptr))
  {
    warning("BinNormalisationFromAttenuationImage could not read attenuation image %s\n",
            attenuation_image_filename.c_str());
    return true;
  }
  if (is_null_ptr(forward_projector_ptr))
    forward_projector_ptr = 
    new ForwardProjectorByBinUsingRayTracing();
  
  warning("\nWARNING: BinNormalisationFromAttenuationImage:\n"
    "\tattenuation image data are supposed to be in units cm^-1\n"
    "\tReference: water has mu .096 cm^-1\n" 
    "\tMax in attenuation image: %g\n" ,
    attenuation_image_ptr->find_max());
#ifndef NORESCALE
    /*
    cerr << "WARNING: multiplying attenuation image by x-voxel size "
    << " to correct for scale factor in forward projectors...\n";
  */
  // projectors work in pixel units, so convert attenuation data 
  // from cm^-1 to pixel_units^-1
  const float rescale = 
    dynamic_cast<DiscretisedDensityOnCartesianGrid<3,float> *>(attenuation_image_ptr.get())->
    get_grid_spacing()[3]/10;
#else
  const float rescale = 
    10.F;
#endif
  *attenuation_image_ptr *= rescale;

  return false;
}


BinNormalisationFromAttenuationImage::
BinNormalisationFromAttenuationImage()
{
  set_defaults();
}

BinNormalisationFromAttenuationImage::
BinNormalisationFromAttenuationImage(const string& filename,
                                     shared_ptr<ForwardProjectorByBin> const& forward_projector_ptr)
  : forward_projector_ptr(forward_projector_ptr),
    attenuation_image_filename(filename)
{
  attenuation_image_ptr = 0;
  post_processing();
}

BinNormalisationFromAttenuationImage::
BinNormalisationFromAttenuationImage(shared_ptr<DiscretisedDensity<3,float> > const& attenuation_image_ptr,
                                     shared_ptr<ForwardProjectorByBin> const& forward_projector_ptr)
  : attenuation_image_ptr(attenuation_image_ptr),
    forward_projector_ptr(forward_projector_ptr)
{
  post_processing();
}

Succeeded 
BinNormalisationFromAttenuationImage::
set_up(const shared_ptr<ProjDataInfo>& proj_data_info_ptr)
{
  forward_projector_ptr->set_up(proj_data_info_ptr, attenuation_image_ptr);
  return Succeeded::yes;
}


void 
BinNormalisationFromAttenuationImage::apply(RelatedViewgrams<float>& viewgrams) const 
{
  RelatedViewgrams<float> attenuation_viewgrams = viewgrams.get_empty_copy();
  forward_projector_ptr->forward_project(attenuation_viewgrams, *attenuation_image_ptr);
	
  // TODO cannot use std::transform ?
  for (RelatedViewgrams<float>::iterator viewgrams_iter = 
          attenuation_viewgrams.begin();
       viewgrams_iter != attenuation_viewgrams.end();
       ++viewgrams_iter)
  {
    in_place_exp(*viewgrams_iter);
  }
  viewgrams *= attenuation_viewgrams;
}

void 
BinNormalisationFromAttenuationImage::
undo(RelatedViewgrams<float>& viewgrams) const 
{
  RelatedViewgrams<float> attenuation_viewgrams = viewgrams.get_empty_copy();
  forward_projector_ptr->forward_project(attenuation_viewgrams, *attenuation_image_ptr);
	
  // TODO cannot use std::transform ?
  for (RelatedViewgrams<float>::iterator viewgrams_iter = 
          attenuation_viewgrams.begin();
       viewgrams_iter != attenuation_viewgrams.end();
       ++viewgrams_iter)
  {
    in_place_exp(*viewgrams_iter);
  }
  viewgrams /= attenuation_viewgrams;
}

float 
BinNormalisationFromAttenuationImage::get_bin_efficiency(const Bin& bin) const
{
  //TODO
  return 1;
}

  
END_NAMESPACE_STIR

