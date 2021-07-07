#include "Fluids.h"
#include <Eigen/src/Core/Matrix.h>
#include <Eigen/src/Core/util/Constants.h>
#include <Eigen/src/SparseCore/SparseMatrix.h>
#include <Eigen/src/SparseLU/SparseLU.h>
#include <cmath>
#include <cstdint>
#include <iomanip>

void Fluids::init(std::shared_ptr<Renderer> renderer)
{
	_renderer = renderer;
    std::srand(std::time(nullptr));
}

void Fluids::update([[maybe_unused]] std::uint64_t iteration)
{
    if (mEntities.size() > 1)
    {
        ERROR("Multiple fluids not supported.");
    }
	for (auto const& entity : mEntities)
	{
		auto& fluid = gCoordinator.GetComponent<Fluid3D>(entity);
        
        /* Testings */
		int N = fluid.N/2;

        float p = 256;
        float z = 16;

        //z *= -1;

        /*
        fluid.substanceField[fluid.IX(N-42, N, 0, 0)]=      p;
        fluid.velocityFieldX[fluid.IX(N-42+1, N, 0, 1)] =   z;
        fluid.substanceField[fluid.IX(N+42, N, 0, 0)]=      p;
        fluid.velocityFieldX[fluid.IX(N+42, N, 0, 1)] =     -z;

        fluid.substanceField[fluid.IX(N, N-42, 0, 0)]=      p;
        fluid.velocityFieldY[fluid.IX(N, N-42+1, 0, 2)] =   z;
        fluid.substanceField[fluid.IX(N, N+42, 0, 0)]=      p;
        fluid.velocityFieldY[fluid.IX(N, N+42, 0, 2)] =     -z;
        */

        fluid.substanceField[fluid.IX(N-(N/2), N-(N/2), 0, 0)] = p;
        fluid.velocityFieldX[fluid.IX(N-(N/2), N-(N/2), 0, 1)] = z;
        fluid.velocityFieldY[fluid.IX(N-(N/2), N-(N/2), 0, 2)] = z;
        fluid.velocityFieldX[fluid.IX(N-(N/2)+1, N-(N/2), 0, 1)] = z;
        fluid.velocityFieldY[fluid.IX(N-(N/2), N-(N/2)+1, 0, 2)] = z;

        fluid.substanceField[fluid.IX(N+(N/2), N+(N/2), 0, 0)] = p;
        fluid.velocityFieldX[fluid.IX(N+(N/2), N+(N/2), 0, 1)] = -z;
        fluid.velocityFieldY[fluid.IX(N+(N/2), N+(N/2), 0, 2)] = -z;
        fluid.velocityFieldX[fluid.IX(N+(N/2)+1, N+(N/2), 0, 1)] = -z;
        fluid.velocityFieldY[fluid.IX(N+(N/2), N+(N/2)+1, 0, 2)] = -z;

        fluid.substanceField[fluid.IX(N+(N/2), N-(N/2), 0, 0)] = p;
        fluid.velocityFieldX[fluid.IX(N+(N/2), N-(N/2), 0, 1)] = -z;
        fluid.velocityFieldY[fluid.IX(N+(N/2), N-(N/2), 0, 2)] = z;
        fluid.velocityFieldX[fluid.IX(N+(N/2)+1, N-(N/2), 0, 1)] = -z;
        fluid.velocityFieldY[fluid.IX(N+(N/2), N-(N/2)+1, 0, 2)] = z;

        fluid.substanceField[fluid.IX(N-(N/2), N+(N/2), 0, 0)] = p;
        fluid.velocityFieldX[fluid.IX(N-(N/2), N+(N/2), 0, 1)] = z;
        fluid.velocityFieldY[fluid.IX(N-(N/2), N+(N/2), 0, 2)] = -z;
        fluid.velocityFieldX[fluid.IX(N-(N/2)+1, N+(N/2), 0, 1)] = z;
        fluid.velocityFieldY[fluid.IX(N-(N/2), N+(N/2)+1, 0, 2)] = -z;


        /* End Testings */

        auto start = std::chrono::high_resolution_clock::now();
        Vstep(fluid);
        auto end = std::chrono::high_resolution_clock::now();
        VstepTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
        start = std::chrono::high_resolution_clock::now();
        Sstep(fluid);
        end = std::chrono::high_resolution_clock::now();
        SstepTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
        updateRender(fluid);

        _renderer->updateDynamicLine(fluid.N, fluid.velocityFieldX, fluid.velocityFieldY);

        /*
        std::cout << std::fixed << std::setprecision(2) << std::endl;
        std::cout << std::endl << "=== X ===" << std::endl;
        for (std::uint64_t j = 0; j < fluid.N; ++j)
        {
            for (std::uint64_t i = 0; i < fluid.N+1; ++i)
            {
                if (fluid.velocityFieldX[fluid.IX(i,j,0,1)] >= 0)
                    std::cout << " ";
                std::cout << fluid.velocityFieldX[fluid.IX(i,j,0,1)] << " ";
            }
            std::cout << std::endl;
        }
        std::cout << std::endl << "=== Y ===" << std::endl;
        for (std::uint64_t j = 0; j < fluid.N+1; ++j)
        {
            for (std::uint64_t i = 0; i < fluid.N; ++i)
            {
                if (fluid.velocityFieldY[fluid.IX(i,j,0,2)] >= 0)
                    std::cout << " ";
                std::cout << fluid.velocityFieldY[fluid.IX(i,j,0,2)] << " ";
            }
            std::cout << std::endl;
        }
        */

	}
}

void Fluids::Vstep(Fluid3D& fluid)
{
    /*
	addSource(fluid, fluid.velocityFieldX, fluid.velocityFieldPrevX);
	addSource(fluid, fluid.velocityFieldY, fluid.velocityFieldPrevY);
	addSource(fluid, fluid.velocityFieldZ, fluid.velocityFieldPrevZ);
    */

    auto start = std::chrono::high_resolution_clock::now();
    diffuse(fluid, fluid.velocityFieldPrevX, fluid.velocityFieldX, 1, fluid.laplacianViscosityX);
    diffuse(fluid, fluid.velocityFieldPrevY, fluid.velocityFieldY, 2, fluid.laplacianViscosityY);
    auto end = std::chrono::high_resolution_clock::now();
    VstepDiffuseTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    project(fluid, fluid.velocityFieldPrevX, fluid.velocityFieldPrevY, fluid.velocityFieldPrevZ, fluid.velocityFieldX, fluid.velocityFieldY);
    end = std::chrono::high_resolution_clock::now();
    VstepProjectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    advect(fluid, fluid.velocityFieldX, fluid.velocityFieldPrevX, fluid.velocityFieldPrevX, fluid.velocityFieldPrevY, fluid.velocityFieldPrevZ, 1);
    advect(fluid, fluid.velocityFieldY, fluid.velocityFieldPrevY, fluid.velocityFieldPrevX, fluid.velocityFieldPrevY, fluid.velocityFieldPrevZ, 2);
    end = std::chrono::high_resolution_clock::now();
    VstepAdvectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    project(fluid, fluid.velocityFieldX, fluid.velocityFieldY, fluid.velocityFieldZ, fluid.velocityFieldPrevX, fluid.velocityFieldPrevY);
    end = std::chrono::high_resolution_clock::now();
    VstepProjectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
}

void Fluids::Sstep(Fluid3D& fluid)
{
	//addSource(fluid, fluid.substanceField, fluid.substanceFieldPrev);
    auto start = std::chrono::high_resolution_clock::now();
    diffuse(fluid, fluid.substanceFieldPrev, fluid.substanceField, 0, fluid.laplacianDiffuse);
    auto end = std::chrono::high_resolution_clock::now();
    SstepDiffuseTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

    start = std::chrono::high_resolution_clock::now();
    advect(fluid, fluid.substanceField, fluid.substanceFieldPrev, fluid.velocityFieldX, fluid.velocityFieldY, fluid.velocityFieldZ, 0);
    end = std::chrono::high_resolution_clock::now();
    SstepAdvectTime += std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();
}

void Fluids::addSource(const Fluid3D& fluid, std::vector<double>& X, const std::vector<double>& S) const
{
    const std::uint64_t N = fluid.N; 
    const std::uint64_t N32 = (N+2)*(N+2)*(N+2); 
	for (std::uint64_t i = 0; i < N32; ++i)
	{
		X[i] += fluid.dt * S[i];
	}
}

void Fluids::diffuse(const Fluid3D& fluid, std::vector<double>& X, const std::vector<double>& Xprev, const std::uint8_t b, const Laplacian& A)
{
    if (fluid.solver == GAUSS_SEIDEL)
    {
        //const double a = b == 0 ? fluid.dt * fluid.diffusion * fluid.N : fluid.dt * fluid.viscosity * fluid.N;
        //GaussSeidelRelaxationLinSolve(fluid, X, Xprev, a, fluid.is2D ? 1+4*a : 1+6*a, b);
        ERROR("Not implemented");
    }
    else
    {
        ConjugateGradientMethodLinSolve(fluid, X, Xprev, b, A);
    }
}

void Fluids::advect(Fluid3D& fluid, std::vector<double>& D, const std::vector<double>& Dprev, const std::vector<double>& X, const std::vector<double>& Y, [[maybe_unused]] const std::vector<double>& Z, const std::uint8_t b) const
{
    const std::uint16_t N = fluid.N; 
	const double dt = fluid.dt * N;

    for (std::uint16_t j = 0; j < fluid.N; ++j)
    {
        for (std::uint16_t i = 0; i < fluid.N; ++i)
        {
            double xvel = 0;
            double yvel = 0;

            if (b != 0)
            {
                if (b == 1 && i == 0)
                {
                    yvel = 0.5*(Y[fluid.IX(i,j,0,2)]+Y[fluid.IX(i,j+1,0,2)]);
                }
                else if (b == 1 && i == fluid.N-1)
                {
                    yvel = 0.5*(Y[fluid.IX(i-1,j,0,2)]+Y[fluid.IX(i-1,j+1,0,2)]);
                }
                else
                {
                    yvel = b == 2 ? Y[fluid.IX(i,j,0,2)] : 0.25*(Y[fluid.IX(i,j,0,2)]+Y[fluid.IX(i,j+1,0,2)]+Y[fluid.IX(i-1,j,0,2)]+Y[fluid.IX(i-1,j+1,0,2)]);
                }

                if (b == 2 && j == 0)
                {
                    xvel = 0.5*(X[fluid.IX(i,j,0,1)]+X[fluid.IX(i+1,j,0,1)]);
                }
                else if (b == 2 && j == fluid.N-1)
                {
                    xvel = 0.5*(X[fluid.IX(i,j-1,0,1)]+X[fluid.IX(i+1,j-1,0,1)]);
                }
                else
                {
                    xvel = b == 1 ? X[fluid.IX(i,j,0,1)] : 0.25*(X[fluid.IX(i,j,0,1)]+X[fluid.IX(i+1,j,0,1)]+X[fluid.IX(i,j-1,0,1)]+X[fluid.IX(i+1,j-1,0,1)]);
                }
            }
            else
            {
                xvel = 0.5*(X[fluid.IX(i,j,0,1)]+X[fluid.IX(i+1,j,0,1)]);
                yvel = 0.5*(Y[fluid.IX(i,j,0,2)]+Y[fluid.IX(i,j+1,0,2)]);
            }


            double posx = static_cast<double>(i);
            double posy = static_cast<double>(j);
            double x = std::clamp((posx-dt*xvel), 0.0, static_cast<double>(N-1+(b==1?1:0))-0.0);
            double y = std::clamp((posy-dt*yvel), 0.0, static_cast<double>(N-1+(b==2?1:0))-0.0);

            std::uint16_t i0 = static_cast<std::uint16_t>(x);
            std::uint16_t i1 = i0 + 1;
            std::uint16_t j0 = static_cast<std::uint16_t>(y);
            std::uint16_t j1 = j0 + 1;

            double s1 = x - i0;
            double s0 = 1.0 - s1;
            double t1 = y - j0;
            double t0 = 1.0 - t1;

            double vA = Dprev[fluid.IX(i0,j0,0,b)];
            double vB = Dprev[fluid.IX(i0,j1,0,b)];
            double vC = Dprev[fluid.IX(i1,j0,0,b)];
            double vD = Dprev[fluid.IX(i1,j1,0,b)];

            D[fluid.IX(i,j,0,b)] = s0*(t0*vA+t1*vB)+s1*(t0*vC+t1*vD);
        }
    }
	setBnd(fluid, D, b);
}

void Fluids::applyPreconditioner(const std::uint64_t N, const Eigen::VectorXd& r, const Laplacian& A, Eigen::VectorXd& z, const Solver solver) const
{
    if (solver == CG)
    {
        z = r;
        return;
    }
    const std::uint64_t N2 = N*N; 
    const std::uint64_t N3 = N*N;

    // Solve Lq = r
    Eigen::VectorXd q = Eigen::VectorXd::Zero(z.size());
    for (std::int64_t n = 0; n < z.size(); ++n)
    {
        const std::uint64_t m = n % N2;
        const std::uint64_t i = m % N;
        const std::uint64_t j = m / N;
        const std::uint64_t k = n / N2;

        const std::uint64_t indmi = (i-1)+j*N+k*N2;
        const std::uint64_t indmj = i+(j-1)*N+k*N2;
        const std::uint64_t indmk = i+j*N+(k-1)*N2;

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
        const std::uint64_t i = m % N;
        const std::uint64_t j = m / N;
        const std::uint64_t k = n / N2;

        const std::uint64_t indpi = (i+1)+j*N+k*N2;
        const std::uint64_t indpj = i+(j+1)*N+k*N2;
        const std::uint64_t indpk = i+j*N+(k+1)*N2;

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

void Fluids::ConjugateGradientMethodLinSolve(const Fluid3D& fluid, std::vector<double>& X, const std::vector<double>& Xprev, const std::uint8_t bs, const Laplacian& A)
{
    const std::uint64_t N = fluid.N;
    const std::uint32_t diagSize = A.diag.size();
    Eigen::VectorXd x(diagSize);
    Eigen::VectorXd b(diagSize);

    std::uint64_t maxX = bs == 1 ? fluid.N+1 : fluid.N;
    std::uint64_t maxY = bs == 2 ? fluid.N+1 : fluid.N;

    // Filling matrices
    std::uint64_t it = 0;
    for (std::uint64_t j = 0; j < maxY; ++j)
    {
        for (std::uint64_t i = 0; i < maxX; ++i)
        {
            b.coeffRef(it) = Xprev[fluid.IX(i,j,0,bs)];
            it++;
        }
    }

    // Solving Ap = b
    Eigen::VectorXd r = b;
    if (r.isZero(0))
    {
        X = Xprev;
        return;
    }
    Eigen::VectorXd p = Eigen::VectorXd::Zero(diagSize);
    Eigen::VectorXd z = p;
    applyPreconditioner(N, r, A, z, fluid.solver);
    Eigen::VectorXd s = z;
    double sig = z.dot(r);

    for (std::uint32_t i = 0; i < b.size(); ++i)
    {
        z = A.A * s;
        const double alpha = sig / s.dot(z);
        p = p + alpha * s;
        r = r - alpha * z;
        if (r.lpNorm<Eigen::Infinity>() < 10e-5)
        {
            break;
        }
        applyPreconditioner(N, r, A, z, fluid.solver);
        const double signew = z.dot(r);
        const double beta = signew / sig;
        s = z + beta * s;
        sig = signew;
    }

    // Write the results
    it = 0;
    for (std::uint64_t j = 0; j < maxY; ++j)
    {
        for (std::uint64_t i = 0; i < maxX; ++i)
        {
            X[fluid.IX(i,j,0,bs)] = p.coeff(it);
            it++;
        }
    }

    setBnd(fluid, X, bs);
}

void Fluids::project(const Fluid3D& fluid, std::vector<double>& X, std::vector<double>& Y, [[maybe_unused]] std::vector<double>& Z, std::vector<double>& p, std::vector<double>& div)
{
    const double h = 1.0/fluid.N;

    for (std::uint16_t j = 0; j < fluid.N; ++j)
    {
        for (std::uint16_t i = 0; i < fluid.N; ++i)
        {
            auto k = 0;
            div[fluid.IX(i,j,k,0)] = - 0.5 * (
                        (X[fluid.IX(i+1,j,k,1)]-X[fluid.IX(i,j,k,1)])+
                        (Y[fluid.IX(i,j+1,k,2)]-Y[fluid.IX(i,j,k,2)])
                        )*h;
            p[fluid.IX(i,j,k,0)] = 0;
        }
    }

    //p.clear();

	setBnd(fluid, div, 0);

    if (fluid.solver == GAUSS_SEIDEL)
    {
        //GaussSeidelRelaxationLinSolve(fluid, p, div, 1, fluid.is2D ? 4 : 6, 0);
        ERROR("Not implemented");
    }
    else
    {
        ConjugateGradientMethodLinSolve(fluid, p, div, 0, fluid.laplacianProject);
    }

    for (std::uint16_t j = 1; j < fluid.N; ++j)
    {
        for (std::uint16_t i = 1; i < fluid.N; ++i)
        {
            auto k = 0;
            X[fluid.IX(i,j,k,1)] -= 0.5*fluid.N*(p[fluid.IX(i,j,k,0)]-p[fluid.IX(i-1,j,k,0)]);
            Y[fluid.IX(i,j,k,2)] -= 0.5*fluid.N*(p[fluid.IX(i,j,k,0)]-p[fluid.IX(i,j-1,k,0)]);
        }
    }

	setBnd(fluid, X, 1);
	setBnd(fluid, Y, 2);
}

void Fluids::setBnd(const Fluid3D& fluid, std::vector<double>& X, const std::uint8_t b) const
{
    if (b == 1)
    {
        for (std::uint16_t i = 0; i < fluid.N+1; ++i)
        {
            if (i < fluid.N)
            {
                X[fluid.IX(0,i,0,b)] = -X[fluid.IX(1,i,0,b)];
                X[fluid.IX(fluid.N,i,0,b)] = -X[fluid.IX(fluid.N-1,i,0,b)];
            }
            X[fluid.IX(i,0,0,b)] = X[fluid.IX(i,1,0,b)];
            X[fluid.IX(i,fluid.N-1,0,b)] = X[fluid.IX(i,fluid.N-2,0,b)];
        }
    }
    else if (b == 2)
    {
        for (std::uint16_t i = 0; i < fluid.N+1; ++i)
        {
            X[fluid.IX(0,i,0,b)] = X[fluid.IX(1,i,0,b)];
            X[fluid.IX(fluid.N-1,i,0,b)] = X[fluid.IX(fluid.N-2,i,0,b)];
            if (i < fluid.N)
            {
                X[fluid.IX(i,0,0,b)] = -X[fluid.IX(i,1,0,b)];
                X[fluid.IX(i,fluid.N,0,b)] = -X[fluid.IX(i,fluid.N-1,0,b)];
            }
        }
    }
}

void Fluids::updateRender(Fluid3D& fluid)
{
    const std::uint64_t N = fluid.N;
    const std::uint64_t N32 = fluid.is2D ? N*N : (N+2)*(N+2)*(N+2);
	std::vector<std::uint8_t> texture(fluid.is2D ? N32*3 : N32, 0);

    std::uint64_t it = 0;
    double sum = 0;
    for (std::uint16_t i = 0; i < fluid.N; ++i)
    {
        for (std::uint16_t j = 0; j < fluid.N; ++j)
        {
            double value = fluid.substanceField[fluid.IX(i,j,0,0)];
		    texture[it*3] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
		    texture[it*3+1] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
		    texture[it*3+2] = static_cast<std::uint8_t>(std::clamp(value, 0.0, 255.0));
            sum += fluid.substanceField[fluid.IX(i,j,0,0)];
            it++;
            /*
            double implicit = fluid.implicitFunctionField[i];
            texture[i*3] = 0;
            texture[i*3+1] = 0;
            texture[i*3+2] = 0;
            std::uint64_t p = 50;
            if (implicit < 0)
            {
                texture[i*3+2] = std::clamp(-implicit*p, 0.0, 255.0);
            }
            else
            {
                texture[i*3] = std::clamp(implicit*p, 0.0, 255.0);
            }
            */
            /*
            const std::uint64_t m = i % N32;
            const std::uint64_t x = m % (N + 2);
            const std::uint64_t y = m / (N + 2);
            double gO = gradLength(fluid, fluid.implicitFunctionFieldPrev, x, y);
            texture[i*3] = std::clamp(gO*128, 0.0, 255.0);
            texture[i*3+1] = 0;
            texture[i*3+2] = 0;
            */
        }
	}

	const auto& textureGL = gCoordinator.GetComponent<Material>(fluid.entity).texture;
    if (!fluid.is2D)
    {
	    _renderer->initTexture3D(texture, textureGL);
    }
    else
    {
	    _renderer->initTexture2D(texture, textureGL);
    }
}

void Fluids::writeVolumeFile(Fluid3D& fluid, std::uint64_t iteration)
{
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
        float value = std::clamp(fluid.substanceField[ind], 0.0, 255.0);
        write(file, value);
    }

    file.close();
}
