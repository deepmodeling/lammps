// clang-format off
/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   This class contains a framework for granular submodels (GSM) including:
   normal, damping, tangential, rolling, twisting, and heat
   These are used to calculate forces/torques/etc based on contact geometry

   Contributing authors:
   Dan Bolintineanu (SNL), Joel Clemmer (SNL)
----------------------------------------------------------------------- */

#include "gsm.h"
#include "error.h"
#include "utils.h"

using namespace LAMMPS_NS;
using namespace Granular_NS;

/* ----------------------------------------------------------------------
   Parent class for all types of granular submodels
------------------------------------------------------------------------- */

GSM::GSM(class GranularModel *gm, LAMMPS *lmp) : Pointers(lmp)
{
  this->gm = gm;

  allocated = 0;
  size_history = 0;
  history_index = 0;
  allow_cohesion = 1;
  beyond_contact = 0;
  num_coeffs = 0;

  nondefault_history_transfer = 0;
  transfer_history_factor = nullptr;
  coeffs = nullptr;
}

/* ---------------------------------------------------------------------- */

GSM::~GSM()
{
  if (allocated) delete [] coeffs;
  delete [] transfer_history_factor;
}

/* ---------------------------------------------------------------------- */

void GSM::allocate_coeffs()
{
  allocated = 1;
  coeffs = new double[num_coeffs];
}

/* ---------------------------------------------------------------------- */

void GSM::mix_coeffs(double* icoeffs, double* jcoeffs)
{
  for (int i = 0; i < num_coeffs; i++)
    coeffs[i] = mix_geom(icoeffs[i], jcoeffs[i]);
  coeffs_to_local();
}

/* ----------------------------------------------------------------------
   mixing of Young's modulus (E)
------------------------------------------------------------------------- */

double GSM::mix_stiffnessE(double E1, double E2,
                                    double pois1, double pois2)
{
  double factor1 = (1 - pois1 * pois1) / E1;
  double factor2 = (1 - pois2 * pois2) / E2;
  return 1 / (factor1 + factor2);
}

/* ----------------------------------------------------------------------
   mixing of shear modulus (G)
------------------------------------------------------------------------ */

double GSM::mix_stiffnessG(double E1, double E2,
                                    double pois1, double pois2)
{
  double factor1 = 2 * (2 - pois1) * (1 + pois1) / E1;
  double factor2 = 2 * (2 - pois2) * (1 + pois2) / E2;
  return 1 / (factor1 + factor2);
}

/* ----------------------------------------------------------------------
   mixing of Young's modulus (E) for walls
------------------------------------------------------------------------- */

double GSM::mix_stiffnessE_wall(double E, double pois)
{
  double factor = 2 * (1 - pois);
  return E / factor;
}

/* ----------------------------------------------------------------------
   mixing of shear modulus (G) for walls
------------------------------------------------------------------------ */

double GSM::mix_stiffnessG_wall(double E, double pois)
{
  double factor = 32.0 * (2 - pois) * (1 + pois);
  return E / factor;
}

/* ----------------------------------------------------------------------
   mixing of everything else
------------------------------------------------------------------------- */

double GSM::mix_geom(double val1, double val2)
{
  return sqrt(val1 * val2);
}
