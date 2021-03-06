

#pragma once

#include "GRID.h"

class WALL_CONDITIONS
{
public:

	GRID grid_;	

public:

	void Initialize(GRID& grid_input)
	{
		grid_ = grid_input;
	}

	void ClampPositionAndVelocity(TV3& pos, TV3& vel, TV3& force)
	{
		TS penalty_coef = 0;
		
		if (grid_.min_.x + grid_.gx_ + grid_.dx_*(TS)0.5 >= pos.x)
		{
			pos.x = grid_.min_.x + grid_.gx_ + grid_.dx_*(TS)0.5 + (TS)FLT_EPSILON;
			vel.x = (TS)0;
		}
		if (grid_.max_.x - grid_.gx_ - grid_.dx_*(TS)0.5 <= pos.x)
		{
			pos.x = grid_.max_.x - grid_.gx_ - grid_.dx_*(TS)0.5 - (TS)FLT_EPSILON;
			vel.x = (TS)0;
		}


		if (grid_.min_.y + grid_.gx_ + grid_.dx_*(TS)0.5 >= pos.y)
		{
			pos.y = grid_.min_.y + grid_.gx_ + grid_.dx_*(TS)0.5 + (TS)FLT_EPSILON;
			vel.y = (TS)0;
		}
		if (grid_.max_.y - grid_.gx_ - grid_.dx_*(TS)0.5 <= pos.y)
		{
			pos.y = grid_.max_.y - grid_.gx_ - grid_.dx_*(TS)0.5 - (TS)FLT_EPSILON;
			vel.y = (TS)0;
		}


		if (grid_.min_.z + grid_.gx_ + grid_.dx_*(TS)0.5 >= pos.z)
		{
			pos.z = grid_.min_.z + grid_.gx_ + grid_.dx_*(TS)0.5 + (TS)FLT_EPSILON;
			vel.z = (TS)0;
		}
		if (grid_.max_.z - grid_.gx_ - grid_.dx_*(TS)0.5 <= pos.z)
		{
			pos.z = grid_.max_.z - grid_.gx_ - grid_.dx_*(TS)0.5 - (TS)FLT_EPSILON;
			vel.z = (TS)0;
		}
	}
};