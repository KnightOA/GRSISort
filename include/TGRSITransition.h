#ifndef TGRSITRANSITION_H
#define TGRSITRANSITION_H

/** \addtogroup Fitting Fitting & Analysis
 *  @{
 */

#include <cstdio>

#include "TClass.h"
#include "TObject.h"

/////////////////////////////////////////////////////////////////
///
/// \class TGRSITransition
///
/// This Class contains the information about a nuclear
/// transition. These transitions are a part of a TNucleus
/// and are typically set within the TNucleus framework
///
/////////////////////////////////////////////////////////////////

class TGRSITransition : public TObject {
   friend class TNucleus;

public:
   TGRSITransition();
   ~TGRSITransition() override;

   bool IsSortable() const override { return true; }
   int Compare(const TObject* obj) const override;

   void SetEnergy(double& tmpenergy) { fEnergy = tmpenergy; }
   void SetEnergyUncertainty(double& tmperror) { fEnergyUncertainty = tmperror; }
   void SetIntensity(double& tmpintens) { fIntensity = tmpintens; }
   void SetIntensityUncertainty(double& tmpinterror) { fIntensityUncertainty = tmpinterror; }

   double GetEnergy() const { return fEnergy; }
   double GetEnergyUncertainty() const { return fEnergyUncertainty; }
   double GetIntensity() const { return fIntensity; }
   double GetIntensityUncertainty() const { return fIntensityUncertainty; }

   void Clear(Option_t* opt = "") override;
   void Print(Option_t* opt = "") const override;

   std::string PrintToString();

protected:
   double fEnergy;               // Energy of the transition
   double fEnergyUncertainty;    // Uncertainty in the energy of the transition
   double fIntensity;            // Intensity of the transition
   double fIntensityUncertainty; // Uncertainty in the intensity

   /// \cond CLASSIMP
   ClassDefOverride(TGRSITransition, 1) // Information about a TNucleus transition
   /// \endcond
};
/*! @} */
#endif
