/*
  This file is part of TACS: The Toolkit for the Analysis of Composite
  Structures, a parallel finite-element code for structural and
  multidisciplinary design optimization.

  Copyright (C) 2014 Georgia Tech Research Corporation

  TACS is licensed under the Apache License, Version 2.0 (the
  "License"); you may not use this software except in compliance with
  the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0
*/

#include "TACSBeamConstitutive.h"

const char *TACSBeamConstitutive::constName = "TACSBeamConstitutive";

const char *TACSBeamConstitutive::getObjectName(){
  return constName;
}

/*
  Constructor for Timoshenko beam theory based constitutive object.

  EA               : axial stiffness
  EI22, EI33, EI23 : bending stiffness
  GJ               : torsional stiffness
  kG22, kG33, kG23 : shearing stiffness
  m00              : mass per unit span
  m11, m22, m33    : moments of inertia such that m11 = m22 + m33
  xm2, xm3         : cross sectional center of mass location
  xc2, xc3         : cross sectional centroid
  xk2, xk3         : cross sectional shear center
  muS              : viscous damping coefficient
*/
TACSBeamConstitutive::TACSBeamConstitutive( const TacsScalar axs[],
                                            TacsScalar EA,
                                            TacsScalar EI22,
                                            TacsScalar EI33,
                                            TacsScalar EI23,
                                            TacsScalar GJ,
                                            TacsScalar kG22,
                                            TacsScalar kG33,
                                            TacsScalar kG23,
                                            TacsScalar m00,
                                            TacsScalar m11,
                                            TacsScalar m22,
                                            TacsScalar m33,
                                            TacsScalar xm2,
                                            TacsScalar xm3,
                                            TacsScalar xc2,
                                            TacsScalar xc3,
                                            TacsScalar xk2,
                                            TacsScalar xk3,
                                            TacsScalar muS ){
  // Set the reference axis and normalize it
  axis[0] = axs[0];
  axis[1] = axs[1];
  axis[2] = axs[2];
  TacsScalar tmp = 1.0/sqrt(axis[0]*axis[0] +
                            axis[1]*axis[1] +
                            axis[2]*axis[2]);
  axis[0] *= tmp;
  axis[1] *= tmp;
  axis[2] *= tmp;

  // Set the entries of the stiffness matrix
  memset(C, 0, NUM_TANGENT_STIFFNESS_ENTRIES*sizeof(TacsScalar));

  // row 1 for axial force
  C[0] = EA;
  C[2] = xc3*EA;
  C[3] = -xc2*EA;

  // row 2 for twisting moment
  C[6] = GJ + xk2*xk2*kG33 + xk3*xk3*kG22 + 2.0*xk2*xk3*kG23;
  C[9] = -xk2*kG23 - xk3*kG22;
  C[10] = xk2*kG33 + xk3*kG23;

  // row 3 for bending moment about axis 2
  C[11] = EI22 + xc3*xc3*EA;
  C[12] = -(EI23 + xc2*xc3*EA);

  // row 4 for bending moment about axis 3
  C[15] = EI33 + xc2*xc2*EA;

  // row 5 for shear 2
  C[18] = kG22;
  C[19] = -kG23;

  // row 6 for shear 3
  C[20] = kG33;

  // Set the entries of the density matrix
  rho[0] = m00;
  rho[1] = m11;
  rho[2] = m22;
  rho[3] = m00*xm2*xm3;
}

/*
  Set the diagonal components of the stiffness matrix and the mass
  moments of the cross-section.
*/
TACSBeamConstitutive::TACSBeamConstitutive( TacsScalar rhoA,
                                            TacsScalar rhoIy,
                                            TacsScalar rhoIz,
                                            TacsScalar rhoIyz,
                                            TacsScalar EA,
                                            TacsScalar GJ,
                                            TacsScalar EIy,
                                            TacsScalar EIz,
                                            TacsScalar kGAy,
                                            TacsScalar kGAz,
                                            const TacsScalar axs[] ){
  // Set the reference axis and normalize it
  axis[0] = axs[0];
  axis[1] = axs[1];
  axis[2] = axs[2];
  TacsScalar tmp = 1.0/sqrt(axis[0]*axis[0] +
                            axis[1]*axis[1] +
                            axis[2]*axis[2]);
  axis[0] *= tmp;
  axis[1] *= tmp;
  axis[2] *= tmp;

  // Set the entries of the stiffness matrix
  memset(C, 0, NUM_TANGENT_STIFFNESS_ENTRIES*sizeof(TacsScalar));
  C[0] = EA;
  C[6] = GJ;
  C[11] = EIy;
  C[15] = EIz;
  C[18] = kGAy;
  C[20] = kGAz;

  // Set the entries of the density matrix
  rho[0] = rhoA;
  rho[1] = rhoIy;
  rho[2] = rhoIz;
  rho[3] = rhoIyz;
}

/*
  Set the full stiffness matrix
*/
TACSBeamConstitutive::TACSBeamConstitutive( const TacsScalar rho0[],
                                            const TacsScalar C0[],
                                            const TacsScalar axs[] ){
  setProperties(rho0, C0, axs);
}

/*
  Set the stiffness/mass/axis properties
*/
void TACSBeamConstitutive::setProperties( const TacsScalar rho0[],
                                          const TacsScalar C0[],
                                          const TacsScalar axs[] ){
  // Set the reference axis and normalize it
  if (axs){
    axis[0] = axs[0];
    axis[1] = axs[1];
    axis[2] = axs[2];
    TacsScalar tmp = 1.0/sqrt(axis[0]*axis[0] +
                              axis[1]*axis[1] +
                              axis[2]*axis[2]);
    axis[0] *= tmp;
    axis[1] *= tmp;
    axis[2] *= tmp;
  }

  // Copy the density/stiffness matrix
  if (rho0){
    memcpy(rho, rho0, 4*sizeof(TacsScalar));
  }
  if (C0){
    memcpy(C, C0, NUM_TANGENT_STIFFNESS_ENTRIES*sizeof(TacsScalar));
  }
}

/*
  Get the stiffness/mass/axis properties
*/
void TACSBeamConstitutive::getProperties( TacsScalar rho0[],
                                          TacsScalar C0[],
                                          TacsScalar axs[] ){
  // Set the reference axis and normalize it
  if (axs){
    axs[0] = axis[0];
    axs[1] = axis[1];
    axs[2] = axis[2];
  }

  // Copy the density/stiffness matrix
  if (rho0){
    memcpy(rho0, rho, 4*sizeof(TacsScalar));
  }
  if (C0){
    memcpy(C0, C, NUM_TANGENT_STIFFNESS_ENTRIES*sizeof(TacsScalar));
  }
}

TACSBeamConstitutive::~TACSBeamConstitutive(){}

/*
  Get the number of stress components
*/
int TACSBeamConstitutive::getNumStresses(){
  return 6;
}

/*
  Evaluate material properties
*/
TacsScalar TACSBeamConstitutive::evalDensity( int elemIndex,
                                              const double pt[],
                                              const TacsScalar X[] ){
  return rho[0];
}

TacsScalar TACSBeamConstitutive::evalSpecificHeat( int elemIndex,
                                                   const double pt[],
                                                   const TacsScalar X[] ){
  return 0.0;
}

// Evaluate the stress and the tangent stiffness matrix
void TACSBeamConstitutive::evalStress( int elemIndex,
                                       const double pt[],
                                       const TacsScalar X[],
                                       const TacsScalar e[],
                                       TacsScalar s[] ){
  computeStress(C, e, s);
}

void TACSBeamConstitutive::evalTangentStiffness( int elemIndex,
                                                 const double pt[],
                                                 const TacsScalar X[],
                                                 TacsScalar C0[] ){
  memcpy(C0, C, NUM_TANGENT_STIFFNESS_ENTRIES*sizeof(TacsScalar));
}
