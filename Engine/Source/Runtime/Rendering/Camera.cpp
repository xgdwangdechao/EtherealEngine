#include "Camera.h"
#include <limits>

//-----------------------------------------------------------------------------
//  Name : Camera ()
/// <summary>
/// Default constructor of this class.
/// </summary>
//-----------------------------------------------------------------------------
Camera::Camera()
{
	// Reset / Clear all required values
	mProjectionMode = ProjectionMode::Perspective;
	mFOV = 60.0f;
	mNearClip = 0.1f;
	mFarClip = 1000.0f;
	mProjectionWindow = { 0, 0, 0, 0 };
	mZoomFactor = 1.0f;

	// Reset / Clear all required values
	mAspectRatio = 1.0f;

	// Internal features are dirty by default
	mViewDirty = true;
	mProjectionDirty = true;
	mAspectDirty = true;
	mFrustumDirty = true;
	mFrustumLocked = false;
	mAspectLocked = false;

	// Assume no jittering initially
	mAAData = math::vec4(0, 0, 0, 0);
}

//-----------------------------------------------------------------------------
//  Name : ~Camera ()
/// <summary>
/// Destructor of this class.
/// </summary>
//-----------------------------------------------------------------------------
Camera::~Camera()
{
}



//-----------------------------------------------------------------------------
// Name : getZoomFactor( )
/// <summary>
/// Get the zoom factor (scale) currently applied to any orthographic view.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::getZoomFactor() const
{
	return mZoomFactor;
}

void Camera::setViewportSize(const uSize& viewportSize)
{
	mViewportSize = viewportSize;
	setAspectRatio(float(viewportSize.width) / float(viewportSize.height));
	const fRect rect =
	{
		-(float)viewportSize.width / 2.0f,
		(float)viewportSize.height / 2.0f,
		(float)viewportSize.width / 2.0f,
		-(float)viewportSize.height / 2.0f
	};
	setProjectionWindow(rect);

	setZoomFactor(estimateZoomFactor(viewportSize, math::vec3{ 0.0f, 0.0f, 0.0f }));
}

//-----------------------------------------------------------------------------
// Name : setZoomFactor( )
/// <summary>
/// Set the zoom factor (scale) currently applied to any orthographic view.
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setZoomFactor(float fZoom)
{
	const float MinZoomFactor = 0.0001f;
	const float MaxZoomFactor = 20000.0f;

	// Clamp the new zoom factor
	if (fZoom < MinZoomFactor) fZoom = MinZoomFactor;
	if (fZoom > MaxZoomFactor) fZoom = MaxZoomFactor;

	mZoomFactor = fZoom;

	onModified();
}


//-----------------------------------------------------------------------------
//  Name : setFOV ()
/// <summary>
/// Sets the field of view angle of this camera (perspective only).
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setFOV(float fFOVY)
{
	// Skip if no-op
	if (mFOV == fFOVY)
		return;

	// Update projection matrix and view frustum
	mFOV = fFOVY;

	onModified();
}

//-----------------------------------------------------------------------------
//  Name : setProjectionWindow ()
/// <summary>
/// Sets offsets for the projection window (orthographic only).
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setProjectionWindow(const fRect& rect)
{
	mProjectionWindow = rect;

	onModified();
}


//-----------------------------------------------------------------------------
//  Name : setProjectionMode ()
/// <summary>
/// Sets the current projection mode for this camera (i.e. orthographic
/// or perspective).
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setProjectionMode(ProjectionMode Mode)
{
	// Bail if this is a no op.
	if (Mode == mProjectionMode)
		return;


	// Alter the projection mode.
	mProjectionMode = Mode;

	onModified();
}


//-----------------------------------------------------------------------------
//  Name : setNearClip ()
/// <summary>
/// Set the near plane distance
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setNearClip(float fDistance)
{
	// Skip if this is a no-op
	if (fDistance == mNearClip)
		return;

	// Store value
	mNearClip = fDistance;

	onModified();

	// Make sure near clip is less than the far clip
	if (mNearClip > mFarClip)
		setFarClip(mNearClip);
}

//-----------------------------------------------------------------------------
//  Name : setFarClip()
/// <summary>
/// Set the far plane distance
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setFarClip(float fDistance)
{
	// Skip if this is a no-op
	if (fDistance == mFarClip)
		return;

	// Store value
	mFarClip = fDistance;

	onModified();

	// Make sure near clip is less than the far clip
	if (mNearClip > mFarClip)
		setNearClip(mFarClip);
}

//-----------------------------------------------------------------------------
//  Name : getFOV()
/// <summary>
/// Retrieve the current field of view angle in degrees.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::getFOV() const
{
	return mFOV;
}

//-----------------------------------------------------------------------------
//  Name : getProjectionWindow ()
/// <summary>
/// Retrieve offsets for the projection window (orthographic only).
/// </summary>
//-----------------------------------------------------------------------------
const fRect& Camera::getProjectionWindow() const
{
	return mProjectionWindow;
}

//-----------------------------------------------------------------------------
//  Name : getNearClip()
/// <summary>
/// Retrieve the distance from the camera to the near clip plane.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::getNearClip() const
{
	return mNearClip;
}

//-----------------------------------------------------------------------------
//  Name : getFarClip()
/// <summary>
/// Retrieve the distance from the camera to the far clip plane.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::getFarClip() const
{
	return mFarClip;
}


//-----------------------------------------------------------------------------
//  Name : getProjectionMode ()
/// <summary>
/// Retrieve the current projection mode for this camera.
/// </summary>
//-----------------------------------------------------------------------------
ProjectionMode Camera::getProjectionMode() const
{
	return mProjectionMode;
}

//-----------------------------------------------------------------------------
//  Name : getLocalBoundingBox ()
/// <summary>
/// Retrieve the bounding box of this object.
/// </summary>
//-----------------------------------------------------------------------------
math::bbox Camera::getLocalBoundingBox()
{
	float fNearSize = math::tan(math::radians<float>(mFOV*0.5f)) * mNearClip;
	float fFarSize = math::tan(math::radians<float>(mFOV*0.5f)) * mFarClip;
	return math::bbox(-fFarSize, -fFarSize, mNearClip, fFarSize, fFarSize, mFarClip);
}

//-----------------------------------------------------------------------------
//  Name : lockFrustum ()
/// <summary>
/// Prevent the frustum from updating.
/// </summary>
//-----------------------------------------------------------------------------
void Camera::lockFrustum(bool Locked)
{
	mFrustumLocked = Locked;
}

//-----------------------------------------------------------------------------
//  Name : isFrustumLocked ()
/// <summary>
/// Inform the caller whether or not the frustum is currently locked
/// This is useful as a debugging tool.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::isFrustumLocked() const
{
	return mFrustumLocked;
}

//-----------------------------------------------------------------------------
//  Name : setAspectRatio ()
/// <summary>
/// Set the aspect ratio that should be used to generate the horizontal
/// FOV angle (perspective only).
/// </summary>
//-----------------------------------------------------------------------------
void Camera::setAspectRatio(float fAspect, bool bLocked /* = false */)
{
	// Is this a no-op?
	if (fAspect == mAspectRatio)
	{
		mAspectLocked = bLocked;
		return;

	} // End if aspect is the same

	// Update camera properties
	mAspectRatio = fAspect;
	mAspectLocked = bLocked;
	mAspectDirty = true;
	mFrustumDirty = true;
	mProjectionDirty = true;
}

//-----------------------------------------------------------------------------
//  Name : getAspectRatio()
/// <summary>
/// Retrieve the aspect ratio used to generate the horizontal FOV angle.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::getAspectRatio() const
{
	return mAspectRatio;
}

//-----------------------------------------------------------------------------
//  Name : isAspectLocked()
/// <summary>
/// Determine if the aspect ratio is currently being updated by the
/// render driver.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::isAspectLocked() const
{
	return (getProjectionMode() == ProjectionMode::Orthographic || mAspectLocked);
}

//-----------------------------------------------------------------------------
//  Name : getProjectionMatrix ()
/// <summary>
/// Return the current projection matrix.
/// </summary>
//-----------------------------------------------------------------------------
const math::transform & Camera::getProj()
{
	// Only update matrix if something has changed
	if (getProjectionMode() == ProjectionMode::Perspective)
	{
		if (mProjectionDirty)
		{
			// Generate the updated perspective projection matrix
			float fFOVRadians = math::radians<float>(getFOV());
			mProj = math::perspective(fFOVRadians, mAspectRatio, getNearClip(), getFarClip());
			mProj[2][0] += mAAData.z;
			mProj[2][1] += mAAData.w;
			// Matrix has been updated
			mProjectionDirty = false;
			mAspectDirty = false;

		} // End if projection matrix needs updating
		else if (mAspectDirty)
		{
			// Just alter the aspect ratio
			mProj[0][0] = mProj[1][1] / mAspectRatio;

			// Matrix has been updated
			mAspectDirty = false;

		} // End if only aspect ratio changed

	} // End if perspective
	else if (getProjectionMode() == ProjectionMode::Orthographic)
	{
		if (mProjectionDirty || mAspectDirty)
		{
			// Generate the updated orthographic projection matrix
			float fZoom = getZoomFactor();
			const auto& rect = getProjectionWindow();
			mProj = math::ortho(rect.left * fZoom, rect.right * fZoom, rect.bottom * fZoom, rect.top * fZoom, getNearClip(), getFarClip());
			mProj[2][0] += mAAData.z;
			mProj[2][1] += mAAData.w;
			// Matrix has been updated
			mProjectionDirty = false;
			mAspectDirty = false;

		} // End if projection matrix needs updating

	} // End if orthographic

	// Return the projection matrix.
	return mProj;
}

//-----------------------------------------------------------------------------
//  Name : getViewMatrix ()
/// <summary>
/// Return the current view matrix.
/// </summary>
//-----------------------------------------------------------------------------
const math::transform & Camera::getView()
{
	// Return the view matrix.
	return mView;
}

void Camera::lookAt(const math::vec3 & vEye, const math::vec3 & vAt)
{
	mView.lookAt(vEye, vAt);

	onModified();
}

void Camera::lookAt(const math::vec3 & vEye, const math::vec3 & vAt, const math::vec3 & vUp)
{
	mView.lookAt(vEye, vAt, vUp);

	onModified();
}

math::vec3 Camera::getPosition() const
{
	return math::inverse(mView).getPosition();
}

math::vec3 Camera::zUnitAxis() const
{
	return math::inverse(mView).zUnitAxis();
}

//-----------------------------------------------------------------------------
//  Name : getFrustum()
/// <summary>
/// Retrieve the current camera object frustum.
/// </summary>
//-----------------------------------------------------------------------------
const math::frustum & Camera::getFrustum()
{
	// Recalculate frustum if necessary
	if (mFrustumDirty == true && mFrustumLocked == false)
	{
		mFrustum.update(getView(), getProj());
		mFrustumDirty = false;

		// Also build the frustum / volume that represents the space between the
		// camera position and its near plane. This frustum represents the
		// 'volume' that can end up clipping geometry.
		using namespace math::VolumeGeometry;
		using namespace math::VolumePlane;
		mClippingVolume = mFrustum;
		mClippingVolume.planes[Far].data.w = -mClippingVolume.planes[Near].data.w; // At near plane
		mClippingVolume.planes[Near].data.w = -math::dot((math::vec3&)mClippingVolume.planes[Near], getPosition()); // At camera

		// The corner points also need adjusting in this case such that they sit
		// precisely on the new planes.
		mClippingVolume.points[LeftBottomFar] = mClippingVolume.points[LeftBottomNear];
		mClippingVolume.points[LeftTopFar] = mClippingVolume.points[LeftTopNear];
		mClippingVolume.points[RightBottomFar] = mClippingVolume.points[RightBottomNear];
		mClippingVolume.points[RightTopFar] = mClippingVolume.points[RightTopNear];
		mClippingVolume.points[LeftBottomNear] = mClippingVolume.position;
		mClippingVolume.points[LeftTopNear] = mClippingVolume.position;
		mClippingVolume.points[RightBottomNear] = mClippingVolume.position;
		mClippingVolume.points[RightTopNear] = mClippingVolume.position;

	} // End if recalc frustum

	// Return the frustum
	return mFrustum;
}

//-----------------------------------------------------------------------------
//  Name : getClippingVolume()
/// <summary>
/// Retrieve the frustum / volume that represents the space between the camera 
/// position and its near plane. This frustum represents the 'volume' that can 
/// end up clipping geometry.
/// </summary>
//-----------------------------------------------------------------------------
const math::frustum & Camera::getClippingVolume()
{
	// Reclaculate frustum if necessary
	if (mFrustumDirty == true && mFrustumLocked == false)
		getFrustum();

	// Return the clipping volume
	return mClippingVolume;
}

//-----------------------------------------------------------------------------
//  Name : boundsInFrustum ()
/// <summary>
/// Determine whether or not the AABB specified falls within the frustum.
/// </summary>
//-----------------------------------------------------------------------------
math::VolumeQuery::E Camera::boundsInFrustum(const math::bbox & AABB)
{
	// Recompute the frustum as necessary.
	const math::frustum& f = getFrustum();

	// Request that frustum classifies
	return f.classifyAABB(AABB);
}

//-----------------------------------------------------------------------------
//  Name : boundsInFrustum ()
/// <summary>
/// Determine whether or not the OOBB specified is within the frustum.
/// </summary>
//-----------------------------------------------------------------------------
math::VolumeQuery::E Camera::boundsInFrustum(const math::bbox &AABB, const math::transform & t)
{
	// Recompute the frustum as necessary.
	const math::frustum & f = getFrustum();

	// Request that frustum classifies
	return math::frustum::classifyOBB(f, AABB, t);
}
//-----------------------------------------------------------------------------
//  Name : worldToViewport()
/// <summary>
/// Transform a point from world space, into screen space. Returns false 
/// if the point was clipped off the screen.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::worldToViewport(const uSize & ViewportSize, const math::vec3 & WorldPos, math::vec3 & ViewportPos, bool bClipX /* = true */, bool bClipY /* = true */, bool bClipZ /* = true */)
{
	// Ensure we have an up-to-date projection and view matrix
	auto mtxTransform = getView() * getProj();

	// Transform the point into clip space
	math::vec4 vClip = { math::transform::transformCoord(WorldPos, mtxTransform), 0 };

	// Was this clipped?
	if (bClipX == true && (vClip.x < -vClip.w || vClip.x > vClip.w)) return false;
	if (bClipY == true && (vClip.y < -vClip.w || vClip.y > vClip.w)) return false;
	if (bClipZ == true && (vClip.z < 0.0f || vClip.z > vClip.w)) return false;

	// Project!
	const float recipW = 1.0f / vClip.w;
	vClip.x *= recipW;
	vClip.y *= recipW;
	vClip.z *= recipW;

	// Transform to final screen space position
	ViewportPos.x = ((vClip.x * 0.5f) + 0.5f) * (float)ViewportSize.width;
	ViewportPos.y = ((vClip.y * -0.5f) + 0.5f) * (float)ViewportSize.height;
	ViewportPos.z = vClip.z;

	// Point on screen!
	return true;
}

//-----------------------------------------------------------------------------
//  Name : viewportToRay()
/// <summary>
/// Convert the specified screen position into a ray origin and direction
/// vector, suitable for use during picking.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::viewportToRay(const uSize & ViewportSize, const math::vec2 & ViewportPos, math::vec3 & vecRayStart, math::vec3 & vecRayDir)
{
	math::vec3 vCursor;

	// Ensure we have an up-to-date projection and view matrix
	math::transform mtxInvView;
	math::transform mtxProj = getProj();
	math::transform mtxView = getView();
	mtxInvView = math::inverse(mtxView);

	// Transform the pick position from viewport space into camera space
	vCursor.x = (((2.0f * ViewportPos.x) / (float)ViewportSize.width) - 1) / mtxProj[0][0];
	vCursor.y = -(((2.0f * ViewportPos.y) / (float)ViewportSize.height) - 1) / mtxProj[1][1];
	vCursor.z = 1.0f;

	// Transform the camera space pick ray into 3D space
	if (getProjectionMode() == ProjectionMode::Orthographic)
	{
		// Obtain the ray from the cursor position
		vecRayStart = math::transform::transformCoord(vCursor, mtxInvView);
		vecRayDir = (math::vec3&)mtxInvView[2];

	} // End If IsOrthohraphic
	else
	{
		// Obtain the ray from the cursor position
		vecRayStart = mtxInvView.getPosition();
		vecRayDir.x = vCursor.x * mtxInvView[0][0] + vCursor.y * mtxInvView[1][0] + vCursor.z * mtxInvView[2][0];
		vecRayDir.y = vCursor.x * mtxInvView[0][1] + vCursor.y * mtxInvView[1][1] + vCursor.z * mtxInvView[2][1];
		vecRayDir.z = vCursor.x * mtxInvView[0][2] + vCursor.y * mtxInvView[1][2] + vCursor.z * mtxInvView[2][2];

	} // End If !IsOrthohraphic

	  // Normalize the ray direction
	vecRayDir = math::normalize(vecRayDir);

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
//  Name : viewportToWorld ()
/// <summary>
/// Given a view screen position (in screen space) this function will cast that 
/// ray and return the world space position on the specified plane. The value
/// is returned via the world parameter passed.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::viewportToWorld(const uSize & ViewportSize, const math::vec2 & ViewportPos, const math::plane & Plane, math::vec3 & WorldPos)
{
	math::vec3 vPickRayDir, vPickRayOrig;
	float   fProjRayLength, fDistance;

	// Convert the screen coordinates to a ray.
	if (viewportToRay(ViewportSize, ViewportPos, vPickRayOrig, vPickRayDir) == false)
		return false;

	// Get the length of the 'adjacent' side of the virtual triangle formed
	// by the direction and normal.
	fProjRayLength = math::dot(vPickRayDir, (math::vec3&)Plane);
	if (math::abs<float>(fProjRayLength) < math::epsilon<float>())
		return false;

	// Calculate distance to plane along its normal
	fDistance = math::dot(vPickRayOrig, (math::vec3&)Plane) + Plane.data.w;

	// If both the "direction" and origin are on the same side of the plane
	// then we can't possibly intersect (perspective rule only)
	if (getProjectionMode() == ProjectionMode::Perspective)
	{
		int nSign1 = (fDistance > 0) ? 1 : (fDistance < 0) ? -1 : 0;
		int nSign2 = (fProjRayLength > 0) ? 1 : (fProjRayLength < 0) ? -1 : 0;
		if (nSign1 == nSign2)
			return false;

	} // End if perspective

	  // Calculate the actual interval (Distance along the adjacent side / length of adjacent side).
	fDistance /= -fProjRayLength;

	// Store the results
	WorldPos = vPickRayOrig + (vPickRayDir * fDistance);

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
//  Name : viewportToMajorAxis ()
/// <summary>
/// Given a view screen position (in screen space) this function will cast that 
/// ray and return the world space intersection point on one of the major axis
/// planes selected based on the camera look vector.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::viewportToMajorAxis(const uSize & ViewportSize, const math::vec2 & ViewportPos, const math::vec3 & Origin, math::vec3 & WorldPos, math::vec3 & MajorAxis)
{
	return viewportToMajorAxis(ViewportSize, ViewportPos, Origin, zUnitAxis(), WorldPos, MajorAxis);
}

//-----------------------------------------------------------------------------
//  Name : viewportToMajorAxis ()
/// <summary>
/// Given a view screen position (in screen space) this function will cast that 
/// ray and return the world space intersection point on one of the major axis
/// planes selected based on the specified normal.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::viewportToMajorAxis(const uSize & ViewportSize, const math::vec2 & ViewportPos, const math::vec3 & Origin, const math::vec3 & Normal, math::vec3 & WorldPos, math::vec3 & MajorAxis)
{
	// First select the major axis plane based on the specified normal
	MajorAxis = math::vec3(1, 0, 0); // YZ

	// Get absolute normal vector
	float x = math::abs<float>(Normal.x);
	float y = math::abs<float>(Normal.y);
	float z = math::abs<float>(Normal.z);

	// If all the components are effectively equal, select one plane
	if (math::abs<float>(x - y) < math::epsilon<float>() && math::abs<float>(x - z) < math::epsilon<float>())
	{
		MajorAxis = math::vec3(0, 0, 1); // XY

	} // End if components equal
	else
	{
		// Calculate which component of the normal is the major axis
		float fNorm = x;
		if (fNorm < y) { fNorm = y; MajorAxis = math::vec3(0, 1, 0); } // XZ
		if (fNorm < z) { fNorm = z; MajorAxis = math::vec3(0, 0, 1); } // XY

	} // End if perform compare

	  // Generate the intersection plane based on this information
	  // and pass through to the standard viewportToWorld method
	math::plane p = math::plane::fromPointNormal(Origin, MajorAxis);
	return viewportToWorld(ViewportSize, ViewportPos, p, WorldPos);
}

//-----------------------------------------------------------------------------
//  Name : viewportToCamera ()
/// <summary>
/// Given a view screen position (in screen space) this function will convert
/// the point into a camera space position at the near plane.
/// </summary>
//-----------------------------------------------------------------------------
bool Camera::viewportToCamera(const uSize & ViewportSize, const math::vec3 & ViewportPos, math::vec3 & CameraPos)
{
	// Ensure that we have an up-to-date projection and view matrix
	auto & mtxProj = getProj();
	auto & mtxView = getView();

	// Transform the pick position from screen space into camera space
	CameraPos.x = (((2.0f * ViewportPos.x) / (float)ViewportSize.width) - 1) / mtxProj[0][0];
	CameraPos.y = -(((2.0f * ViewportPos.y) / (float)ViewportSize.height) - 1) / mtxProj[1][1];
	CameraPos.z = getNearClip();

	// Success!
	return true;
}

//-----------------------------------------------------------------------------
//  Name : estimateZoomFactor ()
/// <summary>
/// Given the current viewport type and projection mode, estimate the "zoom"
/// factor that can be used for scaling various operations relative to their
/// "scale" as it appears in the viewport.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::estimateZoomFactor(const uSize & ViewportSize, const math::plane & Plane)
{
	math::vec3 vWorld;

	// Just return the actual zoom factor if this is orthographic
// 	if (getProjectionMode() == ProjectionMode::Orthographic)
// 		return getZoomFactor();

	// Otherwise, estimate is based on the distance from the grid plane.
	viewportToWorld(ViewportSize, math::vec2((float)ViewportSize.width / 2, (float)ViewportSize.height / 2), Plane, vWorld);

	// Perform full position based estimation
	return estimateZoomFactor(ViewportSize, vWorld);
}

//-----------------------------------------------------------------------------
// Name : estimateZoomFactor ()
/// <summary>
/// Given the current viewport type and projection mode, estimate the "zoom"
/// factor that can be used for scaling various operations relative to the
/// "scale" of an object as it appears in the viewport at the specified position.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::estimateZoomFactor(const uSize & ViewportSize, const math::vec3 & WorldPos)
{
	return estimateZoomFactor(ViewportSize, WorldPos, std::numeric_limits<float>::max());
}

//-----------------------------------------------------------------------------
// Name : estimatePickTolerance ()
/// <summary>
/// Estimate the distance (along each axis) from the specified object space 
/// point to use as a tolerance for picking.
/// </summary>
//-----------------------------------------------------------------------------
math::vec3 Camera::estimatePickTolerance(const uSize & ViewportSize, float WireTolerance, const math::vec3 & Pos, const math::transform & ObjectTransform)
{
	// Scale tolerance based on estimated world space zoom factor.
	math::vec3 v;
	ObjectTransform.transformCoord(v, Pos);
	WireTolerance *= estimateZoomFactor(ViewportSize, v);

	// Convert into object space tolerance.
	math::vec3 ObjectWireTolerance;
	math::vec3 vAxisScale = ObjectTransform.getScale();
	ObjectWireTolerance.x = WireTolerance / vAxisScale.x;
	ObjectWireTolerance.y = WireTolerance / vAxisScale.y;
	ObjectWireTolerance.z = WireTolerance / vAxisScale.z;
	return ObjectWireTolerance;
}

//-----------------------------------------------------------------------------
//  Name : estimateZoomFactor ()
/// <summary>
/// Given the current viewport type and projection mode, estimate the "zoom"
/// factor that can be used for scaling various operations relative to their
/// "scale" as it appears in the viewport.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::estimateZoomFactor(const uSize & ViewportSize, const math::plane & Plane, float fMax)
{
	// Just return the actual zoom factor if this is orthographic
// 	if (getProjectionMode() == ProjectionMode::Orthographic)
// 	{
// 		float fFactor = getZoomFactor();
// 		return std::min<float>(fMax, fFactor);
// 
// 	} // End if Orthographic

	  // Otherwise, estimate is based on the distance from the grid plane.
	math::vec3 vWorld;
	viewportToWorld(ViewportSize, math::vec2((float)ViewportSize.width / 2, (float)ViewportSize.height / 2), Plane, vWorld);

	// Perform full position based estimation
	return estimateZoomFactor(ViewportSize, vWorld, fMax);
}

//-----------------------------------------------------------------------------
// Name : estimateZoomFactor ()
/// <summary>
/// Given the current viewport type and projection mode, estimate the "zoom"
/// factor that can be used for scaling various operations relative to the
/// "scale" of an object as it appears in the viewport at the specified position.
/// </summary>
//-----------------------------------------------------------------------------
float Camera::estimateZoomFactor(const uSize & ViewportSize, const math::vec3 & WorldPos, float fMax)
{
	// Just return the actual zoom factor if this is orthographic
// 	if (getProjectionMode() == ProjectionMode::Orthographic)
// 	{
// 		float fFactor = getZoomFactor();
// 		return std::min<float>(fMax, fFactor);
// 
// 	} // End if Orthographic

	  // New Zoom factor is based on the distance to this position 
	  // along the camera's look vector.
	math::vec3 viewPos = math::transform::transformCoord(WorldPos, getView());
	float distance = viewPos.z / ((float)ViewportSize.height * (45.0f / getFOV()));
	return std::min<float>(fMax, distance);
}

//-----------------------------------------------------------------------------
//  Name : update ()
/// <summary>
/// Update the scene object.
/// </summary>
//-----------------------------------------------------------------------------
void Camera::update()
{
	// Automatically record matrices last used in the previous frame.
	recordCurrentMatrices();

}

//-----------------------------------------------------------------------------
//  Name : recordCurrentMatrices ()
/// <summary>
/// Make a copy of the current view / projection matrices before they
/// are changed. Useful for performing effects such as motion blur.
/// </summary>
//-----------------------------------------------------------------------------
void Camera::recordCurrentMatrices()
{
	mPreviousView = getView();
	mPreviousProj = getProj();
}

//-----------------------------------------------------------------------------
//  Name : getPreviousView ()
/// <summary>
/// Retrieve a copy of the view matrix recorded with the most recent call
/// to recordCurrentMatrices().
/// </summary>
//-----------------------------------------------------------------------------
const math::transform & Camera::getPreviousView() const
{
	return mPreviousView;
}

//-----------------------------------------------------------------------------
//  Name : getPreviousProjection ()
/// <summary>
/// Retrieve a copy of the projection matrix recorded with the most
/// recent call to recordCurrentMatrices().
/// </summary>
//-----------------------------------------------------------------------------
const math::transform & Camera::getPreviousProj() const
{
	return mPreviousProj;
}


//-----------------------------------------------------------------------------
// Name : setJitterAA ()
// Desc : Sets the current jitter value for temporal anti-aliasing
//-----------------------------------------------------------------------------
void Camera::setAAData(const uSize& viewportSize, std::uint32_t currentSubpixelIndex, std::uint32_t temporalAASamples)
{
	if (temporalAASamples > 1)
	{
		float SampleX = math::halton(currentSubpixelIndex, 2) - 0.5f;
		float SampleY = math::halton(currentSubpixelIndex, 3) - 0.5f;
		if (temporalAASamples == 2)
		{

			float SamplesX[] = { -4.0f / 16.0f, 4.0f / 16.0f };
			float SamplesY[] = { 4.0f / 16.0f, -4.0f / 16.0f };

			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 3)
		{
			// 3xMSAA
			//   A..
			//   ..B
			//   .C.
			// Rolling circle pattern (A,B,C).
			float SamplesX[] = { -2.0f / 3.0f,  2.0f / 3.0f,  0.0f / 3.0f };
			float SamplesY[] = { -2.0f / 3.0f,  0.0f / 3.0f,  2.0f / 3.0f };
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 4)
		{
			// 4xMSAA
			// Pattern docs: http://msdn.microsoft.com/en-us/library/windows/desktop/ff476218(v=vs.85).aspx
			//   .N..
			//   ...E
			//   W...
			//   ..S.
			// Rolling circle pattern (N,E,S,W).
			float SamplesX[] = { -2.0f / 16.0f,  6.0f / 16.0f, 2.0f / 16.0f, -6.0f / 16.0f };
			float SamplesY[] = { -6.0f / 16.0f, -2.0f / 16.0f, 6.0f / 16.0f,  2.0f / 16.0f };
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = SamplesX[Index];
			SampleY = SamplesY[Index];
		}
		else if (temporalAASamples == 8)
		{
			// This works better than various orderings of 8xMSAA.
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = math::halton(Index, 2) - 0.5f;
			SampleY = math::halton(Index, 3) - 0.5f;
		}
		else
		{
			// More than 8 samples can improve quality.
			std::uint32_t Index = currentSubpixelIndex;
			SampleX = math::halton(Index, 2) - 0.5f;
			SampleY = math::halton(Index, 3) - 0.5f;
		}

		mAAData = math::vec4(float(currentSubpixelIndex), float(temporalAASamples), SampleX, SampleY);

		float fWidth = static_cast<float>(viewportSize.width);
		float fHeight = static_cast<float>(viewportSize.height);
		mAAData.z *= (2.0f / fWidth);
		mAAData.w *= (2.0f / fHeight);

	}
	else
	{
		mAAData = math::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	}

	mProjectionDirty = true;
}

const math::vec4& Camera::getAAData() const
{
	return mAAData;
}

//-----------------------------------------------------------------------------
//  Name : () (Virtual)
/// <summary>
/// When the camera is modified.
/// </summary>
//-----------------------------------------------------------------------------
void Camera::onModified()
{
	// All modifications require projection matrix and
	// frustum to be updated.
	mViewDirty = true;
	mProjectionDirty = true;
	mFrustumDirty = true;

}