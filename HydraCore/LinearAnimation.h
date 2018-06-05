#pragma once
#include "Animation.h"

namespace HydraCore
{
    class LinearAnimation : public Animation
    {
    private:
        float velocity;
    public:
        LinearAnimation();
        virtual void JumpToPosition(float targetPosition);
        virtual void Update(float deltaTime);

        virtual void SetTargetPosition(float targetPosition);
        void SetVelocity(float velocity);
    };
}
