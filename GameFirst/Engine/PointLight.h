//
// Created by JoshuaShin on 3/4/26.
//

#include "Component.h"
#include "Lighting.h"

class PointLight : public Component
{
public:
    PointLight(Actor* actor, Lighting* lighting);
    ~PointLight() override;
    void LoadProperties(const rapidjson::Value& properties) override;
    void Update(float deltaTime) override;
private:
    Lighting* m_lighting = nullptr;
    PointLightData* m_pointLight = nullptr;

};

