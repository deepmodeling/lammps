/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/ Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifdef COMMAND_CLASS
// clang-format off
CommandStyle(mdi,MDIEngine);
// clang-format on
#else

#ifndef LMP_MDI_ENGINE_H
#define LMP_MDI_ENGINE_H

#include "command.h"
#include "mdi.h"

namespace LAMMPS_NS {

class MDIEngine : public Command {
 public:
  MDIEngine(LAMMPS *lmp) : Command(lmp) {}

  void command(int, char **) override;

  int execute_command(const char *command, MDI_Comm mdicomm);
  void engine_node(const char *node);

 private:
  int lmpunits;      // REAL or METAL or NATIVE
  int root;          // 1 for proc 0, otherwise 0

  int enable_fix;    // 1 if mdi engine command asked for node support

  // state of MDI engine

  int mode;              // which mode engine is in (DEFAULT,MD,OPTG,etc)
  char *mdicmd;          // current MDI command being processed
  char *node_engine;     // which node engine is at
  char *node_driver;     // which node driver has requested
  bool node_match;       // true if driver and engine node currently match
  bool exit_command;     // true if EXIT command received from driver

  MDI_Comm mdicomm;
  class FixMDIEngine *mdi_fix;

  char *id_ke,*id_pe,*id_press;
  class Irregular *irregular;
  class Minimize *minimizer;
  class Compute *ke,*pe,*press;

  int need_evaluation;    // 1 if system has changed, else 0

  int nbytes;         // NBYTES command value used by other commands

  // @INIT_SYS state

  int sys_natoms_flag,sys_types_flag,sys_charges_flag;
  int sys_coords_flag,sys_velocities_flag;
  int sys_cell_flag,sys_cell_displ_flag;

  int sys_natoms;
  int *sys_types;
  double *sys_charges,*sys_coords,*sys_velocities;
  double *sys_cell,*sys_cell_displ;
    
  // unit conversion factors

  double lmp2mdi_length,mdi2lmp_length;
  double lmp2mdi_energy,mdi2lmp_energy;
  double lmp2mdi_velocity,mdi2lmp_velocity;
  double lmp2mdi_force,mdi2lmp_force;
  double lmp2mdi_pressure,mdi2lmp_pressure;
  double lmp2mdi_virial,mdi2lmp_virial;

  // buffers for MDI comm

  int maxbuf;
  double *buf1,*buf1all;
  double *buf3,*buf3all;
  int *ibuf1,*ibuf1all;

  // class methods

  void mdi_engine(int, char **);

  void mdi_commands();
  void mdi_md();
  void mdi_optg();
  void mdi_sys();

  void evaluate();

  void receive_cell();
  void receive_cell_default();
  void receive_cell_sys();

  void receive_cell_displ();
  void receive_cell_displ_default();
  void receive_cell_displ_sys();

  void receive_charges();
  void receive_charges_sys();

  void receive_coords();
  void receive_coords_sys();

  void receive_natoms();
  void receive_natoms_default();
  void receive_natoms_sys();

  void receive_types();
  void receive_types_sys();

  void receive_velocities();
  void receive_velocities_sys();

  void receive_double1(int);
  void receive_int1(int);
  void receive_double3(int, int);

  void send_natoms();
  void send_ntypes();

  void send_double1(int);
  void send_int1(int);
  void send_double3(int);
  void send_labels();

  void send_total_energy();
  void send_pe();
  void send_ke();

  void send_cell();
  void send_cell_displ();

  void nbytes_command();
  void single_command();
  void many_commands();
  void infile();
  void reset_box();
  void create_atoms(int);
  void send_stress();

  void unit_conversions();
  void reallocate();
};

}

#endif
#endif

/* ERROR/WARNING messages:

E: Illegal ... command

Self-explanatory.  Check the input script syntax and compare to the
documentation for the command.  You can use -echo screen as a
command-line option when running LAMMPS to see the offending line.

*/
