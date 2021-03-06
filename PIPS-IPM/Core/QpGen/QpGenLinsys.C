/* OOQP                                                               *
 * Authors: E. Michael Gertz, Stephen J. Wright                       *
 * (C) 2001 University of Chicago. See Copyright Notification in OOQP */

#include "QpGenLinsys.h"

#include "QpGenData.h"
#include "QpGenResiduals.h"
#include "QpGenVars.h"

#include "OoqpVector.h"
#include "DoubleMatrix.h"
#include "DoubleLinearSolver.h"
#include "SimpleVector.h"
#include "LinearAlgebraPackage.h"
#include "QpGen.h"

#ifdef TIMING
#include "mpi.h"
#include <vector>
#endif


#include <fstream>
using namespace std;

extern int gOuterSolve;
extern int gOuterBiCGIter;

QpGenLinsys::QpGenLinsys( QpGen * factory_,
			  QpGenData * prob,
			  LinearAlgebraPackage * la ) 
: factory( factory_), rhs(NULL), dd(NULL), dq(NULL), useRefs(0)
{

  nx = prob->nx; my = prob->my; mz = prob->mz;
  ixlow = prob->ixlow;
  ixupp = prob->ixupp;
  iclow = prob->iclow;
  icupp = prob->icupp;

  nxlow = ixlow->numberOfNonzeros();
  nxupp = ixupp->numberOfNonzeros();
  mclow = iclow->numberOfNonzeros();
  mcupp = icupp->numberOfNonzeros();

  if( nxupp + nxlow > 0 ) {
    dd      = la->newVector( nx );
    dq      = la->newVector( nx );
    prob->getDiagonalOfQ( *dq );
  }
  nomegaInv   = la->newVector( mz );
  rhs         = la->newVector( nx + my + mz );

  if(gOuterSolve) {
    //for iterative refinement or BICGStab
    sol  = la->newVector( nx + my + mz );
    res  = la->newVector( nx + my + mz );
    resx = la->newVector( nx );
    resy = la->newVector( my );
    resz = la->newVector( mz );
    if(gOuterSolve==2) {
      //BiCGStab; additional vectors needed
      sol2 = la->newVector( nx + my + mz );
      res2 = la->newVector( nx + my + mz );
      res3  = la->newVector( nx + my + mz );
      res4  = la->newVector( nx + my + mz );
      res5  = la->newVector( nx + my + mz );
    } else {
      sol2 = res2 = res3 = res4 = res5 = NULL;
    }
  } else {
    sol = res = resx = resy = resz = NULL;
    sol2 = res2 = res3 = res4 = res5 = NULL;
  }
}

QpGenLinsys::QpGenLinsys()
 : factory( NULL), rhs(NULL), dd(NULL), dq(NULL), useRefs(0),
   sol(NULL), res(NULL), resx(NULL), resy(NULL), resz(NULL),
   sol2(NULL), res2(NULL), res3(NULL), res4(NULL), res5(NULL)
{
}

QpGenLinsys::~QpGenLinsys()
{
  if(!useRefs) {
    delete dd; delete dq;
    delete rhs;
    delete nomegaInv;
  }

  if(sol)  delete sol;
  if(res)  delete res;
  if(resx) delete resx;
  if(resy) delete resy;
  if(resz) delete resz;
  if(sol2) delete sol2;
  if(res2) delete res2;
  if(res3) delete res3;
  if(res4) delete res4;
  if(res5) delete res5;
  
}
void QpGenLinsys::factor(Data * /* prob_in */, Variables *vars_in)
{
  QpGenVars * vars = (QpGenVars *) vars_in;

  assert( vars->validNonZeroPattern() );

  if( nxlow + nxupp > 0 ) dd->copyFrom(*dq);
  this->computeDiagonals( *dd, *nomegaInv,
			  *vars->t, *vars->lambda,
			  *vars->u, *vars->pi,
			  *vars->v, *vars->gamma,
			  *vars->w, *vars->phi );
  if( nxlow + nxupp > 0 ) this->putXDiagonal( *dd );

  nomegaInv->invert();
  nomegaInv->negate();

  if( mclow + mcupp > 0 ) this->putZDiagonal( *nomegaInv );
 
}


void QpGenLinsys::computeDiagonals( OoqpVector& dd_, OoqpVector& omega,
				    OoqpVector& t,  OoqpVector& lambda,
				    OoqpVector& u,  OoqpVector& pi,
				    OoqpVector& v,  OoqpVector& gamma,
				    OoqpVector& w,  OoqpVector& phi )
{
  if( nxupp + nxlow > 0 ) {
    if( nxlow > 0 ) dd_.axdzpy( 1.0, gamma, v, *ixlow );
    if( nxupp > 0 ) dd_.axdzpy( 1.0, phi  , w, *ixupp );
  }
  omega.setToZero();
  if ( mclow > 0 ) omega.axdzpy( 1.0, lambda, t, *iclow );
  if ( mcupp > 0 ) omega.axdzpy( 1.0, pi,     u, *icupp );
  //assert( omega->allPositive() );
}

void QpGenLinsys::solve(Data * prob_in, Variables *vars_in,
			Residuals *res_in, Variables *step_in)
{
  QpGenData      * prob  = (QpGenData *) prob_in;
  QpGenVars      * vars  = (QpGenVars *) vars_in;
  QpGenVars      * step  = (QpGenVars *) step_in;
  QpGenResiduals * res   = (QpGenResiduals *) res_in;


  assert( vars->validNonZeroPattern() );
  assert( res ->validNonZeroPattern() );
  
  step->x->copyFrom( *res->rQ );
  if( nxlow > 0 ) {
    OoqpVector & vInvGamma = *step->v;
    vInvGamma.copyFrom( *vars->gamma );
    vInvGamma.divideSome( *vars->v, *ixlow );
	
    step->x->axzpy ( 1.0, vInvGamma, *res->rv );
    step->x->axdzpy( 1.0, *res->rgamma, *vars->v, *ixlow );
  }
  if( nxupp > 0 ) {
    OoqpVector & wInvPhi   = *step->w;
    wInvPhi.copyFrom( *vars->phi );
    wInvPhi.divideSome( *vars->w, *ixupp );
	  
    step->x->axzpy (  1.0, wInvPhi,   *res->rw );
    step->x->axdzpy( -1.0, *res->rphi, *vars->w, *ixupp );
  }
  // start by partially computing step->s
  step->s->copyFrom( *res->rz );
  if( mclow > 0 ) {
    OoqpVector & tInvLambda = *step->t;
	
    tInvLambda.copyFrom( *vars->lambda );
    tInvLambda.divideSome( *vars->t, *iclow );

    step->s->axzpy( 1.0, tInvLambda, *res->rt );
    step->s->axdzpy( 1.0, *res->rlambda, *vars->t, *iclow );
  }

  if( mcupp > 0 ) {
    OoqpVector & uInvPi = *step->u;
	
    uInvPi.copyFrom( *vars->pi );
    uInvPi.divideSome( *vars->u, *icupp );

    step->s-> axzpy(  1.0, uInvPi, *res->ru );
    step->s->axdzpy( -1.0, *res->rpi, *vars->u, *icupp );
  }

  step->y->copyFrom( *res->rA );
  step->z->copyFrom( *res->rC );

  {
    // Unfortunately, we need a temporary  OoqpVector for the solve,
    // Use step->lambda or step->pi
    OoqpVectorHandle ztemp;
    if( mclow > 0 ) {
      ztemp = step->lambda;
    } else {
      ztemp = step->pi;
    }

    this->solveXYZS( *step->x, *step->y, *step->z, *step->s,
		     *ztemp, prob );
  }

  if( mclow > 0 ) {
    step->t->copyFrom( *step->s );
    step->t->axpy( -1.0, *res->rt );
    step->t->selectNonZeros( *iclow );

    step->lambda->copyFrom( *res->rlambda );
    step->lambda->axzpy( -1.0, *vars->lambda, *step->t );
    step->lambda->divideSome( *vars->t, *iclow );
    //!
    step->lambda->selectNonZeros( *iclow );
  }
  if( mcupp > 0 ) {
    step->u->copyFrom( *res->ru );
    step->u->axpy( -1.0, *step->s );
    step->u->selectNonZeros( *icupp );

    step->pi->copyFrom( *res->rpi );
    step->pi->axzpy( -1.0, *vars->pi, *step->u );
    step->pi->divideSome( *vars->u, *icupp );
    //!
    step->pi->selectNonZeros( *icupp );
  }
  if( nxlow > 0 ) {
    step->v->copyFrom( *step->x );
    step->v->axpy( -1.0, *res->rv );
    step->v->selectNonZeros( *ixlow );
	
    step->gamma->copyFrom( *res->rgamma );
    step->gamma->axzpy( -1.0, *vars->gamma, *step->v );
    step->gamma->divideSome( *vars->v, *ixlow );
    //!
    step->gamma->selectNonZeros( *ixlow );
  }
  if( nxupp > 0 ) {
    step->w->copyFrom( *res->rw );
    step->w->axpy( -1.0, *step->x );
    step->w->selectNonZeros( *ixupp );
	
    step->phi->copyFrom( *res->rphi );
    step->phi->axzpy( -1.0, *vars->phi, *step->w );
    step->phi->divideSome( *vars->w, *ixupp );
    //!
    step->phi->selectNonZeros( *ixupp );
  }
  assert( step->validNonZeroPattern() );

}


void QpGenLinsys::solveXYZS( OoqpVector& stepx, OoqpVector& stepy,
			       OoqpVector& stepz, OoqpVector& steps,
			       OoqpVector& /* ztemp */,
			       QpGenData* prob )
{
  stepz.axzpy( -1.0, *nomegaInv, steps );
 
  if(gOuterSolve==1) {
    ///////////////////////////////////////////////////////////////
    // Iterative refinement
    ///////////////////////////////////////////////////////////////
    solveCompressedIterRefin(stepx,stepy,stepz,prob);

  } else if(gOuterSolve==0) {
    ///////////////////////////////////////////////////////////////
    // Default solve - Schur complement based decomposition
    ///////////////////////////////////////////////////////////////
    this->joinRHS( *rhs, stepx, stepy, stepz );
    this->solveCompressed( *rhs );
    this->separateVars( stepx, stepy, stepz, *rhs );

  } else {
    assert(gOuterSolve==2);
    ///////////////////////////////////////////////////////////////
    // BiCGStab
    ///////////////////////////////////////////////////////////////
    solveCompressedBiCGStab(stepx,stepy,stepz,prob);
  }
  stepy.negate();
  stepz.negate();
	
  steps.axpy( -1.0, stepz );
  steps.componentMult( *nomegaInv );
  steps.negate();
}


void QpGenLinsys::solveCompressedBiCGStab(OoqpVector& stepx,
					  OoqpVector& stepy,
					  OoqpVector& stepz,
					  QpGenData* data)
{
#ifdef TIMING
  vector<double> histRelResid;
  double tTot=MPI_Wtime(), tSlv=0., tResid=0., tTmp;
#endif

  this->joinRHS( *rhs, stepx, stepy, stepz );

  //aliases
  OoqpVector &r0=*res2, &dx=*sol2, &v=*res3, &t=*res4, &p=*res5;
  OoqpVector &x=*sol, &r=*res, &b=*rhs;

  const double tol=1e-8, EPS=2e-16; 
  const int maxit=500;

  double n2b=b.twonorm(), tolb=n2b*tol;
  int flag; double iter=0.;
  double rho=1., omega=1., alpha;  


#ifdef TIMING
  gOuterBiCGIter=0;
  int myRank; MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
  tTmp=MPI_Wtime();
#endif
  //starting guess/point
  x.copyFrom(b); solveCompressed(x); //solution to the approx. system
#ifdef TIMING
  tSlv += (MPI_Wtime()-tTmp);
  tTmp=MPI_Wtime();
  gOuterBiCGIter++;
#endif
  //initial residual: res=res-A*x
  r.copyFrom(b); matXYZMult(1.0,r, -1.0,x, data, stepx,stepy,stepz);
#ifdef TIMING
  tResid += (MPI_Wtime()-tTmp);
#endif  
  double normr=r.twonorm(), normr_min=normr, normr_act=normr;
#ifdef TIMING
  histRelResid.push_back(normr/n2b);
#endif
  
  //quick return if solve is accurate enough
  if(normr<tolb) { 
     this->separateVars( stepx, stepy, stepz, x );
#ifdef TIMING
     tTot = MPI_Wtime() - tTot;
     if(0==myRank) {
       cout << "Outer BiCGStab not needed. (Rel)res.nrm:" << normr/n2b  << endl;
       cout << "solveXYZS w/ BiCGStab times: solve " << tSlv
	    << "  matvec " << tResid
	    << "  total " << tTot << endl; 
     }
#endif
     return;
  }

  //arbitrary vector
  r0.copyFrom(b);

  //main loop
  int it=0; while(it<maxit) {
    flag=-1; //reset flag
    double rho1=rho; double beta;
    rho=r0.dotProductWith(r);
    if(0.0==rho) {flag=4; break;}

    //first half of the iterate
    {
      if(it==0) p.copyFrom(r);
      else {
	beta = (rho/rho1)*(alpha/omega);
	if(beta==0.0) { flag=4; break; }
	
	//-------- p = r + beta*(p - omega*v) --------
	p.axpy(-omega, v); p.scale(beta); p.axpy(1.0, r);
      }
#ifdef TIMING
      tTmp = MPI_Wtime();
#endif
      //precond: ph = \tilde{K}^{-1} p
      dx.copyFrom(p); solveCompressed(dx); 
#ifdef TIMING
      tSlv += (MPI_Wtime()-tTmp);
      tTmp = MPI_Wtime();
      gOuterBiCGIter++;
#endif
      //mat-vec: v = K*ph
      matXYZMult(0.0,v, 1.0,dx, data, stepx,stepy,stepz);
#ifdef TIMING
      tResid += (MPI_Wtime()-tTmp);
#endif

      double rtv=r0.dotProductWith(v);
      if(rtv==0.0) { flag=4; break; }
      
      alpha=rho/rtv; 
      // x = x + alpha*dx (x=x+alpha*ph)
      x.axpy( alpha, dx);
      // r = r-alpha*v (s=r-alpha*v)
      r.axpy(-alpha, v);
      
      //check for convergence
      normr=r.twonorm();

#ifdef TIMING
      histRelResid.push_back(normr/n2b);
#endif
      if(normr<=tolb) {
#ifdef TIMING
      tTmp=MPI_Wtime();
#endif
	//compute the actual residual
	OoqpVector& res=dx; //use dx 
	res.copyFrom(b); matXYZMult(1.0,res, -1.0,x, data, stepx,stepy,stepz);
#ifdef TIMING
	tResid += (MPI_Wtime()-tTmp);
#endif

	normr_act = res.twonorm();
	if(normr_act<=tolb) {
	  //converged
#ifdef TIMING
	  histRelResid[histRelResid.size()-1]=normr_act/n2b;
#endif
	  flag=0; iter=it+0.5; break;
	}
      } //~end of convergence test
    }

    
    //second half of the iterate now
    {
#ifdef TIMING
      tTmp=MPI_Wtime();
#endif
      //preconditioner
      dx.copyFrom(r); solveCompressed(dx);
#ifdef TIMING
      tSlv += (MPI_Wtime()-tTmp);
      tTmp=MPI_Wtime();
      gOuterBiCGIter++;
#endif
      //mat-vec
      matXYZMult(0.0,t, 1.0,dx, data, stepx,stepy,stepz);
#ifdef TIMING
      tResid += (MPI_Wtime()-tTmp);
#endif
      double tt = t.dotProductWith(t);
      if(tt==0.0) { flag=4; break;}
      
      omega=t.dotProductWith(r); omega /= tt;
      
      // x=x+omega*dx  (x=x+omega*sh)
      x.axpy( omega, dx);
      // r = r-omega*t (r=s-omega*sh)
      r.axpy(-omega, t);
      //check for convergence
      normr=r.twonorm();
#ifdef TIMING
      histRelResid.push_back(normr/n2b);
#endif

      if(normr<=tolb) {
#ifdef TIMING
      tTmp=MPI_Wtime();
#endif
	//compute the actual residual
	OoqpVector& res=dx; //use dx 
	res.copyFrom(b); matXYZMult(1.0,res, -1.0,x, data, stepx,stepy,stepz);
#ifdef TIMING
      tResid += (MPI_Wtime()-tTmp);
#endif

	normr_act = res.twonorm();
	//cout << "Outer BiCG - actual rel.res.nrm: " << normr_act/n2b << endl;
	if(normr_act<=tolb) {
	  //converged
#ifdef TIMING 
	  histRelResid[histRelResid.size()-1]=normr_act/n2b;
#endif
	  flag=0; iter=it+1.; break;
	} // else continue - To Do: detect stagnation (flag==3)
      } else {
	//To Do: detect stagnation/divergence and rollback to min.norm. iterate
	//for now we print a warning and exit in case residual increases.
	if(normr>normr_min) {
#ifdef TIMING
	  if(0==myRank) 
	    cout << "Outer BiCG - Increase in BiCGStab residual. Old=" << normr_min
		 << "  New=" << normr << endl;
#endif
	  flag=5; iter=it+1.; break;
	} else normr_min=normr;
	
      } //~end of convergence test
    } //~end of scoping
    
    it++;
  } //~ end of BiCGStab loop
  
  //warning/error messaging
  if(flag!=0) {
#ifdef TIMING
    if(0==myRank) 
      cout << "Outer BiCG - convergence issues: flag=" << flag << ". "
	   << iter << " iterations" 
	   << " rel.res.norm=" << normr_act/n2b << endl;
#endif
  }
  this->separateVars( stepx, stepy, stepz, x );

#ifdef TIMING
  tTot = MPI_Wtime()-tTot;
  if(0==myRank) {
    cout << "Outer BiCGStab " << iter << " iterations. (Rel)resid.nrm.history:";
    for(size_t it=0; it<histRelResid.size(); it++)
      cout << histRelResid[it] << " | ";    
    cout << endl;
    cout << "solveXYZS w/ BiCGStab times: solve=" << tSlv
	 << "  matvec=" << tResid
	 << "  total=" << tTot << endl; 
  }
#endif  

}


/**
 * res = beta*res - alpha*mat*sol
 * stepx, stepy, stepz are used as temporary buffers
 */
void QpGenLinsys::matXYZMult(double beta,  OoqpVector& res, 
			     double alpha, OoqpVector& sol, 
			     QpGenData* data,
			     OoqpVector& solx, 
			     OoqpVector& soly, 
			     OoqpVector& solz)
{
  this->separateVars( solx, soly, solz, sol );
  this->separateVars( *resx, *resy, *resz, res);

  data->Qmult(beta, *resx, alpha, solx);
  resx->axzpy(alpha, *dd, solx);
  data->ATransmult(1.0, *resx, alpha, soly);
  data->CTransmult(1.0, *resx, alpha, solz);

  data->Amult(beta, *resy, alpha, solx);
  //cout << "resy norm: " << resy->twonorm() << endl;
  data->Cmult(beta, *resz, alpha, solx);
  resz->axzpy(alpha, *nomegaInv, solz);
  //cout << "resz norm: " << resz->twonorm() << endl;
  this->joinRHS( res, *resx, *resy, *resz );
}

void QpGenLinsys::solveCompressedIterRefin(OoqpVector& stepx,
					   OoqpVector& stepy,
					   OoqpVector& stepz,
					   QpGenData* prob)
{
  this->joinRHS( *rhs, stepx, stepy, stepz );
#ifdef TIMING
    int myRank; MPI_Comm_rank(MPI_COMM_WORLD, &myRank);
    vector<double> histRelResid;

    double tTot=MPI_Wtime(), tSlv=0., tResid=0., tTmp;
#endif
    res->copyFrom(*rhs);
    sol->setToZero();
    double bnorm=rhs->twonorm();
    int refinSteps=-1;  double resNorm;

    do{
#ifdef TIMING
      tTmp=MPI_Wtime();
#endif
      this->solveCompressed( *res );
#ifdef TIMING
      tSlv += (MPI_Wtime()-tTmp);
#endif

      //x = x+dx
      sol->axpy(1.0, *res);
      refinSteps++;
      

#ifdef TIMING
      tTmp=MPI_Wtime();
#endif
      res->copyFrom(*rhs);    
      //  stepx, stepy, stepz are used as temporary buffers
      computeResidualXYZ( *sol, *res, stepx, stepy, stepz, prob );
#ifdef TIMING
      tResid += (MPI_Wtime()-tTmp);
#endif 

      resNorm=res->twonorm(); 
#ifdef TIMING
      histRelResid.push_back(resNorm/bnorm);
      //histRelResidInf.push_back(res->infnorm()/bnorm);
      //if(0==myRank) cout << "resid.nrm xyz: " << resNorm << "   "
      //		 << "rhs.nrm xyz: " << bnorm << endl;
#endif      
      
      if(resNorm/bnorm<1e-9)
	break;

    } while(true);
#ifdef TIMING
    tTot = MPI_Wtime() - tTot;
    if(0==myRank) {// && refinSteps>0)  {
      cout << "Outer Iter Refin " << refinSteps 
	   << " iterations. Rel.resid.nrm:"; //Norm rel res:" 
      for(size_t it=0; it<histRelResid.size(); it++) 
	  cout << histRelResid[it] << " | ";
      cout << endl;
      cout << "solveXYZS w/ iter. refin. times: solve=" << tSlv 
	   << "  matvec=" << tResid 
	   << "  total=" << tTot << endl; 
    }
#endif
    this->separateVars( stepx, stepy, stepz, *sol );
}

/**
 * res = res - mat*sol
 * stepx, stepy, stepz are used as temporary buffers
 */
void QpGenLinsys::computeResidualXYZ(OoqpVector& sol, 
				     OoqpVector& res, 
				     OoqpVector& solx, 
				     OoqpVector& soly, 
				     OoqpVector& solz, 
				     QpGenData* data)
{
  this->separateVars( solx, soly, solz, sol );
  this->separateVars( *resx, *resy, *resz, res);

  data->Qmult(1.0, *resx, -1.0, solx);
  resx->axzpy(-1.0, *dd, solx);
  data->ATransmult(1.0, *resx, -1.0, soly);
  data->CTransmult(1.0, *resx, -1.0, solz);
  //cout << "resx norm: " << resx->twonorm() << endl;
  
  data->Amult(1.0, *resy, -1.0, solx);
  //cout << "resy norm: " << resy->twonorm() << endl;
  data->Cmult(1.0, *resz, -1.0, solx);
  resz->axzpy(-1.0, *nomegaInv, solz);
  //cout << "resz norm: " << resz->twonorm() << endl;
  this->joinRHS( res, *resx, *resy, *resz );
}


void QpGenLinsys::joinRHS( OoqpVector& rhs_in,  OoqpVector& rhs1_in,
			     OoqpVector& rhs2_in, OoqpVector& rhs3_in )
{
  // joinRHS has to be delegated to the factory. This is true because
  // the rhs may be distributed across processors, so the factory is the
  // only object that knows with certainly how to scatter the elements.
  factory->joinRHS( rhs_in, rhs1_in, rhs2_in, rhs3_in );
}

void QpGenLinsys::separateVars( OoqpVector& x_in, OoqpVector& y_in,
				  OoqpVector& z_in, OoqpVector& vars_in )
{
  // separateVars has to be delegated to the factory. This is true because
  // the rhs may be distributed across processors, so the factory is the
  // only object that knows with certainly how to scatter the elements.
  factory->separateVars( x_in, y_in, z_in, vars_in );
}

