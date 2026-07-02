#pragma once

#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <cmath>
#include <algorithm>
#include <map>

// ============================================================================
// 1. DATA TYPES, STRUCTS, AND ENUMS
// ============================================================================

struct Vector3D {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    float Distance(const Vector3D& o) const {
        return std::sqrt((x - o.x) * (x - o.x) + (y - o.y) * (y - o.y) + (z - o.z) * (z - o.z));
    }
    
    Vector3D Normalize() const {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0.0f) return { x / len, y / len, z / len };
        return { 0.0f, 0.0f, 0.0f };
    }
};

enum class PedType {
    Player,
    CivilianWorker,
    CivilianTourist,
    CivilianElderly,
    GangDiablos,
    GangYardies,
    GangMafia,
    CopPatrol,
    CopSwat,
    Medic,
    Firefighter,
    CriminalThief
};

enum class PedState {
    Idle,
    Wandering,
    MovingToSchedule,
    FleeingPanic,
    Cowering,
    SeekingCover,
    CombatMelee,
    CombatRanged,
    Arresting,
    MedicalRevive,
    ExtinguishingFire,
    Dead
};

enum class RoutineState {
    Sleeping,
    CommutingWalk,
    WorkingOffice,
    WorkingRetail,
    PatrollingTurf,
    PatrollingBeat,
    GuardingArea,
    Loitering,
    SocializingBar,
    Dining,
    Exercising,
    Shoplifting
};

enum class StimulusType {
    GunfireHeard,
    ExplosionWitnessed,
    MeleeAttackSeen,
    CarCrashWitnessed,
    PlayerThreatPointing,
    FireDetected,
    DeadBodyFound,
    NormalAmbient
};

struct ThreatStimulus {
    StimulusType type;
    Vector3D position;
    float intensity;
    float timestamp;
};

struct ScheduleNode {
    int startHour;
    int endHour;
    RoutineState routine;
    Vector3D location;
    std::string taskDescription;
};

// ============================================================================
// 2. BEHAVIOR AND SUBSYSTEM INTERFACES
// ============================================================================

class Pedestrian;

class IKModule {
public:
    Vector3D headLookTarget;
    float lookWeight = 0.0f;

    void SetLookTarget(const Vector3D& target) {
        headLookTarget = target;
        lookWeight = 1.0f;
    }

    void ClearLookTarget() {
        lookWeight = 0.0f;
    }

    void UpdateIKTransforms(const Vector3D& currentPos) {
        if (lookWeight > 0.0f) {
            // Internal bone transform math would modify the mesh vertex hierarchy here
        }
    }
};

class AudioSpeechModule {
public:
    void TriggerVoiceLine(const std::string& model, const std::string& context) {
        std::cout << "[AUDIO][" << model << "]: Playing voice flag -> " << context << "\n";
    }
};

class CombatModule {
public:
    void ProcessAttack(Pedestrian* attacker, Pedestrian* target, float damage, bool ranged);
};

// ============================================================================
// 3. BASE PEDESTRIAN COMPONENT
// ============================================================================

class Pedestrian {
public:
    Pedestrian(PedType type, const std::string& model, Vector3D spawnPos, float accuracy, float courage)
        : mType(type), mModelName(model), mPosition(spawnPos), mState(PedState::Idle),
          mCurrentRoutine(RoutineState::Wandering), mHealth(100.0f), mAccuracy(accuracy),
          mCourage(courage), mTarget(nullptr), mIsSprinting(false) {
        mHomeLocation = spawnPos;
    }

    virtual ~Pedestrian() = default;

    virtual void Initialize() {
        std::cout << "[SYSTEM]: Spawned entity " << mModelName << " into active memory streams.\n";
    }

    void ProcessAISystems(int currentHour, float dt, const std::vector<ThreatStimulus>& globalThreats) {
        if (mHealth <= 0.0f) {
            mState = PedState::Dead;
            return;
        }

        // 1. Perception Step
        EvaluatePerceptionGrid(globalThreats);

        // 2. Decision Tree Step
        if (mState != PedState::FleeingPanic && mState != PedState::CombatMelee && 
            mState != PedState::CombatRanged && mState != PedState::Cowering && mState != PedState::SeekingCover) {
            EvaluateScheduleChronology(currentHour);
        } else {
            EvaluateTacticalDecisions(dt);
        }

        // 3. Mechanical Physics / Transform Step
        ExecutePhysicalMovement(dt);
        mIK.UpdateIKTransforms(mPosition);
    }

    void ReceivePhysicalDamage(float points, Pedestrian* source, const std::string& weaponType) {
        if (mState == PedState::Dead) return;

        mHealth -= points;
        std::cout << "[DAMAGE]: " << mModelName << " hit for " << points << " by " << weaponType << ". Health: " << mHealth << "\n";

        if (mHealth <= 0.0f) {
            mHealth = 0.0f;
            mState = PedState::Dead;
            mAudio.TriggerVoiceLine(mModelName, "SPEECH_DEATH_SCREAM");
            return;
        }

        mAudio.TriggerVoiceLine(mModelName, "SPEECH_PAIN_GRUNT");

        // Courage assessment logic
        if (mCourage > 50.0f && source && source != this) {
            mTarget = source;
            mState = (mType == PedType::CopPatrol || mType == PedType::CopSwat || mType == PedType::GangMafia) 
                     ? PedState::CombatRanged : PedState::CombatMelee;
        } else {
            if (source) mDestination = GenerateEscapePoint(source->GetPosition());
            mState = PedState::FleeingPanic;
        }
    }

    void InjectScheduleBlock(int start, int end, RoutineState routine, Vector3D targetLoc, const std::string& desc) {
        mScheduleList.push_back({start, end, routine, targetLoc, desc});
    }

    // Getters and Core Queries
    Vector3D GetPosition() const { return mPosition; }
    void ForcePosition(Vector3D pos) { mPosition = pos; }
    PedState GetState() const { return mState; }
    PedType GetPedType() const { return mType; }
    std::string GetModel() const { return mModelName; }
    float GetHealth() const { return mHealth; }

protected:
    PedType mType;
    std::string mModelName;
    Vector3D mPosition;
    Vector3D mDestination;
    Vector3D mHomeLocation;
    PedState mState;
    RoutineState mCurrentRoutine;
    
    float mHealth;
    float mAccuracy;
    float mCourage;
    bool mIsSprinting;

    Pedestrian* mTarget;
    std::vector<ScheduleNode> mScheduleList;

    IKModule mIK;
    AudioSpeechModule mAudio;
    CombatModule mCombat;

private:
    void EvaluatePerceptionGrid(const std::vector<ThreatStimulus>& threats) {
        for (const auto& threat : threats) {
            float distance = mPosition.Distance(threat.position);
            if (distance > 50.0f) continue; // Beyond threat audibility/visibility thresholds

            mIK.SetLookTarget(threat.position);

            // Behavioral branching table based on personality metrics
            if (threat.type == StimulusType::GunfireHeard || threat.type == StimulusType::ExplosionWitnessed) {
                if (mCourage < 40.0f) {
                    mDestination = GenerateEscapePoint(threat.position);
                    mState = PedState::FleeingPanic;
                    mIsSprinting = true;
                } else if (mType == PedType::CopPatrol || mType == PedType::CopSwat) {
                    mState = PedState::SeekingCover;
                    mDestination = threat.position; // Move to engage source
                }
            } else if (threat.type == StimulusType::FireDetected) {
                mDestination = GenerateEscapePoint(threat.position);
                mState = PedState::FleeingPanic;
                mIsSprinting = true;
            } else if (threat.type == StimulusType::DeadBodyFound && mType == PedType::CivilianWorker) {
                mDestination = mHomeLocation;
                mState = PedState::FleeingPanic;
                mAudio.TriggerVoiceLine(mModelName, "SPEECH_CALL_POLICE");
            }
        }
    }

    void EvaluateScheduleChronology(int hour) {
        for (const auto& node : mScheduleList) {
            if (hour >= node.startHour && hour < node.endHour) {
                if (mCurrentRoutine != node.routine) {
                    mCurrentRoutine = node.routine;
                    mDestination = node.location;
                    mState = PedState::MovingToSchedule;
                    mIsSprinting = false;
                    std::cout << "[SCHEDULE]: " << mModelName << " updated routine task -> " << node.taskDescription << "\n";
                }
                break;
            }
        }
    }

    void EvaluateTacticalDecisions(float dt) {
        if ((mState == PedState::CombatMelee || mState == PedState::CombatRanged) && !mTarget) {
            mState = PedState::Idle;
            return;
        }

        if (mState == PedState::CombatMelee && mTarget) {
            if (mTarget->GetHealth() <= 0.0f) { mTarget = nullptr; mState = PedState::Idle; return; }
            float dist = mPosition.Distance(mTarget->GetPosition());
            if (dist < 1.8f) {
                mCombat.ProcessAttack(this, mTarget, 25.0f * dt, false);
            } else {
                mDestination = mTarget->GetPosition();
            }
        }

        if (mState == PedState::CombatRanged && mTarget) {
            if (mTarget->GetHealth() <= 0.0f) { mTarget = nullptr; mState = PedState::Idle; return; }
            float dist = mPosition.Distance(mTarget->GetPosition());
            if (dist < 30.0f) {
                mCombat.ProcessAttack(this, mTarget, 45.0f * dt, true);
            } else {
                mDestination = mTarget->GetPosition(); // Reposition within firing envelope
            }
        }
    }

    void ExecutePhysicalMovement(float dt) {
        if (mState != PedState::MovingToSchedule && mState != PedState::FleeingPanic && 
            mState != PedState::CombatMelee && mState != PedState::CombatRanged) {
            return;
        }

        float speed = mIsSprinting ? 6.5f : 2.2f;
        float dist = mPosition.Distance(mDestination);

        if (dist > 0.5f) {
            Vector3D dir = { mDestination.x - mPosition.x, mDestination.y - mPosition.y, mDestination.z - mPosition.z };
            dir = dir.Normalize();

            mPosition.x += dir.x * speed * dt;
            mPosition.y += dir.y * speed * dt;
            mPosition.z += dir.z * speed * dt;
        } else if (mState == PedState::MovingToSchedule) {
            mState = PedState::Idle;
        }
    }

    Vector3D GenerateEscapePoint(Vector3D threat) {
        Vector3D diff = { mPosition.x - threat.x, mPosition.y - threat.y, mPosition.z - threat.z };
        diff = diff.Normalize();
        return { mPosition.x + diff.x * 45.0f, mPosition.y, mPosition.z + diff.z * 45.0f };
    }
};

inline void CombatModule::ProcessAttack(Pedestrian* attacker, Pedestrian* target, float damage, bool ranged) {
    std::string mode = ranged ? "Ranged Bullet Vector" : "Melee Impact Force";
    target->ReceivePhysicalDamage(damage, attacker, mode);
}

// ============================================================================
// 4. CONCRETE VARIANT INJECTIONS & REACTION MATRIX PROFILES
// ============================================================================

class WorkerCitizen : public Pedestrian {
public:
    WorkerCitizen(std::string variantModel, Vector3D spawn)
        : Pedestrian(PedType::CivilianWorker, variantModel, spawn, 10.0f, 15.0f) {
        
        // Hour 00-06: Home sleeping block
        InjectScheduleBlock(0, 6, RoutineState::Sleeping, spawn, "Sleeping in residential apartment complex.");
        // Hour 06-08: Pedestrian transit routing block
        InjectScheduleBlock(6, 8, RoutineState::CommutingWalk, {spawn.x + 30.0f, 0.0f, spawn.z + 120.0f}, "Walking towards public transit stations.");
        // Hour 08-16: Core business simulation layer
        InjectScheduleBlock(8, 16, RoutineState::WorkingOffice, {230.0f, 4.0f, -450.0f}, "Performing desk duties inside office tower.");
        // Hour 16-18: Restaurant dining matrix
        InjectScheduleBlock(16, 18, RoutineState::Dining, {-12.0f, 0.0f, 85.0f}, "Eating dinner at downtown commercial center.");
        // Hour 18-24: Night cycle wandering relaxation
        InjectScheduleBlock(18, 24, RoutineState::Loitering, spawn, "Returning home to watch television.");
    }
};

class TouristCitizen : public Pedestrian {
public:
    TouristCitizen(std::string variantModel, Vector3D spawn)
        : Pedestrian(PedType::CivilianTourist, variantModel, spawn, 5.0f, 25.0f) {
        
        InjectScheduleBlock(0, 9, RoutineState::Sleeping, spawn, "Resting at central hotel structure room.");
        InjectScheduleBlock(9, 14, RoutineState::Wandering, {400.0f, 0.0f, 10.0f}, "Taking photos around historic city square monuments.");
        InjectScheduleBlock(14, 16, RoutineState::Dining, {15.0f, 0.0f, -20.0f}, "Consuming fast food at central plaza benches.");
        InjectScheduleBlock(16, 22, RoutineState::SocializingBar, {-110.0f, 1.0f, 300.0f}, "Drinking liquids at underground entertainment venue.");
        InjectScheduleBlock(22, 24, RoutineState::CommutingWalk, spawn, "Walking back to hotel block.");
    }
};

class ElderlyCitizen : public Pedestrian {
public:
    ElderlyCitizen(std::string variantModel, Vector3D spawn)
        : Pedestrian(PedType::CivilianElderly, variantModel, spawn, 0.0f, 5.0f) {
        
        InjectScheduleBlock(0, 7, RoutineState::Sleeping, spawn, "Resting.");
        InjectScheduleBlock(7, 10, RoutineState::Exercising, {spawn.x + 5.0f, 0.0f, spawn.z + 5.0f}, "Slow aerobic jogging routines through neighborhood.");
        InjectScheduleBlock(10, 15, RoutineState::Loitering, {30.0f, 0.0f, 30.0f}, "Sitting on concrete benches watching traffic networks.");
        InjectScheduleBlock(15, 18, RoutineState::WorkingRetail, {50.0f, 0.0f, -90.0f}, "Managing item logistics at grocery marketplace store.");
        InjectScheduleBlock(18, 24, RoutineState::Sleeping, spawn, "Retiring early to residential unit.");
    }
};

class DiabloGangMember : public Pedestrian {
public:
    DiabloGangMember(std::string variantModel, Vector3D turfSpawn)
        : Pedestrian(PedType::GangDiablos, variantModel, turfSpawn, 45.0f, 85.0f) {
        
        InjectScheduleBlock(0, 4, RoutineState::PatrollingTurf, turfSpawn, "Monitoring dark street blocks for hostile entities.");
        InjectScheduleBlock(4, 13, RoutineState::Sleeping, turfSpawn, "Sleeping in secure safehouse bunker units.");
        InjectScheduleBlock(13, 19, RoutineState::Loitering, {turfSpawn.x + 20.0f, 0.0f, turfSpawn.z + 5.0f}, "Leaning against brick walls discussing operations.");
        InjectScheduleBlock(19, 24, RoutineState::PatrollingTurf, {turfSpawn.x - 40.0f, 0.0f, turfSpawn.z - 40.0f}, "Executing active block security protocols.");
    }
};

class YardieGangMember : public Pedestrian {
public:
    YardieGangMember(std::string variantModel, Vector3D turfSpawn)
        : Pedestrian(PedType::GangYardies, variantModel, turfSpawn, 55.0f, 75.0f) {
        
        InjectScheduleBlock(0, 5, RoutineState::SocializingBar, {turfSpawn.x, 0.0f, turfSpawn.z}, "Playing pool music tracks inside neighborhood club.");
        InjectScheduleBlock(5, 14, RoutineState::Sleeping, turfSpawn, "Resting.");
        InjectScheduleBlock(14, 20, RoutineState::Shoplifting, {10.0f, 0.0f, 400.0f}, "Attempting to gather unregistered commercial products.");
        InjectScheduleBlock(20, 24, RoutineState::PatrollingTurf, turfSpawn, "Selling contraband materials down street intersections.");
    }
};

class LeoneMafiaMember : public Pedestrian {
public:
    LeoneMafiaMember(std::string variantModel, Vector3D mansionSpawn)
        : Pedestrian(PedType::GangMafia, variantModel, mansionSpawn, 75.0f, 95.0f) {
        
        InjectScheduleBlock(0, 8, RoutineState::GuardingArea, mansionSpawn, "Standing watch outside courtyard gates holding submachine guns.");
        InjectScheduleBlock(8, 16, RoutineState::GuardingArea, {mansionSpawn.x + 10.0f, 2.0f, mansionSpawn.z}, "Patrolling interior balcony infrastructure layers.");
        InjectScheduleBlock(16, 24, RoutineState::Dining, {mansionSpawn.x, 0.0f, mansionSpawn.z + 20.0f}, "Attending gang syndicate dinner meetings.");
    }
};

class PatrolOfficer : public Pedestrian {
public:
    PatrolOfficer(Vector3D precinctSpawn)
        : Pedestrian(PedType::CopPatrol, "COP_SQUAD_UNIFORM", precinctSpawn, 60.0f, 90.0f) {
        
        InjectScheduleBlock(0, 8, RoutineState::PatrollingBeat, {100.0f, 0.0f, 100.0f}, "Walking regular beats down commercial avenues checking storefront doors.");
        InjectScheduleBlock(8, 16, RoutineState::PatrollingBeat, {-200.0f, 0.0f, -200.0f}, "Securing high-crime industrial sector areas.");
        InjectScheduleBlock(16, 24, RoutineState::PatrollingBeat, {0.0f, 0.0f, 0.0f}, "Standing sentry watch inside central transit terminals.");
    }
};

class SwatTacticalUnit : public Pedestrian {
public:
    SwatTacticalUnit(Vector3D baseSpawn)
        : Pedestrian(PedType::CopSwat, "COP_SWAT_HEAVY_HELMET", baseSpawn, 90.0f, 100.0f) {
        
        InjectScheduleBlock(0, 24, RoutineState::GuardingArea, baseSpawn, "Sitting inside armored tactical deployment van awaiting dispatch markers.");
    }
};

class ParamedicMedic : public Pedestrian {
public:
    ParamedicMedic(Vector3D hospitalSpawn)
        : Pedestrian(PedType::Medic, "MEDIC_AMBULANCE_JACKET", hospitalSpawn, 20.0f, 60.0f) {
        
        InjectScheduleBlock(0, 12, RoutineState::WorkingOffice, hospitalSpawn, "Stocking medical cabinets inside ER trauma bays.");
        InjectScheduleBlock(12, 24, RoutineState::Loitering, {hospitalSpawn.x + 15.0f, 0.0f, hospitalSpawn.z}, "Drinking coffee outside ambulance bay processing zone.");
    }
};

class FirefighterUnit : public Pedestrian {
public:
    FirefighterUnit(Vector3D stationSpawn)
        : Pedestrian(PedType::Firefighter, "FIRE_RESCUE_TURN_OUTS", stationSpawn, 30.0f, 85.0f) {
        
        InjectScheduleBlock(0, 24, RoutineState::WorkingOffice, stationSpawn, "Maintaining technical apparatus tools inside engine bay floor grids.");
    }
};

class ThiefCriminal : public Pedestrian {
public:
    ThiefCriminal(Vector3D backAlleySpawn)
        : Pedestrian(PedType::CriminalThief, "CRIM_JACKET_MASK", backAlleySpawn, 40.0f, 35.0f) {
        
        InjectScheduleBlock(0, 6, RoutineState::Shoplifting, {50.0f, 0.0f, 50.0f}, "Monitoring jewelry shop display window setups.");
        InjectScheduleBlock(6, 15, RoutineState::Sleeping, backAlleySpawn, "Hiding from police units inside secure squat houses.");
        InjectScheduleBlock(15, 24, RoutineState::Loitering, {0.0f, 0.0f, 0.0f}, "Wandering crowded shopping plazas targeting active purses.");
    }
};

// ============================================================================
// 5. THE GLOBAL ARCHITECTURAL POPULATION MANAGER
// ============================================================================

class ComprehensivePopulationSystem {
public:
    ComprehensivePopulationSystem() : mCurrentWorldTimeHour(8), mSimulationRadius(120.0f) {}

    void SetClockHour(int hour) {
        if (hour >= 0 && hour < 24) mCurrentWorldTimeHour = hour;
    }

    void RaiseEnvironmentalStimulus(StimulusType type, Vector3D position, float radiusIntensity) {
        mActiveThreatStream.push_back({type, position, radiusIntensity, static_cast<float>(mCurrentWorldTimeHour)});
    }

    void InitializeCityGridSchedules(Vector3D referenceCenter) {
        // Explicitly instantiating multiple distinct profiles to verify loop handling capacity
        mGlobalRegistry.push_back(std::make_shared<WorkerCitizen>("CIV_MALE_SUIT_A", Vector3D{referenceCenter.x + 10.0f, 0.0f, referenceCenter.z + 10.0f}));
        mGlobalRegistry.push_back(std::make_shared<WorkerCitizen>("CIV_FEMALE_SUIT_B", Vector3D{referenceCenter.x - 15.0f, 0.0f, referenceCenter.z + 20.0f}));
        mGlobalRegistry.push_back(std::make_shared<TouristCitizen>("CIV_MALE_HAWAIIAN", Vector3D{referenceCenter.x + 50.0f, 0.0f, referenceCenter.z - 30.0f}));
        mGlobalRegistry.push_back(std::make_shared<ElderlyCitizen>("CIV_OLD_MALE_PANTS", Vector3D{referenceCenter.x - 40.0f, 0.0f, referenceCenter.z - 40.0f}));
        
        mGlobalRegistry.push_back(std::make_shared<DiabloGangMember>("GANG_DIABLO_BANDANA", Vector3D{-300.0f, 0.0f, -300.0f}));
        mGlobalRegistry.push_back(std::make_shared<YardieGangMember>("GANG_YARDIE_SHIRT", Vector3D{400.0f, 0.0f, -100.0f}));
        mGlobalRegistry.push_back(std::make_shared<LeoneMafiaMember>("GANG_MAFIA_BLACK_SUIT", Vector3D{600.0f, 5.0f, 600.0f}));
        
        mGlobalRegistry.push_back(std::make_shared<PatrolOfficer>(Vector3D{0.0f, 0.0f, 20.0f}));
        mGlobalRegistry.push_back(std::make_shared<SwatTacticalUnit>(Vector3D{5.0f, 0.0f, 25.0f}));
        mGlobalRegistry.push_back(std::make_shared<ParamedicMedic>(Vector3D{-150.0f, 0.0f, -150.0f}));
        mGlobalRegistry.push_back(std::make_shared<FirefighterUnit>(Vector3D{250.0f, 0.0f, 50.0f}));
        mGlobalRegistry.push_back(std::make_shared<ThiefCriminal>(Vector3D{-20.0f, 0.0f, -80.0f}));
    }

    void RunSimulationFrameTick(Vector3D playerFocusPos, float deltaTime) {
        // Culling Phase: Clean memory pools if distance parameters collapse
        mGlobalRegistry.erase(
            std::remove_if(mGlobalRegistry.begin(), mGlobalRegistry.end(), [&](const std::shared_ptr<Pedestrian>& agent) {
                return agent->GetPosition().Distance(playerFocusPos) > mSimulationRadius * 2.5f;
            }),
            mGlobalRegistry.end()
        );

        // Generation Phase: Dynamically inject replacement nodes if target density limits lower
        if (mGlobalRegistry.size() < 12) {
            Vector3D randomizedOffset = { playerFocusPos.x + (rand() % 80 - 40), playerFocusPos.y, playerFocusPos.z + (rand() % 80 - 40) };
            int dice = rand() % 5;
            
            if (dice == 0) mGlobalRegistry.push_back(std::make_shared<WorkerCitizen>("CIV_AMBIENT_MALE", randomizedOffset));
            else if (dice == 1) mGlobalRegistry.push_back(std::make_shared<TouristCitizen>("CIV_AMBIENT_FEMALE", randomizedOffset));
            else if (dice == 2) mGlobalRegistry.push_back(std::make_shared<PatrolOfficer>(randomizedOffset));
            else if (dice == 3) mGlobalRegistry.push_back(std::make_shared<ThiefCriminal>(randomizedOffset));
            else mGlobalRegistry.push_back(std::make_shared<ElderlyCitizen>("CIV_AMBIENT_OLD", randomizedOffset));

            mGlobalRegistry.back()->Initialize();
        }

        // Ticking Evaluation Phase: Run logic over every active simulation component
        for (auto& pedestrianNode : mGlobalRegistry) {
            pedestrianNode->ProcessAISystems(mCurrentWorldTimeHour, deltaTime, mActiveThreatStream);
        }

        // Processing Pipeline Cleanup Phase
        mActiveThreatStream.clear();
    }

    const std::vector<std::shared_ptr<Pedestrian>>& InspectActivePoolMemory() const {
        return mGlobalRegistry;
    }

private:
    int mCurrentWorldTimeHour;
    float mSimulationRadius;
    std::vector<std::shared_ptr<Pedestrian>> mGlobalRegistry;
    std::vector<ThreatStimulus> mActiveThreatStream;
};