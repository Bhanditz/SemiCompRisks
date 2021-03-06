FreqID_HReg <- function(Formula, data, model = "semi-Markov", frailty=TRUE, na.action = "na.fail",
subset=NULL)
{
    if(na.action != "na.fail" & na.action != "na.omit")
    {
        stop("na.action should be either na.fail or na.omit")
    }
    
    form2 <- as.Formula(paste(Formula[2], Formula[1], Formula[3], sep = ""))    
    data <- model.frame(form2, data=data, na.action = na.action, subset = subset)
    
    ##
    time1 <- model.part(Formula, data=data, lhs=1)
    time2 <- model.part(Formula, data=data, lhs=2)
    
    Y <- cbind(time1[1], time1[2], time2[1], time2[2])
    y1     <- as.vector(Y[,1])
    delta1 <- as.vector(Y[,2])
    y2     <- as.vector(Y[,3])
    delta2 <- as.vector(Y[,4])
    
    Xmat1 <- model.frame(formula(Formula, lhs=0, rhs=1), data=data)
    Xmat2 <- model.frame(formula(Formula, lhs=0, rhs=2), data=data)
    Xmat3 <- model.frame(formula(Formula, lhs=0, rhs=3), data=data)
    
    ##
    fit.survreg.1 <- survreg(as.formula(paste("Surv(y1, delta1) ", as.character(formula(Formula, lhs=0, rhs=1))[1], as.character(formula(Formula, lhs=0, rhs=1))[2])), dist="weibull", data=data)
    fit.survreg.2 <- survreg(as.formula(paste("Surv(y2, delta2) ", as.character(formula(Formula, lhs=0, rhs=2))[1], as.character(formula(Formula, lhs=0, rhs=2))[2])), dist="weibull", data=data)
    fit.survreg.3 <- survreg(as.formula(paste("Surv(y2, delta2) ", as.character(formula(Formula, lhs=0, rhs=3))[1], as.character(formula(Formula, lhs=0, rhs=3))[2])), dist="weibull", data=data)
    alpha1      <- 1 / fit.survreg.1$scale
    alpha2      <- 1 / fit.survreg.2$scale
    alpha3     	<- 1 / fit.survreg.3$scale
    
    startVals     <- c(-alpha1*coef(fit.survreg.1)[1], log(alpha1),
    -alpha2*coef(fit.survreg.2)[1], log(alpha2),
    -alpha3*coef(fit.survreg.3)[1], log(alpha3))
    if(frailty == TRUE) startVals <- c(startVals, 0.5)
    startVals     <- c(startVals,
    -coef(fit.survreg.1)[-1] * alpha1,
    -coef(fit.survreg.2)[-1] * alpha2,
    -coef(fit.survreg.3)[-1] * alpha3)
    
    if(model == "Markov")
    {
        ##
        fit0  <- suppressWarnings(nlm(logLike.weibull.SCR, p=startVals * runif(length(startVals), 0.9, 1.1),
        y1=y1, delta1=delta1, y2=y2, delta2=delta2, Xmat1=as.matrix(Xmat1), Xmat2=as.matrix(Xmat2), Xmat3=as.matrix(Xmat3), frailty=frailty,
        iterlim=1000, hessian=TRUE))
    }
    if(model == "semi-Markov")
    {
        ##
        fit0  <- suppressWarnings(nlm(logLike.weibull.SCR.SM, p=startVals * runif(length(startVals), 0.9, 1.1),
        y1=y1, delta1=delta1, y2=y2, delta2=delta2, Xmat1=as.matrix(Xmat1), Xmat2=as.matrix(Xmat2), Xmat3=as.matrix(Xmat3), frailty=frailty,
        iterlim=1000, hessian=TRUE))
    }
    
    ##
    if(fit0$code == 1 | fit0$code == 2)
    {
        ##
        myLabels <- c("log(kappa1)", "log(alpha1)", "log(kappa2)", "log(alpha2)", "log(kappa3)", "log(alpha3)")
        if(frailty == TRUE) myLabels <- c(myLabels, "log(theta)")
        myLabels <- c(myLabels, colnames(Xmat1), colnames(Xmat2), colnames(Xmat3))
        nP <- c(ncol(Xmat1), ncol(Xmat2), ncol(Xmat3))
        ##
        value <- list(estimate=fit0$estimate, Finv=solve(fit0$hessian), logLike=-fit0$minimum, myLabels=myLabels, frailty=frailty, nP=nP, Xmat=list(Xmat1, Xmat2, Xmat3))
        
        if(model == "semi-Markov")
        {
            value$class <- c("Freq_HReg", "ID", "Ind", "WB", "semi-Markov")
        }
        if(model == "Markov")
        {
            value$class <- c("Freq_HReg", "ID", "Ind", "WB", "Markov")
        }
        
        class(value) <- "Freq_HReg"
        
        return(value)
    }
    
    ##
    invisible()
}
