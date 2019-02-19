#include "FPSCamera.h"

FPSCamera::FPSCamera()
{
	yaw_ = pitch_  = 0;
	rotation_quat_ = XMQuaternionIdentity();
	position_	  = XMVectorSet(0, 0, 0, 1);
}

const XMMATRIX& FPSCamera::getView() const
{
	return view_;
}

const XMMATRIX& FPSCamera::getProjection() const
{
	return projection_;
}

const XMVECTOR FPSCamera::getRotationQuat() const
{
	return rotation_quat_;
}

const XMVECTOR FPSCamera::getPosition() const
{
	return position_;
}

void FPSCamera::update(float deltaTime)
{
	XMMATRIX rotation_mat = XMMatrixRotationQuaternion(rotation_quat_);

	rotation_mat = XMMatrixTranspose(rotation_mat);

	XMFLOAT3 translation;
	XMStoreFloat3(&translation, position_);

	XMMATRIX translation_mat = XMMatrixTranslation(-translation.x, -translation.y, -translation.z);

	view_ = translation_mat * rotation_mat;

	projection_ = XMMatrixPerspectiveFovRH(3.14f * 0.2f, 1920.0f / 1080.0f, 0.001f, 100.0f);
}

void FPSCamera::update(float deltaTime, XMVECTOR position, XMVECTOR lookAt, XMVECTOR upVec)
{
	view_ = XMMatrixLookAtLH(position, lookAt, upVec);

	projection_ = XMMatrixPerspectiveFovRH(3.14f * 0.2f, 1920.0f / 1080.0f, 0.1f, 100.0f);
}

void FPSCamera::setPosition(XMVECTOR position)
{
	position_ = position;
}

void FPSCamera::move(XMVECTOR velocity)
{
	position_ += velocity;
}

void FPSCamera::rotate(XMVECTOR deltaQuat)
{
	rotation_quat_ = XMQuaternionMultiply(deltaQuat, rotation_quat_);
}

void FPSCamera::rotate(float deltaYaw, float deltaPitch)
{
	yaw_ += deltaYaw;
	pitch_ += deltaPitch;

	rotation_quat_ = XMQuaternionRotationRollPitchYaw(pitch_, yaw_, 0.0f);
}