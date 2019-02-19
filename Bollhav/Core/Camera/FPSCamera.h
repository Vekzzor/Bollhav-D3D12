#pragma once

class FPSCamera
{
	XMMATRIX view_, projection_;

	XMVECTOR position_, rotation_quat_;
	float yaw_, pitch_;

public:
	FPSCamera();
	const XMMATRIX& getView() const;
	const XMMATRIX& getProjection() const;

	const XMVECTOR getRotationQuat() const;
	const XMVECTOR getPosition() const;

	void update(float deltaTime);
	void update(float deltaTime, XMVECTOR position, XMVECTOR lookAt, XMVECTOR upVec);

	void setPosition(XMVECTOR position);
	void move(XMVECTOR velocity);
	void rotate(XMVECTOR deltaQuat);
	void rotate(float deltaYaw, float deltaPitch);
};