/****************************************************************************************
* mexschur.c : C mex file to compute  
*          
*    mexschur(blk,Avec,nzlistA1,nzlistA2,permA,U,V,colend,type,schur);  
*
*    schur(I,J) = schur(I,J) + Trace(Ai U Aj V),
*    where I = permA[i], J = permA[j],   1<=i,j<=colend. 
* 
*   input:  blk  = 1x2 a cell array describing the block structure of A.
*           Avec =  
*           nzlistA = 
*           permA = a permutation vector.  
*           U,V  = real symmetric matrices.
*           type = 0, compute Trace(Ai*(U Aj V + V Aj U)/2)) = Trace(Ai*(U Aj V))
*                = 1, compute Trace(Ai*(U Aj U)).
*
* SDPT3: version 3.0
* Copyright (c) 1997 by
* K.C. Toh, M.J. Todd, R.H. Tutuncu
* Last Modified: 2 Feb 01   
****************************************************************************************/

#include <math.h>
#include <mex.h>

#if !defined(MAX)
#define  MAX(A, B)   ((A) > (B) ? (A) : (B))
#endif

#if !defined(r2)
#define  r2   1.41421356237309504880      /* sqrt(2) */
#endif

#if !defined(ir2)
#define  ir2  0.70710678118654752440      /* 1/sqrt(2) */
#endif

/**********************************************************
*  compute  Trace(B U*A*U)
*
*  A,B are assumed to be real,sparse,symmetric.
*  U  is assumed to be real,dense,symmetric. 
**********************************************************/
void schurij1( int n, 
               double *Avec, int *idxstart, int *nzlistAi, int *nzlistAj,
               double *U, int col, double *schurcol)

{ int    i, ra, ca, rb, cb, rbn, cbn, l, k, kstart, kend, lstart, lend; 
  double tmp1, tmp2, tmp3, tmp4; 

  lstart = idxstart[col]; lend = idxstart[col+1]; 

  for (i=0; i<=col; i++) {
     kstart = idxstart[i]; kend = idxstart[i+1]; 
     tmp1 = 0; tmp2 = 0;  
     for (l=lstart; l<lend; ++l) { 
        rb = nzlistAi[l];    
        cb = nzlistAj[l];
        if (rb > cb) { mexErrMsgTxt("mexschur: nzlistA2 is incorrect"); }
        rbn = rb*n; cbn = cb*n;   
        tmp3 = 0; tmp4 = 0;
        for (k=kstart; k<kend; ++k) { 
           ra = nzlistAi[k];
           ca = nzlistAj[k];             
           if (ra<ca) {  
              tmp3 += Avec[k] * (U[ra+rbn]*U[ca+cbn]+U[ra+cbn]*U[ca+rbn]); }
           else { 
              tmp4 += Avec[k] * (U[ra+rbn]*U[ca+cbn]); }
	}
        if (rb<cb) { tmp1 += Avec[l]*(ir2*tmp3 + tmp4); }
        else       { tmp2 += Avec[l]*(ir2*tmp3 + tmp4); } 
     }
     schurcol[i] = r2*tmp1+tmp2; 
  }
return;
}
/**********************************************************
*  compute  Trace(B (U*A*V + V*A*U)/2) = Trace(B U*A*V)
*
*  A,B are assumed to be real,sparse,symmetric.
*  U,V are assumed to be real,dense,symmetric. 
**********************************************************/
void schurij3(int n,  
              double *Avec, int *idxstart, int *nzlistAi, int *nzlistAj,
              double *U, double *V, int col, double *schurcol)

{ int    ra, ca, rb, cb, rbn, cbn, l, k, idx1, idx2, idx3, idx4;
  int    i, kstart, kend, lstart, lend; 
  double tmp1, tmp2, tmp3, tmp4; 

  lstart = idxstart[col]; lend = idxstart[col+1]; 

  for (i=0; i<=col; i++) {
     kstart = idxstart[i]; kend = idxstart[i+1]; 
     tmp1 = 0; tmp2 = 0;  
     for (l=lstart; l<lend; ++l) { 
        rb = nzlistAi[l];    
        cb = nzlistAj[l];
        if (rb > cb) { mexErrMsgTxt("mexschur: nzlistA2 is incorrect"); }
        rbn = rb*n; cbn = cb*n;   
        tmp3 = 0; tmp4 = 0; 
        for (k=kstart; k<kend; ++k) { 
           ra = nzlistAi[k];
           ca = nzlistAj[k];
           idx1 = ra+rbn; idx2 = ca+cbn;
           if (ra<ca) { 
              idx3 = ra+cbn; idx4 = ca+rbn; 
	      tmp3 += Avec[k] *(U[idx1]*V[idx2]+U[idx2]*V[idx1] \
                                +U[idx3]*V[idx4]+U[idx4]*V[idx3]);  }
           else {
	      tmp4 += Avec[k] * (U[idx1]*V[idx2]+U[idx2]*V[idx1]);  }
	}
        if (rb<cb) { tmp1 += Avec[l]*(ir2*tmp3+tmp4); }
        else       { tmp2 += Avec[l]*(ir2*tmp3+tmp4); } 
     }
     schurcol[i] = ir2*tmp1+tmp2/2; 
  }
return; 
}
/**********************************************************
* stack multiple blocks into a long column vector
**********************************************************/
void vec(int numblk, int *cumblksize, int *blknnz, 
         double *A, int *irA, int *jcA, double *B) 

{  int idx0, idx, i, j, l, jstart, jend, istart, blksize;
   int k, kstart, kend; 
   
      for (l=0; l<numblk; l++) { 
  	  jstart = cumblksize[l]; 
  	  jend   = cumblksize[l+1];  
          blksize = jend-jstart; 
          istart = jstart;
          idx0 = blknnz[l]; 
          for (j=jstart; j<jend; j++) { 
              idx = idx0 + (j-jstart)*blksize; 
              kstart = jcA[j]; kend = jcA[j+1]; 
              for (k=kstart; k<kend; k++) { 
                  i = irA[k];
                  B[idx+i-istart] = A[k]; }
          }
      }  
return;
}
/**********************************************************
*  compute  Trace(B U*A*U)
*
*  A,B are assumed to be real,sparse,symmetric.
*  U  is assumed to be real,sparse,symmetric. 
**********************************************************/
void schurij2( double *Avec, 
               int *idxstart, int *nzlistAi, int *nzlistAj, double *Utmp,
               int *nzlistAr, int *nzlistAc, int *cumblksize, 
               int *blkidx, int col, double *schurcol)

{ int    r, ra, ca, rb, cb, l, k, kstart, kend, kstartnew, lstart, lend;
  int    colcb1, idxrb, idxcb, idx1, idx2, idx3, idx4;
  int    i, cblk, calk, firstime; 
  double tmp0, tmp1, tmp2, tmp3, tmp4; 

  lstart = idxstart[col]; lend = idxstart[col+1]; 

  for (i=0; i<=col; i++) { 
      kstart = idxstart[i]; kend = idxstart[i+1]; 
      kstartnew = kstart;
      tmp1 = 0; tmp2 = 0; 
      for (l=lstart; l<lend; ++l) { 
          rb = nzlistAi[l];    
          cb = nzlistAj[l];
          cblk = blkidx[cb];  
          idxcb = nzlistAc[l]; 
          idxrb = nzlistAr[l];
          tmp3 = 0; tmp4 = 0; firstime = 1; 
          for (k=kstart; k<kend; ++k) { 
              ca = nzlistAj[k];
              calk = blkidx[ca]; 
              if (calk==cblk) {
                 ra = nzlistAi[k];
                 idx1 = ra+idxrb; idx2 = ca+idxcb; 
                 if (ra<ca) {  
                    idx3 = ra+idxcb; idx4 = ca+idxrb; 
                    tmp3 += Avec[k] * (Utmp[idx1]*Utmp[idx2]+Utmp[idx3]*Utmp[idx4]); }
                 else {
	            tmp4 += Avec[k] * (Utmp[idx1]*Utmp[idx2]); }
                 if (firstime) { kstartnew = k; firstime = 0; } 
	      }
              else if (calk > cblk) {
	         break;
              }
	  }
          kstart = kstartnew; 
          if (rb<cb) { tmp1 += Avec[l]*(ir2*tmp3 + tmp4); }
          else       { tmp2 += Avec[l]*(ir2*tmp3 + tmp4); } 
      }
      tmp0 = r2*tmp1+tmp2; 
      schurcol[i] = tmp0; 
  }
return;
}
/**********************************************************
*  compute  Trace(B (U*A*V + V*A*U)/2) = Trace(B U*A*V)
*
*  A,B are assumed to be real,sparse,symmetric.
*  U,V are assumed to be real,sparse,symmetric. 
**********************************************************/
void schurij4( double *Avec, 
               int *idxstart, int *nzlistAi, int *nzlistAj,
               double *Utmp, double *Vtmp, 
               int *nzlistAr, int *nzlistAc, int *cumblksize, 
               int *blkidx, int col, double *schurcol)

{ int    r, ra, ca, rb, cb, l, k, kstart, kend, kstartnew, lstart, lend;
  int    colcb1, idxrb, idxcb, idx1, idx2, idx3, idx4; 
  int    i, cblk, calk, firstime;
  double tmp0, tmp1, tmp2, tmp3, tmp4;
  double hlf=0.5;  

  lstart = idxstart[col]; lend = idxstart[col+1];

  for (i=0; i<=col; i++) {
      kstart = idxstart[i]; kend = idxstart[i+1]; 
      kstartnew = kstart;
      tmp1 = 0; tmp2 = 0;  
      for (l=lstart; l<lend; ++l) { 
          rb = nzlistAi[l];    
          cb = nzlistAj[l];
          cblk = blkidx[cb];  
          idxcb = nzlistAc[l]; 
          idxrb = nzlistAr[l];
          tmp3 = 0; tmp4 = 0; firstime = 1; 
          for (k=kstart; k<kend; ++k) { 
              ca = nzlistAj[k];
              calk = blkidx[ca]; 
              if (calk == cblk) { 
                 ra = nzlistAi[k];
                 idx1 = ra+idxrb; idx2 = ca+idxcb; 
                 if (ra<ca) {
                    idx3 = ra+idxcb; idx4 = ca+idxrb; 
	            tmp3 += Avec[k] * (Utmp[idx1]*Vtmp[idx2] +Utmp[idx2]*Vtmp[idx1] \
                                   +Utmp[idx3]*Vtmp[idx4] +Utmp[idx4]*Vtmp[idx3]); 
                 } else {
	            tmp4 += Avec[k] * (Utmp[idx1]*Vtmp[idx2] +Utmp[idx2]*Vtmp[idx1]); 
		 }
                 if (firstime) { kstartnew = k; firstime = 0; }  
	      }
              else if (calk > cblk) {
	         break;
	      }
	  }
          kstart = kstartnew; 
          if (rb<cb) { tmp1 += Avec[l]*(ir2*tmp3 + tmp4); }
          else       { tmp2 += Avec[l]*(ir2*tmp3 + tmp4); } 
      }
      tmp0 = ir2*tmp1+hlf*tmp2;
      schurcol[i] = tmp0;
  }
return; 
}
/**********************************************************/
void mexFunction(
     int nlhs,   mxArray  *plhs[], 
     int nrhs,   const mxArray  *prhs[] )
{    
     mxArray  *blk_cell_pr;  
     double   *Avec, *idxstarttmp, *nzlistAtmp, *permAtmp, *U, *V, *schur;
     double   *blksizetmp, *Utmp, *Vtmp, *schurcol, *nzschur, *P;  
     int      *idxstart, *irP, *jcP; 
     int      *irU, *jcU, *irV, *jcV, *colm, *permA, *nzlistAr, *nzlistAc;
     int      *nzlistAi, *nzlistAj, *blksize, *cumblksize, *blknnz, *blkidx; 

     int      subs[2];
     int      nsubs=2; 
     int      index, colend, type, isspU, isspV, numblk, nzP; 
     int      len, row, col, nU, nV, n, m, m1, idx1, idx2, l, k, nsub, n1, n2, opt, opt2;
     int      kstart, kend, rb, cb, cblk, colcb, count; 
     double   tmp; 

/* CHECK THE DIMENSIONS */

    if (nrhs != 10) {
       mexErrMsgTxt(" mexschur: must have 10 inputs"); }
    if (!mxIsCell(prhs[0])) {
       mexErrMsgTxt("mexschur: 1ST input must be the cell array blk"); }  
    if (mxGetM(prhs[0])>1) {
       mexErrMsgTxt("mexschur: blk can have only 1 row"); }  
    subs[0] = 0; 
    subs[1] = 1;
    index = mxCalcSingleSubscript(prhs[0],nsubs,subs); 
    blk_cell_pr = mxGetCell(prhs[0],index); 
    numblk  = mxGetN(blk_cell_pr);
    blksizetmp = mxGetPr(blk_cell_pr); 
    blksize = mxCalloc(numblk,sizeof(int)); 
    for (k=0; k<numblk; k++) { 
        blksize[k] = (int)blksizetmp[k];
    }
/**** get pointers ****/    

    Avec = mxGetPr(prhs[1]); 
    if (!mxIsSparse(prhs[1])) { 
       mexErrMsgTxt("mexschur: Avec must be sparse"); }
    idxstarttmp = mxGetPr(prhs[2]);  
    len = MAX(mxGetM(prhs[2]),mxGetN(prhs[2])); 
    idxstart = mxCalloc(len,sizeof(int)); 
    for (k=0; k<len; k++) { 
        idxstart[k] = (int)idxstarttmp[k]; 
    }
    nzlistAtmp = mxGetPr(prhs[3]); 
    len = mxGetM(prhs[3]);
    nzlistAi = mxCalloc(len,sizeof(int)); 
    nzlistAj = mxCalloc(len,sizeof(int)); 
    for (k=0; k<len; k++) { 
        nzlistAi[k] = (int)nzlistAtmp[k] -1; /* -1 to adjust for matlab index */
        nzlistAj[k] = (int)nzlistAtmp[k+len] -1; 
    }
    permAtmp = mxGetPr(prhs[4]); 
    m1 = mxGetN(prhs[4]); 
    permA = mxCalloc(m1,sizeof(int)); 
    for (k=0; k<m1; k++) {
        permA[k] = (int)permAtmp[k]-1; /* -1 to adjust for matlab index */
    }
    U = mxGetPr(prhs[5]);  nU = mxGetM(prhs[5]); 
    isspU = mxIsSparse(prhs[5]); 
    if (isspU) { irU = mxGetIr(prhs[5]); jcU = mxGetJc(prhs[5]); }
    V = mxGetPr(prhs[6]);  nV = mxGetM(prhs[6]); 
    isspV = mxIsSparse(prhs[6]);
    if (isspV) { irV = mxGetIr(prhs[6]); jcV = mxGetJc(prhs[6]); }
    if ((isspU & !isspV) || (!isspU & isspV)) { 
       mexErrMsgTxt("mexschur: U,V must be both dense or both sparse"); 
    }
    colend = (int)*mxGetPr(prhs[7]); 
    type   = (int)*mxGetPr(prhs[8]); 

    schur = mxGetPr(prhs[9]); 
    m = mxGetM(prhs[9]);    
    if (m!= m1) {
       mexErrMsgTxt("mexschur: schur and permA are not compatible"); }

/************************************
* output 
************************************/

    plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL); 
    nzschur = mxGetPr(plhs[0]); 

/************************************
* initialization 
************************************/
    if (isspU & isspV) { 
       cumblksize = mxCalloc(numblk+1,sizeof(int)); 
       blknnz = mxCalloc(numblk+1,sizeof(int)); 
       cumblksize[0] = 0; blknnz[0] = 0; 
       n1 = 0; n2 = 0; 
       for (k=0; k<numblk; ++k) {
           nsub = blksize[k];
           n1 += nsub;  
           n2 += nsub*nsub;  
           cumblksize[k+1] = n1; 
           blknnz[k+1] = n2;  }
       if (nU != n1 || nV != n1) { 
          mexErrMsgTxt("mexschur: blk and dimension of U not compatible"); }
       Utmp = mxCalloc(n2,sizeof(double)); 
       vec(numblk,cumblksize,blknnz,U,irU,jcU,Utmp); 
       Vtmp = mxCalloc(n2,sizeof(double)); 
       vec(numblk,cumblksize,blknnz,V,irV,jcV,Vtmp); 
       blkidx = mxCalloc(nU,sizeof(int));
       for (l=0; l<numblk; l++) {  
 	   kstart=cumblksize[l]; kend=cumblksize[l+1];
           for (k=kstart; k<kend; k++) { blkidx[k] = l; }           
       }
       nzlistAc = mxCalloc(len,sizeof(int)); 
       nzlistAr = mxCalloc(len,sizeof(int)); 
       for (k=0; k<len; k++) {
          rb = nzlistAi[k]; 
	  cb = nzlistAj[k]; 
	  cblk = blkidx[cb]; colcb = cumblksize[cblk];             
          nzlistAc[k] = blknnz[cblk]+(cb-colcb)*blksize[cblk]-colcb;
          nzlistAr[k] = blknnz[cblk]+(rb-colcb)*blksize[cblk]-colcb;  
       }
    }
/************************************
* compute schur(i,j)
************************************/

    colm = mxCalloc(colend,sizeof(int));     
    for (k=0; k<colend; k++) { colm[k] = permA[k]*m; } 

    n = nU; 
    if      (type==1 & !isspU)  { opt=1; }
    else if (type==0 & !isspU)  { opt=3; } 
    else if (type==1 &  isspU)  { opt=2; }
    else if (type==0 &  isspU)  { opt=4; }

    /*************************************/
    schurcol = mxCalloc(colend,sizeof(double)); 
    count = 0;
 
    if (opt==1) { 
       for (col=0; col<colend; col++) { 
  	   schurij1(n,Avec,idxstart,nzlistAi,nzlistAj,U,col,schurcol);
           for (row=0; row<=col; row++) {
	       if (schurcol[row] != 0) {
		  count++; 
   	          idx1 = permA[row]+colm[col]; 
                  idx2 = permA[col]+colm[row]; 
                  schur[idx1] += schurcol[row];
                  schur[idx2] = schur[idx1]; }
	   }
       }
    }
    else if (opt==3) {
       for (col=0; col<colend; col++) { 
           schurij3(n,Avec,idxstart,nzlistAi,nzlistAj,U,V,col,schurcol);
           for (row=0; row<=col; row++) { 
               if (schurcol[row] != 0) {
		  count++; 
   	          idx1 = permA[row]+colm[col]; 
                  idx2 = permA[col]+colm[row]; 
                  schur[idx1] += schurcol[row];
                  schur[idx2] = schur[idx1];  }
	   }
       }
    }
    else if (opt == 2) { 
       for (col=0; col<colend; col++) { 
           schurij2(Avec,idxstart,nzlistAi,nzlistAj,Utmp, \
		    nzlistAr,nzlistAc,cumblksize,blkidx,col,schurcol); 
           for (row=0; row<=col; row++) { 
	       if (schurcol[row] != 0) {
	          count++; 
   	          idx1 = permA[row]+colm[col]; 
                  idx2 = permA[col]+colm[row]; 
                  schur[idx1] += schurcol[row];
                  schur[idx2] = schur[idx1]; }
	   }	   
       }
    }
    else if (opt==4) {
       for (col=0; col<colend; col++) { 
           schurij4(Avec,idxstart,nzlistAi,nzlistAj,Utmp,Vtmp, \
	   nzlistAr,nzlistAc,cumblksize,blkidx,col,schurcol);  
   	   for (row=0; row<=col; row++) {
	       if (schurcol[row] != 0) {
   		  count++; 
 	          idx1 = permA[row]+colm[col]; 
                  idx2 = permA[col]+colm[row]; 
                  schur[idx1] += schurcol[row];
                  schur[idx2] = schur[idx1]; } 
	   }	   
       }
    }
    nzschur[0] = count;

    mxFree(blksize); mxFree(nzlistAi); mxFree(nzlistAj); 
    mxFree(permA);   mxFree(idxstart); mxFree(schurcol); 
    if (isspU) { 
       mxFree(Utmp);     mxFree(Vtmp); 
       mxFree(nzlistAc); mxFree(nzlistAr); 
       mxFree(blknnz); mxFree(cumblksize); mxFree(blkidx); 
    } 
return;
}
/**********************************************************/



