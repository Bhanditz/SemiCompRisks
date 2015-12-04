#include <stdio.h>
#include <math.h>

#include "gsl/gsl_matrix.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_blas.h"
#include "gsl/gsl_sort_vector.h"
#include "gsl/gsl_heapsort.h"
#include "gsl/gsl_sf.h"

#include "gsl/gsl_rng.h"
#include "gsl/gsl_randist.h"

#include "R.h"
#include "Rmath.h"

#include "BpeDpCorSurv.h"



/* updating regression parameter: beta */

/**/

void BpeDpCorSurv_updateRP(gsl_vector *beta,
                            gsl_vector *xbeta,
                           gsl_vector *lambda,
                           gsl_vector *s,
                           int K,
                           gsl_vector *V,
                           gsl_vector *survTime,
                           gsl_vector *survEvent,
                           gsl_vector *cluster,
                           gsl_matrix *survCov,
                           gsl_vector *accept_beta)
{
    double D1, D2, logLH, Del;
    double D1_prop, D2_prop, logLH_prop;
    double beta_prop_me, beta_prop_var, temp_prop;
    double beta_prop_me_prop, beta_prop_var_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int u;
    
    int n = survTime -> size;
    int p = survCov -> size2;
    int i, j, jj, k;
    
    gsl_vector *beta_prop = gsl_vector_calloc(p);
        
    j = (int) runif(0, p);
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        
        
        if(gsl_vector_get(survEvent, i) == 1)
        {
            logLH   += gsl_vector_get(xbeta, i);
            D1      += gsl_matrix_get(survCov, i, j);
        }
        
        for(k = 0; k < K+1; k++)
        {
            if(k > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - gsl_vector_get(s, k-1)));
            }
            if(k == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - 0);
            }
            if(Del > 0)
            {
                logLH   += - Del*exp(gsl_vector_get(lambda, k))*exp(gsl_vector_get(xbeta, i) + gsl_vector_get(V, jj));
                D1      += - Del*exp(gsl_vector_get(lambda, k))*exp(gsl_vector_get(xbeta, i) + gsl_vector_get(V, jj))*gsl_matrix_get(survCov, i, j);
                D2      += - Del*exp(gsl_vector_get(lambda, j))*exp(gsl_vector_get(xbeta, i) + gsl_vector_get(V, jj))*pow(gsl_matrix_get(survCov, i, j), 2);
            }
        }
    }
    
    beta_prop_me    = gsl_vector_get(beta, j) - D1/D2;
    beta_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(beta_prop_me, sqrt(beta_prop_var));
    
    gsl_vector_memcpy(beta_prop, beta);
    gsl_vector_set(beta_prop, j, temp_prop);
    
    gsl_vector *xbeta_prop = gsl_vector_calloc(n);
    gsl_blas_dgemv(CblasNoTrans, 1, survCov, beta_prop, 0, xbeta_prop);
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        
        if(gsl_vector_get(survEvent, i) == 1)        
        {
            logLH_prop   += gsl_vector_get(xbeta_prop, i);
            D1_prop      += gsl_matrix_get(survCov, i, j);
        }
        
        for(k = 0; k < K+1; k++)
        {
            if(k > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - gsl_vector_get(s, k-1)));
            }
            if(k == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - 0);
            }
            if(Del > 0)
            {
                logLH_prop   += - Del*exp(gsl_vector_get(lambda, k))*exp(gsl_vector_get(xbeta_prop, i) + gsl_vector_get(V, jj));
                D1_prop      += - Del*exp(gsl_vector_get(lambda, k))*exp(gsl_vector_get(xbeta_prop, i) + gsl_vector_get(V, jj))*gsl_matrix_get(survCov, i, j);
                D2_prop      += - Del*exp(gsl_vector_get(lambda, j))*exp(gsl_vector_get(xbeta_prop, i) + gsl_vector_get(V, jj))*pow(gsl_matrix_get(survCov, i, j), 2);
            }
        }
    }
     
    
    beta_prop_me_prop   = temp_prop - D1_prop/D2_prop;
    beta_prop_var_prop  = - pow(2.4, 2)/D2_prop;
    
    logProp_IniToProp = dnorm(temp_prop, beta_prop_me, sqrt(beta_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(beta, j), beta_prop_me_prop, sqrt(beta_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logProp_PropToIni - logProp_IniToProp;
    
    u = log(runif(0, 1)) < logR;


    
    if(u == 1)
    {
        gsl_vector_set(beta, j, temp_prop);
        gsl_vector_swap(xbeta, xbeta_prop);
        gsl_vector_set(accept_beta, j, (gsl_vector_get(accept_beta, j) + u));
    }
    
    gsl_vector_free(beta_prop);
    gsl_vector_free(xbeta_prop);    
    
    return;
}







/* updating log-baseline hazard function parameter: lambda */



void BpeDpCorSurv_updateBH(gsl_vector *lambda,
                            gsl_vector *s,
                            gsl_vector *xbeta,
                            gsl_vector *V,
                            gsl_vector *survTime,
                            gsl_vector *survEvent,
                            gsl_vector *cluster,
                            gsl_matrix *Sigma_lam,
                            gsl_matrix *invSigma_lam,
                            gsl_matrix *W,
                            gsl_matrix *Q,
                            double mu_lam,
                            double sigSq_lam,
                            int K)
{
    double D1, D2, logLH, Del, inc;
    double D1_prop, D2_prop, logLH_prop;
    double lambda_prop_me, lambda_prop_var, temp_prop;
    double lambda_prop_me_prop, lambda_prop_var_prop;
    double logPrior, logPrior_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int u, i, j, jj;
    
    double nu_lam, nu_lam_prop;
    
    int n = xbeta -> size;
    
    j = (int) runif(0, K+1);
    
    
    if(K+1 > 1)
    {
        if(j == 0) nu_lam = mu_lam + gsl_matrix_get(W, 0, 1) * (gsl_vector_get(lambda, 1) - mu_lam);
        if(j == K) nu_lam = mu_lam + gsl_matrix_get(W, K, K-1) * (gsl_vector_get(lambda, K-1) - mu_lam);
        if(j != 0 && j !=K) nu_lam = mu_lam + gsl_matrix_get(W, j, j-1) * (gsl_vector_get(lambda, j-1) - mu_lam) + gsl_matrix_get(W, j, j+1) * (gsl_vector_get(lambda, j+1) - mu_lam);
    }
    
    if(K+1 == 1)
    {
        nu_lam = mu_lam;
    }
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    
    gsl_vector *Delta = gsl_vector_calloc(n);
    
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        
        if(gsl_vector_get(survEvent, i) == 1)
        {
            if(j == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s, 0))
            {
                logLH += gsl_vector_get(lambda, j);
                D1 += 1;
            }
            if(j != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s, j-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s, j))
            {
                logLH += gsl_vector_get(lambda, j);
                D1 += 1;
            }
            
        }
        
        if(j > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s, j), gsl_vector_get(survTime, i)) - gsl_vector_get(s, j-1)));
        }
        if(j == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s, j), gsl_vector_get(survTime, i)) - 0);
        }
        
        gsl_vector_set(Delta, i, Del);
        
        if(Del > 0)
        {
            inc = - Del * exp(gsl_vector_get(lambda, j))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
            logLH   += inc;
            D1      += inc;
            D2      += inc;
        }
    }
    
    
    
    
    D1      += -1/(sigSq_lam * gsl_matrix_get(Q, j, j))*(gsl_vector_get(lambda, j)-nu_lam);
    D2      += -1/(sigSq_lam * gsl_matrix_get(Q, j, j));
    
    
    lambda_prop_me    = gsl_vector_get(lambda, j) - D1/D2;
    lambda_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(lambda_prop_me, sqrt(lambda_prop_var));
    
    gsl_vector *lambda_prop = gsl_vector_calloc(K+1);
    
    gsl_vector_view lambda_sub = gsl_vector_subvector(lambda, 0, K+1);
    
    gsl_vector_memcpy(lambda_prop, &lambda_sub.vector);
    gsl_vector_set(lambda_prop, j, temp_prop);
    
    if(K+1 > 1)
    {
        if(j == 0) nu_lam_prop = mu_lam + gsl_matrix_get(W, 0, 1) * (gsl_vector_get(lambda_prop, 1) - mu_lam);
        if(j == K) nu_lam_prop = mu_lam + gsl_matrix_get(W, K, K-1) * (gsl_vector_get(lambda_prop, K-1) - mu_lam);
        if(j != 0 && j != K) nu_lam_prop = mu_lam + gsl_matrix_get(W, j, j-1) * (gsl_vector_get(lambda_prop, j-1) - mu_lam) + gsl_matrix_get(W, j, j+1) * (gsl_vector_get(lambda_prop, j+1) - mu_lam);
    }
    
    if(K+1 == 1)
    {
        nu_lam_prop = mu_lam;
    }
    
    
    
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        if(gsl_vector_get(survEvent, i) == 1)
        {
            if(j == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s, 0))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            if(j != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s, j-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s, j))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            
        }
        
         
        Del = gsl_vector_get(Delta, i);
        
        
        if(Del > 0)
        {
            inc = - Del * exp(gsl_vector_get(lambda_prop, j))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
            logLH_prop   += inc;
            D1_prop      += inc;
            D2_prop      += inc;
        }
    }
    
    
    D1_prop += -1/(sigSq_lam * gsl_matrix_get(Q, j, j))*(gsl_vector_get(lambda_prop, j)-nu_lam);
    D2_prop += -1/(sigSq_lam * gsl_matrix_get(Q, j, j));
    
    
    
    
    lambda_prop_me_prop    = gsl_vector_get(lambda_prop, j) - D1_prop/D2_prop;
    lambda_prop_var_prop   = - pow(2.4, 2)/D2_prop;
    
    gsl_matrix_view invS_sub = gsl_matrix_submatrix(invSigma_lam, 0, 0, K+1, K+1);
    
    if(K+1 > 1)
    {
        c_dmvnormSH(&lambda_sub.vector, mu_lam, sqrt(sigSq_lam), &invS_sub.matrix, &logPrior);
        c_dmvnormSH(lambda_prop, mu_lam, sqrt(sigSq_lam), &invS_sub.matrix, &logPrior_prop);
    }
    if(K+1 == 1)
    {
        logPrior        = dnorm(gsl_vector_get(lambda, j), mu_lam, sqrt(sigSq_lam*gsl_matrix_get(Sigma_lam, 0, 0)), 1);
        logPrior_prop   = dnorm(temp_prop, mu_lam, sqrt(sigSq_lam*gsl_matrix_get(Sigma_lam, 0, 0)), 1);
    }
    
    logProp_IniToProp = dnorm(temp_prop, lambda_prop_me, sqrt(lambda_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(lambda, j), lambda_prop_me_prop, sqrt(lambda_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logPrior_prop - logPrior +  logProp_PropToIni - logProp_IniToProp;
    
    
    u = log(runif(0, 1)) < logR;
    
    if(u == 1) gsl_vector_set(lambda, j, temp_prop);
    
    gsl_vector_free(lambda_prop);
    gsl_vector_free(Delta);
    

    
    return;
}













/* Updating second stage survival components: mu_lam and sigSq_lam */

void BpeDpCorSurv_updateSP(double *mu_lam,
                            double *sigSq_lam,
                            gsl_vector *lambda,
                            gsl_matrix *Sigma_lam,
                            gsl_matrix *invSigma_lam,
                            double a,
                            double b,
                            int K)
{
    double num, den, sigSH, sigRT, sigSC, tau, mu_lam_mean, mu_lam_var;
    
    gsl_vector *ones = gsl_vector_calloc(K+1);
    gsl_vector_set_all(ones, 1);
    
    gsl_matrix_view invSlam_sub = gsl_matrix_submatrix(invSigma_lam, 0, 0, K+1, K+1);
    gsl_vector_view lam_sub     = gsl_vector_subvector(lambda, 0, K+1);
    
    c_quadform_vMu(ones, &invSlam_sub.matrix, &lam_sub.vector, &num);
    c_quadform_vMv(ones, &invSlam_sub.matrix, &den);
    
    mu_lam_mean = num/den;
    mu_lam_var = *sigSq_lam/den;
    
    *mu_lam = rnorm(mu_lam_mean, sqrt(mu_lam_var));
    
    gsl_vector *diff = gsl_vector_calloc(K+1);
    gsl_vector_set_all(diff, *mu_lam);
    gsl_vector_sub(diff, &lam_sub.vector);
    c_quadform_vMv(diff, &invSlam_sub.matrix, &sigRT);
    sigRT /= 2;
    sigRT += b;
    sigSC = 1/sigRT;
    sigSH = a + (double) (K+1)/2;
    tau = rgamma(sigSH, sigSC);
    *sigSq_lam = 1/tau;
    
    gsl_vector_free(ones);
    gsl_vector_free(diff);
    
    return;
}















/* Updating the number of splits and their positions: K and s (Birth move) */


void BpeDpCorSurv_updateBI(gsl_vector *s,
                            int *K,
                            int *accept_BI,
                            gsl_vector *survTime,
                            gsl_vector *survEvent,
                            gsl_vector *xbeta,
                            gsl_vector *V,
                            gsl_vector *cluster,
                            gsl_matrix *Sigma_lam,
                            gsl_matrix *invSigma_lam,
                            gsl_matrix *W,
                            gsl_matrix *Q,
                            gsl_vector *lambda,
                            gsl_vector *s_propBI,
                            int num_s_propBI,
                            double delPert,
                            int alpha,
                            double c_lam,
                            double mu_lam,
                            double sigSq_lam,
                            double s_max)
{
    int count, num_s_propBI_fin, skip;
    int star_inx, j_old, K_new, i, j, u, jj;
    double s_star, Upert, newLam1, newLam2;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta -> size;
    
    count = 0;
    
    gsl_vector *interInx = gsl_vector_calloc(num_s_propBI);
    
    for(i = 0; i < num_s_propBI; i++)
    {
        for(j = 0; j < *K+1; j++)
        {
            if(gsl_vector_get(s_propBI, i) == gsl_vector_get(s, j))
            {
                count += 1;
                gsl_vector_set(interInx, count-1, i);
            }
        }
    }
    
    gsl_vector *s_propBI_fin = gsl_vector_calloc(num_s_propBI-count);
    
    num_s_propBI_fin = s_propBI_fin -> size;
    skip = 0;
    
    if(count > 0)
    {
        for(i = 0; i < num_s_propBI; i++)
        {
            if(i != gsl_vector_get(interInx, skip))
            {
                gsl_vector_set(s_propBI_fin, i-skip, gsl_vector_get(s_propBI, i));
            }
            if(i == gsl_vector_get(interInx, skip)) skip += 1;
        }
    }
    if(count == 0) gsl_vector_memcpy(s_propBI_fin, s_propBI);
    
    
    star_inx = (int) runif(0, num_s_propBI_fin);
    s_star = gsl_vector_get(s_propBI_fin, star_inx);
    
    j_old = -1;
    i = 0;
    
    while(j_old < 0)
    {
        if(gsl_vector_get(s, i) >= s_star) j_old = i;
        else i += 1;
    }
    
    gsl_vector *s_new = gsl_vector_calloc(*K+2);
    for(i = 0; i < *K+1; i++)
    {
        gsl_vector_set(s_new, i, gsl_vector_get(s, i));
    }
    gsl_vector_set(s_new, *K+1, s_star);
    gsl_sort_vector(s_new);
    
    K_new = *K+1;
    
    Upert = runif(0.5 - delPert, 0.5 + delPert);
        
    if(j_old != 0)
    {
        newLam1 = gsl_vector_get(lambda, j_old) - (gsl_vector_get(s, j_old) - s_star)/(gsl_vector_get(s, j_old) - gsl_vector_get(s, j_old-1)) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda, j_old) + (s_star - gsl_vector_get(s, j_old-1))/(gsl_vector_get(s, j_old) - gsl_vector_get(s, j_old-1)) * log((1-Upert)/Upert);
    }
    
    if(j_old == 0)
    {
        newLam1 = gsl_vector_get(lambda, j_old) - (gsl_vector_get(s, j_old) - s_star)/(gsl_vector_get(s, j_old) - 0) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda, j_old) + (s_star - 0)/(gsl_vector_get(s, j_old) - 0) * log((1-Upert)/Upert);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(*K+2);
    
    skip = 0;
    for(i = 0; i < *K+2; i++)
    {
        if(i == j_old)
        {
            gsl_vector_set(lambda_new, i, newLam1);
        }
        else if(i == j_old + 1)
        {
            skip += 1;
            gsl_vector_set(lambda_new, i, newLam2);
        }
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda, i-skip));
    }
    
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(K_new+1, K_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam, K_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        
        if(gsl_vector_get(survEvent, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s, 0))
            {
                logLH += gsl_vector_get(lambda, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s, j_old-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s, j_old))
            {
                logLH += gsl_vector_get(lambda, j_old);
            }
        }
        
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s, j_old), gsl_vector_get(survTime, i)) - gsl_vector_get(s, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s, j_old), gsl_vector_get(survTime, i)) - 0);
        }
        if(Del > 0)
        {
            logLH   += - Del * exp(gsl_vector_get(lambda, j_old))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
        }
    }
    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            jj = (int) gsl_vector_get(cluster, i) - 1;
            
            if(gsl_vector_get(survEvent, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s_new, 0))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                }
                if(j != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s_new, j-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s_new, j))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime, i)) - gsl_vector_get(s_new, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime, i)) - 0);
            }
            if(Del > 0)
            {
                logLH_prop   += - Del*exp(gsl_vector_get(lambda_new, j))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
            }
        }
    }
    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda, 0, *K+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam, 0, 0, *K+1, *K+1);
    
    if(*K+1 != 1)
    {
        c_dmvnormSH(&lambda_sub.vector, mu_lam, sqrt(sigSq_lam), &invS_sub.matrix, &logPrior);
        c_dmvnormSH(lambda_new, mu_lam, sqrt(sigSq_lam), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log( (2*(*K) + 3)*(2*(*K) + 2) * pow(gsl_vector_get(s, *K), -2) * (s_star - gsl_vector_get(s, j_old-1)) * (gsl_vector_get(s, j_old) - s_star)/(gsl_vector_get(s, j_old) - gsl_vector_get(s, j_old-1)) );
        }
        if(j_old == 0)
        {
            logPrior_prop += log( (2*(*K) + 3)*(2*(*K) + 2) * pow(gsl_vector_get(s, *K), -2) * (s_star - 0) * (gsl_vector_get(s, j_old) - s_star)/(gsl_vector_get(s, j_old) - 0) );
        }
    }
    
    if(*K+1 == 1)
    {
        logPrior = dnorm(gsl_vector_get(lambda, 0), mu_lam, sqrt(sigSq_lam*gsl_matrix_get(Sigma_lam, 0, 0)), 1);
        c_dmvnormSH(lambda_new, mu_lam, sqrt(sigSq_lam), invSigma_lam_new, &logPrior_prop);
        
        logPrior_prop += log( (2*(*K) + 3)*(2*(*K) + 2) * pow(gsl_vector_get(s, *K), -2) * (s_star - 0) * (gsl_vector_get(s, j_old) - s_star)/(gsl_vector_get(s, j_old) - 0) );
    }
    
    
    logPriorR = log((double) alpha/((*K) + 1)) + logPrior_prop - logPrior;
        
    logPropR = log(num_s_propBI_fin/alpha) - dunif(Upert, 0.5-delPert, 0.5+delPert, 1);
    
    logJacob = log(1/(1-Upert)/Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    if(u == 1)
    {
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q, 0, 0, K_new+1, K_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s, 0, K_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda, 0, K_new+1);
        
        *accept_BI += 1;
        *K = K_new;
        
        gsl_matrix_memcpy(&Sigma_lam_save.matrix, Sigma_lam_new);
        gsl_matrix_memcpy(&invSigma_lam_save.matrix, invSigma_lam_new);
        gsl_matrix_memcpy(&W_save.matrix, W_new);
        gsl_matrix_memcpy(&Q_save.matrix, Q_new);
        gsl_vector_memcpy(&s_save.vector, s_new);
        gsl_vector_memcpy(&lambda_save.vector, lambda_new);
    }
    
    gsl_vector_free(interInx);
    gsl_vector_free(s_propBI_fin);
    gsl_vector_free(s_new);
    gsl_vector_free(lambda_new);
    gsl_matrix_free(Sigma_lam_new);
    gsl_matrix_free(invSigma_lam_new);
    gsl_matrix_free(W_new);
    gsl_matrix_free(Q_new);
    
    return;
}








/* Updating the number of splits and their positions: K (K in the function) and s (Death move) */


void BpeDpCorSurv_updateDI(gsl_vector *s,
                            int *K,
                            int *accept_DI,
                            gsl_vector *survTime,
                            gsl_vector *survEvent,
                            gsl_vector *xbeta,
                            gsl_vector *V,
                            gsl_vector *cluster,
                            gsl_matrix *Sigma_lam,
                            gsl_matrix *invSigma_lam,
                            gsl_matrix *W,
                            gsl_matrix *Q,
                            gsl_vector *lambda,
                            gsl_vector *s_propBI,
                            int num_s_propBI,
                            double delPert,
                            int alpha,
                            double c_lam,
                            double mu_lam,
                            double sigSq_lam,
                            double s_max,
                            int K_max)
{
    
    
    int skip, i, j, jj;
    int j_old, K_new, u;
    double Upert, newLam;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta -> size;
    
    j_old = (int) runif(0, *K);
    
    gsl_vector *s_new = gsl_vector_calloc(*K);
    
    skip = 0;
    for(i = 0; i < *K+1; i++)
    {
        if(i == j_old) skip += 1;
        else gsl_vector_set(s_new, i-skip, gsl_vector_get(s, i));
    }
    
    K_new = *K-1;
    
    Upert = 1/(exp(gsl_vector_get(lambda, j_old+1) - gsl_vector_get(lambda, j_old)) + 1);
    
    if(j_old != 0)
    {
        newLam = ((gsl_vector_get(s, j_old) - gsl_vector_get(s, j_old-1)) * gsl_vector_get(lambda, j_old) + (gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old)) * gsl_vector_get(lambda, j_old+1)) / (gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old-1));
    }
    
    if(j_old == 0)
    {
        newLam = ((gsl_vector_get(s, j_old) - 0) * gsl_vector_get(lambda, j_old) + (gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old)) * gsl_vector_get(lambda, j_old+1)) / (gsl_vector_get(s, j_old+1) - 0);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(K_new+1);
    
    skip = 0;
    for(i = 0; i < K_new+1; i++)
    {
        if(i == j_old){
            gsl_vector_set(lambda_new, i, newLam);
            skip += 1;
        }
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda, i+skip));
    }
    
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(K_new+1, K_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(K_new+1, K_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam, K_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            jj = (int) gsl_vector_get(cluster, i) - 1;
            
            if(gsl_vector_get(survEvent, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s, 0))
                {
                    logLH += gsl_vector_get(lambda, j);
                }
                if(j != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s, j-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s, j))
                {
                    logLH += gsl_vector_get(lambda, j);
                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s, j), gsl_vector_get(survTime, i)) - gsl_vector_get(s, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s, j), gsl_vector_get(survTime, i)) - 0);
            }
            if(Del > 0)
            {
                logLH   += - Del*exp(gsl_vector_get(lambda, j))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
            }
        }
    }
    
    
    
    for(i = 0; i < n; i++)
    {
        jj = (int) gsl_vector_get(cluster, i) - 1;
        
        if(gsl_vector_get(survEvent, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime, i) <= gsl_vector_get(s_new, 0))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime, i) > gsl_vector_get(s_new, j_old-1) && gsl_vector_get(survTime, i) <= gsl_vector_get(s_new, j_old))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
        }
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime, i)) - gsl_vector_get(s_new, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime, i)) - 0);
        }
        if(Del > 0)
        {
            logLH_prop   += - Del*exp(gsl_vector_get(lambda_new, j_old))*exp(gsl_vector_get(xbeta, i)+gsl_vector_get(V, jj));
        }
    }
    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda, 0, *K+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam, 0, 0, *K+1, *K+1);
    
    
    if(*K+1 != 2)
    {
        c_dmvnormSH(&lambda_sub.vector, mu_lam, sqrt(sigSq_lam), &invS_sub.matrix, &logPrior);
        c_dmvnormSH(lambda_new, mu_lam, sqrt(sigSq_lam), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*K) + 1)/(2*(*K)))*pow(gsl_vector_get(s, *K), 2)*(gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old-1))/(gsl_vector_get(s, j_old) - gsl_vector_get(s, j_old-1))/(gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old)));
        }
        if(j_old == 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*K)+1)/(2*(*K)))*pow(gsl_vector_get(s, *K), 2)*(gsl_vector_get(s, j_old+1) - 0)/(gsl_vector_get(s, j_old) - 0)/(gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old)));
        }
    }
    
    
    if(*K+1 == 2)
    {
        logPrior_prop = dnorm(gsl_vector_get(lambda_new, 0), mu_lam, sqrt(sigSq_lam*gsl_matrix_get(Sigma_lam_new, 0, 0)), 1);
        c_dmvnormSH(lambda, mu_lam, sqrt(sigSq_lam), invSigma_lam, &logPrior);
        
        logPrior_prop += log(( (double) 1/(2*(*K)+1)/(2*(*K)))*pow(gsl_vector_get(s, *K), 2)*(gsl_vector_get(s, j_old+1) - 0)/(gsl_vector_get(s, j_old) - 0)/(gsl_vector_get(s, j_old+1) - gsl_vector_get(s, j_old)));
    }
    
    
    logPriorR = log((double) *K/alpha) + logPrior_prop - logPrior;
    
    logPropR = log((double) alpha/num_s_propBI) - dunif(Upert, 0.5-delPert, 0.5+delPert, 1);
    

    logJacob = log((1-Upert)*Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    
    if(u == 1)
    {
        
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W, 0, 0, K_new+1, K_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q, 0, 0, K_new+1, K_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s, 0, K_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda, 0, K_new+1);
        
        gsl_matrix_memcpy(&Sigma_lam_save.matrix, Sigma_lam_new);
        gsl_matrix_memcpy(&invSigma_lam_save.matrix, invSigma_lam_new);
        gsl_matrix_memcpy(&W_save.matrix, W_new);
        gsl_matrix_memcpy(&Q_save.matrix, Q_new);
        gsl_vector_memcpy(&s_save.vector, s_new);
        gsl_vector_memcpy(&lambda_save.vector, lambda_new);
        
        
        gsl_vector *zeroVec_J = gsl_vector_calloc(K_max+1);
        
        
        gsl_matrix_set_col(Sigma_lam, *K, zeroVec_J);
        gsl_matrix_set_row(Sigma_lam, *K, zeroVec_J);
        gsl_matrix_set_col(invSigma_lam, *K, zeroVec_J);
        gsl_matrix_set_row(invSigma_lam, *K, zeroVec_J);
        gsl_matrix_set_col(W, *K, zeroVec_J);
        gsl_matrix_set_row(W, *K, zeroVec_J);
        gsl_matrix_set_col(Q, *K, zeroVec_J);
        gsl_matrix_set_row(Q, *K, zeroVec_J);
        gsl_vector_set(s, *K, 0);
        gsl_vector_set(lambda, *K, 0);
        
        *accept_DI += 1;
        *K = K_new;
        
        gsl_vector_free(zeroVec_J);
        
    }
    
    gsl_vector_free(s_new);
    gsl_vector_free(lambda_new);
    gsl_matrix_free(Sigma_lam_new);
    gsl_matrix_free(invSigma_lam_new);
    gsl_matrix_free(W_new);
    gsl_matrix_free(Q_new);
    
    
    return;
}






















/* updating cluster-specific random effects */

void BpeDpCorSurv_updateCP(gsl_vector *beta,
                          gsl_vector *lambda,
                          gsl_vector *s,
                          int K,
                          gsl_vector *V,
                          gsl_vector *survTime,
                          gsl_vector *survEvent,
                          gsl_vector *cluster,
                          gsl_matrix *survCov,
                          gsl_vector *n_j,
                          gsl_vector *mu_all,
                          gsl_vector *zeta_all,
                          gsl_vector *c,
                          gsl_vector *accept_V,
                          double mu0,
                          double zeta0,
                          double a0,
                          double b0,
                          double tau,
                          int *nClass_DP,
                          gsl_rng *rr)
{
    int i, j, jj, k, u, n_jc, c_ind;
    double prob2, b_mc, sum_prob, val, mu, zeta;
    
    int n = survTime -> size;
    int J = V -> size;
    
    
    gsl_vector *cUniq = gsl_vector_calloc(J);
    gsl_vector *cUniq_count = gsl_vector_calloc(J);
    gsl_vector *cTemp = gsl_vector_calloc(J);
    
    gsl_vector *prob1 = gsl_vector_calloc(J+1);
    
    double Vbar, Vsum, muA, zetaA, aA, bA, tempSum;

    
    gsl_matrix *Delta = gsl_matrix_calloc(n, K+1);
    
    gsl_vector_set_zero(mu_all);
    gsl_vector_set_zero(zeta_all);
    
    
    
    /*************************************************************/
    
    /* Step 1: updating latent classes (c) */
    
    /*************************************************************/
    
    
    for(jj = 0; jj < J; jj++)
    {
        /* identfy "u" unique values */
        
        gsl_vector_memcpy(cTemp, c);
        
        u = 1;
        
        for(i = 0; i < J; i++)
        {
            if(i == 0)
            {
                gsl_vector_set(cUniq, u-1, gsl_vector_get(cTemp, i));
                
                for(j = i; j < J; j++)
                {
                    if(gsl_vector_get(cTemp, j) == gsl_vector_get(cUniq, u-1))
                    {
                        gsl_vector_set(cUniq_count, u-1, gsl_vector_get(cUniq_count, u-1)+1);
                        gsl_vector_set(cTemp, j, 0);
                    }
                }
            }
            
            if(i != 0 && gsl_vector_get(cTemp, i) != 0)
            {
                u += 1;
                gsl_vector_set(cUniq, u-1, gsl_vector_get(cTemp, i));
                
                for(j = i; j < J; j++)
                {
                    if(gsl_vector_get(cTemp, j) == gsl_vector_get(cUniq, u-1))
                    {
                        gsl_vector_set(cUniq_count, u-1, gsl_vector_get(cUniq_count, u-1)+1);
                        gsl_vector_set(cTemp, j, 0);
                    }
                }
                
            }
        }
        

        /* calculating probabilities for each of "u" latent class */
        

        
        gsl_vector_set_zero(prob1);
        
        for(j = 0; j < u; j++)
        {
            
            n_jc = gsl_vector_get(cUniq_count, j);
            
            if(gsl_vector_get(c, jj) == gsl_vector_get(cUniq, j)) n_jc -= 1;
            
            zetaA = zeta0 + gsl_vector_get(cUniq_count, j);
            
            aA = a0 + gsl_vector_get(cUniq_count, j)/2;
            
            Vsum = 0;
            
            for(k = 0; k < J; k++)
            {
                if(gsl_vector_get(c, k) == gsl_vector_get(cUniq, j) && k != jj)
                {
                    Vsum += gsl_vector_get(V, k);
                }
            }
            Vbar = Vsum;
            
            if(n_jc != 0)
            {
                Vbar = Vsum/n_jc;
            }
                        
            muA = (mu0*zeta0 + Vsum) / (zeta0 + gsl_vector_get(cUniq_count, j));
            
            bA = b0;
            bA += (zeta0*gsl_vector_get(cUniq_count, j)*pow(Vbar - mu0, 2))/ (2*(zeta0 + gsl_vector_get(cUniq_count, j)));
            
            for(k = 0; k < J; k++)
            {
                if(gsl_vector_get(c, k) == gsl_vector_get(cUniq, j))
                {
                    tempSum = gsl_vector_get(V, k) - Vbar;
                    bA += pow(tempSum, 2)/2;
                }
            }

            
            val = (double) n_jc / (double)(J-1+tau) * Qfunc_univ(gsl_vector_get(V, jj), muA, zetaA, aA, bA);
            
            gsl_vector_set(prob1, j, val);
            
        }
        
        
        prob2 = tau/(double)(J-1+tau) * Qfunc_univ(gsl_vector_get(V, jj), mu0, zeta0, a0, b0);
        
        sum_prob = 0;
        for(j = 0; j < u; j++) sum_prob += gsl_vector_get(prob1, j);
        sum_prob += prob2;
        b_mc = 1/sum_prob;
        
        gsl_vector_scale(prob1, b_mc);
        prob2 *= b_mc;
        
        gsl_vector_set(prob1, u, prob2);
        
        
        /* sample c based on the probabilities */
        
        c_ind = c_multinom_sample(rr, prob1, u+1);
        
        
        if(c_ind <= u)
        {
            gsl_vector_set(c, jj, gsl_vector_get(cUniq, c_ind-1));
        }
        if(c_ind > u)
        {
            gsl_vector_set(c, jj, gsl_vector_max(cUniq)+1);
        }
        
        /* initializing vectors and matrices */
        
        gsl_vector_set_zero(cUniq);
        gsl_vector_set_zero(cUniq_count);
        
    }
    
    
    
    
    
    
    /*************************************************************/
    
    /* Step 2: update (mu, zeta) using the posterior distribution that is based on {Vj :j∈{k:ck =c}}. */
    
    /*************************************************************/
    
    
    
    /* identfy "u" unique values */
    
    gsl_vector_memcpy(cTemp, c);
    
    u = 1;
    
    for(i = 0; i < J; i++)
    {
        if(i == 0)
        {
            gsl_vector_set(cUniq, u-1, gsl_vector_get(cTemp, i));
            
            for(j = i; j < J; j++)
            {
                if(gsl_vector_get(cTemp, j) == gsl_vector_get(cUniq, u-1))
                {
                    gsl_vector_set(cUniq_count, u-1, gsl_vector_get(cUniq_count, u-1)+1);
                    gsl_vector_set(cTemp, j, 0);
                }
            }
        }
        
        if(i != 0 && gsl_vector_get(cTemp, i) != 0)
        {
            u += 1;
            gsl_vector_set(cUniq, u-1, gsl_vector_get(cTemp, i));
            
            for(j = i; j < J; j++)
            {
                if(gsl_vector_get(cTemp, j) == gsl_vector_get(cUniq, u-1))
                {
                    gsl_vector_set(cUniq_count, u-1, gsl_vector_get(cUniq_count, u-1)+1);
                    gsl_vector_set(cTemp, j, 0);
                }
            }
            
        }
    }
    
    *nClass_DP = u;
    
    
    
    for(j = 0; j < u; j++)
    {
        
        n_jc = gsl_vector_get(cUniq_count, j);        
        
        zetaA = zeta0 + gsl_vector_get(cUniq_count, j);
        
        aA = a0 + gsl_vector_get(cUniq_count, j)/2;
        
        Vsum = 0;
        
        for(k = 0; k < J; k++)
        {
            if(gsl_vector_get(c, k) == gsl_vector_get(cUniq, j))
            {
                Vsum += gsl_vector_get(V, k);
            }
        }
        Vbar = Vsum;
        
        if(n_jc != 0)
        {
            Vbar = Vsum/n_jc;
        }
        
        muA = (mu0*zeta0 + Vsum) / zetaA;

        bA = b0;
        bA += (zeta0*gsl_vector_get(cUniq_count, j)*pow(Vbar - mu0, 2))/ (2*(zeta0 + gsl_vector_get(cUniq_count, j)));
        
        for(k = 0; k < J; k++)
        {
            if(gsl_vector_get(c, k) == gsl_vector_get(cUniq, j))
            {
                tempSum = gsl_vector_get(V, k) - Vbar;
                bA += pow(tempSum, 2)/2;
            }
        }
        
        zeta = rgamma(aA, 1/bA);
        mu = rnorm(muA, sqrt(1/(zetaA*zeta)));
        
        gsl_vector_set(mu_all, j, mu);
        gsl_vector_set(zeta_all, j, zeta);
        
    }
  
    
    
    /*************************************************************/
    
    /* Step 3: updating the Vj using MH algorithm */
    
    /*************************************************************/
    
    
    double LP, logLH, logLH_prop, Del, term;
    double D1, D2, D1_prop, D2_prop;    
    double logPrior, logPrior_prop, temp_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR, mu_temp, zeta_temp;
    double v_prop_me, v_prop_var, v_prop_me_prop, v_prop_var_prop;
    int uu;
    
    int startInx = 0;
    int endInx = 0;
    
    for(j = 0; j < J; j++)
    {
        for(i = 0; i < u; i++)
        {
            if(gsl_vector_get(c, j) == gsl_vector_get(cUniq, i)) jj = i;
        }
        
        mu_temp = gsl_vector_get(mu_all, jj);
        zeta_temp = gsl_vector_get(zeta_all, jj);
        
        logLH = 0; logLH_prop = 0;
        D1 = 0; D2 = 0;
        D1_prop = 0; D2_prop = 0;
        
        endInx += (int) gsl_vector_get(n_j, j);
        
        for(i = startInx; i < endInx; i++)
        {
            gsl_vector_view Xi = gsl_matrix_row(survCov, i);
            gsl_blas_ddot(&Xi.vector, beta, &LP);
            
            if(gsl_vector_get(survEvent, i) == 1)
            {
                logLH += gsl_vector_get(V, j);
                D1 += 1;                
            }

            
            for(k = 0; k < K+1; k++)
            {
                if(k > 0)
                {
                    Del = c_max(0, (c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - gsl_vector_get(s, k-1)));
                }
                if(k == 0)
                {
                    Del = c_max(0, c_min(gsl_vector_get(s, k), gsl_vector_get(survTime, i)) - 0);
                }
                
                gsl_matrix_set(Delta, i, k, Del);
                
                
                if(Del > 0)
                {
                    term = - Del*exp(gsl_vector_get(lambda, k))*exp(LP+gsl_vector_get(V, j));
                    logLH +=  term;
                    D1 += term;
                    D2 += term;
                }
            }
            
            
            
            
        } /* the end of the loop with i */
        
        /*         */
         D1 += -zeta_temp*gsl_vector_get(V, j);
         D2 += -zeta_temp;

        
        v_prop_me    = gsl_vector_get(V, j) - D1/D2;
        v_prop_var   = - pow(2.4, 2)/D2;
        
        temp_prop = rnorm(v_prop_me, sqrt(v_prop_var));
        
        for(i = startInx; i < endInx; i++)
        {
            gsl_vector_view Xi = gsl_matrix_row(survCov, i);
            gsl_blas_ddot(&Xi.vector, beta, &LP);
            
            if(gsl_vector_get(survEvent, i) == 1)
            {
                logLH_prop += temp_prop;
                D1_prop += 1;                
            }
            
            for(k = 0; k < K+1; k++)
            {
                Del = gsl_matrix_get(Delta, i, k);
                
                if(Del > 0)
                {
                    term = - Del*exp(gsl_vector_get(lambda, k))*exp(LP+temp_prop);
                    logLH_prop +=  term;
                    D1_prop += term;
                    D2_prop += term;
                }
            }
            
        } /* the end of the loop with i */
        
        D1_prop += -zeta_temp*temp_prop;
        D2_prop += -zeta_temp;
        
        v_prop_me_prop   = temp_prop - D1_prop/D2_prop;
        v_prop_var_prop  = - pow(2.4, 2)/D2_prop;
        
        startInx = endInx;
        
        logPrior = -(double) zeta_temp/2*pow(gsl_vector_get(V, j), 2);
        logPrior_prop = -(double) zeta_temp/2*pow(temp_prop, 2);
        
        logProp_PropToIni = dnorm(gsl_vector_get(V, j), v_prop_me_prop, sqrt(v_prop_var_prop), 1);
        logProp_IniToProp = dnorm(temp_prop, v_prop_me, sqrt(v_prop_var), 1);
        
        logR = logLH_prop - logLH + logPrior_prop - logPrior + logProp_PropToIni - logProp_IniToProp;
        
        uu = log(runif(0, 1)) < logR;
        
        if(uu == 1)
        {
            gsl_vector_set(V, j, temp_prop);
            gsl_vector_set(accept_V, j, (gsl_vector_get(accept_V, j) + uu));
        }
        
    }/* the end of the loop with j */
    
    
    
    gsl_vector_free(cUniq);
    gsl_vector_free(cUniq_count);
    gsl_vector_free(cTemp);
    gsl_vector_free(prob1);
    gsl_matrix_free(Delta);
    
    
    return;
    
    
    
    
}








/* updating precision parameter of DP prior: tau */

void BpeDpCorSurv_updatePP(int *n,
                          double *tau,
                          double aTau,
                          double bTau,
                          int *nClass_DP)
{
    double eta, pEta, tau_shape, tau_rate, tau_scale, dist1Ind;
    
    eta = rbeta(*tau+1, *n);
    
    pEta = (aTau + (double) *nClass_DP - 1)/(*n * bTau - *n * log(eta) + aTau + (double) *nClass_DP - 1);
    
    dist1Ind = rbinom(1, pEta);
    
    tau_shape = aTau + (double) *nClass_DP - 1 + dist1Ind;
    tau_rate = bTau - log(eta);
    tau_scale = 1/tau_rate;
    
    *tau = rgamma(tau_shape, tau_scale);
    
    return;
}

