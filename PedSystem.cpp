#include "PedSystem.h"

// ============================================================================
// CORE SUBSYSTEM IMPLEMENTATIONS
// ============================================================================

void CombatModule::ProcessAttack(Pedestrian* attacker, Pedestrian* target, float damage, bool ranged) {
    if (!attacker || !target) return;
    std::string mode = ranged ? "Ranged Bullet Vector" : "Melee Impact Force";
    target->ReceivePhysicalDamage(damage, attacker, mode);
}

// ============================================================================
// BASE PEDESTRIAN MEMBER FUNCTIONS
// ============================================================================

void Pedestrian::EvaluatePerceptionGrid(const std::vector<ThreatStimulus>& threats) {
    for (const auto& threat : threats) {
        float distance = mPosition.Distance(threat.position);
        if (distance > 50.0f) continue; // Outside sensory tracking envelope

        mIK.SetLookTarget(threat.position);

        // Reactive sorting tree
        if (threat.type == StimulusType::GunfireHeard || threat.type == StimulusType::ExplosionWitnessed) {
            if (mCourage < 40.0f) {
                mDestination = GenerateEscapePoint(threat.position);
                mState = PedState::FleeingPanic;
                mIsSprinting = true;
                mAudio.TriggerVoiceLine(mModelName, "SPEECH_SCREAM_PANIC");
            } else if (mType == PedType::CopPatrol || mType == PedType::CopSwat) {
                mState = PedState::SeekingCover;
                mDestination = threat.position; 
                mIsSprinting = true;
            }
        } else if (threat.type == StimulusType::FireDetected) {
            mDestination = GenerateEscapePoint(threat.position);
            mState = PedState::FleeingPanic;
            mIsSprinting = true;
        } else if (threat.type == StimulusType::DeadBodyFound && mType == PedType::CivilianWorker) {
            mDestination = mHomeLocation;
            mState = PedState::FleeingPanic;
            mIsSprinting = true;
            mAudio.TriggerVoiceLine(mModelName, "SPEECH_CALL_POLICE");
        }
    }
}

void Pedestrian::EvaluateScheduleChronology(int hour) {
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

void Pedestrian::EvaluateTacticalDecisions(float dt) {
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
            mDestination = mTarget->GetPosition(); 
        }
    }
}

void Pedestrian::ExecutePhysicalMovement(float dt) {
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

Vector3D Pedestrian::GenerateEscapePoint(Vector3D threat) {
    Vector3D diff = { mPosition.x - threat.x, mPosition.y - threat.y, mPosition.z - threat.z };
    diff = diff.Normalize();
    return { mPosition.x + diff.x * 45.0f, mPosition.y, mPosition.z + diff.z * 45.0f };
}

// ============================================================================
// GLOBAL POPULATION MANAGER SYSTEM IMPLEMENTATION
// ============================================================================

void ComprehensivePopulationSystem::RunSimulationFrameTick(Vector3D playerFocusPos, float deltaTime) {
    // 1. Memory Cleanup Culling Step
    mGlobalRegistry.erase(
        std::remove_if(mGlobalRegistry.begin(), mGlobalRegistry.end(), [&](const std::shared_ptr<Pedestrian>& agent) {
            return agent->GetPosition().Distance(playerFocusPos) > mSimulationRadius * 2.5f;
        }),
        mGlobalRegistry.end()
    );

    // 2. Continuous Ambient Density Injection Step
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

    // 3. Thread Safe Sequential Task Execution Sweep
    for (auto& pedestrianNode : mGlobalRegistry) {
        pedestrianNode->ProcessAISystems(mCurrentWorldTimeHour, deltaTime, mActiveThreatStream);
    }

    // 4. Reset Immediate Stimulus Array
    mActiveThreatStream.clear();
}