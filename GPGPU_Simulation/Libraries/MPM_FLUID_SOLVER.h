
#pragma once

#include "KERNEL_FUNCTIONS.h"
#include "PARTICLE_MANAGER_3D.h"
#include "WALL_CONDITIONS.h"
#include "MATH_CORE.h"

#include "PARTICLE_OBJECT.h"
#include "PARTICLE_WORLD_BUILDER.h"

#include "GL\glut.h"

class MPM_FLUID_SOLVER
{
public:

	GRID_UNIFORM_3D grid_;

	PARTICLE_MANAGER_3D particle_manager_;

	FLT mass_;
	FLT rest_density_;

	FLT stiffness_;
	FLT smoothing_;

	FLT normal_stress_coef_;
	FLT shear_stress_coef_;

	Vec3* pts_position_arr_;
	Vec3* pts_velocity_arr_;
	Vec3* pts_force_arr_;
	Vec3* pts_grid_vel_arr_;
	FLT*  pts_density_arr_;
	Mat3* pts_tensor_arr_;

	Vec3* vector_temp_arr_;
	Vec3* scalar_temp_arr_;

	FLT*  density_field_;
	Vec3* velocity_field_;
	Vec3* force_field_;

	Vec3 gravity_;

	WALL_CONDITIONS wall_conditions_;

	// Object properties
	PARTICLE_OBJECT cube_object_;
	PARTICLE_WORLD_BUILDER particle_world_;

public:

	MPM_FLUID_SOLVER() : density_field_(0), velocity_field_(0), force_field_(0), mass_(1), rest_density_(1), normal_stress_coef_((FLT)0), shear_stress_coef_((FLT)0), smoothing_((FLT)0.3)
	{}

	~MPM_FLUID_SOLVER()
	{
		if (density_field_)  delete[] density_field_;
		if (velocity_field_) delete[] velocity_field_;
		if (force_field_)    delete[] force_field_;
	}

	void Initialize(const Vec3 min, const Vec3 max, const int i_res, const int j_res, const int k_res, const int ghost_width, const int num_pts_res);

	void AdvanceTimeStep(const FLT spf, const int steps)
	{
		SourceParticles();
		RebuildParticleDataStructure();

		const FLT dt = spf / (FLT)steps;
		for (int i = 0; i < steps; i++)
		{			
			RasterizeParticlesDensityAndVelocityToGrid();

			ComputeParticleDenistyFromGrid();

			ComputeStressTensors();

			ComputeGridForces();

			UpdateParticleAndGridVelocity(dt);

			AdvectParticles(dt);			

			CouplingWithObjects(dt);

			RebuildParticleDataStructure();
		}
	}

	void RasterizeParticlesDensityAndVelocityToGrid();

	void RebuildParticleDataStructure();

	void ComputeParticleDenistyFromGrid();

	void ComputeStressTensors();

	void ComputeGridForces();

	void UpdateParticleAndGridVelocity(const FLT dt);

	void AdvectParticles(const FLT dt);

	void CouplingWithObjects(const FLT dt);

	void SourceParticles()
	{
		SourceFormSphere(Vec3(0.2, 0.7, 0.5), Vec3( 2, 0.0, 0.0), 0.05, 1000);
		SourceFormSphere(Vec3(0.8, 0.7, 0.5), Vec3(-2, 0.0, 0.0), 0.05, 1000);
	}

	void SourceFormSphere(const Vec3& pos, const Vec3& vel, const FLT rad, const int num)
	{	
		Vec3* pos_arr = pts_position_arr_;
		Vec3* vel_arr = pts_velocity_arr_;

		int num_pts = 0;
		while (num_pts < num)
		{
			Vec3 new_pos = pos + RandomVector()*rad;

			FLT dist = glm::length((new_pos - pos));
			if (dist <= rad)
			{
				num_pts++;
				int ix = particle_manager_.AddParticle();
				
				pos_arr[ix] = new_pos;
				vel_arr[ix] = vel;
			}
		}
	}

	FLT ComputePressure(const FLT density)
	{
//		return (stiffness_*rest_density_) * ((density / rest_density_) - (FLT)1);
		return (stiffness_*rest_density_ / (FLT)3) * (POW3(density / rest_density_) - (FLT)1);
	}

	void ComputeStrainRate(const Vec3& pos, Mat3& tensor)
	{
		int i, j, k;
		grid_.CellCenterIndex(pos, i, j, k);

		FLT dudx((FLT)0), dudy((FLT)0), dudz((FLT)0), dvdx((FLT)0), dvdy((FLT)0), dvdz((FLT)0), dwdx((FLT)0), dwdy((FLT)0), dwdz((FLT)0);

		BEGIN_STENCIL_LOOP(grid_, i, j, k, l, m, n)
		{
			int s_ix = grid_.Index3Dto1D(l, m, n);

			Vec3 cell_pos = grid_.CellCenterPosition(l, m, n);
			Vec3 cell_vel = velocity_field_[s_ix];

			Vec3 grad = QuadBSplineKernelGradient(cell_pos-pos, grid_.one_over_dx_, grid_.one_over_dy_, grid_.one_over_dz_);

			dudx += cell_vel.x * grad.x;
			dudy += cell_vel.x * grad.y;
			dudz += cell_vel.x * grad.z;

			dvdx += cell_vel.y * grad.x;
			dvdy += cell_vel.y * grad.y;
			dvdz += cell_vel.y * grad.z;

			dwdx += cell_vel.z * grad.x;
			dwdy += cell_vel.z * grad.y;
			dwdz += cell_vel.z * grad.z;
		}
		END_STENCIL_LOOP;

		FLT D00 = dudx;
		FLT D11 = dvdy;
		FLT D22 = dwdz;
		FLT D01 = (dudy+dvdx)*(FLT)0.5;
		FLT D02 = (dudz+dwdx)*(FLT)0.5;
		FLT D12 = (dvdz+dwdy)*(FLT)0.5;

		tensor[0] = Vec3(D00, D01, D02);
		tensor[1] = Vec3(D01, D11, D12);
		tensor[2] = Vec3(D02, D12, D22);		
	}

	void RenderDensityField()
	{
		glDisable(GL_LIGHTING);

		glPushMatrix();
		glColor3f(0, 1, 0);

		glBegin(GL_POINTS);
		for (int c = 0; c < grid_.ijk_res_; c++)
		{
			int i, j, k;
			grid_.Index1Dto3D(c, i, j, k);

			Vec3 cell_center = grid_.CellCenterPosition(i, j, k);

			FLT density = density_field_[c];

			if (density > FLT_EPSILON)
			{
				glVertex3f(cell_center.x, cell_center.y, cell_center.z);
			}
		}
		glEnd();

		glPopMatrix();
	}
};
