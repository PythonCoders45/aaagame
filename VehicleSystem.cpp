#include <vector>
#include <string>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <memory>

// ============================================================================
// 1. DYNAMIC ADVANCED VEHICLE MATRIX TYPES
// ============================================================================

struct WorldVector3 {
    float x = 0.0f, y = 0.0f, z = 0.0f;

    WorldVector3 operator+(const WorldVector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    WorldVector3 operator-(const WorldVector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    WorldVector3 operator*(float scalar) const { return { x * scalar, y * scalar, z * scalar }; }
    
    float Magnitude() const { return std::sqrt(x * x + y * y + z * z); }
    
    WorldVector3 Normalize() const {
        float len = Magnitude();
        if (len > 0.0001f) return { x / len, y / len, z / len };
        return { 0.0f, 0.0f, 0.0f };
    }
    
    static float Dot(const WorldVector3& a, const WorldVector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
};

enum class VehicleClass {
    Automobile,
    Helicopter,
    Airplane,
    PrivateJet,
    Watercraft
};

struct DynamicState {
    WorldVector3 position;
    WorldVector3 linearVelocity;
    WorldVector3 orientationEuler; // Pitch, Yaw, Roll
};

struct Derivative {
    WorldVector3 deltaPosition;
    WorldVector3 deltaVelocity;
};

struct VehicleProfile {
    VehicleClass classification;
    float massKg;
    WorldVector3 dimensionsScale;
    float aerodynamicDragCoeff;
    float frontalAreaM2;
    float peakEngineThrustTorque;
    float brakeForceMax;
    float liftEfficiency; // Unique scaling coefficient for wings/rotors
};

// ============================================================================
// 2. ADVANCED FLIGHT METRIC CONFIGURATION PROFILE LOOKUP
// ============================================================================

inline VehicleProfile GetProfileByAssetPath(const std::string& path, VehicleClass cls) {
    VehicleProfile p;
    p.classification = cls;
    
    switch (cls) {
        case VehicleClass::Helicopter:
            p.massKg = 2400.0f;
            p.dimensionsScale = { 1.2f, 1.2f, 1.5f };
            p.aerodynamicDragCoeff = 0.45f; // High parasitic rotor head drag
            p.frontalAreaM2 = 4.2f;
            p.peakEngineThrustTorque = 18000.0f; // Direct vertical collective thrust lift cap
            p.brakeForceMax = 1500.0f;
            p.liftEfficiency = 1.8f; // High static hover lift multiplier
            break;

        case VehicleClass::Airplane:
            p.massKg = 41000.0f; // Large commercial airliner specs
            p.dimensionsScale = { 3.5f, 3.2f, 8.5f };
            p.aerodynamicDragCoeff = 0.026f; // Highly streamlined wing architecture
            p.frontalAreaM2 = 22.0f;
            p.peakEngineThrustTorque = 125000.0f; // Twin-turbofan system output
            p.brakeForceMax = 85000.0f;
            p.liftEfficiency = 3.4f; // High forward speed velocity dependent lift
            break;

        case VehicleClass::PrivateJet:
            p.massKg = 9800.0f; // Lighter weight twin-engine corporate flyer
            p.dimensionsScale = { 1.8f, 1.4f, 4.2f };
            p.aerodynamicDragCoeff = 0.021f; // Extremely low drag swept wings
            p.frontalAreaM2 = 6.8f;
            p.peakEngineThrustTorque = 48000.0f; // High thrust-to-weight performance profile
            p.brakeForceMax = 22000.0f;
            p.liftEfficiency = 2.8f;
            break;

        case VehicleClass::Automobile:
            if (path.find("lambo") != std::string::npos) {
                p.massKg = 1420.0f;
                p.dimensionsScale = { 1.25f, 0.82f, 1.18f };
                p.aerodynamicDragCoeff = 0.33f;
                p.frontalAreaM2 = 2.05f;
                p.peakEngineThrustTorque = 690.0f; // Ground tractive force torque calculations
                p.brakeForceMax = 18000.0f;
                p.liftEfficiency = 0.0f;
            } else { // Taxi Standard
                p.massKg = 1600.0f;
                p.dimensionsScale = { 1.1f, 1.1f, 1.1f };
                p.aerodynamicDragCoeff = 0.42f;
                p.frontalAreaM2 = 2.3f;
                p.peakEngineThrustTorque = 350.0f;
                p.brakeForceMax = 8000.0f;
                p.liftEfficiency = 0.0f;
            }
            break;

        case VehicleClass::Watercraft:
            p.massKg = 3500.0f;
            p.dimensionsScale = { 1.1f, 1.0f, 2.2f };
            p.aerodynamicDragCoeff = 0.65f;
            p.frontalAreaM2 = 4.5f;
            p.peakEngineThrustTorque = 850.0f;
            p.brakeForceMax = 4000.0f;
            p.liftEfficiency = 0.0f;
            break;
    }
    return p;
}

// ============================================================================
// 3. VEHICLE SYSTEM MAIN ARCHITECTURE DECLARATION
// ============================================================================

class VehicleSystem {
public:
    struct SimulatedInstance {
        int uid;
        std::string modelMeshPath;
        VehicleProfile profile;
        DynamicState state;
        
        float inputThrottle = 0.0f;
        float inputBrake = 0.0f;
        float inputSteer = 0.0f;
        float inputPitch = 0.0f;
        float inputRoll = 0.0f;
        
        bool isUserPossessed = false;
    };

    VehicleSystem() : mPlayerVehicleId(-1), mUniqueCounter(7000) {}

    void SpawnAdvancedVehicle(VehicleClass cls, const std::string& assetPath, WorldVector3 birthPos, float spawnYaw) {
        SimulatedInstance inst;
        inst.uid = mUniqueCounter++;
        inst.modelMeshPath = assetPath;
        inst.profile = GetProfileByAssetPath(assetPath, cls);
        
        inst.state.position = birthPos;
        inst.state.linearVelocity = { 0.0f, 0.0f, 0.0f };
        inst.state.orientationEuler = { 0.0f, spawnYaw, 0.0f };

        mActiveRegistry.push_back(inst);
    }

    void PossessVehicle(int vehicleId) {
        mPlayerVehicleId = vehicleId;
        for (auto& inst : mActiveRegistry) inst.isUserPossessed = (inst.uid == vehicleId);
    }

    void UpdateUserInputs(float throttle, float brake, float steer, float roll, float pitch) {
        for (auto& inst : mActiveRegistry) {
            if (inst.isUserPossessed) {
                inst.inputThrottle = std::clamp(throttle, 0.0f, 1.0f);
                inst.inputBrake    = std::clamp(brake, 0.0f, 1.0f);
                inst.inputSteer    = std::clamp(steer, -1.0f, 1.0f);
                inst.inputRoll     = std::clamp(roll, -1.0f, 1.0f);
                inst.inputPitch    = std::clamp(pitch, -1.0f, 1.0f);
                break;
            }
        }
    }

    void StepSimulationPipeline(float dt);
    const std::vector<SimulatedInstance>& GetRegistry() const { return mActiveRegistry; }

private:
    WorldVector3 EvaluateNetForces(const SimulatedInstance& inst, const DynamicState& state);
    Derivative EvaluateDerivative(const SimulatedInstance& inst, const DynamicState& initial, float dt, const Derivative& d);
    void IntegrateStateRK4(SimulatedInstance& inst, float dt);

private:
    std::vector<SimulatedInstance> mActiveRegistry;
    int mPlayerVehicleId;
    int mUniqueCounter;
};

// ============================================================================
// 4. ADVANCED RUNGE-KUTTA 4 SYSTEM FLIGHT CALCULATOR PIPELINE
// ============================================================================

WorldVector3 VehicleSystem::EvaluateNetForces(const SimulatedInstance& inst, const DynamicState& state) {
    WorldVector3 netForce = { 0.0f, 0.0f, 0.0f };
    
    float yawRad = state.orientationEuler.y * (3.14159265f / 180.0f);
    float pitchRad = state.orientationEuler.x * (3.14159265f / 180.0f);
    
    WorldVector3 forwardVec = {
        std::sin(yawRad) * std::cos(pitchRad),
        std::sin(pitchRad),
        std::cos(yawRad) * std::cos(pitchRad)
    };

    float forwardVelocity = WorldVector3::Dot(state.linearVelocity, forwardVec);

    // Aerodynamic Parasitic Fluid Drag Dynamic Model Equations
    const float airDensityRho = 1.225f; 
    float dynamicPressure = 0.5f * airDensityRho * (forwardVelocity * forwardVelocity);
    float dragMagnitude = dynamicPressure * inst.profile.aerodynamicDragCoeff * inst.profile.frontalAreaM2;
    netForce = netForce + (forwardVec * (-dragMagnitude * (forwardVelocity > 0.0f ? 1.0f : -1.0f)));

    // --- BRANCH ENGINE SELECTION LOGIC SYSTEMS ---
    if (inst.profile.classification == VehicleClass::Automobile) {
        float engineForce = inst.inputThrottle * (inst.profile.peakEngineThrustTorque / 0.33f);
        float brakingForce = inst.inputBrake * inst.profile.brakeForceMax;
        netForce = netForce + (forwardVec * (engineForce - (brakingForce * (forwardVelocity > 0.0f ? 1.0f : -1.0f))));
        netForce = netForce + (forwardVec * (-0.015f * inst.profile.massKg * 9.806f * (forwardVelocity > 0.05f ? 1.0f : 0.0f)));
    }
    else if (inst.profile.classification == VehicleClass::Helicopter) {
        // Helicopter Main Rotor Mechanics: Vertical engine lift vector is tied directly to the rotor axis system
        float collectiveThrust = inst.inputThrottle * inst.profile.peakEngineThrustTorque;
        
        // Tilt lift calculations using the helicopter pitch coordinates
        WorldVector3 rotorLiftDirection = {
            std::sin(yawRad) * std::sin(pitchRad),
            std::cos(pitchRad), // Strongest vertical vector thrust component
            std::cos(yawRad) * std::sin(pitchRad)
        };
        
        netForce = netForce + (rotorLiftDirection * collectiveThrust);
        netForce.y -= inst.profile.massKg * 9.806f; // Structural gravity weight penalty
    }
    else if (inst.profile.classification == VehicleClass::Airplane || inst.profile.classification == VehicleClass::PrivateJet) {
        // High Speed Jet Turbine Longitudinal Force
        float activeThrust = inst.inputThrottle * inst.profile.peakEngineThrustTorque;
        netForce = netForce + (forwardVec * activeThrust);

        // Fixed-Wing Induced Mechanical Aerodynamic Lift Force Model Equation
        // Lift = 0.5 * rho * v^2 * Area * Cl (Lift dynamic scaling is linked to angle of attack pitch)
        float angleOfAttackRad = pitchRad + (5.0f * 3.14159265f / 180.0f); // Default built-in wing angle of attack offset
        float liftCoefficient = inst.profile.liftEfficiency * std::sin(angleOfAttackRad);
        
        float liftMag = 0.5f * airDensityRho * (forwardVelocity * forwardVelocity) * inst.profile.frontalAreaM2 * liftCoefficient;
        if (forwardVelocity < 0.0f) liftMag = 0.0f; // Airplane shapes generate no lift traveling backward

        WorldVector3 localUpVector = { 0.0f, 1.0f, 0.0f }; // Vertical lift tracking vector alignment
        netForce = netForce + (localUpVector * liftMag);
        
        // Landing Gear Braking Friction Loop Logic
        if (state.position.y <= 0.1f && inst.inputBrake > 0.01f) {
            netForce = netForce + (forwardVec * (-inst.inputBrake * inst.profile.brakeForceMax));
        }
        
        netForce.y -= inst.profile.massKg * 9.806f;
    }
    else if (inst.profile.classification == VehicleClass::Watercraft) {
        float engineForce = inst.inputThrottle * inst.profile.peakEngineThrustTorque * 12.0f;
        netForce = netForce + (forwardVec * engineForce);
    }

    return netForce;
}

Derivative VehicleSystem::EvaluateDerivative(const SimulatedInstance& inst, const DynamicState& initial, float dt, const Derivative& d) {
    DynamicState shiftedState;
    shiftedState.position = initial.position + d.deltaPosition * dt;
    shiftedState.linearVelocity = initial.linearVelocity + d.deltaVelocity * dt;
    shiftedState.orientationEuler = initial.orientationEuler;

    Derivative output;
    output.deltaPosition = shiftedState.linearVelocity;
    
    WorldVector3 forceTotal = EvaluateNetForces(inst, shiftedState);
    output.deltaVelocity = forceTotal * (1.0f / inst.profile.massKg);
    return output;
}

void VehicleSystem::IntegrateStateRK4(SimulatedInstance& inst, float dt) {
    DynamicState& s = inst.state;

    Derivative zeroDerivative = { {0.0f,0.0f,0.0f}, {0.0f,0.0f,0.0f} };
    Derivative a = EvaluateDerivative(inst, s, 0.0f, zeroDerivative);
    Derivative b = EvaluateDerivative(inst, s, dt * 0.5f, a);
    Derivative c = EvaluateDerivative(inst, s, dt * 0.5f, b);
    Derivative d = EvaluateDerivative(inst, s, dt, c);

    WorldVector3 finalVelocityD = (a.deltaPosition + (b.deltaPosition * 2.0f) + (c.deltaPosition * 2.0f) + d.deltaPosition) * (1.0f / 6.0f);
    WorldVector3 finalAccelD    = (a.deltaVelocity + (b.deltaVelocity * 2.0f) + (c.deltaVelocity * 2.0f) + d.deltaVelocity) * (1.0f / 6.0f);

    s.position = s.position + finalVelocityD * dt;
    s.linearVelocity = s.linearVelocity + finalAccelD * dt;

    // --- AERODYNAMIC CONTROL SURFACE ANGLE STEERING TRANSFORMS ---
    float speedMagnitude = s.linearVelocity.Magnitude();

    if (inst.profile.classification == VehicleClass::Automobile || inst.profile.classification == VehicleClass::Watercraft) {
        s.orientationEuler.y += inst.inputSteer * (speedMagnitude * 0.4f) * 45.0f * dt;
        s.orientationEuler.x = 0.0f; s.orientationEuler.roll = 0.0f;
    } 
    else { // Hyper-Advanced 3D Flight Control Systems Matrix
        float airControlFactor = (inst.profile.classification == VehicleClass::Helicopter) ? 1.0f : std::clamp(speedMagnitude / 30.0f, 0.05f, 1.0f);
        
        s.orientationEuler.x += inst.inputPitch * 35.0f * airControlFactor * dt;
        s.orientationEuler.y += inst.inputSteer * 20.0f * airControlFactor * dt;
        s.orientationEuler.roll += inst.inputRoll  * 55.0f * airControlFactor * dt;
        
        s.orientationEuler.x = std::clamp(s.orientationEuler.x, -80.0f, 80.0f);
    }

    // Ground Height Map Bounds Checking
    float h = std::sin(s.position.x * 0.002f) * std::cos(s.position.z * 0.002f) * 15.0f;
    float floorHeight = (h < 0.0f || inst.profile.classification == VehicleClass::Watercraft) ? 0.0f : h;

    if (s.position.y < floorHeight) {
        s.position.y = floorHeight;
        if (s.linearVelocity.y < -2.0f) {
            s.linearVelocity.y = -s.linearVelocity.y * 0.12f; // Mechanical suspension bounce damping
        } else {
            s.linearVelocity.y = 0.0f;
        }
    }
}

void VehicleSystem::StepSimulationPipeline(float deltaTime) {
    for (auto& inst : mActiveRegistry) {
        if (!inst.isUserPossessed) {
            inst.inputThrottle = 0.4f;
            if (inst.profile.classification == VehicleClass::Airplane || inst.profile.classification == VehicleClass::PrivateJet) {
                inst.inputThrottle = 0.8f; // Ensure AI aircraft maintain proper cruising airspeed parameters
            }
        }
        IntegrateStateRK4(inst, deltaTime);
    }
}