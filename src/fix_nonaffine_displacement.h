/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef FIX_CLASS
// clang-format off
FixStyle(nonaffine/displacement,FixNonaffineDisplacement)
// clang-format on
#else

#ifndef LMP_FIX_NONAFFINE_DISPLACEMENT_H
#define LMP_FIX_NONAFFINE_DISPLACEMENT_H

#include "fix.h"

namespace LAMMPS_NS {

class FixNonaffineDisplacement : public Fix {
 public:
  FixNonaffineDisplacement(class LAMMPS *, int, char **);
  ~FixNonaffineDisplacement() override;
  int setmask() override;
  void post_constructor() override;
  void init() override;
  void init_list(int, class NeighList *) override;
  void setup(int);
  void post_force(int) override;
  void write_restart(FILE *fp) override;
  void restart(char *buf) override;
  int pack_forward_comm(int, int *, double *, int, int *) override;
  void unpack_forward_comm(int, int, double *) override;
  int pack_reverse_comm(int, int, double *) override;
  void unpack_reverse_comm(int, int *, double *) override;

 private:
  double dtv;
  char *new_fix_id;
  int nad_index, nmax, comm_flag;
  int nad_style, cut_style;
  int reference_style, offset_timestep, reference_timestep, update_timestep;
  int reference_saved;
  double cutoff_custom, cutsq_custom, mycutneigh;
  double xprd0, yprd0, zprd0, xprd0_half, yprd0_half, zprd0_half, xy0, xz0, yz0;

  double ***X, ***Y, ***F;
  int *norm;

  class NeighList *list;    // half neighbor list


  void integrate_velocity();
  void calculate_D2Min();
  void save_reference_state();
  void minimum_image0(double *);
  void grow_arrays(int);
};

}    // namespace LAMMPS_NS

#endif
#endif
