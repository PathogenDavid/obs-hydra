#pragma once

namespace HydraCore
{
    class Animation
    {
    protected:
        bool isAnimating;

        float targetPosition;
        float currentPosition;
    public:
        Animation();
        virtual void JumpToPosition(float targetPosition) = 0;
        virtual void Update(float deltaTime) = 0;

        virtual void SetTargetPosition(float targetPosition) = 0;

        inline bool IsAnimating()
        {
            return isAnimating;
        }

        inline float GetCurrentPosition()
        {
            return currentPosition;
        }

        inline float GetTargetPosition()
        {
            return targetPosition;
        }
    };
}
