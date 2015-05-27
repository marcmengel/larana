//=============================================================================
// AlgoFirstPeak.h
// This is a hit finding algorithm that makes an optical hit out of
// everything above threshold, but uses only the first peak to assign hit time
//
// Gleb Sinev, Duke, 2015
// Based on AlgoTreshold.h
//=============================================================================

#ifndef ALGOFIRSTPEAK_H
#define ALGOFIRSTPEAK_H

#include "PMTPulseRecoBase.h"

#include <vector>

namespace pmtana {

  class AlgoFirstPeak : public PMTPulseRecoBase {

    public:

      // Default constructor
      AlgoFirstPeak();

      // Default destructor
      virtual ~AlgoFirstPeak();

      // Implementation of PMTPulseRecoBase::RecoPulse() method
      virtual bool RecoPulse(const std::vector< short > &wf);

      // Implementation of PMTPulseRecoBase::Reset() method
      virtual void Reset();

      // A method to set user-defined ADC threshold value
      void SetADCThreshold(double v) {_adc_thres = v;};

      // A method to set a multiplication factor to the pedestal standard deviation
      // which is used as one of two input values to define a threshold
      void SetNSigma(double v) {_nsigma = v;};

    protected:

      // A variable holder for a user-defined absolute ADC threshold value
      double _adc_thres;

      // Minimum width for a hit to be recorded
      int _min_width;

      // A variable holder for a multiplicative factor for the pedestal 
      // standard deviation to define the threshold
      double _nsigma;

  };

}

#endif