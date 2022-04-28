#if !defined(CAMERA_DECL)

#    include "foundation/core.h"

#    if defined(CAMERA_INTERNAL)
#        define CAMERA_API CF_INTERNAL
#    else
#        define CAMERA_API extern
#    endif

typedef struct Camera
{
    // Camera position and coordinate system
    Vec3 pos, right, up, front;
    // Utilities for computing orientation, in radians
    F32 pitch, yaw;
    // Field-of-view (for zooming) in degrees
    F32 fov;
    // Parameters for orientation and motion behavior
    F32 speed, sensitivity;
} Camera;

/// Change camera orientation according to the given offset applied in the given time delta
CAMERA_API void cameraMove(Camera *self, Vec2 dpos, F32 dt);

/// Change camera orientation according to the given cursor delta
CAMERA_API void cameraDirect(Camera *self, Vec2 dpos);

/// Apply the given zoom amount
CAMERA_API void cameraZoom(Camera *self, F32 amount);

/// Compute the camera view transform
CAMERA_API Mat4 cameraView(Camera *self);

#    define CAMERA_DECL
#endif

#if defined(CAMERA_IMPL)

#    include "foundation/math.inl"

#    if defined CAMERA_INTERNAL
CF_DIAGNOSTIC_PUSH()
CF_DIAGNOSTIC_IGNORE_CLANG("-Wunused-function")
#    endif

#    define HALF_PI (0.5f * M_PI32)

void
cameraMove(Camera *self, Vec2 dpos, F32 dt)
{
    Vec2 offset = vecMul(dpos, self->speed * dt);
    self->pos = vecAdd3(self->pos, vecMul3(self->front, offset.y));
    self->pos = vecAdd3(self->pos, vecMul3(self->right, offset.x));
}

void
cameraDirect(Camera *self, Vec2 dpos)
{
    self->yaw += dpos.x * self->sensitivity;
    self->pitch += dpos.y * self->sensitivity;
    self->pitch = cfClamp(self->pitch, -HALF_PI + 0.01f, HALF_PI - 0.01f);

    F32 cosp = mCos(self->pitch);
    self->front = (Vec3){
        .x = mCos(self->yaw) * cosp,
        .y = mSin(self->pitch),
        .z = mSin(self->yaw) * cosp,
    };
    self->front = vecNormalize3(self->front);
    self->right = vecCross3(self->front, VEC3_Y);
    self->up = vecCross3(self->right, self->front);
}

void
cameraZoom(Camera *self, F32 amount)
{
    self->fov = cfClamp(self->fov + amount, 0.1f, 90.0f);
}

Mat4
cameraView(Camera *self)
{
    return matLookAt(self->pos, vecAdd3(self->pos, self->front), self->up);
}

#    if defined CAMERA_INTERNAL
CF_DIAGNOSTIC_POP()
#    endif

#endif // CAMERA_IMPL
