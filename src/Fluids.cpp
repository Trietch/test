#include "Fluids.h"

Fluids::Fluids()
{
    initCG();
}

void Fluids::update([[maybe_unused]] std::uint64_t iteration)
{
    
    /* Testings */
    std::uint16_t N = _N/2;

    /*
    float p = 256;
    float z = 32;
    */

    /*
    _substance(N-(N/2), N+(N/2)) = p;
    _fieldX(N-(N/2), N+(N/2)) = z;
    _fieldY(N-(N/2), N+(N/2)) = -z;
    _fieldX(N-(N/2)+1, N+(N/2)) = z;
    _fieldY(N-(N/2), N+(N/2)+1) = -z;

    _substance(N-(N/2), N-(N/2)) = p;
    _fieldX(N-(N/2), N-(N/2)) = z;
    _fieldY(N-(N/2), N-(N/2)) = z;
    _fieldX(N-(N/2)+1, N-(N/2)) = z;
    _fieldY(N-(N/2), N-(N/2)+1) = z;

    _substance(N+(N/2), N+(N/2)) = p;
    _fieldX(N+(N/2), N+(N/2)) = -z;
    _fieldY(N+(N/2), N+(N/2)) = -z;
    _fieldX(N+(N/2)+1, N+(N/2)) = -z;
    _fieldY(N+(N/2), N+(N/2)+1) = -z;

    _substance(N+(N/2), N-(N/2)) = p;
    _fieldX(N+(N/2), N-(N/2)) = -z;
    _fieldY(N+(N/2), N-(N/2)) = z;
    _fieldX(N+(N/2)+1, N-(N/2)) = -z;
    _fieldY(N+(N/2), N-(N/2)+1) = z;
    */



    /* End Testings */

    /*
    if (iteration > 75)
    {
        std::cout << _fieldX << std::endl;
        std::cout << _fieldY << std::endl;
        exit(0);
    }
    */

    //std::cin.get();

    // LEVEL-SET
    double r = 9.9;
    if (iteration == 0)
    {
        glm::vec2 pt;
        pt.x = N-(N/2);
        pt.y = N;
        particles.emplace_back(pt);
        pt.x = N+(N/2);
        pt.y = N;
        particles.emplace_back(pt);

        for (std::uint16_t j = 0; j < _N; ++j)
        {
            for (std::uint16_t i = 0; i < _N; ++i)
            {
                double dist = std::sqrt(std::pow(i-particles.front().x,2)+std::pow(j-particles.front().y,2))-r;
                for (const auto& p : particles)
                {
                    dist = std::min(dist, std::sqrt(std::pow(i-p.x,2)+std::pow(j-p.y,2))-r);
                    //dist = std::cos(static_cast<double>(i)/4.0)+std::cos(static_cast<double>(j)/4);
                }
                _prevImplicit(i,j) = dist;
            }
        }
    }

    levelSetStep();


    for (std::uint16_t j = 0; j < _implicit.y(); ++j)
    {
        for (std::uint16_t i = 0; i < _implicit.x(); ++i)
        {
            if (_implicit(i,j) < 0)
            {
                if (i < N)
                {
                    _fieldX(i,j) = 4;
                    _fieldX(i+1,j) = 4;
                    _fieldX.set(i,j, true);
                    _fieldX.set(i+1,j, true);
                }
                else if (i > N)
                {
                    _fieldX(i+1,j) = -4;
                    _fieldX(i,j) = -4;
                    _fieldX.set(i+1,j, true);
                    _fieldX.set(i,j, true);
                }
                else
                {
                   _fieldX.set(i,j, true); 
                }
                //_fieldY.set(i,j, true);
            }
        }
    }

    extrapolate(_fieldX);
    extrapolate(_fieldY);

    _fieldX.resetBool();
    _fieldY.resetBool();
    

    //std::cout << _fieldX << std::endl;

    //setBnd(_fieldY, 2);
    
    /*
    std::cout << _implicit << std::endl;
    std::cout << _fieldY << std::endl;
    */

    auto start = std::chrono::high_resolution_clock::now();
    vStep();
    auto end = std::chrono::high_resolution_clock::now();
    VstepTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
    start = std::chrono::high_resolution_clock::now();
    //sStep();
    end = std::chrono::high_resolution_clock::now();
    SstepTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    updateTexture();

    /*
    std::cout << _implicit << std::endl;
    std::cout << _fieldX << std::endl;
    std::cout << _fieldY << std::endl;
    */
}

void Fluids::levelSetStep()
{
    advect(_implicit, _prevImplicit, _fieldX, _fieldY, 0);
    _prevImplicit = _implicit;
    //reinitLevelSet(32);
}

void Fluids::extrapolate(Field<double,std::uint16_t>& F)
{
    Field<double,std::uint16_t> temp {F.x(), F.y()};
    std::uint64_t nbNeg = 0;
    do
    {
        nbNeg = 0;
        #pragma omp parallel for
        for (std::uint64_t n = 0; n < F.maxIt(); ++n)
        {
            const std::uint64_t m = n % F.maxIt();
            const std::uint16_t i = m % F.x();
            const std::uint16_t j = m / F.x();

            if (F.isSet(i,j))
            {
                temp(i,j) = F(i,j);
                temp.set(i,j, true);
            }
            else
            {
                std::uint8_t nbNeighbors = 0;
                double value = 0.0;
                if (i < _N-1 && F.isSet(i+1,j))
                {
                    nbNeighbors++;
                    value += F(i+1,j);
                }
                if (i > 0 && F.isSet(i-1,j))
                {
                    nbNeighbors++;
                    value += F(i-1,j);
                }
                if (j < _N-1 && F.isSet(i,j+1))
                {
                    nbNeighbors++;
                    value += F(i,j+1);
                }
                if (j > 0 && F.isSet(i,j-1))
                {
                    nbNeighbors++;
                    value += F(i,j-1);
                }
                if (nbNeighbors > 0)
                {
                    nbNeg++;
                    temp(i,j) = value/nbNeighbors;
                    temp.set(i,j, true);
                }
            }
        }
        F = temp;
    } while (nbNeg > 0);
}

void Fluids::reinitLevelSet(const std::uint64_t nbIte)
{
    Field<double, std::uint16_t> Ssf = _prevImplicit;
    Field<double, std::uint16_t> n = _prevImplicit;
    const double dx = 1.0/_N;
    // Init smoothing function
    for (std::uint16_t j = 0; j < _N; ++j)
    {
        for (std::uint16_t i = 0; i < _N; ++i)
        {
            const double O0 = _prevImplicit(i,j);
            Ssf(i,j) = O0 / (std::sqrt(std::pow(O0, 2) + std::pow(dx, 2)));
        }
    }
    // Step forward in fictious time
    for (std::uint64_t relaxit = 0; relaxit < nbIte; ++relaxit)
    {
        for (std::uint16_t j = 0; j < _N; ++j)
        {
            for (std::uint16_t i = 0; i < _N; ++i)
            {
                const double gO = gradLength(_prevImplicit, i, j);
                n(i,j) = _prevImplicit(i,j) + (0.5 * dx * (- Ssf(i,j) * (gO - 1.0)));
            }
        }
        _prevImplicit = n;
    }
}

double Fluids::gradLength(const Field<double,std::uint16_t>& F, const std::uint16_t i, const std::uint16_t j) const
{
    double gradI = 0;
    double gradJ = 0;
    if (i == 0)
    {
        gradI = F(1,j) - F(0,j);
    }
    else if (i == _N-1)
    {
        gradI = F(_N-1,j) - F(_N-2,j);
    }
    else
    {
        if (std::abs(F(i+1,j)) < std::abs(F(i-1,j)))
        {
            gradI = F(i,j) - F(i+1,j);
        }
        else
        {
            gradI = F(i-1,j) - F(i,j);
        }
    }
    if (j == 0)
    {
        gradJ = F(i,1) - F(i,0);
    }
    else if (j == _N-1)
    {
        gradJ = F(i,_N-1) - F(i,_N-2);
    }
    else
    {
        if (std::abs(F(i,j+1)) < std::abs(F(i,j-1)))
        {
            gradJ = F(i,j) - F(i,j+1);
        }
        else
        {
            gradJ = F(i,j-1) - F(i,j);
        }
    }
    return std::sqrt(std::pow(gradI, 2)+std::pow(gradJ, 2));
}


void Fluids::vStep()
{
    auto start = std::chrono::high_resolution_clock::now();
    /*
    diffuse(_prevFieldX, _fieldX, 1, _laplacianViscosityX);
    diffuse(_prevFieldY, _fieldY, 2, _laplacianViscosityY);
    */
    auto end = std::chrono::high_resolution_clock::now();
    VstepDiffuseTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    _prevFieldX = _fieldX;
    _prevFieldY = _fieldY;

    start = std::chrono::high_resolution_clock::now();
    project(_prevFieldX, _prevFieldY);
    end = std::chrono::high_resolution_clock::now();
    VstepProjectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    advect(_fieldX, _prevFieldX, _prevFieldX, _prevFieldY, 1);
    advect(_fieldY, _prevFieldY, _prevFieldX, _prevFieldY, 2);
    end = std::chrono::high_resolution_clock::now();
    VstepAdvectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    project(_fieldX, _fieldY);
    end = std::chrono::high_resolution_clock::now();
    VstepProjectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
}

void Fluids::sStep()
{
    auto start = std::chrono::high_resolution_clock::now();
    diffuse(_prevSubstance, _substance, 0, _laplacianDiffuse);
    auto end = std::chrono::high_resolution_clock::now();
    SstepDiffuseTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    advect(_substance, _prevSubstance, _fieldX, _fieldY, 0);
    end = std::chrono::high_resolution_clock::now();
    SstepAdvectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
}

void Fluids::diffuse(Field<double,std::uint16_t>& F, const Field<double,std::uint16_t>& Fprev, const std::uint8_t b, const Laplacian& A)
{
    Eigen::VectorXd x(A.diag.size());
    ConjugateGradient(A, x, Fprev.vec());
    F.setFromVec(x);
    setBnd(F, b);
}

void Fluids::advect(Field<double,std::uint16_t>& F, const Field<double,std::uint16_t>& Fprev, const Field<double,std::uint16_t>& X, const Field<double,std::uint16_t>& Y, const std::uint8_t b) const
{
	const double dt = _dt * _N;

    for (std::uint16_t j = 0; j < _N; ++j)
    {
        for (std::uint16_t i = 0; i < _N; ++i)
        {
            double xvel = 0;
            double yvel = 0;

            if (b != 0)
            {
                if (b == 1 && i == 0)
                {
                    yvel = 0.5*(Y(i,j)+Y(i,j+1));
                }
                else if (b == 1 && i == _N-1)
                {
                    yvel = 0.5*(Y(i-1,j)+Y(i-1,j+1));
                }
                else
                {
                    yvel = b == 2 ? Y(i,j) : 0.25*(Y(i,j)+Y(i,j+1)+Y(i-1,j)+Y(i-1,j+1));
                }

                if (b == 2 && j == 0)
                {
                    xvel = 0.5*(X(i,j)+X(i+1,j));
                }
                else if (b == 2 && j == _N-1)
                {
                    xvel = 0.5*(X(i,j-1)+X(i+1,j-1));
                }
                else
                {
                    xvel = b == 1 ? X(i,j) : 0.25*(X(i,j)+X(i+1,j)+X(i,j-1)+X(i+1,j-1));
                }
            }
            else
            {
                xvel = 0.5*(X(i,j)+X(i+1,j));
                yvel = 0.5*(Y(i,j)+Y(i,j+1));
            }

            const double posx = static_cast<double>(i);
            const double posy = static_cast<double>(j);
            const double x = std::clamp((posx-dt*xvel), 0.0, static_cast<double>(F.x()-1));
            const double y = std::clamp((posy-dt*yvel), 0.0, static_cast<double>(F.y()-1));

            const std::uint16_t i0 = static_cast<std::uint16_t>(x);
            const std::uint16_t i1 = i0 + 1;
            const std::uint16_t j0 = static_cast<std::uint16_t>(y);
            const std::uint16_t j1 = j0 + 1;

            const double s1 = x - i0;
            const double s0 = 1.0 - s1;
            const double t1 = y - j0;
            const double t0 = 1.0 - t1;

            const double vA = Fprev(i0,j0);
            const double vB = Fprev(i0,j1);
            const double vC = Fprev(i1,j0);
            const double vD = Fprev(i1,j1);

            F(i,j) = s0*(t0*vA+t1*vB)+s1*(t0*vC+t1*vD);
        }
    }
	setBnd(F, b);
}

void Fluids::applyPreconditioner(const Eigen::VectorXd& r, const Laplacian& A, Eigen::VectorXd& z, const Solver solver) const
{
    if (solver == CG)
    {
        z = r;
        return;
    }
    const std::uint64_t N2 = _N*_N; 
    const std::uint64_t N3 = _N*_N;

    // Solve Lq = r
    Eigen::VectorXd q = Eigen::VectorXd::Zero(z.size());
    for (std::int64_t n = 0; n < z.size(); ++n)
    {
        const std::uint64_t m = n % N2;
        const std::uint64_t i = m % _N;
        const std::uint64_t j = m / _N;
        const std::uint64_t k = n / N2;

        const std::uint64_t indmi = (i-1)+j*_N+k*N2;
        const std::uint64_t indmj = i+(j-1)*_N+k*N2;
        const std::uint64_t indmk = i+j*_N+(k-1)*N2;

        const double a = i > 0 ? A.plusi.coeff(indmi) * A.precon.coeff(indmi) * q.coeff(indmi) : 0;
        const double b = j > 0 ? A.plusj.coeff(indmj) * A.precon.coeff(indmj) * q.coeff(indmj) : 0;
        const double c = k > 0 ? A.plusk.coeff(indmk) * A.precon.coeff(indmk) * q.coeff(indmk) : 0;

        const double t = r.coeff(n) - a - b - c;
        q.coeffRef(n) = t * A.precon.coeff(n);
    }

    // Solve L'z = q
    for (std::int64_t n = z.size()-1; n >= 0; --n)
    {
        const std::uint64_t m = n % N2;
        const std::uint64_t i = m % _N;
        const std::uint64_t j = m / _N;
        const std::uint64_t k = n / N2;

        const std::uint64_t indpi = (i+1)+j*_N+k*N2;
        const std::uint64_t indpj = i+(j+1)*_N+k*N2;
        const std::uint64_t indpk = i+j*_N+(k+1)*N2;

        const double a = indpi < N3 ? z.coeff(indpi) : 0;
        const double b = indpj < N3 ? z.coeff(indpj) : 0;
        const double c = indpk < N3 ? z.coeff(indpk) : 0;

        const double prec = A.precon.coeff(n);
        const double t = q.coeff(n) - A.plusi.coeff(n) * prec * a
                                    - A.plusj.coeff(n) * prec * b
                                    - A.plusk.coeff(n) * prec * c;
        z.coeffRef(n) = t * prec;
    }
}

void Fluids::ConjugateGradient(const Laplacian& A, Eigen::VectorXd& x, const Eigen::VectorXd& b)
{
    const std::uint64_t diagSize = A.diag.size();

#ifdef DEBUG
    Eigen::SimplicialLLT<Eigen::SparseMatrix<double>> lltOfA(A.A);
    if(lltOfA.info() == Eigen::NumericalIssue)
    {
        ERROR("DEBUG: Numerical Issue on the A matrix");
    }
#endif

    // Solving Ap = b
    Eigen::VectorXd r = b;
    if (r.isZero(0))
    {
        x = b;
        return;
    }
    x = Eigen::VectorXd::Zero(diagSize);
    
    Eigen::VectorXd z = x;
    applyPreconditioner(r, A, z, _solverType);
    Eigen::VectorXd s = z;
    double sig = z.dot(r);

    for (std::uint64_t i = 0; i < static_cast<std::uint64_t>(b.size()); ++i)
    {
        z = A.A * s;
        const double alpha = sig / s.dot(z);
        x = x + alpha * s;
        r = r - alpha * z;
        if (r.lpNorm<Eigen::Infinity>() < 10e-5)
        {
            break;
        }
        applyPreconditioner(r, A, z, _solverType);
        const double signew = z.dot(r);
        const double beta = signew / sig;
        s = z + beta * s;
        sig = signew;
    }
}

inline double Fluids::getPressure(const std::uint16_t i, const std::uint16_t j, const std::uint16_t i2, const std::uint16_t j2)
{
    return _p(i,j);
    if (_implicit(i,j) > 0 && !((_implicit(i,j) >= 0) ^ (_implicit(i2,j2) < 0)))
    {
        double omega = std::clamp(_implicit(i,j)/(_implicit(i,j)-_implicit(i2,j2)), 10e-7, 1.0);
        double res = - (( 1.0 - omega ) / omega) * _implicit(i2,j2);
        std::cout << omega << std::endl;
        std::cout << res << std::endl;
        return res;
    }
    return _p(i,j);
    std::cout << _implicit(i,j) << " " << _implicit(i2,j2) << std::endl;;
    double omega = std::clamp(_implicit(i,j)/(_implicit(i,j)-_implicit(i2,j2)), 10e-7, 1.0);
    std::cout << omega << std::endl;
    double res = - (( 1 - omega ) / omega) * _implicit(i,j);
    if (omega < 1)
    {
        std::cout << res << std::endl;
    }
    return - (( 1 - omega ) / omega) * _implicit(i,j);
}

void Fluids::project(Field<double,std::uint16_t>& X, Field<double,std::uint16_t>& Y)
{
    const double h = 1.0/_N;

    auto test = _laplacianProject;

    std::vector<std::uint64_t> liquidIdx;
    std::vector<double> liquidDiv;
    std::vector<Eigen::Triplet<double>> tripletListA;

    /*
    std::cout << _implicit << std::endl;
    std::cout << X << std::endl;
    */

    std::uint64_t label = 1;
    for (std::uint16_t j = 0; j < _N; ++j)
    {
        for (std::uint16_t i = 0; i < _N; ++i)
        {
            if (_implicit(i,j) <= 0)
            {
                liquidIdx.emplace_back(_implicit.idx(i,j));
                liquidDiv.emplace_back(
                    - 0.5 * (
                        (X(i+1,j)-X(i,j))+
                        (Y(i,j+1)-Y(i,j))
                    )*h);
                _implicit.label(i,j) = label;
                label++;
            }
        }
    }

    if (liquidIdx.size() > 0)
    {
        for (auto idx : liquidIdx)
        {
            const std::uint64_t m = idx % (_N*_N);
            const std::uint64_t i = m % _N;
            const std::uint64_t j = m / _N;
           
            std::uint64_t label = _implicit.label(i,j);
            if (i+1 < _N && _implicit.label(i+1,j) > 0)
            {
                tripletListA.emplace_back(Eigen::Triplet<double>(label-1,_implicit.label(i+1,j)-1,-1));
            }
            if (i != 0 && _implicit.label(i-1,j) > 0)
            {
                tripletListA.emplace_back(Eigen::Triplet<double>(label-1,_implicit.label(i-1,j)-1,-1));
            }
            if (j+1 < _N && _implicit(i,j+1) <= 0)
            {
                tripletListA.emplace_back(Eigen::Triplet<double>(label-1,_implicit.label(i,j+1)-1,-1));
            }
            if (j != 0 && _implicit(i,j-1) <= 0)
            {
                tripletListA.emplace_back(Eigen::Triplet<double>(label-1,_implicit.label(i,j-1)-1,-1));
            }
            tripletListA.emplace_back(Eigen::Triplet<double>(label-1, label-1, 4));
        }

        Eigen::SparseMatrix<double> A = Eigen::SparseMatrix<double>(liquidIdx.size(), liquidIdx.size());

        A.setFromTriplets(tripletListA.begin(), tripletListA.end());
        Laplacian laplacian {};
        laplacian.A = A;
        setAMatrices(laplacian);
        setPrecon(laplacian);
        
        _p.reset();

        setBnd(_div, 0);

        Eigen::VectorXd x(laplacian.diag.size());
        Eigen::VectorXd b(laplacian.diag.size());
        for (std::uint64_t it = 0; it < liquidDiv.size(); ++it)
        {
            b.coeffRef(it) = liquidDiv[it];
        }

        /*
        std::cout << laplacian.A << std::endl;
        std::cout << b << std::endl;
        */

        ConjugateGradient(laplacian, x, b);

        // TODO optimize this
        std::uint64_t it = 0;
        for (std::uint16_t j = 0; j < _N; ++j)
        {
            for (std::uint16_t i = 0; i < _N; ++i)
            {
                if (_implicit(i,j) <= 0)
                {
                    _p(i,j) = x.coeffRef(it);
                    it++;
                }
            }
        }

        setBnd(_p, 0);
        for (std::uint16_t j = 1; j < _N; ++j)
        {
            for (std::uint16_t i = 1; i < _N; ++i)
            {
                if (_implicit(i,j) <= 0 || _implicit(i-1,j) <= 0)
                {
                    X(i,j) -= 0.5*_N*(_p(i,j)-_p(i-1,j)); // probably not i-1 and start from i = 0 not i = 1 ...
                    X.set(i,j,true);
                }
                if (_implicit(i,j) <= 0 || _implicit(i,j-1) <= 0)
                {
                    Y(i,j) -= 0.5*_N*(_p(i,j)-_p(i,j-1)); // idem
                    Y.set(i,j,true);
                }
            }
        }
        //std::cout << X << std::endl;
        
        //std::cout << _p << std::endl;

        setBnd(X, 1);
        setBnd(Y, 2);
    }
    _implicit.resetLabels();

}

void Fluids::setBnd(Field<double,std::uint16_t>& F, const std::uint8_t b) const
{
    if (b == 1)
    {
        for (std::uint16_t i = 0; i < _N+1; ++i)
        {
            if (i < _N)
            {
                F(0,i) = -F(1,i);
                F(_N,i) = -F(_N-1,i);
            }
            F(i,0) = F(i,1);
            F(i,_N-1) = F(i,_N-2);
        }
    }
    else if (b == 2)
    {
        for (std::uint16_t i = 0; i < _N+1; ++i)
        {
            F(0,i) = F(1,i);
            F(_N-1,i)= F(_N-2,i);
            if (i < _N)
            {
                F(i,0) = -F(i,1);
                F(i,_N) = -F(i,_N-1);
            }
        }
    }
}

void Fluids::updateTexture()
{
    std::uint64_t it = 0;
    //double sum = 0;
    for (std::uint16_t i = 0; i < _N; ++i)
    {
        for (std::uint16_t j = 0; j < _N; ++j)
        {
            /*
            double value = _substance(i,j);
		    texture[it*3] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
		    texture[it*3+1] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
		    texture[it*3+2] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
            sum += value;
            */
            const double implicit = _implicit(i,j);
            _texture[it*3] = 0;
            _texture[it*3+1] = 0;
            _texture[it*3+2] = 0;
            const std::uint64_t p = 50;
            if (implicit < 0)
            {
                _texture[it*3+2] = std::clamp(-implicit*p, 0.0, 255.0);
            }
            else
            {
                _texture[it*3+2] = std::clamp(implicit*p, 0.0, 255.0);
                _texture[it*3+1] = std::clamp(implicit*p, 0.0, 255.0);
                _texture[it*3+0] = std::clamp(implicit*p, 0.0, 255.0);
            }
            /*
            const std::uint64_t m = i % N32;
            const std::uint64_t x = m % (N + 2);
            const std::uint64_t y = m / (N + 2);
            double gO = gradLength(fluid, fluid.implicitFunctionFieldPrev, x, y);
            texture[i*3] = std::clamp(gO*128, 0.0, 255.0);
            texture[i*3+1] = 0;
            texture[i*3+2] = 0;
            */
            it++;
       }
	}
}

const std::vector<std::uint8_t>& Fluids::texture() const
{
    return _texture;
}

void Fluids::writeVolumeFile([[maybe_unused]] const std::uint64_t iteration)
{
    /*
    const std::uint64_t N = fluid.N;
    const std::uint64_t N2 = N*N;
    const std::uint64_t N22 = (N+2)*(N+2);
    const std::uint64_t N3 = N*N*N;
    std::string path = "result/";
    path += std::to_string(iteration);
    path += ".vol";
    std::ofstream file (path, std::ios::binary | std::ofstream::trunc);
    file << 'V';
    file << 'O';
    file << 'L';
    write(file, (uint8_t)3); // Version
    write(file, (int32_t)1); // Type
    std::uint16_t n = fluid.N;
    write(file, (int32_t)n);
    write(file, (int32_t)n);
    write(file, (int32_t)n);
    write(file, (int32_t)1); // Nb channels
    float xmin = -0.5f;
    float ymin = -0.5f;
    float zmin = -0.5f;
    float xmax = 0.5f;
    float ymax = 0.5f;
    float zmax = 0.5f;
    write(file, xmin);
    write(file, ymin);
    write(file, zmin);
    write(file, xmax);
    write(file, ymax);
    write(file, zmax);

    for (std::uint64_t n = 0; n < N3; ++n)
    {
        const std::uint32_t m = n % N2;
        const std::uint16_t i = m % N + 1;
        const std::uint16_t j = m / N + 1;
        const std::uint16_t k = n / N2 + 1;
        const std::uint64_t ind = i+j*(N+2)+k*N22;
        float value = std::clamp(_substance(ind), 0.0, 255.0);
        write(file, value);
    }

    file.close();
    */
}

void Fluids::initCG()
{
    INFO("Start to initialize Conjugate Gradient matrices.");
    const std::uint64_t N3 = _N*_N;
    const std::uint64_t N31 = (_N+1)*_N;

    const double visc = _dt * _viscosity * _N;
    const double diff = _dt * _diffusion * _N;

    const std::uint8_t minus = 0;
    _laplacianProject.minus = minus;

    Eigen::SparseMatrix<double> AProject    = Eigen::SparseMatrix<double>(N3-minus, N3-minus);
    Eigen::SparseMatrix<double> ADiffuse    = Eigen::SparseMatrix<double>(N3, N3);
    Eigen::SparseMatrix<double> AViscosityX = Eigen::SparseMatrix<double>(N31, N31);
    Eigen::SparseMatrix<double> AViscosityY = Eigen::SparseMatrix<double>(N31, N31);

    std::vector<Eigen::Triplet<double>> tripletListProject;
    std::vector<Eigen::Triplet<double>> tripletListViscosityX;
    std::vector<Eigen::Triplet<double>> tripletListViscosityY;
    std::vector<Eigen::Triplet<double>> tripletListDiffuse;

    for(std::uint64_t i = 0; i < N31; ++i)
    {
        if (i+1 < N31 && static_cast<std::uint16_t>(i%_N) != _N-1)
        {
            tripletListViscosityY.emplace_back(Eigen::Triplet<double>(i, i+1, -visc));
        }
        if (i+1 < N31 && static_cast<std::uint16_t>(i%(_N+1)) != _N)
        {
            tripletListViscosityX.emplace_back(Eigen::Triplet<double>(i, i+1, -visc));
        }
        if (i+_N < N31 && (i+_N)%((_N+1)*_N) >= _N)
        {
            tripletListViscosityY.emplace_back(Eigen::Triplet<double>(i, i+_N, -visc));
        }
        if (i+_N+1 < N31 && (i+_N+1)%((_N+1)*_N) >= _N)
        {
            tripletListViscosityX.emplace_back(Eigen::Triplet<double>(i, i+_N+1, -visc));
        }
        if (i < N3)
        {
            if (i+_N < N3-minus && (i+_N)%(_N*_N) >= _N)
            {
                tripletListProject.emplace_back(Eigen::Triplet<double>(i, i+_N, -1));
            }
            if (i+_N < N3 && (i+_N)%(_N*_N) >= _N)
            {
                tripletListDiffuse.emplace_back(Eigen::Triplet<double>(i, i+_N, -diff));
            }
            if (i+1 < N3-minus && static_cast<std::uint16_t>(i%_N) != _N-1)
            {
                tripletListProject.emplace_back(Eigen::Triplet<double>(i, i+1, -1));
            }
            if (i+1 < N3 && static_cast<std::uint16_t>(i%_N) != _N-1)
            {
                tripletListDiffuse.emplace_back(Eigen::Triplet<double>(i, i+1, -diff));
            }
            if (i < N3-minus)
            {
                tripletListProject.emplace_back(Eigen::Triplet<double>(i, i, 4));
            }
            tripletListDiffuse.emplace_back(Eigen::Triplet<double>(i, i, 1+4*diff));
        }
        // TODO UNCOMMENT WHEN 3D
        /*
        if (i+N*N < N3)
        {
            tripletListViscosity.emplace_back(Eigen::Triplet<double>(i, i+N*N, -visc));
            tripletListDiffuse.emplace_back(Eigen::Triplet<double>(i, i+N*N, -diff));
        }
        if (i+N*N < N3-minus)
        {
            tripletListProject.emplace_back(Eigen::Triplet<double>(i, i+N*N, -1));
        }
        */
        // TODO CHANGE 4 TO 6 WHEN 3D
        tripletListViscosityX.emplace_back(Eigen::Triplet<double>(i, i, 1+4*visc));
        tripletListViscosityY.emplace_back(Eigen::Triplet<double>(i, i, 1+4*visc));
    }
    AProject.setFromTriplets(tripletListProject.begin(), tripletListProject.end());
    AViscosityX.setFromTriplets(tripletListViscosityX.begin(), tripletListViscosityX.end());
    AViscosityY.setFromTriplets(tripletListViscosityY.begin(), tripletListViscosityY.end());
    ADiffuse.setFromTriplets(tripletListDiffuse.begin(), tripletListDiffuse.end());
    AProject = AProject+Eigen::SparseMatrix<double>(AProject.transpose());
    AViscosityX = AViscosityX+Eigen::SparseMatrix<double>(AViscosityX.transpose());
    AViscosityY = AViscosityY+Eigen::SparseMatrix<double>(AViscosityY.transpose());
    ADiffuse = ADiffuse+Eigen::SparseMatrix<double>(ADiffuse.transpose());
    for(std::uint64_t i = 0; i < N31; ++i)
    {
        if (i < N3-minus)
        {
            AProject.coeffRef(i, i) *= 0.5;
        }
        AViscosityX.coeffRef(i, i) *= 0.5;
        AViscosityY.coeffRef(i, i) *= 0.5;
        if (i < N3)
        {
            ADiffuse.coeffRef(i, i) *= 0.5;
        }
    }

    _laplacianViscosityX.A = AViscosityX;
    setAMatrices(_laplacianViscosityX);
    setPrecon(_laplacianViscosityX);

    _laplacianViscosityY.A = AViscosityY;
    setAMatrices(_laplacianViscosityY);
    setPrecon(_laplacianViscosityY);

    _laplacianProject.A = AProject;
    setAMatrices(_laplacianProject);
    setPrecon(_laplacianProject);

    _laplacianDiffuse.A = ADiffuse;
    setAMatrices(_laplacianDiffuse);
    setPrecon(_laplacianDiffuse);

    INFO("Conjugate Gradient matrices are initialized.");
}

void Fluids::setPrecon(Laplacian& A) const
{
    const std::uint64_t N3 = A.A.rows();
    const std::uint64_t N2 = A.A.rows(); // TEMP
    A.precon = Eigen::VectorXd::Zero(N3);
    #pragma omp parallel for
    for (std::uint32_t n = 0; n < N3; ++n)
    {
        const std::uint32_t m = n % N2;
        const std::uint16_t i = m % _N;
        const std::uint16_t j = m / _N;
        const std::uint16_t k = n / N2;
        const std::uint32_t indmi = (i-1)+j*_N+k*N2;
        const std::uint32_t indmj = i+(j-1)*_N+k*N2;
        const std::uint32_t indmk = i+j*_N+(k-1)*N2;

        double a = 0;
        double b = 0;
        double c = 0;

        double i0 = 0;
        double i1 = 0;
        double i2 = 0;
        double i3 = 0;
        double j0 = 0;
        double j1 = 0;
        double j2 = 0;
        double j3 = 0;
        double k0 = 0;
        double k1 = 0;
        double k2 = 0;
        double k3 = 0;

        if (i > 0)
        {
            a = std::pow(A.plusi.coeff(indmi) * A.precon.coeff(indmi), 2);
            i0 = A.plusi.coeff(indmi);
            i1 = A.plusj.coeff(indmi);
            i2 = A.plusk.coeff(indmi);
            i3 = std::pow(A.precon.coeff(indmi), 2);
        }
        if (j > 0)
        {
            b = std::pow(A.plusj.coeff(indmj) * A.precon.coeff(indmj), 2);
            j0 = A.plusj.coeff(indmj);
            j1 = A.plusi.coeff(indmj);
            j2 = A.plusk.coeff(indmj);
            j3 = std::pow(A.precon.coeff(indmj), 2);
        }
        if (k > 0)
        {
            c = std::pow(A.plusk.coeff(indmk) * A.precon.coeff(indmk), 2);
            k0 = A.plusk.coeff(indmk);
            k1 = A.plusi.coeff(indmk);
            k2 = A.plusj.coeff(indmk);
            k3 = std::pow(A.precon.coeff(indmk), 2);
        }

        double e = A.diag.coeff(n) - a - b - c 
            - 0.97 * (
                    i0 * (i1 + i2) * i3
                +   j0 * (j1 + j2) * j3
                +   k0 * (k1 + k2) * k3
            );

        if (e < 0.25 * A.diag.coeff(n))
        {
            e = A.diag.coeff(n);
        }
        A.precon.coeffRef(n) = 1/std::sqrt(e);
    }
}

void Fluids::setAMatrices(Laplacian& laplacian) const
{
    const std::uint64_t N3 = laplacian.A.rows();
    const std::uint64_t N2 = laplacian.A.rows(); // TEMP
    laplacian.diag = Eigen::VectorXd::Zero(N3);
    laplacian.plusi = Eigen::VectorXd::Zero(N3);
    laplacian.plusj = Eigen::VectorXd::Zero(N3);
    laplacian.plusk = Eigen::VectorXd::Zero(N3);

    #pragma omp parallel for
    for (std::uint32_t n = 0; n < N3; ++n)
    {
        const std::uint32_t m = n % N2;
        const std::uint16_t i = m % _N;
        const std::uint16_t j = m / _N;
        const std::uint16_t k = n / N2;
        laplacian.diag.coeffRef(n) = laplacian.A.coeff(n, n);
        if (n+1 < N3 && i+1 < _N)
        {
            laplacian.plusi[n] = laplacian.A.coeff(n, (i+1)+j*_N+k*N2);
        }
        if (n+_N < N3)
        {
            laplacian.plusj[n] = laplacian.A.coeff(n, i+(j+1)*_N+k*N2);
        }
        if (n+N2 < N3)
        {
            laplacian.plusk[n] = laplacian.A.coeff(n, i+j*_N+(k+1)*N2);
        }
    }
}

const std::vector<double>& Fluids::X() const
{
    return _fieldX.data();
}

const std::vector<double>& Fluids::Y() const
{
    return _fieldY.data();
}

const std::uint16_t& Fluids::N() const
{
    return _N;
}