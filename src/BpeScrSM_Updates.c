#include <stdio.h>
#include <math.h>

#include "gsl/gsl_matrix.h"
#include "gsl/gsl_linalg.h"
#include "gsl/gsl_blas.h"
#include "gsl/gsl_sort_vector.h"
#include "gsl/gsl_heapsort.h"

#include "R.h"
#include "Rmath.h"

#include "BpeScrSM.h"






/* updating regression parameter: beta1 */

/**/

void BscrSM_updateRP1(gsl_vector *beta1,
               gsl_vector *xbeta1,
               gsl_vector *accept_beta1,
               gsl_vector *gamma,
               gsl_vector *lambda1,
               gsl_vector *s1,
               gsl_vector *survTime1,
               gsl_vector *survEvent1,
               gsl_matrix *survCov1,
               int J1)
{
    double D1, D2, logLH;
    double D1_prop, D2_prop, logLH_prop;
    double beta_prop_me, beta_prop_var, temp_prop;
    double beta_prop_me_prop, beta_prop_var_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR, Del, gam;
    int u, m, i, j;
    
    int p = beta1 -> size;
    int n = survTime1 -> size;
    
    gsl_vector *beta_prop = gsl_vector_calloc(p);
    
    m = (int) runif(0, p);
    
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    gsl_matrix *Delta = gsl_matrix_calloc(n, J1+1);
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            logLH += gsl_vector_get(xbeta1, i);
            D1 += gsl_matrix_get(survCov1, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);

        for(j = 0; j < J1+1; j++)
        {
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s1, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - 0);
            }
            
            gsl_matrix_set(Delta, i, j, Del);
            
            
            if(Del > 0)
            {
                logLH   += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta1, i));
                D1      += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta1, i))*gsl_matrix_get(survCov1, i, m);
                D2      += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta1, i))*pow(gsl_matrix_get(survCov1, i, m), 2);
            }
        }
    }
            
    
    beta_prop_me    = gsl_vector_get(beta1, m) - D1/D2;
    beta_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(beta_prop_me, sqrt(beta_prop_var));
    
    
    gsl_vector_memcpy(beta_prop, beta1);
    gsl_vector_set(beta_prop, m, temp_prop);
    
    gsl_vector *xbeta_prop = gsl_vector_calloc(n);
    
    gsl_blas_dgemv(CblasNoTrans, 1, survCov1, beta_prop, 0, xbeta_prop);
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            logLH_prop += gsl_vector_get(xbeta_prop, i);
            D1_prop += gsl_matrix_get(survCov1, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);        
        
        for(j = 0; j < J1+1; j++)
        {
                        
            Del = gsl_matrix_get(Delta, i, j);
            
            if(Del > 0)
            {
                logLH_prop   += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta_prop, i));
                D1_prop      += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta_prop, i))*gsl_matrix_get(survCov1, i, m);
                D2_prop      += - gam * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta_prop, i))*pow(gsl_matrix_get(survCov1, i, m), 2);
            }
        }
    }
    
    beta_prop_me_prop   = temp_prop - D1_prop/D2_prop;
    beta_prop_var_prop  = - pow(2.4, 2)/D2_prop;
    
    logProp_IniToProp = dnorm(temp_prop, beta_prop_me, sqrt(beta_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(beta1, m), beta_prop_me_prop, sqrt(beta_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logProp_PropToIni - logProp_IniToProp;
    
    u = log(runif(0, 1)) < logR;
    
    
    if(u == 1)
    {
        gsl_vector_set(beta1, m, temp_prop);
        gsl_vector_swap(xbeta1, xbeta_prop);
        gsl_vector_set(accept_beta1, m, (gsl_vector_get(accept_beta1, m) + u));
    }
    
    gsl_vector_free(beta_prop);
    gsl_vector_free(xbeta_prop);
    gsl_matrix_free(Delta);
    
        
    return;
}
























/* updating regression parameter: beta2 */

/**/

void BscrSM_updateRP2(gsl_vector *beta2,
               gsl_vector *xbeta2,
               gsl_vector *accept_beta2,
               gsl_vector *gamma,
               gsl_vector *lambda2,
               gsl_vector *s2,
               gsl_vector *survTime1,
               gsl_vector *case01,
               gsl_matrix *survCov2,
               int J2)
{
    double D1, D2, logLH;
    double D1_prop, D2_prop, logLH_prop;
    double beta_prop_me, beta_prop_var, temp_prop;
    double beta_prop_me_prop, beta_prop_var_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR, Del, gam;
    int u, m, i, j;
    
    int p = beta2 -> size;
    int n = survTime1 -> size;
    
    gsl_vector *beta_prop = gsl_vector_calloc(p);
    
    m = (int) runif(0, p);
    

    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    
    gsl_matrix *Delta = gsl_matrix_calloc(n, J2+1);
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case01, i) ==  1)
        {
            logLH += gsl_vector_get(xbeta2, i);
            D1 += gsl_matrix_get(survCov2, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);
        
        for(j = 0; j < J2+1; j++)
        {
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s2, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - 0);
            }
            
            gsl_matrix_set(Delta, i, j, Del);
            
            if(Del > 0)
            {
                logLH   += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta2, i));
                D1      += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta2, i))*gsl_matrix_get(survCov2, i, m);
                D2      += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta2, i))*pow(gsl_matrix_get(survCov2, i, m), 2);
            }
        }
    }
    
    
    beta_prop_me    = gsl_vector_get(beta2, m) - D1/D2;
    beta_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(beta_prop_me, sqrt(beta_prop_var));    
    
    gsl_vector_memcpy(beta_prop, beta2);
    gsl_vector_set(beta_prop, m, temp_prop);
    
    gsl_vector *xbeta_prop = gsl_vector_calloc(n);
    
    gsl_blas_dgemv(CblasNoTrans, 1, survCov2, beta_prop, 0, xbeta_prop);
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case01, i) ==  1)
        {
            logLH_prop += gsl_vector_get(xbeta_prop, i);
            D1_prop += gsl_matrix_get(survCov2, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);
        
        for(j = 0; j < J2+1; j++)
        {
            
            Del = gsl_matrix_get(Delta, i, j);
            
            if(Del > 0)
            {
                logLH_prop   += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta_prop, i));
                D1_prop      += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta_prop, i))*gsl_matrix_get(survCov2, i, m);
                D2_prop      += - gam * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta_prop, i))*pow(gsl_matrix_get(survCov2, i, m), 2);
            }
        }
    }
    
    beta_prop_me_prop   = temp_prop - D1_prop/D2_prop;
    beta_prop_var_prop  = - pow(2.4, 2)/D2_prop;
    
    logProp_IniToProp = dnorm(temp_prop, beta_prop_me, sqrt(beta_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(beta2, m), beta_prop_me_prop, sqrt(beta_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logProp_PropToIni - logProp_IniToProp;
    
    u = log(runif(0, 1)) < logR;
    
    if(u == 1)
    {
        gsl_vector_set(beta2, m, temp_prop);
        gsl_vector_swap(xbeta2, xbeta_prop);
        gsl_vector_set(accept_beta2, m, (gsl_vector_get(accept_beta2, m) + u));
    }
    
    gsl_vector_free(beta_prop);
    gsl_vector_free(xbeta_prop);
    gsl_matrix_free(Delta);
    
    return;
}












/* updating regression parameter: beta3 */

/**/

void BscrSM_updateRP3(gsl_vector *beta3,
               gsl_vector *xbeta3,
               gsl_vector *accept_beta3,
               gsl_vector *gamma,
               gsl_vector *lambda3,
               gsl_vector *s3,
               gsl_vector *yStar,
               gsl_vector *case11,
               gsl_matrix *survCov3,
               int J3)
{
    double D1, D2, logLH;
    double D1_prop, D2_prop, logLH_prop;
    double beta_prop_me, beta_prop_var, temp_prop;
    double beta_prop_me_prop, beta_prop_var_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR, Del, gam;
    int u, m, i, j;
    
    int p = beta3 -> size;
    int n = yStar -> size;
    
    gsl_vector *beta_prop = gsl_vector_calloc(p);
    
    m = (int) runif(0, p);
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    
    gsl_matrix *Delta = gsl_matrix_calloc(n, J3+1);
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case11, i) == 1)
        {
            logLH += gsl_vector_get(xbeta3, i);
            D1 += gsl_matrix_get(survCov3, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);
        
        for(j = 0; j < J3+1; j++)
        {
            
            
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - gsl_vector_get(s3, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - 0);
            }
            
            gsl_matrix_set(Delta, i, j, Del);
            
            if(Del > 0)
            {
                logLH   += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta3, i));
                D1      += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta3, i))*gsl_matrix_get(survCov3, i, m);
                D2      += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta3, i))*pow(gsl_matrix_get(survCov3, i, m), 2);
            }
        }
    }
    
    
    beta_prop_me    = gsl_vector_get(beta3, m) - D1/D2;
    beta_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(beta_prop_me, sqrt(beta_prop_var));
    
    
    
    gsl_vector_memcpy(beta_prop, beta3);
    gsl_vector_set(beta_prop, m, temp_prop);
    
    gsl_vector *xbeta_prop = gsl_vector_calloc(n);
    
    gsl_blas_dgemv(CblasNoTrans, 1, survCov3, beta_prop, 0, xbeta_prop);
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case11, i) == 1)
        {
            logLH_prop += gsl_vector_get(xbeta_prop, i);
            D1_prop += gsl_matrix_get(survCov3, i, m);
        }
        
        gam = gsl_vector_get(gamma, i);
        
        for(j = 0; j < J3+1; j++)
        {

            
            Del = gsl_matrix_get(Delta, i, j);
            
            if(Del > 0)
            {
                logLH_prop   += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta_prop, i));
                D1_prop      += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta_prop, i))*gsl_matrix_get(survCov3, i, m);
                D2_prop      += - gam * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta_prop, i))*pow(gsl_matrix_get(survCov3, i, m), 2);
            }
        }
    }
    
    beta_prop_me_prop   = temp_prop - D1_prop/D2_prop;
    beta_prop_var_prop  = - pow(2.4, 2)/D2_prop;
    
    logProp_IniToProp = dnorm(temp_prop, beta_prop_me, sqrt(beta_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(beta3, m), beta_prop_me_prop, sqrt(beta_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logProp_PropToIni - logProp_IniToProp;
    
    u = log(runif(0, 1)) < logR;
    

    
    if(u == 1)
    {
        gsl_vector_set(beta3, m, temp_prop);
        gsl_vector_swap(xbeta3, xbeta_prop);
        gsl_vector_set(accept_beta3, m, (gsl_vector_get(accept_beta3, m) + u));
    }
    
    gsl_vector_free(beta_prop);
    gsl_vector_free(xbeta_prop);
    gsl_matrix_free(Delta);
      
    return;
}




































/* updating log-baseline hazard function parameter: lambda1 */



void BscrSM_updateBH1(gsl_vector *lambda1,
               gsl_vector *s1,
               gsl_vector *xbeta1,
               gsl_vector *gamma,               
               gsl_vector *survTime1,
               gsl_vector *survEvent1,
               gsl_matrix *Sigma_lam1,
               gsl_matrix *invSigma_lam1,
               gsl_matrix *W1,
               gsl_matrix *Q1,
               double mu_lam1,
               double sigSq_lam1,
               int J1)
{
    double D1, D2, logLH, Del, inc, gam;
    double D1_prop, D2_prop, logLH_prop;
    double lambda_prop_me, lambda_prop_var, temp_prop;
    double lambda_prop_me_prop, lambda_prop_var_prop;
    double logPrior, logPrior_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int u, i, j;
    
    double nu_lam, nu_lam_prop;
    
    int n = xbeta1 -> size;
    
    j = (int) runif(0, J1+1);

    
    if(J1+1 > 1)
    {
        if(j == 0) nu_lam = mu_lam1 + gsl_matrix_get(W1, 0, 1) * (gsl_vector_get(lambda1, 1) - mu_lam1);
        if(j == J1) nu_lam = mu_lam1 + gsl_matrix_get(W1, J1, J1-1) * (gsl_vector_get(lambda1, J1-1) - mu_lam1);
        if(j != 0 && j !=J1) nu_lam = mu_lam1 + gsl_matrix_get(W1, j, j-1) * (gsl_vector_get(lambda1, j-1) - mu_lam1) + gsl_matrix_get(W1, j, j+1) * (gsl_vector_get(lambda1, j+1) - mu_lam1);
    }
    
    if(J1+1 == 1)
    {
        nu_lam = mu_lam1;
    }
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    
    gsl_vector *Delta = gsl_vector_calloc(n);
  
    
    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            if(j == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, 0))
            {
                logLH += gsl_vector_get(lambda1, j);
                D1 += 1;
            }
            if(j != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s1, j-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, j))
            {
                logLH += gsl_vector_get(lambda1, j);
                D1 += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);        

        if(j > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s1, j-1)));
        }
        if(j == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - 0);
        }
        
        gsl_vector_set(Delta, i, Del);
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta1, i));
            logLH   += inc;
            D1      += inc;
            D2      += inc;
        }
    }
    
    
    

    D1      += -1/(sigSq_lam1 * gsl_matrix_get(Q1, j, j))*(gsl_vector_get(lambda1, j)-nu_lam);
    D2      += -1/(sigSq_lam1 * gsl_matrix_get(Q1, j, j));
    
    
    lambda_prop_me    = gsl_vector_get(lambda1, j) - D1/D2;
    lambda_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(lambda_prop_me, sqrt(lambda_prop_var));
    
    gsl_vector *lambda_prop = gsl_vector_calloc(J1+1);
    
    gsl_vector_view lambda_sub = gsl_vector_subvector(lambda1, 0, J1+1);
    
    gsl_vector_memcpy(lambda_prop, &lambda_sub.vector);
    gsl_vector_set(lambda_prop, j, temp_prop);
    
    if(J1+1 > 1)
    {
        if(j == 0) nu_lam_prop = mu_lam1 + gsl_matrix_get(W1, 0, 1) * (gsl_vector_get(lambda_prop, 1) - mu_lam1);
        if(j == J1) nu_lam_prop = mu_lam1 + gsl_matrix_get(W1, J1, J1-1) * (gsl_vector_get(lambda_prop, J1-1) - mu_lam1);
        if(j != 0 && j != J1) nu_lam_prop = mu_lam1 + gsl_matrix_get(W1, j, j-1) * (gsl_vector_get(lambda_prop, j-1) - mu_lam1) + gsl_matrix_get(W1, j, j+1) * (gsl_vector_get(lambda_prop, j+1) - mu_lam1);
    }
    
    if(J1+1 == 1)
    {
        nu_lam_prop = mu_lam1;
    }
    


    
    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            if(j == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, 0))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            if(j != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s1, j-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, j))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);
                
        Del = gsl_vector_get(Delta, i);
        
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda_prop, j))*exp(gsl_vector_get(xbeta1, i));
            logLH_prop   += inc;
            D1_prop      += inc;
            D2_prop      += inc;
        }
    }
    
    
    D1_prop += -1/(sigSq_lam1 * gsl_matrix_get(Q1, j, j))*(gsl_vector_get(lambda_prop, j)-nu_lam);
    D2_prop += -1/(sigSq_lam1 * gsl_matrix_get(Q1, j, j));
  

    
    
    lambda_prop_me_prop    = gsl_vector_get(lambda_prop, j) - D1_prop/D2_prop;
    lambda_prop_var_prop   = - pow(2.4, 2)/D2_prop;
    
    gsl_matrix_view invS_sub = gsl_matrix_submatrix(invSigma_lam1, 0, 0, J1+1, J1+1);
    
    if(J1+1 > 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam1, sqrt(sigSq_lam1), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_prop, mu_lam1, sqrt(sigSq_lam1), &invS_sub.matrix, &logPrior_prop);
    }
    if(J1+1 == 1)
    {
        logPrior        = dnorm(gsl_vector_get(lambda1, j), mu_lam1, sqrt(sigSq_lam1*gsl_matrix_get(Sigma_lam1, 0, 0)), 1);
        logPrior_prop   = dnorm(temp_prop, mu_lam1, sqrt(sigSq_lam1*gsl_matrix_get(Sigma_lam1, 0, 0)), 1);
    }
    
    logProp_IniToProp = dnorm(temp_prop, lambda_prop_me, sqrt(lambda_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(lambda1, j), lambda_prop_me_prop, sqrt(lambda_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logPrior_prop - logPrior +  logProp_PropToIni - logProp_IniToProp;
    
    
    u = log(runif(0, 1)) < logR;
  
    if(u == 1) gsl_vector_set(lambda1, j, temp_prop);
    
    gsl_vector_free(lambda_prop);
    gsl_vector_free(Delta);
    
    
    return;
}










/* updating log-baseline hazard function parameter: lambda2 */



void BscrSM_updateBH2(gsl_vector *lambda2,
               gsl_vector *s2,
               gsl_vector *xbeta2,
               gsl_vector *gamma,
               gsl_vector *survTime1,
               gsl_vector *survTime2,
               gsl_vector *case01,
               gsl_matrix *Sigma_lam2,
               gsl_matrix *invSigma_lam2,
               gsl_matrix *W2,
               gsl_matrix *Q2,
               double mu_lam2,
               double sigSq_lam2,
               int J2)
{
    double D1, D2, logLH, Del, inc, gam;
    double D1_prop, D2_prop, logLH_prop;
    double lambda_prop_me, lambda_prop_var, temp_prop;
    double lambda_prop_me_prop, lambda_prop_var_prop;
    double logPrior, logPrior_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int u, i, j;
    
    double nu_lam, nu_lam_prop;
    
    int n = xbeta2 -> size;
    
    j = (int) runif(0, J2+1);
    
    
    if(J2+1 > 1)
    {
        if(j == 0) nu_lam = mu_lam2 + gsl_matrix_get(W2, 0, 1) * (gsl_vector_get(lambda2, 1) - mu_lam2);
        if(j == J2) nu_lam = mu_lam2 + gsl_matrix_get(W2, J2, J2-1) * (gsl_vector_get(lambda2, J2-1) - mu_lam2);
        if(j != 0 && j !=J2) nu_lam = mu_lam2 + gsl_matrix_get(W2, j, j-1) * (gsl_vector_get(lambda2, j-1) - mu_lam2) + gsl_matrix_get(W2, j, j+1) * (gsl_vector_get(lambda2, j+1) - mu_lam2);
    }
    
    if(J2+1 == 1)
    {
        nu_lam = mu_lam2;
    }
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    gsl_vector *Delta = gsl_vector_calloc(n);
    
    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(case01, i) ==  1)
        {
            if(j == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, 0))
            {
                logLH += gsl_vector_get(lambda2, j);
                D1 += 1;
            }
            if(j != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s2, j-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, j))
            {
                logLH += gsl_vector_get(lambda2, j);
                D1 += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);
        
        if(j > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s2, j-1)));
        }
        if(j == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - 0);
        }
        
        gsl_vector_set(Delta, i, Del);
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta2, i));
            logLH   += inc;
            D1      += inc;
            D2      += inc;
        }
    }
    
    
    
    
    D1      += -1/(sigSq_lam2 * gsl_matrix_get(Q2, j, j))*(gsl_vector_get(lambda2, j)-nu_lam);
    D2      += -1/(sigSq_lam2 * gsl_matrix_get(Q2, j, j));
    
    
    lambda_prop_me    = gsl_vector_get(lambda2, j) - D1/D2;
    lambda_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(lambda_prop_me, sqrt(lambda_prop_var));
    
    
    gsl_vector *lambda_prop = gsl_vector_calloc(J2+1);
    
    gsl_vector_view lambda_sub = gsl_vector_subvector(lambda2, 0, J2+1);
    
    gsl_vector_memcpy(lambda_prop, &lambda_sub.vector);
    gsl_vector_set(lambda_prop, j, temp_prop);
    
    if(J2+1 > 1)
    {
        if(j == 0) nu_lam_prop = mu_lam2 + gsl_matrix_get(W2, 0, 1) * (gsl_vector_get(lambda_prop, 1) - mu_lam2);
        if(j == J2) nu_lam_prop = mu_lam2 + gsl_matrix_get(W2, J2, J2-1) * (gsl_vector_get(lambda_prop, J2-1) - mu_lam2);
        if(j != 0 && j != J2) nu_lam_prop = mu_lam2 + gsl_matrix_get(W2, j, j-1) * (gsl_vector_get(lambda_prop, j-1) - mu_lam2) + gsl_matrix_get(W2, j, j+1) * (gsl_vector_get(lambda_prop, j+1) - mu_lam2);
    }
    
    if(J2+1 == 1)
    {
        nu_lam_prop = mu_lam2;
    }
    
    
    
    
    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(case01, i) ==  1)
        {
            if(j == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, 0))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            if(j != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s2, j-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, j))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);
        
        
        Del = gsl_vector_get(Delta, i);
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda_prop, j))*exp(gsl_vector_get(xbeta2, i));
            logLH_prop   += inc;
            D1_prop      += inc;
            D2_prop      += inc;
        }
    }
    
    
    D1_prop += -1/(sigSq_lam2 * gsl_matrix_get(Q2, j, j))*(gsl_vector_get(lambda_prop, j)-nu_lam);
    D2_prop += -1/(sigSq_lam2 * gsl_matrix_get(Q2, j, j));
    
    
    
    
    lambda_prop_me_prop    = gsl_vector_get(lambda_prop, j) - D1_prop/D2_prop;
    lambda_prop_var_prop   = - pow(2.4, 2)/D2_prop;
    
    gsl_matrix_view invS_sub = gsl_matrix_submatrix(invSigma_lam2, 0, 0, J2+1, J2+1);
    
    if(J2+1 > 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam2, sqrt(sigSq_lam2), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_prop, mu_lam2, sqrt(sigSq_lam2), &invS_sub.matrix, &logPrior_prop);
    }
    if(J2+1 == 1)
    {
        logPrior        = dnorm(gsl_vector_get(lambda2, j), mu_lam2, sqrt(sigSq_lam2*gsl_matrix_get(Sigma_lam2, 0, 0)), 1);
        logPrior_prop   = dnorm(temp_prop, mu_lam2, sqrt(sigSq_lam2*gsl_matrix_get(Sigma_lam2, 0, 0)), 1);
    }
    
    logProp_IniToProp = dnorm(temp_prop, lambda_prop_me, sqrt(lambda_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(lambda2, j), lambda_prop_me_prop, sqrt(lambda_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logPrior_prop - logPrior +  logProp_PropToIni - logProp_IniToProp;
    
    
    u = log(runif(0, 1)) < logR;
    
    /*u = 0;*/
    
    if(u == 1) gsl_vector_set(lambda2, j, temp_prop);
    
    gsl_vector_free(lambda_prop);
    gsl_vector_free(Delta);
    
    
    
    return;
}





















/* updating log-baseline hazard function parameter: lambda3 */



void BscrSM_updateBH3(gsl_vector *lambda3,
               gsl_vector *s3,
               gsl_vector *xbeta3,
               gsl_vector *gamma,
               gsl_vector *yStar,
               gsl_vector *case11,
               gsl_matrix *Sigma_lam3,
               gsl_matrix *invSigma_lam3,
               gsl_matrix *W3,
               gsl_matrix *Q3,
               double mu_lam3,
               double sigSq_lam3,
               int J3)
{
    double D1, D2, logLH, Del, inc, gam;
    double D1_prop, D2_prop, logLH_prop;
    double lambda_prop_me, lambda_prop_var, temp_prop;
    double lambda_prop_me_prop, lambda_prop_var_prop;
    double logPrior, logPrior_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int u, i, j;
    
    double nu_lam, nu_lam_prop;
    
    int n = xbeta3 -> size;
    
    j = (int) runif(0, J3+1);
    
    if(J3+1 > 1)
    {
        if(j == 0) nu_lam = mu_lam3 + gsl_matrix_get(W3, 0, 1) * (gsl_vector_get(lambda3, 1) - mu_lam3);
        if(j == J3) nu_lam = mu_lam3 + gsl_matrix_get(W3, J3, J3-1) * (gsl_vector_get(lambda3, J3-1) - mu_lam3);
        if(j != 0 && j !=J3) nu_lam = mu_lam3 + gsl_matrix_get(W3, j, j-1) * (gsl_vector_get(lambda3, j-1) - mu_lam3) + gsl_matrix_get(W3, j, j+1) * (gsl_vector_get(lambda3, j+1) - mu_lam3);
    }
    
    if(J3+1 == 1)
    {
        nu_lam = mu_lam3;
    }
    
    logLH = 0; D1 = 0; D2 = 0;
    logLH_prop = 0; D1_prop = 0; D2_prop = 0;
    
    gsl_vector *Delta = gsl_vector_calloc(n);

    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(case11, i) == 1)
        {
            if(j == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, 0))
            {
                logLH += gsl_vector_get(lambda3, j);
                D1 += 1;
            }
            if(j != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s3, j-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, j))
            {
                logLH += gsl_vector_get(lambda3, j);
                D1 += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);
        
 
        if(j > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - gsl_vector_get(s3, j-1)));
        }
        if(j == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - 0);
        }
        gsl_vector_set(Delta, i, Del);
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta3, i));
            logLH   += inc;
            D1      += inc;
            D2      += inc;
        }
    }
    
    
    
    
    D1      += -1/(sigSq_lam3 * gsl_matrix_get(Q3, j, j))*(gsl_vector_get(lambda3, j)-nu_lam);
    D2      += -1/(sigSq_lam3 * gsl_matrix_get(Q3, j, j));
    
    
    lambda_prop_me    = gsl_vector_get(lambda3, j) - D1/D2;
    lambda_prop_var   = - pow(2.4, 2)/D2;
    
    temp_prop = rnorm(lambda_prop_me, sqrt(lambda_prop_var));
    

    
    gsl_vector *lambda_prop = gsl_vector_calloc(J3+1);
    
    gsl_vector_view lambda_sub = gsl_vector_subvector(lambda3, 0, J3+1);
    
    gsl_vector_memcpy(lambda_prop, &lambda_sub.vector);
    gsl_vector_set(lambda_prop, j, temp_prop);
    
    if(J3+1 > 1)
    {
        if(j == 0) nu_lam_prop = mu_lam3 + gsl_matrix_get(W3, 0, 1) * (gsl_vector_get(lambda_prop, 1) - mu_lam3);
        if(j == J3) nu_lam_prop = mu_lam3 + gsl_matrix_get(W3, J3, J3-1) * (gsl_vector_get(lambda_prop, J3-1) - mu_lam3);
        if(j != 0 && j != J3) nu_lam_prop = mu_lam3 + gsl_matrix_get(W3, j, j-1) * (gsl_vector_get(lambda_prop, j-1) - mu_lam3) + gsl_matrix_get(W3, j, j+1) * (gsl_vector_get(lambda_prop, j+1) - mu_lam3);
    }
    
    if(J3+1 == 1)
    {
        nu_lam_prop = mu_lam3;
    }
    
    
    
    
    for(i = 0; i < n; i++)
    {
        
        if(gsl_vector_get(case11, i) == 1)
        {
            if(j == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, 0))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            if(j != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s3, j-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, j))
            {
                logLH_prop += gsl_vector_get(lambda_prop, j);
                D1_prop += 1;
            }
            
        }
        
        gam = gsl_vector_get(gamma, i);
        
        
        Del = gsl_vector_get(Delta, i);
        
        if(Del > 0)
        {
            inc = - gam * Del * exp(gsl_vector_get(lambda_prop, j))*exp(gsl_vector_get(xbeta3, i));
            logLH_prop   += inc;
            D1_prop      += inc;
            D2_prop      += inc;
        }
    }
    
    
    D1_prop += -1/(sigSq_lam3 * gsl_matrix_get(Q3, j, j))*(gsl_vector_get(lambda_prop, j)-nu_lam);
    D2_prop += -1/(sigSq_lam3 * gsl_matrix_get(Q3, j, j));
    
    
    
    
    lambda_prop_me_prop    = gsl_vector_get(lambda_prop, j) - D1_prop/D2_prop;
    lambda_prop_var_prop   = - pow(2.4, 2)/D2_prop;
    
    gsl_matrix_view invS_sub = gsl_matrix_submatrix(invSigma_lam3, 0, 0, J3+1, J3+1);
    
    if(J3+1 > 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam3, sqrt(sigSq_lam3), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_prop, mu_lam3, sqrt(sigSq_lam3), &invS_sub.matrix, &logPrior_prop);
    }
    if(J3+1 == 1)
    {
        logPrior        = dnorm(gsl_vector_get(lambda3, j), mu_lam3, sqrt(sigSq_lam3*gsl_matrix_get(Sigma_lam3, 0, 0)), 1);
        logPrior_prop   = dnorm(temp_prop, mu_lam3, sqrt(sigSq_lam3*gsl_matrix_get(Sigma_lam3, 0, 0)), 1);
    }
    
    logProp_IniToProp = dnorm(temp_prop, lambda_prop_me, sqrt(lambda_prop_var), 1);
    logProp_PropToIni = dnorm(gsl_vector_get(lambda3, j), lambda_prop_me_prop, sqrt(lambda_prop_var_prop), 1);
    
    logR = logLH_prop - logLH + logPrior_prop - logPrior +  logProp_PropToIni - logProp_IniToProp;
    
    
    u = log(runif(0, 1)) < logR;
    
    /* u = 0;*/
    
    if(u == 1) gsl_vector_set(lambda3, j, temp_prop);
    
    gsl_vector_free(lambda_prop);
    gsl_vector_free(Delta);
    
    
    
      
    
    return;
}






















/* Updating second stage survival components: mu_lam1 and sigSq_lam1 */

void BscrSM_updateSP1(double *mu_lam1,
               double *sigSq_lam1,
               gsl_vector *lambda1,
               gsl_matrix *Sigma_lam1,
               gsl_matrix *invSigma_lam1,
               double a1,
               double b1,
               int J1)
{
    double num, den, sigSH, sigRT, sigSC, tau, mu_lam_mean, mu_lam_var;
    
    gsl_vector *ones = gsl_vector_calloc(J1+1);
    gsl_vector_set_all(ones, 1);
    
    gsl_matrix_view invSlam_sub = gsl_matrix_submatrix(invSigma_lam1, 0, 0, J1+1, J1+1);
    gsl_vector_view lam_sub     = gsl_vector_subvector(lambda1, 0, J1+1);
    
    c_quadform_vMu(ones, &invSlam_sub.matrix, &lam_sub.vector, &num);
    c_quadform_vMv(ones, &invSlam_sub.matrix, &den);
    
    mu_lam_mean = num/den;
    mu_lam_var = *sigSq_lam1/den;
    
    *mu_lam1 = rnorm(mu_lam_mean, sqrt(mu_lam_var));
    
    gsl_vector *diff = gsl_vector_calloc(J1+1);
    gsl_vector_set_all(diff, *mu_lam1);
    gsl_vector_sub(diff, &lam_sub.vector);
    c_quadform_vMv(diff, &invSlam_sub.matrix, &sigRT);
    sigRT /= 2;
    sigRT += b1;
    sigSC = 1/sigRT;
    sigSH = a1 + (double) (J1+1)/2;
    tau = rgamma(sigSH, sigSC);
    *sigSq_lam1 = 1/tau;
    
    gsl_vector_free(ones);
    gsl_vector_free(diff);
    
    return;
}













/* Updating second stage survival components: mu_lam2 and sigSq_lam2 */

void BscrSM_updateSP2(double *mu_lam2,
               double *sigSq_lam2,
               gsl_vector *lambda2,
               gsl_matrix *Sigma_lam2,
               gsl_matrix *invSigma_lam2,
               double a2,
               double b2,
               int J2)
{
    double num, den, sigSH, sigRT, sigSC, tau, mu_lam_mean, mu_lam_var;
    
    gsl_vector *ones = gsl_vector_calloc(J2+1);
    gsl_vector_set_all(ones, 1);
    
    gsl_matrix_view invSlam_sub = gsl_matrix_submatrix(invSigma_lam2, 0, 0, J2+1, J2+1);
    gsl_vector_view lam_sub     = gsl_vector_subvector(lambda2, 0, J2+1);
    
    c_quadform_vMu(ones, &invSlam_sub.matrix, &lam_sub.vector, &num);
    c_quadform_vMv(ones, &invSlam_sub.matrix, &den);
    
    mu_lam_mean = num/den;
    mu_lam_var = *sigSq_lam2/den;
    
    *mu_lam2 = rnorm(mu_lam_mean, sqrt(mu_lam_var));
    
    gsl_vector *diff = gsl_vector_calloc(J2+1);
    gsl_vector_set_all(diff, *mu_lam2);
    gsl_vector_sub(diff, &lam_sub.vector);
    c_quadform_vMv(diff, &invSlam_sub.matrix, &sigRT);
    sigRT /= 2;
    sigRT += b2;
    sigSC = 1/sigRT;
    sigSH = a2 + (double) (J2+1)/2;
    tau = rgamma(sigSH, sigSC);
    *sigSq_lam2 = 1/tau;
    
    gsl_vector_free(ones);
    gsl_vector_free(diff);
    
    return;
}










/* Updating second stage survival components: mu_lam3 and sigSq_lam3 */

void BscrSM_updateSP3(double *mu_lam3,
               double *sigSq_lam3,
               gsl_vector *lambda3,
               gsl_matrix *Sigma_lam3,
               gsl_matrix *invSigma_lam3,
               double a3,
               double b3,
               int J3)
{
    double num, den, sigSH, sigRT, sigSC, tau, mu_lam_mean, mu_lam_var;
    
    gsl_vector *ones = gsl_vector_calloc(J3+1);
    gsl_vector_set_all(ones, 1);
    
    gsl_matrix_view invSlam_sub = gsl_matrix_submatrix(invSigma_lam3, 0, 0, J3+1, J3+1);
    gsl_vector_view lam_sub     = gsl_vector_subvector(lambda3, 0, J3+1);
    
    c_quadform_vMu(ones, &invSlam_sub.matrix, &lam_sub.vector, &num);
    c_quadform_vMv(ones, &invSlam_sub.matrix, &den);
    
    mu_lam_mean = num/den;
    mu_lam_var = *sigSq_lam3/den;
    
    *mu_lam3 = rnorm(mu_lam_mean, sqrt(mu_lam_var));
    
    gsl_vector *diff = gsl_vector_calloc(J3+1);
    gsl_vector_set_all(diff, *mu_lam3);
    gsl_vector_sub(diff, &lam_sub.vector);
    c_quadform_vMv(diff, &invSlam_sub.matrix, &sigRT);
    sigRT /= 2;
    sigRT += b3;
    sigSC = 1/sigRT;
    sigSH = a3 + (double) (J3+1)/2;
    tau = rgamma(sigSH, sigSC);
    *sigSq_lam3 = 1/tau;
    
    gsl_vector_free(ones);
    gsl_vector_free(diff);
    
    return;
}












/* updating frailty parameter: gamma */

/**/



void BscrSM_updateFP(gsl_vector *gamma,
              double theta,
              gsl_vector *xbeta1,
              gsl_vector *xbeta2,
              gsl_vector *xbeta3,
              gsl_vector *lambda1,
              gsl_vector *lambda2,
              gsl_vector *lambda3,
              gsl_vector *s1,
              gsl_vector *s2,
              gsl_vector *s3,
              int J1,
              int J2,
              int J3,
              gsl_vector *survTime1,
              gsl_vector *yStar,
              gsl_vector *survEvent1,
              gsl_vector *survEvent2)
{
    int n = survTime1 -> size;
    int i;
    double gamma_shape, gamma_rate, gamma_scale;
    
    for(i = 0; i< n; i++)
    {
        gamma_shape = gsl_vector_get(survEvent1, i) + gsl_vector_get(survEvent2, i) + 1/theta;
        gamma_rate  = BscrSM_wFunc(i, xbeta1, xbeta2, xbeta3, lambda1, lambda2, lambda3, s1, s2, s3, J1, J2, J3, survTime1, yStar) + 1/theta;
        
        gamma_scale = 1/gamma_rate;
        gsl_vector_set(gamma, i, rgamma(gamma_shape, gamma_scale));
        
    }
       
    return;
}



















/* updating variance parameter: theta */

/* use the random walk proposal */

/**/

void BscrSM_updateDP(gsl_vector *gamma,
              double *theta,
              double mhProp_theta_var,
              double psi,
              double omega,
              int *accept_theta)
{
    double logPost, logPost_prop;
    double logProp_IniToProp, logProp_PropToIni;
    double logR;
    int n = gamma -> size;
    int u;
    int i;
    double xi = 1/(*theta);
    double temp_prop;
    
    logPost = 0; logPost_prop = 0;
    temp_prop = rgamma(pow(xi, 2)/mhProp_theta_var, mhProp_theta_var/(xi));
    
    for(i = 0; i < n; i++)
    {
        logPost      += xi * (log(gsl_vector_get(gamma, i)) - gsl_vector_get(gamma, i));
        logPost_prop += temp_prop * (log(gsl_vector_get(gamma, i)) - gsl_vector_get(gamma, i));
    }
    
    logPost         += (n * xi + psi - 1)*log(xi) - xi*omega - n*lgamma(xi);
    logPost_prop    += (n * temp_prop + psi - 1)*log(temp_prop) - temp_prop*omega - n*lgamma(temp_prop);
    
    logProp_PropToIni = dgamma(xi, pow(temp_prop, 2)/mhProp_theta_var, mhProp_theta_var/(temp_prop), 1);
    logProp_IniToProp = dgamma(temp_prop, pow(xi, 2)/mhProp_theta_var, mhProp_theta_var/(xi), 1);
    
    logR = logPost_prop - logPost + logProp_PropToIni - logProp_IniToProp;
    
    u = log(runif(0, 1)) <logR;
    
    if(u == 1)
    {
        *theta = 1/temp_prop;
        *accept_theta += u;
    }
    
    return;
}
























/* Updating the number of splits and their positions: J1 and s1 (Birth move) */


void BscrSM_updateBI1(gsl_vector *s1,
               int *J1,
               int *accept_BI1,
               gsl_vector *survTime1,
               gsl_vector *survEvent1,
               gsl_vector *gamma,
               gsl_vector *xbeta1,
               gsl_matrix *Sigma_lam1,
               gsl_matrix *invSigma_lam1,
               gsl_matrix *W1,
               gsl_matrix *Q1,
               gsl_vector *lambda1,
               gsl_vector *s_propBI1,
               int num_s_propBI1,
               double delPert1,
               int alpha1,
               double c_lam1,
               double mu_lam1,
               double sigSq_lam1,
               double s1_max)
{
    int count, num_s_propBI_fin, skip;
    int star_inx, j_old, J_new, i, j, u;
    double s_star, Upert, newLam1, newLam2;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta1 -> size;
    
    count = 0;
    
    gsl_vector *interInx = gsl_vector_calloc(num_s_propBI1);
    
    for(i = 0; i < num_s_propBI1; i++)
    {
        for(j = 0; j < *J1+1; j++)
        {
            if(gsl_vector_get(s_propBI1, i) == gsl_vector_get(s1, j))
            {
                count += 1;
                gsl_vector_set(interInx, count-1, i);
            }
        }
    }
    
    gsl_vector *s_propBI_fin = gsl_vector_calloc(num_s_propBI1-count);
    
    num_s_propBI_fin = s_propBI_fin -> size;
    skip = 0;
    
    if(count > 0)
    {
        for(i = 0; i < num_s_propBI1; i++)
        {
            if(i != gsl_vector_get(interInx, skip))
            {
                gsl_vector_set(s_propBI_fin, i-skip, gsl_vector_get(s_propBI1, i));
            }
            if(i == gsl_vector_get(interInx, skip)) skip += 1;
        }
    }
    if(count == 0) gsl_vector_memcpy(s_propBI_fin, s_propBI1);
    
    
    star_inx = (int) runif(0, num_s_propBI_fin);
    s_star = gsl_vector_get(s_propBI_fin, star_inx);
    
    
    j_old = -1;
    i = 0;
    
    while(j_old < 0)
    {
        if(gsl_vector_get(s1, i) >= s_star) j_old = i;
        else i += 1;
    }
    
       
    gsl_vector *s_new = gsl_vector_calloc(*J1+2);
    for(i = 0; i < *J1+1; i++)
    {
        gsl_vector_set(s_new, i, gsl_vector_get(s1, i));
    }
    gsl_vector_set(s_new, *J1+1, s_star);
    gsl_sort_vector(s_new);
    
    J_new = *J1+1;
    
    Upert = runif(0.5 - delPert1, 0.5 + delPert1);
    
    if(j_old != 0)
    {
        newLam1 = gsl_vector_get(lambda1, j_old) - (gsl_vector_get(s1, j_old) - s_star)/(gsl_vector_get(s1, j_old) - gsl_vector_get(s1, j_old-1)) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda1, j_old) + (s_star - gsl_vector_get(s1, j_old-1))/(gsl_vector_get(s1, j_old) - gsl_vector_get(s1, j_old-1)) * log((1-Upert)/Upert);
    }
    
    if(j_old == 0)
    {
        newLam1 = gsl_vector_get(lambda1, j_old) - (gsl_vector_get(s1, j_old) - s_star)/(gsl_vector_get(s1, j_old) - 0) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda1, j_old) + (s_star - 0)/(gsl_vector_get(s1, j_old) - 0) * log((1-Upert)/Upert);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(*J1+2);
    
    skip = 0;
    for(i = 0; i < *J1+2; i++)
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
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda1, i-skip));
    }
    
    
    
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam1, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    

    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, 0))
            {
                logLH += gsl_vector_get(lambda1, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s1, j_old-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, j_old))
            {
                logLH += gsl_vector_get(lambda1, j_old);                
            }
        }
        
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s1, j_old), gsl_vector_get(survTime1, i)) - gsl_vector_get(s1, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s1, j_old), gsl_vector_get(survTime1, i)) - 0);
        }
        if(Del > 0)
        {
            logLH   += - gsl_vector_get(gamma, i) * Del * exp(gsl_vector_get(lambda1, j_old))*exp(gsl_vector_get(xbeta1, i));
        }
    }
    
       
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(survEvent1, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s_new, 0))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                   
                }
                if(j != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s_new, j-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s_new, j))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                   
                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s_new, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime1, i)) - 0);
            }
            if(Del > 0)
            {
                logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j))*exp(gsl_vector_get(xbeta1, i));
               
            }
        }
    }
        
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda1, 0, *J1+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam1, 0, 0, *J1+1, *J1+1);
    
    if(*J1+1 != 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam1, sqrt(sigSq_lam1), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam1, sqrt(sigSq_lam1), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log( (2*(*J1) + 3)*(2*(*J1) + 2) * pow(gsl_vector_get(s1, *J1), -2) * (s_star - gsl_vector_get(s1, j_old-1)) * (gsl_vector_get(s1, j_old) - s_star)/(gsl_vector_get(s1, j_old) - gsl_vector_get(s1, j_old-1)) );
        }
        if(j_old == 0)
        {
            logPrior_prop += log( (2*(*J1) + 3)*(2*(*J1) + 2) * pow(gsl_vector_get(s1, *J1), -2) * (s_star - 0) * (gsl_vector_get(s1, j_old) - s_star)/(gsl_vector_get(s1, j_old) - 0) );
        }
    }
    
    if(*J1+1 == 1)
    {
        logPrior = dnorm(gsl_vector_get(lambda1, 0), mu_lam1, sqrt(sigSq_lam1*gsl_matrix_get(Sigma_lam1, 0, 0)), 1);
        c_dmvnorm(lambda_new, mu_lam1, sqrt(sigSq_lam1), invSigma_lam_new, &logPrior_prop);
        
        logPrior_prop += log( (2*(*J1) + 3)*(2*(*J1) + 2) * pow(gsl_vector_get(s1, *J1), -2) * (s_star - 0) * (gsl_vector_get(s1, j_old) - s_star)/(gsl_vector_get(s1, j_old) - 0) );
    }
    
    
    logPriorR = log((double) alpha1/((*J1) + 1)) + logPrior_prop - logPrior;
    

    
    logPropR = log(num_s_propBI_fin/alpha1) - dunif(Upert, 0.5-delPert1, 0.5+delPert1, 1);
    
    logJacob = log(1/(1-Upert)/Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob; 
    
    u = log(runif(0, 1)) < logR;
 
    if(u == 1)
    {
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q1, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s1, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda1, 0, J_new+1);
        
        *accept_BI1 += 1;
        *J1 = J_new;
        
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















/* Updating the number of splits and their positions: J1 and s1 (Death move) */


void BscrSM_updateDI1(gsl_vector *s1,
               int *J1,
               int *accept_DI1,
               gsl_vector *survTime1,
               gsl_vector *survEvent1,
               gsl_vector *gamma,               
               gsl_vector *xbeta1,
               gsl_matrix *Sigma_lam1,
               gsl_matrix *invSigma_lam1,
               gsl_matrix *W1,
               gsl_matrix *Q1,
               gsl_vector *lambda1,
               gsl_vector *s_propBI1,
               int num_s_propBI1,
               double delPert1,
               int alpha1,
               double c_lam1,
               double mu_lam1,
               double sigSq_lam1,
               double s1_max,
               int J1_max)
{
    
    
    int skip, i, j;
    int j_old, J_new, u;
    double Upert, newLam;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta1 -> size;
    
    j_old = (int) runif(0, *J1);
    
    gsl_vector *s_new = gsl_vector_calloc(*J1);
    
    skip = 0;
    for(i = 0; i < *J1+1; i++)
    {
        if(i == j_old) skip += 1;
        else gsl_vector_set(s_new, i-skip, gsl_vector_get(s1, i));
    }
    
       
    
    J_new = *J1-1;
    
    Upert = 1/(exp(gsl_vector_get(lambda1, j_old+1) - gsl_vector_get(lambda1, j_old)) + 1);

    
    if(j_old != 0)
    {
        newLam = ((gsl_vector_get(s1, j_old) - gsl_vector_get(s1, j_old-1)) * gsl_vector_get(lambda1, j_old) + (gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old)) * gsl_vector_get(lambda1, j_old+1)) / (gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old-1));
    }
    
    if(j_old == 0)
    {
        newLam = ((gsl_vector_get(s1, j_old) - 0) * gsl_vector_get(lambda1, j_old) + (gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old)) * gsl_vector_get(lambda1, j_old+1)) / (gsl_vector_get(s1, j_old+1) - 0);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(J_new+1);
    
    skip = 0;
    for(i = 0; i < J_new+1; i++)
    {
        if(i == j_old){
            gsl_vector_set(lambda_new, i, newLam);
            skip += 1;
        }
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda1, i+skip));
    }
    
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam1, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(survEvent1, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, 0))
                {
                    logLH += gsl_vector_get(lambda1, j);
                }
                if(j != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s1, j-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s1, j))
                {
                    logLH += gsl_vector_get(lambda1, j);
                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s1, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s1, j), gsl_vector_get(survTime1, i)) - 0);
            }
            if(Del > 0)
            {
                logLH   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda1, j))*exp(gsl_vector_get(xbeta1, i));
            }
        }
    }
    
       
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(survEvent1, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime1, i) <= gsl_vector_get(s_new, 0))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime1, i) > gsl_vector_get(s_new, j_old-1) && gsl_vector_get(survTime1, i) <= gsl_vector_get(s_new, j_old))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
        }
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime1, i)) - gsl_vector_get(s_new, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime1, i)) - 0);
        }
        if(Del > 0)
        {
            logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j_old))*exp(gsl_vector_get(xbeta1, i));
        }
    }
    
    
    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda1, 0, *J1+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam1, 0, 0, *J1+1, *J1+1);
    
    
    if(*J1+1 != 2)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam1, sqrt(sigSq_lam1), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam1, sqrt(sigSq_lam1), invSigma_lam_new, &logPrior_prop);

         
        if(j_old != 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J1) + 1)/(2*(*J1)))*pow(gsl_vector_get(s1, *J1), 2)*(gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old-1))/(gsl_vector_get(s1, j_old) - gsl_vector_get(s1, j_old-1))/(gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old)));
        }
        if(j_old == 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J1)+1)/(2*(*J1)))*pow(gsl_vector_get(s1, *J1), 2)*(gsl_vector_get(s1, j_old+1) - 0)/(gsl_vector_get(s1, j_old) - 0)/(gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old)));
        }
    }
    
    
    if(*J1+1 == 2)
    {
        logPrior_prop = dnorm(gsl_vector_get(lambda_new, 0), mu_lam1, sqrt(sigSq_lam1*gsl_matrix_get(Sigma_lam_new, 0, 0)), 1);
        c_dmvnorm(lambda1, mu_lam1, sqrt(sigSq_lam1), invSigma_lam1, &logPrior);
        
        logPrior_prop += log(( (double) 1/(2*(*J1)+1)/(2*(*J1)))*pow(gsl_vector_get(s1, *J1), 2)*(gsl_vector_get(s1, j_old+1) - 0)/(gsl_vector_get(s1, j_old) - 0)/(gsl_vector_get(s1, j_old+1) - gsl_vector_get(s1, j_old)));
    }
    
    
    logPriorR = log((double) *J1/alpha1) + logPrior_prop - logPrior;
    
    logPropR = log((double) alpha1/num_s_propBI1) - dunif(Upert, 0.5-delPert1, 0.5+delPert1, 1);

    logJacob = log((1-Upert)*Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    
    if(u == 1)
    {
 
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W1, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q1, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s1, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda1, 0, J_new+1);
        
        gsl_matrix_memcpy(&Sigma_lam_save.matrix, Sigma_lam_new);
        gsl_matrix_memcpy(&invSigma_lam_save.matrix, invSigma_lam_new);
        gsl_matrix_memcpy(&W_save.matrix, W_new);
        gsl_matrix_memcpy(&Q_save.matrix, Q_new);
        gsl_vector_memcpy(&s_save.vector, s_new);
        gsl_vector_memcpy(&lambda_save.vector, lambda_new);
        

        gsl_vector *zeroVec_J = gsl_vector_calloc(J1_max+1);
        

        gsl_matrix_set_col(Sigma_lam1, *J1, zeroVec_J);
        gsl_matrix_set_row(Sigma_lam1, *J1, zeroVec_J);
        gsl_matrix_set_col(invSigma_lam1, *J1, zeroVec_J);
        gsl_matrix_set_row(invSigma_lam1, *J1, zeroVec_J);
        gsl_matrix_set_col(W1, *J1, zeroVec_J);
        gsl_matrix_set_row(W1, *J1, zeroVec_J);
        gsl_matrix_set_col(Q1, *J1, zeroVec_J);
        gsl_matrix_set_row(Q1, *J1, zeroVec_J);
        gsl_vector_set(s1, *J1, 0);
        gsl_vector_set(lambda1, *J1, 0);
        
        *accept_DI1 += 1;
        *J1 = J_new;
        
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



















/* Updating the number of splits and their positions: J2 and s2 (Birth move) */


void BscrSM_updateBI2(gsl_vector *s2,
               int *J2,
               int *accept_BI2,
               gsl_vector *survTime1,
               gsl_vector *survTime2,
               gsl_vector *case01,
               gsl_vector *gamma,
               gsl_vector *xbeta2,
               gsl_matrix *Sigma_lam2,
               gsl_matrix *invSigma_lam2,
               gsl_matrix *W2,
               gsl_matrix *Q2,
               gsl_vector *lambda2,
               gsl_vector *s_propBI2,
               int num_s_propBI2,
               double delPert2,
               int alpha2,
               double c_lam2,
               double mu_lam2,
               double sigSq_lam2,
               double s2_max)
{
    int count, num_s_propBI_fin, skip;
    int star_inx, j_old, J_new, i, j, u;
    double s_star, Upert, newLam1, newLam2;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta2 -> size;
    
    count = 0;
    
    gsl_vector *interInx = gsl_vector_calloc(num_s_propBI2);
    
    for(i = 0; i < num_s_propBI2; i++)
    {
        for(j = 0; j < *J2+1; j++)
        {
            if(gsl_vector_get(s_propBI2, i) == gsl_vector_get(s2, j))
            {
                count += 1;
                gsl_vector_set(interInx, count-1, i);
            }
        }
    }
    
    gsl_vector *s_propBI_fin = gsl_vector_calloc(num_s_propBI2-count);
    
    num_s_propBI_fin = s_propBI_fin -> size;
    skip = 0;
    
    if(count > 0)
    {
        for(i = 0; i < num_s_propBI2; i++)
        {
            if(i != gsl_vector_get(interInx, skip))
            {
                gsl_vector_set(s_propBI_fin, i-skip, gsl_vector_get(s_propBI2, i));
            }
            if(i == gsl_vector_get(interInx, skip)) skip += 1;
        }
    }
    if(count == 0) gsl_vector_memcpy(s_propBI_fin, s_propBI2);
    
    
    star_inx = (int) runif(0, num_s_propBI_fin);
    s_star = gsl_vector_get(s_propBI_fin, star_inx);
    
    j_old = -1;
    i = 0;
    
    while(j_old < 0)
    {
        if(gsl_vector_get(s2, i) >= s_star) j_old = i;
        else i += 1;
    }
    
    gsl_vector *s_new = gsl_vector_calloc(*J2+2);
    for(i = 0; i < *J2+1; i++)
    {
        gsl_vector_set(s_new, i, gsl_vector_get(s2, i));
    }
    gsl_vector_set(s_new, *J2+1, s_star);
    gsl_sort_vector(s_new);
    
    J_new = *J2+1;
    
    Upert = runif(0.5 - delPert2, 0.5 + delPert2);
    
    if(j_old != 0)
    {
        newLam1 = gsl_vector_get(lambda2, j_old) - (gsl_vector_get(s2, j_old) - s_star)/(gsl_vector_get(s2, j_old) - gsl_vector_get(s2, j_old-1)) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda2, j_old) + (s_star - gsl_vector_get(s2, j_old-1))/(gsl_vector_get(s2, j_old) - gsl_vector_get(s2, j_old-1)) * log((1-Upert)/Upert);
    }
    
    if(j_old == 0)
    {
        newLam1 = gsl_vector_get(lambda2, j_old) - (gsl_vector_get(s2, j_old) - s_star)/(gsl_vector_get(s2, j_old) - 0) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda2, j_old) + (s_star - 0)/(gsl_vector_get(s2, j_old) - 0) * log((1-Upert)/Upert);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(*J2+2);
    
    skip = 0;
    for(i = 0; i < *J2+2; i++)
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
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda2, i-skip));
    }
    
    
    
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam2, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case01, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, 0))
            {
                logLH += gsl_vector_get(lambda2, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s2, j_old-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, j_old))
            {
                logLH += gsl_vector_get(lambda2, j_old);
            }
        }
        
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s2, j_old), gsl_vector_get(survTime1, i)) - gsl_vector_get(s2, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s2, j_old), gsl_vector_get(survTime1, i)) - 0);
        }
        if(Del > 0)
        {
            logLH   += - gsl_vector_get(gamma, i) * Del * exp(gsl_vector_get(lambda2, j_old))*exp(gsl_vector_get(xbeta2, i));
        }
    }

    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(case01, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s_new, 0))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                }
                if(j != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s_new, j-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s_new, j))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);

                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s_new, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s_new, j), gsl_vector_get(survTime1, i)) - 0);
            }
            if(Del > 0)
            {
                logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j))*exp(gsl_vector_get(xbeta2, i));

            }
        }
    }
    

    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda2, 0, *J2+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam2, 0, 0, *J2+1, *J2+1);
    
    if(*J2+1 != 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam2, sqrt(sigSq_lam2), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam2, sqrt(sigSq_lam2), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log( (2*(*J2) + 3)*(2*(*J2) + 2) * pow(gsl_vector_get(s2, *J2), -2) * (s_star - gsl_vector_get(s2, j_old-1)) * (gsl_vector_get(s2, j_old) - s_star)/(gsl_vector_get(s2, j_old) - gsl_vector_get(s2, j_old-1)) );
        }
        if(j_old == 0)
        {
            logPrior_prop += log( (2*(*J2) + 3)*(2*(*J2) + 2) * pow(gsl_vector_get(s2, *J2), -2) * (s_star - 0) * (gsl_vector_get(s2, j_old) - s_star)/(gsl_vector_get(s2, j_old) - 0) );
        }
    }
    
    if(*J2+1 == 1)
    {
        logPrior = dnorm(gsl_vector_get(lambda2, 0), mu_lam2, sqrt(sigSq_lam2*gsl_matrix_get(Sigma_lam2, 0, 0)), 1);
        c_dmvnorm(lambda_new, mu_lam2, sqrt(sigSq_lam2), invSigma_lam_new, &logPrior_prop);
        
        logPrior_prop += log( (2*(*J2) + 3)*(2*(*J2) + 2) * pow(gsl_vector_get(s2, *J2), -2) * (s_star - 0) * (gsl_vector_get(s2, j_old) - s_star)/(gsl_vector_get(s2, j_old) - 0) );
    }
    
    
    logPriorR = log((double) alpha2/((*J2) + 1)) + logPrior_prop - logPrior;
    
    logPropR = log(num_s_propBI_fin/alpha2) - dunif(Upert, 0.5-delPert2, 0.5+delPert2, 1);

    logJacob = log(1/(1-Upert)/Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    if(u == 1)
    {
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q2, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s2, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda2, 0, J_new+1);
        
        *accept_BI2 += 1;
        *J2 = J_new;
        
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











/* Updating the number of splits and their positions: J2 and s2 (Death move) */


void BscrSM_updateDI2(gsl_vector *s2,
               int *J2,
               int *accept_DI2,
               gsl_vector *survTime1,
               gsl_vector *survTime2,
               gsl_vector *case01,
               gsl_vector *gamma,
               gsl_vector *xbeta2,
               gsl_matrix *Sigma_lam2,
               gsl_matrix *invSigma_lam2,
               gsl_matrix *W2,
               gsl_matrix *Q2,
               gsl_vector *lambda2,
               gsl_vector *s_propBI2,
               int num_s_propBI2,
               double delPert2,
               int alpha2,
               double c_lam2,
               double mu_lam2,
               double sigSq_lam2,
               double s2_max,
               int J2_max)
{
    
    
    int skip, i, j;
    int j_old, J_new, u;
    double Upert, newLam;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta2 -> size;
    
    j_old = (int) runif(0, *J2);

    gsl_vector *s_new = gsl_vector_calloc(*J2);
    
    skip = 0;
    for(i = 0; i < *J2+1; i++)
    {
        if(i == j_old) skip += 1;
        else gsl_vector_set(s_new, i-skip, gsl_vector_get(s2, i));
    }
    
    
    J_new = *J2-1;
    
    Upert = 1/(exp(gsl_vector_get(lambda2, j_old+1) - gsl_vector_get(lambda2, j_old)) + 1);
    
    
    if(j_old != 0)
    {
        newLam = ((gsl_vector_get(s2, j_old) - gsl_vector_get(s2, j_old-1)) * gsl_vector_get(lambda2, j_old) + (gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old)) * gsl_vector_get(lambda2, j_old+1)) / (gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old-1));
    }
    
    if(j_old == 0)
    {
        newLam = ((gsl_vector_get(s2, j_old) - 0) * gsl_vector_get(lambda2, j_old) + (gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old)) * gsl_vector_get(lambda2, j_old+1)) / (gsl_vector_get(s2, j_old+1) - 0);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(J_new+1);
    
    skip = 0;
    for(i = 0; i < J_new+1; i++)
    {
        if(i == j_old){
            gsl_vector_set(lambda_new, i, newLam);
            skip += 1;
        }
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda2, i+skip));
    }
    

    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam2, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(case01, i) == 1)
            {
                if(j == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, 0))
                {
                    logLH += gsl_vector_get(lambda2, j);
                }
                if(j != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s2, j-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s2, j))
                {
                    logLH += gsl_vector_get(lambda2, j);
                }
            }
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - gsl_vector_get(s2, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s2, j), gsl_vector_get(survTime1, i)) - 0);
            }
            if(Del > 0)
            {
                logLH   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda2, j))*exp(gsl_vector_get(xbeta2, i));
            }
        }
    }
    
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case01, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(survTime2, i) <= gsl_vector_get(s_new, 0))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
            if(j_old != 0 && gsl_vector_get(survTime2, i) > gsl_vector_get(s_new, j_old-1) && gsl_vector_get(survTime2, i) <= gsl_vector_get(s_new, j_old))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
        }
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime1, i)) - gsl_vector_get(s_new, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(survTime1, i)) - 0);
        }
        if(Del > 0)
        {
            logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j_old))*exp(gsl_vector_get(xbeta2, i));
        }
    }

    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda2, 0, *J2+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam2, 0, 0, *J2+1, *J2+1);
    
    
    if(*J2+1 != 2)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam2, sqrt(sigSq_lam2), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam2, sqrt(sigSq_lam2), invSigma_lam_new, &logPrior_prop);
        

        if(j_old != 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J2) + 1)/(2*(*J2)))*pow(gsl_vector_get(s2, *J2), 2)*(gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old-1))/(gsl_vector_get(s2, j_old) - gsl_vector_get(s2, j_old-1))/(gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old)));
        }
        if(j_old == 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J2)+1)/(2*(*J2)))*pow(gsl_vector_get(s2, *J2), 2)*(gsl_vector_get(s2, j_old+1) - 0)/(gsl_vector_get(s2, j_old) - 0)/(gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old)));
        }
    }
    
    
    if(*J2+1 == 2)
    {
        logPrior_prop = dnorm(gsl_vector_get(lambda_new, 0), mu_lam2, sqrt(sigSq_lam2*gsl_matrix_get(Sigma_lam_new, 0, 0)), 1);
        c_dmvnorm(lambda2, mu_lam2, sqrt(sigSq_lam2), invSigma_lam2, &logPrior);
        
        logPrior_prop += log(( (double) 1/(2*(*J2)+1)/(2*(*J2)))*pow(gsl_vector_get(s2, *J2), 2)*(gsl_vector_get(s2, j_old+1) - 0)/(gsl_vector_get(s2, j_old) - 0)/(gsl_vector_get(s2, j_old+1) - gsl_vector_get(s2, j_old)));
    }
    
    
    logPriorR = log((double) *J2/alpha2) + logPrior_prop - logPrior;
    
    logPropR = log((double) alpha2/num_s_propBI2) - dunif(Upert, 0.5-delPert2, 0.5+delPert2, 1);    

    logJacob = log((1-Upert)*Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    
    if(u == 1)
    {
        
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W2, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q2, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s2, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda2, 0, J_new+1);
        
        gsl_matrix_memcpy(&Sigma_lam_save.matrix, Sigma_lam_new);
        gsl_matrix_memcpy(&invSigma_lam_save.matrix, invSigma_lam_new);
        gsl_matrix_memcpy(&W_save.matrix, W_new);
        gsl_matrix_memcpy(&Q_save.matrix, Q_new);
        gsl_vector_memcpy(&s_save.vector, s_new);
        gsl_vector_memcpy(&lambda_save.vector, lambda_new);
        
        
        gsl_vector *zeroVec_J = gsl_vector_calloc(J2_max+1);
        
        
        gsl_matrix_set_col(Sigma_lam2, *J2, zeroVec_J);
        gsl_matrix_set_row(Sigma_lam2, *J2, zeroVec_J);
        gsl_matrix_set_col(invSigma_lam2, *J2, zeroVec_J);
        gsl_matrix_set_row(invSigma_lam2, *J2, zeroVec_J);
        gsl_matrix_set_col(W2, *J2, zeroVec_J);
        gsl_matrix_set_row(W2, *J2, zeroVec_J);
        gsl_matrix_set_col(Q2, *J2, zeroVec_J);
        gsl_matrix_set_row(Q2, *J2, zeroVec_J);
        gsl_vector_set(s2, *J2, 0);
        gsl_vector_set(lambda2, *J2, 0);
        
        *accept_DI2 += 1;
        *J2 = J_new;
        
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









/* Updating the number of splits and their positions: J3 and s3 (Birth move) */


void BscrSM_updateBI3(gsl_vector *s3,
               int *J3,
               int *accept_BI3,
               gsl_vector *survTime1,
               gsl_vector *yStar,
               gsl_vector *case11,
               gsl_vector *gamma,
               gsl_vector *xbeta3,
               gsl_matrix *Sigma_lam3,
               gsl_matrix *invSigma_lam3,
               gsl_matrix *W3,
               gsl_matrix *Q3,
               gsl_vector *lambda3,
               gsl_vector *s_propBI3,
               int num_s_propBI3,
               double delPert3,
               int alpha3,
               double c_lam3,
               double mu_lam3,
               double sigSq_lam3,
               double s3_max)
{
    int count, num_s_propBI_fin, skip;
    int star_inx, j_old, J_new, i, j, u;
    double s_star, Upert, newLam1, newLam2;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta3 -> size;
    
    count = 0;
    
    gsl_vector *interInx = gsl_vector_calloc(num_s_propBI3);
    
    for(i = 0; i < num_s_propBI3; i++)
    {
        for(j = 0; j < *J3+1; j++)
        {
            if(gsl_vector_get(s_propBI3, i) == gsl_vector_get(s3, j))
            {
                count += 1;
                gsl_vector_set(interInx, count-1, i);
            }
        }
    }
    
    gsl_vector *s_propBI_fin = gsl_vector_calloc(num_s_propBI3-count);
    
    num_s_propBI_fin = s_propBI_fin -> size;
    skip = 0;
    
    if(count > 0)
    {
        for(i = 0; i < num_s_propBI3; i++)
        {
            if(i != gsl_vector_get(interInx, skip))
            {
                gsl_vector_set(s_propBI_fin, i-skip, gsl_vector_get(s_propBI3, i));
            }
            if(i == gsl_vector_get(interInx, skip)) skip += 1;
        }
    }
    if(count == 0) gsl_vector_memcpy(s_propBI_fin, s_propBI3);
    
    
    star_inx = (int) runif(0, num_s_propBI_fin);
    s_star = gsl_vector_get(s_propBI_fin, star_inx);
    
    j_old = -1;
    i = 0;
    
    while(j_old < 0)
    {
        if(gsl_vector_get(s3, i) >= s_star) j_old = i;
        else i += 1;
    }
    
    gsl_vector *s_new = gsl_vector_calloc(*J3+2);
    for(i = 0; i < *J3+1; i++)
    {
        gsl_vector_set(s_new, i, gsl_vector_get(s3, i));
    }
    gsl_vector_set(s_new, *J3+1, s_star);
    gsl_sort_vector(s_new);
    
    
 
    
    J_new = *J3+1;
    
    Upert = runif(0.5 - delPert3, 0.5 + delPert3);
    
    
    if(j_old != 0)
    {
        newLam1 = gsl_vector_get(lambda3, j_old) - (gsl_vector_get(s3, j_old) - s_star)/(gsl_vector_get(s3, j_old) - gsl_vector_get(s3, j_old-1)) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda3, j_old) + (s_star - gsl_vector_get(s3, j_old-1))/(gsl_vector_get(s3, j_old) - gsl_vector_get(s3, j_old-1)) * log((1-Upert)/Upert);
    }
    
    if(j_old == 0)
    {
        newLam1 = gsl_vector_get(lambda3, j_old) - (gsl_vector_get(s3, j_old) - s_star)/(gsl_vector_get(s3, j_old) - 0) * log((1-Upert)/Upert);
        newLam2 = gsl_vector_get(lambda3, j_old) + (s_star - 0)/(gsl_vector_get(s3, j_old) - 0) * log((1-Upert)/Upert);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(*J3+2);
    
    skip = 0;
    for(i = 0; i < *J3+2; i++)
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
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda3, i-skip));
    }
      
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam3, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case11, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, 0))
            {
                logLH += gsl_vector_get(lambda3, j_old);
            }
            if(j_old != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s3, j_old-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, j_old))
            {
                logLH += gsl_vector_get(lambda3, j_old);
            }
        }
        
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s3, j_old), gsl_vector_get(yStar, i)) - gsl_vector_get(s3, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s3, j_old), gsl_vector_get(yStar, i)) - 0);
        }

        
        if(Del > 0)
        {
            logLH   += - gsl_vector_get(gamma, i) * Del * exp(gsl_vector_get(lambda3, j_old))*exp(gsl_vector_get(xbeta3, i));
        }
    }
       
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(case11, i) == 1)
            {
                if(j == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s_new, 0))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                }
                if(j != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s_new, j-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s_new, j))
                {
                    logLH_prop += gsl_vector_get(lambda_new, j);
                }
            }
            
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s_new, j), gsl_vector_get(yStar, i)) - gsl_vector_get(s_new, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s_new, j), gsl_vector_get(yStar, i)) - 0);
            }
            
 
            if(Del > 0)
            {
                logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j))*exp(gsl_vector_get(xbeta3, i));
                           }
        }
    }
    
    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda3, 0, *J3+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam3, 0, 0, *J3+1, *J3+1);
    
    if(*J3+1 != 1)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam3, sqrt(sigSq_lam3), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam3, sqrt(sigSq_lam3), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log( (2*(*J3) + 3)*(2*(*J3) + 2) * pow(gsl_vector_get(s3, *J3), -2) * (s_star - gsl_vector_get(s3, j_old-1)) * (gsl_vector_get(s3, j_old) - s_star)/(gsl_vector_get(s3, j_old) - gsl_vector_get(s3, j_old-1)) );
        }
        if(j_old == 0)
        {
            logPrior_prop += log( (2*(*J3) + 3)*(2*(*J3) + 2) * pow(gsl_vector_get(s3, *J3), -2) * (s_star - 0) * (gsl_vector_get(s3, j_old) - s_star)/(gsl_vector_get(s3, j_old) - 0) );
        }
    }
    
    if(*J3+1 == 1)
    {
        logPrior = dnorm(gsl_vector_get(lambda3, 0), mu_lam3, sqrt(sigSq_lam3*gsl_matrix_get(Sigma_lam3, 0, 0)), 1);
        c_dmvnorm(lambda_new, mu_lam3, sqrt(sigSq_lam3), invSigma_lam_new, &logPrior_prop);
        
        logPrior_prop += log( (2*(*J3) + 3)*(2*(*J3) + 2) * pow(gsl_vector_get(s3, *J3), -2) * (s_star - 0) * (gsl_vector_get(s3, j_old) - s_star)/(gsl_vector_get(s3, j_old) - 0) );
    }
    
    
    logPriorR = log((double) alpha3/((*J3) + 1)) + logPrior_prop - logPrior;
    
    logPropR = log(num_s_propBI_fin/alpha3) - dunif(Upert, 0.5-delPert3, 0.5+delPert3, 1);
    
    logJacob = log(1/(1-Upert)/Upert);
    
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;

    if(u == 1)
    {
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q3, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s3, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda3, 0, J_new+1);
        
        *accept_BI3 += 1;
        *J3 = J_new;
        
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





















/* Updating the number of splits and their positions: J3 and s3 (Death move) */


void BscrSM_updateDI3(gsl_vector *s3,
               int *J3,
               int *accept_DI3,
               gsl_vector *survTime1,
               gsl_vector *yStar,
               gsl_vector *case11,
               gsl_vector *gamma,
               gsl_vector *xbeta3,
               gsl_matrix *Sigma_lam3,
               gsl_matrix *invSigma_lam3,
               gsl_matrix *W3,
               gsl_matrix *Q3,
               gsl_vector *lambda3,
               gsl_vector *s_propBI3,
               int num_s_propBI3,
               double delPert3,
               int alpha3,
               double c_lam3,
               double mu_lam3,
               double sigSq_lam3,
               double s3_max,
               int J3_max)
{
    
    
    int skip, i, j;
    int j_old, J_new, u;
    double Upert, newLam;
    double logLH, logLH_prop, Del;
    double logPrior, logPrior_prop, logPriorR, logPropR;
    double logJacob, logR;
    
    int n = xbeta3 -> size;
    
    j_old = (int) runif(0, *J3);
    
    gsl_vector *s_new = gsl_vector_calloc(*J3);
    
    skip = 0;
    for(i = 0; i < *J3+1; i++)
    {
        if(i == j_old) skip += 1;
        else gsl_vector_set(s_new, i-skip, gsl_vector_get(s3, i));
    }
    
    J_new = *J3-1;
    
    Upert = 1/(exp(gsl_vector_get(lambda3, j_old+1) - gsl_vector_get(lambda3, j_old)) + 1);
    
    if(j_old != 0)
    {
        newLam = ((gsl_vector_get(s3, j_old) - gsl_vector_get(s3, j_old-1)) * gsl_vector_get(lambda3, j_old) + (gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old)) * gsl_vector_get(lambda3, j_old+1)) / (gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old-1));
    }
    
    if(j_old == 0)
    {
        newLam = ((gsl_vector_get(s3, j_old) - 0) * gsl_vector_get(lambda3, j_old) + (gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old)) * gsl_vector_get(lambda3, j_old+1)) / (gsl_vector_get(s3, j_old+1) - 0);
    }
    
    gsl_vector *lambda_new = gsl_vector_calloc(J_new+1);
    
    skip = 0;
    for(i = 0; i < J_new+1; i++)
    {
        if(i == j_old){
            gsl_vector_set(lambda_new, i, newLam);
            skip += 1;
        }
        else gsl_vector_set(lambda_new, i, gsl_vector_get(lambda3, i+skip));
    }
        
    gsl_matrix *Sigma_lam_new       = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *invSigma_lam_new    = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *W_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    gsl_matrix *Q_new               = gsl_matrix_calloc(J_new+1, J_new+1);
    
    cal_Sigma(Sigma_lam_new, invSigma_lam_new, W_new, Q_new, s_new, c_lam3, J_new);
    
    logLH = 0; logLH_prop = 0;
    logPrior = 0; logPrior_prop = 0; logPropR = 0;
    
    
    for(j = j_old; j < j_old + 2; j++)
    {
        for(i = 0; i < n; i++)
        {
            if(gsl_vector_get(case11, i) == 1)
            {
                if(j == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, 0))
                {
                    logLH += gsl_vector_get(lambda3, j);
                }
                if(j != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s3, j-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s3, j))
                {
                    logLH += gsl_vector_get(lambda3, j);
                }
            }
            
            if(j > 0)
            {
                Del = c_max(0, (c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - gsl_vector_get(s3, j-1)));
            }
            if(j == 0)
            {
                Del = c_max(0, c_min(gsl_vector_get(s3, j), gsl_vector_get(yStar, i)) - 0);
            }
            
            if(Del > 0)
            {
                logLH   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda3, j))*exp(gsl_vector_get(xbeta3, i));
            }
        }
    }
    
    
    
    for(i = 0; i < n; i++)
    {
        if(gsl_vector_get(case11, i) == 1)
        {
            if(j_old == 0 && gsl_vector_get(yStar, i) <= gsl_vector_get(s_new, 0))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
            if(j_old != 0 && gsl_vector_get(yStar, i) > gsl_vector_get(s_new, j_old-1) && gsl_vector_get(yStar, i) <= gsl_vector_get(s_new, j_old))
            {
                logLH_prop += gsl_vector_get(lambda_new, j_old);
            }
        }
        
        if(j_old > 0)
        {
            Del = c_max(0, (c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(yStar, i)) - gsl_vector_get(s_new, j_old-1)));
        }
        if(j_old == 0)
        {
            Del = c_max(0, c_min(gsl_vector_get(s_new, j_old), gsl_vector_get(yStar, i)) - 0);
        }
        
        if(Del > 0)
        {
            logLH_prop   += - gsl_vector_get(gamma, i) * Del*exp(gsl_vector_get(lambda_new, j_old))*exp(gsl_vector_get(xbeta3, i));
        }
    }
    
    gsl_vector_view lambda_sub  = gsl_vector_subvector(lambda3, 0, *J3+1);
    gsl_matrix_view invS_sub    = gsl_matrix_submatrix(invSigma_lam3, 0, 0, *J3+1, *J3+1);
    
    
    if(*J3+1 != 2)
    {
        c_dmvnorm(&lambda_sub.vector, mu_lam3, sqrt(sigSq_lam3), &invS_sub.matrix, &logPrior);
        c_dmvnorm(lambda_new, mu_lam3, sqrt(sigSq_lam3), invSigma_lam_new, &logPrior_prop);
        
        if(j_old != 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J3) + 1)/(2*(*J3)))*pow(gsl_vector_get(s3, *J3), 2)*(gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old-1))/(gsl_vector_get(s3, j_old) - gsl_vector_get(s3, j_old-1))/(gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old)));
        }
        if(j_old == 0)
        {
            logPrior_prop += log(( (double) 1/(2*(*J3)+1)/(2*(*J3)))*pow(gsl_vector_get(s3, *J3), 2)*(gsl_vector_get(s3, j_old+1) - 0)/(gsl_vector_get(s3, j_old) - 0)/(gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old)));
        }
    }
    
    
    if(*J3+1 == 2)
    {
        logPrior_prop = dnorm(gsl_vector_get(lambda_new, 0), mu_lam3, sqrt(sigSq_lam3*gsl_matrix_get(Sigma_lam_new, 0, 0)), 1);
        c_dmvnorm(lambda3, mu_lam3, sqrt(sigSq_lam3), invSigma_lam3, &logPrior);
        
        logPrior_prop += log(( (double) 1/(2*(*J3)+1)/(2*(*J3)))*pow(gsl_vector_get(s3, *J3), 2)*(gsl_vector_get(s3, j_old+1) - 0)/(gsl_vector_get(s3, j_old) - 0)/(gsl_vector_get(s3, j_old+1) - gsl_vector_get(s3, j_old)));
    }
    
    
    logPriorR = log((double) *J3/alpha3) + logPrior_prop - logPrior;
    
    logPropR = log((double) alpha3/num_s_propBI3) - dunif(Upert, 0.5-delPert3, 0.5+delPert3, 1);
    
    logJacob = log((1-Upert)*Upert);
 
    logR = logLH_prop - logLH + logPriorR + logPropR + logJacob;
    
    u = log(runif(0, 1)) < logR;
    
    
    if(u == 1)
    {
        
        gsl_matrix_view Sigma_lam_save      = gsl_matrix_submatrix(Sigma_lam3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view invSigma_lam_save   = gsl_matrix_submatrix(invSigma_lam3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view W_save              = gsl_matrix_submatrix(W3, 0, 0, J_new+1, J_new+1);
        gsl_matrix_view Q_save              = gsl_matrix_submatrix(Q3, 0, 0, J_new+1, J_new+1);
        gsl_vector_view s_save              = gsl_vector_subvector(s3, 0, J_new+1);
        gsl_vector_view lambda_save         = gsl_vector_subvector(lambda3, 0, J_new+1);
        
        gsl_matrix_memcpy(&Sigma_lam_save.matrix, Sigma_lam_new);
        gsl_matrix_memcpy(&invSigma_lam_save.matrix, invSigma_lam_new);
        gsl_matrix_memcpy(&W_save.matrix, W_new);
        gsl_matrix_memcpy(&Q_save.matrix, Q_new);
        gsl_vector_memcpy(&s_save.vector, s_new);
        gsl_vector_memcpy(&lambda_save.vector, lambda_new);
        
        
        gsl_vector *zeroVec_J = gsl_vector_calloc(J3_max+1);
        
        
        gsl_matrix_set_col(Sigma_lam3, *J3, zeroVec_J);
        gsl_matrix_set_row(Sigma_lam3, *J3, zeroVec_J);
        gsl_matrix_set_col(invSigma_lam3, *J3, zeroVec_J);
        gsl_matrix_set_row(invSigma_lam3, *J3, zeroVec_J);
        gsl_matrix_set_col(W3, *J3, zeroVec_J);
        gsl_matrix_set_row(W3, *J3, zeroVec_J);
        gsl_matrix_set_col(Q3, *J3, zeroVec_J);
        gsl_matrix_set_row(Q3, *J3, zeroVec_J);
        gsl_vector_set(s3, *J3, 0);
        gsl_vector_set(lambda3, *J3, 0);
        
        *accept_DI3 += 1;
        *J3 = J_new;
        
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



















