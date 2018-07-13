/*
  This file is part of TACS: The Toolkit for the Analysis of Composite
  Structures, a parallel finite-element code for structural and
  multidisciplinary design optimization.

  Copyright (C) 2010 University of Toronto
  Copyright (C) 2012 University of Michigan
  Copyright (C) 2014 Georgia Tech Research Corporation
  Additional copyright (C) 2010 Graeme J. Kennedy and Joaquim
  R.R.A. Martins All rights reserved.

  TACS is licensed under the Apache License, Version 2.0 (the
  "License"); you may not use this software except in compliance with
  the License.  You may obtain a copy of the License at
  
  http://www.apache.org/licenses/LICENSE-2.0 
*/

#ifndef TACS_POISSON_ELEMENT_H
#define TACS_POISSON_ELEMENT_H

#include "TACSElement.h"

template <int order>
class PoissonQuad : public TACSElement {
 public:
  PoissonQuad( const TacsScalar _f[] ){
    for ( int i = 0; i < order*order; i++ ){
      f[i] = _f[i];
    }
    // Set the knot locations
    if (order == 2){
      knots[0] = -1.0;
      knots[1] = 1.0;
    }
    else if (order == 3){
      knots[0] = -1.0;
      knots[1] = 0.0;
      knots[2] = 1.0;
    }
    else {
      // Set a co-sine spacing for the knot locations
      for ( int k = 0; k < order; k++ ){
        knots[k] = -cos(M_PI*k/(order-1));
      }
    }    
  }
  ~PoissonQuad(){}
  
  const char* elementName(){
    return "PoissonQuad";
  }
  const char* displacementName( int i ){
    return "phi";
  }
  const char* stressName( int i ){
    if (i == 0){ return "px"; }
    else if (i == 1){ return "py"; }
    return NULL;
  }
  const char* strainName( int i ){
    if (i == 0){ return "px"; }
    else if (i == 1){ return "py"; }
    return NULL;
  }
  const char* extraName( int i ){
    return NULL;
  }
  int numDisplacements(){ return 1; }
  int numNodes(){ return order*order; }
  int numStresses(){ return 2; }
  int numExtras(){ return 0; }
  ElementType getElementType(){
    return TACS_POISSON_2D_ELEMENT;
  }
  
  int getNumGaussPts(){
    return order*order;
  }
  double getGaussWtsPts( const int num, double *pt ){
    int n = num % order;
    int m = num/order;
    
    const double *pts, *wts;
    FElibrary::getGaussPtsWts(order, &pts, &wts);
    pt[0] = pts[n];
    pt[1] = pts[m];
    return wts[n]*wts[m];
  }
  TacsScalar getDetJacobian( const double *pt,
                             const TacsScalar Xpts[] ){
    // Get the shape functions
    double N[order*order], Na[order*order], Nb[order*order];
    getShapeFunctions(pt, N, Na, Nb);

    // Compute the Jacobian transformation
    TacsScalar Xd[4];
    getJacobianTransform(Na, Nb, Xpts, Xd);
    return Xd[0]*Xd[3] - Xd[1]*Xd[2];
  }
    
  void getShapeFunctions( const double pt[], double N[] ){
    double na[order], nb[order];
    FElibrary::lagrangeSFKnots(na, pt[0], knots, order);
    FElibrary::lagrangeSFKnots(nb, pt[1], knots, order);
    for ( int j = 0; j < order; j++ ){
      for ( int i = 0; i < order; i++ ){
        N[0] = na[i]*nb[j];
        N++;
      }
    }
  }
  void getShapeFunctions( const double pt[], double N[],
                          double Na[], double Nb[] ){
    double na[order], nb[order];
    double dna[order], dnb[order];
    FElibrary::lagrangeSFKnots(na, dna, pt[0], knots, order);
    FElibrary::lagrangeSFKnots(nb, dnb, pt[1], knots, order);
    for ( int j = 0; j < order; j++ ){
      for ( int i = 0; i < order; i++ ){
        N[0] = na[i]*nb[j];
        Na[0] = dna[i]*nb[j];
        Nb[0] = na[i]*dnb[j];
        N++;  Na++;  Nb++;
      }
    }
  }
  void getJacobianTransform( const double Na[], const double Nb[],
                             const TacsScalar Xpts[],
                             TacsScalar Xd[] ){
    Xd[0] = Xd[1] = Xd[2] = Xd[3] = 0.0;
    for ( int i = 0; i < order*order; i++ ){
      Xd[0] += Na[i]*Xpts[3*i];
      Xd[1] += Nb[i]*Xpts[3*i];
      Xd[2] += Na[i]*Xpts[3*i+1];
      Xd[3] += Nb[i]*Xpts[3*i+1];
    }
  }
  void addResidual( double time, TacsScalar res[],
                    const TacsScalar Xpts[],
                    const TacsScalar vars[],
                    const TacsScalar dvars[],
                    const TacsScalar ddvars[] ){
    const double *pts, *wts;
    FElibrary::getGaussPtsWts(order, &pts, &wts);

    for ( int m = 0; m < order; m++ ){
      for ( int n = 0; n < order; n++ ){
        // Set the quadrature points
        double pt[2];
        pt[0] = pts[n];
        pt[1] = pts[m];

        // Get the shape functions
        double N[order*order], Na[order*order], Nb[order*order];
        getShapeFunctions(pt, N, Na, Nb);

        // Compute the Jacobian transformation
        TacsScalar Xd[4];
        getJacobianTransform(Na, Nb, Xpts, Xd);

        // Compute the inverse of the Jacobian
        TacsScalar J[4];
        TacsScalar h = FElibrary::jacobian2d(Xd, J);
        h *= wts[n]*wts[m];

        TacsScalar fval = 0.0;
        TacsScalar px = 0.0, py = 0.0;
        for ( int i = 0; i < order*order; i++ ){
          fval += N[i]*f[i];
          px += (Na[i]*J[0] + Nb[i]*J[2])*vars[i];
          py += (Na[i]*J[1] + Nb[i]*J[3])*vars[i];
        }

        // Add the term to the residual
        for ( int i = 0; i < order*order; i++ ){
          TacsScalar Nxi = Na[i]*J[0] + Nb[i]*J[2];
          TacsScalar Nyi = Na[i]*J[1] + Nb[i]*J[3];
          res[i] += h*(Nxi*px + Nyi*py - fval*N[i]);
        }
      }
    }
  }
  void addJacobian( double time, TacsScalar mat[],
                    double alpha, double beta, double gamma,
                    const TacsScalar Xpts[],
                    const TacsScalar vars[],
                    const TacsScalar dvars[],
                    const TacsScalar ddvars[] ){
    const double *pts, *wts;
    FElibrary::getGaussPtsWts(order, &pts, &wts);

    for ( int m = 0; m < order; m++ ){
      for ( int n = 0; n < order; n++ ){
        // Set the quadrature points
        double pt[2];
        pt[0] = pts[n];
        pt[1] = pts[m];

        // Get the shape functions
        double N[order*order], Na[order*order], Nb[order*order];
        getShapeFunctions(pt, N, Na, Nb);

        // Compute the Jacobian transformation
        TacsScalar Xd[4];
        getJacobianTransform(Na, Nb, Xpts, Xd);

        // Compute the inverse of the Jacobian
        TacsScalar J[4];
        TacsScalar h = FElibrary::jacobian2d(Xd, J);
        h *= alpha*wts[n]*wts[m];

        for ( int j = 0; j < order*order; j++ ){
          TacsScalar Nxj = Na[j]*J[0] + Nb[j]*J[2];
          TacsScalar Nyj = Na[j]*J[1] + Nb[j]*J[3];

          for ( int i = 0; i < order*order; i++ ){
            TacsScalar Nxi = Na[i]*J[0] + Nb[i]*J[2];
            TacsScalar Nyi = Na[i]*J[1] + Nb[i]*J[3];

            mat[i + j*order*order] += h*(Nxi*Nxj + Nyi*Nyj);
          }
        }
      }
    }
  }
  void addLocalizedError( double time, TacsScalar err[],
                          const TacsScalar adjoint[],
                          const TacsScalar Xpts[],
                          const TacsScalar vars[] ){
    const double *pts, *wts;
    FElibrary::getGaussPtsWts(order, &pts, &wts);

    for ( int m = 0; m < order; m++ ){
      for ( int n = 0; n < order; n++ ){
        // Set the quadrature points
        double pt[2];
        pt[0] = pts[n];
        pt[1] = pts[m];

        // Get the shape functions
        double N[order*order], Na[order*order], Nb[order*order];
        getShapeFunctions(pt, N, Na, Nb);

        // Compute the Jacobian transformation
        TacsScalar Xd[4];
        getJacobianTransform(Na, Nb, Xpts, Xd);

        // Compute the inverse of the Jacobian
        TacsScalar J[4];
        TacsScalar h = FElibrary::jacobian2d(Xd, J);
        h *= wts[n]*wts[m];

        // Compute the terms needed to localize the error
        TacsScalar fval = 0.0, adj = 0.0;
        TacsScalar px = 0.0, py = 0.0;
        TacsScalar ax = 0.0, ay = 0.0;
        for ( int i = 0; i < order*order; i++ ){
          TacsScalar Nx = Na[i]*J[0] + Nb[i]*J[2];
          TacsScalar Ny = Na[i]*J[1] + Nb[i]*J[3];
          fval += N[i]*f[i];
          adj += N[i]*adjoint[i];
          px += Nx*vars[i];
          py += Ny*vars[i];
          ax += Nx*adjoint[i];
          ay += Ny*adjoint[i];
        }

        TacsScalar product = h*(ax*px + ay*py - adj*fval);

        // Compute the partition of unity shape functions
        double Nerr[4];
        Nerr[0] = 0.25*(1.0 - pt[0])*(1.0 - pt[1]);
        Nerr[1] = 0.25*(1.0 + pt[0])*(1.0 - pt[1]);
        Nerr[2] = 0.25*(1.0 - pt[0])*(1.0 + pt[1]);
        Nerr[3] = 0.25*(1.0 + pt[0])*(1.0 + pt[1]);

        err[0] += Nerr[0]*product;
        err[order-1] += Nerr[1]*product;
        err[order*(order-1)] += Nerr[2]*product;
        err[order*order-1] += Nerr[3]*product;
      }
    }
  }

  void addOutputCount( int *nelems, int *nnodes, int *ncsr ){
    *nelems += (order-1)*(order-1);
    *nnodes += order*order;
    *ncsr += 4*(order-1)*(order-1);
  }
  void getOutputData( unsigned int out_type, 
                      double *data, int ld_data,
                      const TacsScalar Xpts[],
                      const TacsScalar vars[] ){
    for ( int m = 0; m < order; m++ ){
      for ( int n = 0; n < order; n++ ){
        int p = n + order*m;
        int index = 0;
        if (out_type & TACSElement::OUTPUT_NODES){
          for ( int k = 0; k < 3; k++ ){
            data[index+k] = TacsRealPart(Xpts[3*p+k]);
          }
          index += 3;
        }
        if (out_type & TACSElement::OUTPUT_DISPLACEMENTS){
          data[index] = TacsRealPart(vars[p]);
          index++;
        }
      
        // Set the parametric point to extract the data
        double pt[2];
        pt[0] = knots[n];
        pt[1] = knots[m];
        
        // Get the shape functions
        double N[order*order], Na[order*order], Nb[order*order];
        getShapeFunctions(pt, N, Na, Nb);

        // Compute the Jacobian transformation
        TacsScalar Xd[4];
        getJacobianTransform(Na, Nb, Xpts, Xd);

        // Compute the determinant of Xa and the transformation
        TacsScalar J[4];
        FElibrary::jacobian2d(Xd, J);

        TacsScalar px = 0.0, py = 0.0;
        for ( int i = 0; i < order*order; i++ ){
          px += (Na[i]*J[0] + Nb[i]*J[2])*vars[i];
          py += (Na[i]*J[1] + Nb[i]*J[3])*vars[i];
        }
       
        if (out_type & TACSElement::OUTPUT_STRAINS){
          data[index] = px;
          data[index+1] = py;
          index += 2;
        }
        if (out_type & TACSElement::OUTPUT_STRESSES){
          data[index] = px;
          data[index+1] = py;
          index += 2;
        }
        data += ld_data;
      }
    }
  }
  void getOutputConnectivity( int *con, int node ){
    int p = 0;
    for ( int m = 0; m < order-1; m++ ){
      for ( int n = 0; n < order-1; n++ ){
        con[4*p]   = node + n   + m*order;
        con[4*p+1] = node + n+1 + m*order; 
        con[4*p+2] = node + n+1 + (m+1)*order;
        con[4*p+3] = node + n   + (m+1)*order;
        p++;
      }
    }
  }

 private:
  TacsScalar f[order*order];
  double knots[order];
};

#endif // TACS_POISSON_ELEMENT_H