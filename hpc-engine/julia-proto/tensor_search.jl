# tensor_search.jl - CALC4 K<=5 tensor-based search
# Algorithm and idea: A. Odrzywolek, 2026-01-07 
# Code assist Claude 4.5 Opus
using SpecialFunctions

C = [pi, exp(1), -1.0, (1+sqrt(5))/2, 1.,2.,3.,4.,5.,6.,7.,8.,9.]
U = [log, exp, inv, gamma, sqrt, x->x^2,
     sin, asin, cos, acos, tan, atan,
     sinh, asinh, cosh, acosh, tanh, atanh]
B = [+, *, -, /, ^]

n_c, n_u, n_b = length(C), length(U), length(B)

T_c     = C
T_cu    = [U[j](T_c[i])               for i=1:n_c, j=1:n_u]
T_cuu   = [U[k](T_cu[i,j])            for i=1:n_c, j=1:n_u, k=1:n_u]
T_ccb   = [B[k](T_c[i], T_c[j])       for i=1:n_c, j=1:n_c, k=1:n_b]
T_cuuu  = [U[l](T_cuu[i,j,k])         for i=1:n_c, j=1:n_u, k=1:n_u, l=1:n_u]
T_ccbu  = [U[l](T_ccb[i,j,k])         for i=1:n_c, j=1:n_c, k=1:n_b, l=1:n_u]
T_cucb  = [B[l](T_cu[i,j], T_c[k])    for i=1:n_c, j=1:n_u, k=1:n_c, l=1:n_b]
T_ccub  = [B[l](T_c[i], T_cu[j,k])    for i=1:n_c, j=1:n_c, k=1:n_u, l=1:n_b]
T_cuuuu = [U[m](T_cuuu[i,j,k,l])      for i=1:n_c, j=1:n_u, k=1:n_u, l=1:n_u, m=1:n_u]
T_ccbuu = [U[m](T_ccbu[i,j,k,l])      for i=1:n_c, j=1:n_c, k=1:n_b, l=1:n_u, m=1:n_u]
T_cucbu = [U[m](T_cucb[i,j,k,l])      for i=1:n_c, j=1:n_u, k=1:n_c, l=1:n_b, m=1:n_u]
T_ccubu = [U[m](T_ccub[i,j,k,l])      for i=1:n_c, j=1:n_c, k=1:n_u, l=1:n_b, m=1:n_u]
T_cuucb = [B[m](T_cuu[i,j,k], T_c[l]) for i=1:n_c, j=1:n_u, k=1:n_u, l=1:n_c, m=1:n_b]
T_ccbcb = [B[m](T_ccb[i,j,k], T_c[l]) for i=1:n_c, j=1:n_c, k=1:n_b, l=1:n_c, m=1:n_b]
T_cucub = [B[m](T_cu[i,j], T_cu[k,l]) for i=1:n_c, j=1:n_u, k=1:n_c, l=1:n_u, m=1:n_b]
T_ccuub = [B[m](T_c[i], T_cuu[j,k,l]) for i=1:n_c, j=1:n_c, k=1:n_u, l=1:n_u, m=1:n_b]
T_cccbb = [B[m](T_c[i], T_ccb[j,k,l]) for i=1:n_c, j=1:n_c, k=1:n_c, l=1:n_b, m=1:n_b]

function search(target)
    tensors = [T_c, T_cu, T_cuu, T_ccb, T_cuuu, T_ccbu, T_cucb, T_ccub,
               T_cuuuu, T_ccbuu, T_cucbu, T_ccubu, T_cuucb, T_ccbcb, T_cucub, T_ccuub, T_cccbb]
    best_err, best_idx, best_tensor = Inf, nothing, nothing
    for T in tensors
        for idx in CartesianIndices(T)
            err = abs(T[idx] - target)
            if isfinite(err) && err < best_err
                best_err, best_idx, best_tensor = err, idx, T
            end
        end
    end
    println("Target: $target")
    println("Best error: $best_err")
    println("Value: $(best_tensor[best_idx])")
    println("Index: $best_idx")
end

search(666)
