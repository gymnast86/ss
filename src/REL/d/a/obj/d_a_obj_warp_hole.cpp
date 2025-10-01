#include "d/a/obj/d_a_obj_warp_hole.h"

#include "c/c_lib.h"
#include "d/a/d_a_player.h"
#include "d/a/npc/d_a_npc_talk_kensei.h"
#include "d/col/cc/d_cc_s.h"
#include "d/d_sc_game.h"
#include "d/snd/d_snd_wzsound.h"
#include "toBeSorted/event_manager.h"

SPECIAL_ACTOR_PROFILE(OBJ_WARP_HOLE, dAcOwarpHole_c, fProfile::OBJ_WARP_HOLE, 0x25C, 0, 0);

dCcD_SrcCyl dAcOwarpHole_c::sCylSrc = {
    /* mObjInf */
    {/* mObjAt */ {0, 0, {0, 0, 0}, 0, 0, 0, 0, 0, 0},
     /* mObjTg */
     {~(AT_TYPE_BUGNET | AT_TYPE_0x80000 | AT_TYPE_BEETLE | AT_TYPE_WIND | AT_TYPE_0x8000), 0x111, {0, 0, 0x407}, 0, 0},
     /* mObjCo */ {0x29}},
    /* mCylInf */
    {320.f, 300.f}
};

// Unused(?) 600.0f float at start of rodata
const float dAcOwarpHole_c::dummy600 = 600.0f;

bool dAcOwarpHole_c::createHeap() {
    return true;
}

int dAcOwarpHole_c::create() {
    mExitListIdx = mParams;
    CREATE_ALLOCATOR(dAcOwarpHole_c);
    mStts.SetRank(0xD);
    mCcCyl.Set(sCylSrc);
    mCcCyl.SetStts(mStts);
    dCcS::GetInstance()->Set(&mCcCyl);
    mCcCyl.OnCoSet();
    mCcCyl.ClrTgSet();
    mLinkPos = dAcPy_c::GetLink()->mPosition;
    mEff.init(this);
    mWalkFramesMaybe = 0;
    mPositionCopy2.set(mPosition.x, mPosition.y + 170.0f, mPosition.z);
    mPositionCopy3 = mPositionCopy2;

    return SUCCEEDED;
}

int dAcOwarpHole_c::doDelete() {
    return SUCCEEDED;
}

int dAcOwarpHole_c::actorExecute() {
    if (!dAcPy_c::GetLink()->checkObjectProperty(0x200) && getDistanceTo(dAcPy_c::GetLinkM()->mPosition) < 600.0f) {
        Event ev = Event("BeforeLastBossBattleTalk", 100, 0x100001, nullptr, nullptr);
        EventManager::alsoSetAsCurrentEvent(dAcNpcTalkKensei_c::GetInstance(), &ev, nullptr);
    }

    dCcS::GetInstance()->Set(&mCcCyl);
    updateMatrix();
    mEff.createContinuousEffect(PARTICLE_RESOURCE_ID_MAPPING_914_, mWorldMtx, nullptr, nullptr);
    holdSound(SE_WarpH_Wait);

    return SUCCEEDED;
}

int dAcOwarpHole_c::actorExecuteInEvent() {
    mEff.createContinuousEffect(PARTICLE_RESOURCE_ID_MAPPING_914_, mWorldMtx, nullptr, nullptr);
    holdSound(SE_WarpH_Wait);
    int retVal = NOT_READY;
    bool advance = mEvent.isAdvance();
    mPositionCopy2.set(mPosition.x, mPosition.y + 170.0f, mPosition.z);

    switch (mEvent.getCurrentEventCommand()) {
        case 'none':
            mEvent.advanceNext();
            retVal = SUCCEEDED;
            break;
        case 'plwk':
            if (advance) {
                float multiplier330 = 330.0f;
                mLinkPos = dAcPy_c::GetLinkM()->mPosition;
                mAng targetAngleY = (mAng)cLib::targetAngleY(mPosition, dAcPy_c::GetLinkM()->getPosition());
                mLinkPos.x += multiplier330 * targetAngleY.sin();
                mLinkPos.z += multiplier330 * targetAngleY.cos();
            }
            if (EventManager::isInEvent()) {
                if (EventManager::isCurrentEvent("BeforeLastBossBattleChicken")) {
                    dAcPy_c* player = dAcPy_c::GetLinkM();
                    player->vt_0x2AC();
                    player->triggerMoveEventMaybe(2, 0, 0, mLinkPos, 0, 0, 0);
                }
            }
            if (dAcPy_c::GetLinkM()->mPosition.absXZTo(mLinkPos) < 10.0f) {
                mEvent.advanceNext();
            }
            retVal = SUCCEEDED;
            break;
        case 'warp':
            if (advance) {
                dJEffManager_c::spawnEffect(PARTICLE_RESOURCE_ID_MAPPING_916_, mWorldMtx, nullptr, nullptr, 0, 0);
                mWalkFramesMaybe = 80;
            }
            if (mWalkFramesMaybe != 0) {
                mWalkFramesMaybe -= 1;
            }
            if (mWalkFramesMaybe == 77) {
                dAcPy_c::GetLinkM()->setObjectProperty(OBJ_PROP_0x200);
            }
            if (mWalkFramesMaybe == 20) {
                dScGame_c::GetInstance()->triggerExit(mRoomID, mExitListIdx);
            }
            if (mWalkFramesMaybe == 0) {
                mEvent.advanceNext();
            }
            retVal = SUCCEEDED;
    }

    return retVal;
}

int dAcOwarpHole_c::draw() {
    return SUCCEEDED;
}
