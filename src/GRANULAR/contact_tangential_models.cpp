/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "contact_damping_models.h"
#include "contact_normal_models.h"
#include "contact_tangential_models.h"
#include "contact.h"
#include "error.h"
#include "math_const.h"
#include "math_extra.h"

using namespace LAMMPS_NS;
using namespace Contact;
using namespace MathConst;
using namespace MathExtra;

/* ----------------------------------------------------------------------
   Linear model with no history
------------------------------------------------------------------------- */

TangentialLinearNoHistory::TangentialLinearNoHistory()
{
  num_coeffs = 2;
  size_history = 3;
}

/* ---------------------------------------------------------------------- */

void TangentialLinearNoHistory::coeffs_to_local()
{
  k = 0.0; // No tangential stiffness with no history
  xt = coeffs[0];
  mu = coeffs[1];
  damp = xt * contact->damping_model->damp;

  if (k < 0.0 || xt < 0.0 || mu < 0.0)
    error->all(FLERR, "Illegal linear no history tangential model");
}

/* ---------------------------------------------------------------------- */

void TangentialLinearNoHistory::mix_coeffs(TangentialModel* imodel, TangentialModel* jmodel)
{
  coeffs[0] = mix_geom(imodel->coeffs[0], jmodel->coeffs[0]);
  coeffs[1] = mix_geom(imodel->coeffs[1], jmodel->coeffs[1]);
  coeffs_to_local();
}

/* ---------------------------------------------------------------------- */

void TangentialLinearNoHistory::calculate_forces()
{
  double Fscrit, fsmag, Ft;

  // classic pair gran/hooke (no history)
  Fscrit = mu * contact->normal_model->Fncrit;
  fsmag = damp * contact->vrel;
  if (contact->vrel != 0.0) Ft = MIN(Fscrit, fsmag) / contact->vrel;
  else Ft = 0.0;

  Ft = -Ft;
  scale3(Ft, contact->vtr, contact->fs);
}

/* ----------------------------------------------------------------------
   Linear model with history
------------------------------------------------------------------------- */

TangentialLinearHistory::TangentialLinearHistory()
{
  num_coeffs = 3;
  size_history = 3;
}

/* ---------------------------------------------------------------------- */

void TangentialLinearHistory::coeffs_to_local()
{
  k = coeffs[0];
  xt = coeffs[1];
  mu = coeffs[2];
  damp = xt * contact->damping_model->damp;

  if (k < 0.0 || xt < 0.0 || mu < 0.0)
    error->all(FLERR, "Illegal linear tangential model");
}

/* ---------------------------------------------------------------------- */

void TangentialLinearHistory::mix_coeffs(TangentialModel* imodel, TangentialModel* jmodel)
{
  coeffs[0] = mix_geom(imodel->coeffs[0], jmodel->coeffs[0]);
  coeffs[1] = mix_geom(imodel->coeffs[1], jmodel->coeffs[1]);
  coeffs[2] = mix_geom(imodel->coeffs[2], jmodel->coeffs[2]);
  coeffs_to_local();
}

/* ---------------------------------------------------------------------- */

void TangentialLinearHistory::calculate_forces()
{
  double Fscrit, magfs, rsht, shrmag, prjmag, temp_dbl, temp_array[3];
  int frame_update = 0;

  Fscrit = contact->normal_model->Fncrit * mu;

  double *history = & contact->history[history_index];

  // rotate and update displacements / force.
  // see e.g. eq. 17 of Luding, Gran. Matter 2008, v10,p235
  if (contact->history_update) {
    rsht = dot3(history, contact->nx);
    frame_update = fabs(rsht) * k > EPSILON * Fscrit;

    if (frame_update) {
      shrmag = len3(history);
      // projection
      scale3(rsht, contact->nx, history);
      // also rescale to preserve magnitude
      prjmag = len3(history);
      if (prjmag > 0) temp_dbl = shrmag / prjmag;
      else temp_dbl = 0;
      scale3(temp_dbl, history);
    }

    // update history
    // tangential force
    // see e.g. eq. 18 of Thornton et al, Pow. Tech. 2013, v223,p30-46
    temp_dbl = k * contact->dt;
    scale3(temp_dbl, contact->vtr, temp_array);
    sub3(history, temp_array, history);
  }

  // tangential forces = history + tangential velocity damping
  temp_dbl = -damp;
  scale3(temp_dbl, contact->vtr, contact->fs);

  // rescale frictional displacements and forces if needed
  magfs = len3(contact->fs);
  if (magfs > Fscrit) {
    shrmag = len3(history);
    if (shrmag != 0.0) {
      temp_dbl = Fscrit / magfs;
      scale3(temp_dbl, contact->fs, history);
      scale3(damp, contact->vtr, temp_array);
      add3(history, temp_array, history);
      temp_dbl = Fscrit / magfs;
      scale3(temp_dbl, contact->fs);
    } else {
      zero3(contact->fs);
    }
  }
}

/* ----------------------------------------------------------------------
   Mindlin model
------------------------------------------------------------------------- */

TangentialMindlin::TangentialMindlin()
{
  num_coeffs = 3;
  size_history = 3;
  mindlin_force = 0;
  mindlin_rescale = 0;
}

/* ---------------------------------------------------------------------- */

void TangentialMindlin::coeffs_to_local()
{
  k = coeffs[0];
  xt = coeffs[1];
  mu = coeffs[2];

  if (k == -1) {
    if (!contact->normal_model->material_properties)
      error->all(FLERR, "Must either specify tangential stiffness or material properties for normal model for the Mindlin tangential style");
    k = 8.0 * mix_stiffnessG(contact->normal_model->Emod, contact->normal_model->Emod, contact->normal_model->poiss, contact->normal_model->poiss);
  }

  if (k < 0.0 || xt < 0.0 || mu < 0.0)
    error->all(FLERR, "Illegal Mindlin tangential model");

  damp = xt * contact->damping_model->damp;
}

/* ---------------------------------------------------------------------- */

void TangentialMindlin::mix_coeffs(TangentialModel* imodel, TangentialModel* jmodel)
{
  if (imodel->coeffs[0] == -1 || imodel->coeffs[0] == -1) coeffs[0] = -1;
  else coeffs[0] = mix_geom(imodel->coeffs[0], jmodel->coeffs[0]);
  coeffs[1] = mix_geom(imodel->coeffs[1], jmodel->coeffs[1]);
  coeffs[2] = mix_geom(imodel->coeffs[2], jmodel->coeffs[2]);
  coeffs_to_local();
}

/* ---------------------------------------------------------------------- */

void TangentialMindlin::calculate_forces()
{
  double Fscrit, k_scaled, magfs, rsht, shrmag, prjmag, temp_dbl;
  double temp_array[3];
  int frame_update = 0;

  double *history = & contact->history[history_index];
  Fscrit = contact->normal_model->Fncrit * mu;

  k_scaled = k * contact->area;
  if (mindlin_rescale) {
    // on unloading, rescale the shear displacements/force
    if (contact->area < history[3]) {
      temp_dbl = contact->area / history[3];
      scale3(temp_dbl, history);
    }
  }

  // rotate and update displacements / force.
  // see e.g. eq. 17 of Luding, Gran. Matter 2008, v10,p235
  if (contact->history_update) {
    rsht = dot3(history, contact->nx);
    if (mindlin_force)
      frame_update = fabs(rsht) > EPSILON * Fscrit;
    else
      frame_update = fabs(rsht) * k_scaled > EPSILON * Fscrit;

    if (frame_update) {
      shrmag = len3(history);
      // projection
      scale3(rsht, contact->nx, history);
      // also rescale to preserve magnitude
      prjmag = len3(history);
      if (prjmag > 0) temp_dbl = shrmag / prjmag;
      else temp_dbl = 0;
      scale3(temp_dbl, history);
    }

    // update history
    if (mindlin_force) {
      // tangential force
      // see e.g. eq. 18 of Thornton et al, Pow. Tech. 2013, v223,p30-46
      temp_dbl = -k_scaled * contact->dt;
      scale3(temp_dbl, contact->vtr, temp_array);
    } else {
      scale3(contact->dt, contact->vtr, temp_array);
    }
    add3(history, temp_array, history);

    if (mindlin_rescale) history[3] = contact->area;
  }

  // tangential forces = history + tangential velocity damping
  temp_dbl = -damp;
  scale3(temp_dbl, contact->vtr, contact->fs);

  if (! mindlin_force) {
    scale3(k_scaled, history, temp_array);
    add3(contact->fs, temp_array, contact->fs);
  }

  // rescale frictional displacements and forces if needed
  magfs = len3(contact->fs);
  if (magfs > Fscrit) {
    shrmag = len3(history);
    if (shrmag != 0.0) {
      temp_dbl = Fscrit / magfs;
      scale3(temp_dbl, contact->fs, history);
      scale3(damp, contact->vtr, temp_array);
      add3(history, temp_array, history);
      if (! mindlin_force) {
        temp_dbl = -1.0 / k_scaled;
        scale3(temp_dbl, history);
      }
      temp_dbl = Fscrit / magfs;
      scale3(temp_dbl, contact->fs);
    } else {
      zero3(contact->fs);
    }
  }
}

/* ----------------------------------------------------------------------
   Mindlin force model
------------------------------------------------------------------------- */

TangentialMindlinForce::TangentialMindlinForce()
{
  num_coeffs = 3;
  size_history = 3;
  mindlin_force = 1;
  mindlin_rescale = 0;
}

/* ----------------------------------------------------------------------
   Mindlin rescale model
------------------------------------------------------------------------- */

TangentialMindlinRescale::TangentialMindlinRescale()
{
  num_coeffs = 3;
  size_history = 4;
  mindlin_force = 0;
  mindlin_rescale = 1;

  nondefault_history_transfer = 1;
  transfer_history_factor = new double(size_history);
  for (int i = 0; i < size_history; i++) transfer_history_factor[i] = -1.0;
  transfer_history_factor[3] = +1;
}

/* ----------------------------------------------------------------------
   Mindlin rescale force model
------------------------------------------------------------------------- */

TangentialMindlinRescaleForce::TangentialMindlinRescaleForce()
{
  num_coeffs = 3;
  size_history = 4;
  mindlin_force = 1;
  mindlin_rescale = 1;

  nondefault_history_transfer = 1;
  transfer_history_factor = new double(size_history);
  for (int i = 0; i < size_history; i++) transfer_history_factor[i] = -1.0;
  transfer_history_factor[3] = +1;
}
