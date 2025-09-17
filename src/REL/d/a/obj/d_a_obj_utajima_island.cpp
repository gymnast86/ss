#include "d/a/obj/d_a_obj_utajima_island.h"

#include "common.h"
#include "d/a/obj/d_a_obj_base.h"
#include "d/a/obj/d_a_obj_utajima_main_mecha.h"
#include "d/col/bg/d_bg_s.h"
#include "d/d_stage.h"
#include "d/snd/d_snd_wzsound.h"
#include "f/f_base.h"
#include "f/f_manager.h"
#include "f/f_profile_name.h"
#include "m/m_angle.h"
#include "nw4r/g3d/res/g3d_resmdl.h"
#include "s/s_Math.h"

SPECIAL_ACTOR_PROFILE(OBJ_UTAJIMA_ISLAND, dAcOutajimaIsland_c, fProfile::OBJ_UTAJIMA_ISLAND, 0x1D2, 0, 6);

bool dAcOutajimaIsland_c::sPlatformsSettled;

STATE_DEFINE(dAcOutajimaIsland_c, Wait);

// ?????
const f32 dAcOutajimaIsland_c::floats[] = {
    2500.0f, 1400.0f, 100000.0f, 3.67342e-39f, 9.0072003E15f,
};

bool dAcOutajimaIsland_c::createHeap() {
    mRes = nw4r::g3d::ResFile(getOarcResFile("IslPuzIslet00"));
    dStage_c::bindStageResToFile(&mRes);
    dStage_c::bindSkyCmnToResFile(&mRes);
    nw4r::g3d::ResMdl m = mRes.GetResMdl("IslPuzIslet00");

    TRY_CREATE(mMdl.create(m, &mAllocator, 0x120));

    void *dzb = getOarcFile("IslPuzIslet00", "dzb/IslPuzIslet00.dzb");
    void *plc = getOarcFile("IslPuzIslet00", "dat/IslPuzIslet00.plc");
    updateMatrix();
    TRY_CREATE(!mBgW.Set((cBgD_t *)dzb, (PLC *)plc, cBgW::MOVE_BG_e, &mWorldMtx, &mScale));

    return true;
}

int dAcOutajimaIsland_c::actorCreate() {
    mInitialPos = mPosition;
    mInitialRot = mRotation;

    mRingLayer = getRingLayer();
    mParam2 = getParm2();
    s32 v = getParm2() * 30;
    field_0x5BC = mAng::fromDeg(v);
    field_0x5BE.x = field_0x5BC;
    field_0x5BE.y = field_0x5BE.z = mAng(0x88);

    movePlatforms();
    CREATE_ALLOCATOR(dAcOutajimaIsland_c);
    mBgW.SetCrrFunc(dBgS_MoveBGProc_Typical);
    dBgS::GetInstance()->Regist(&mBgW, this);
    mStateMgr.changeState(StateID_Wait);
    mMdl.setPriorityDraw(0x1C, 9);

    mVec3_c min;
    mVec3_c max;
    mMdl.getBounds(&min, &max);
    boundingBox.Set(min, max);
    mCullingDistance = 500000.0f;
    return SUCCEEDED;
}

int dAcOutajimaIsland_c::actorPostCreate() {
    fBase_c *p = nullptr;
    do {
        p = fManager_c::searchBaseByProfName(fProfile::OBJ_UTAJIMA_MAIN_MECHA, p);
        if (p != nullptr) {
            mMainMechaRef.link(static_cast<dAcOutajimaMainMecha_c *>(p));
        }
    } while (p != nullptr);

    return SUCCEEDED;
}

int dAcOutajimaIsland_c::doDelete() {
    return SUCCEEDED;
}

int dAcOutajimaIsland_c::actorExecute() {
    mStateMgr.executeState();
    updateMatrix();
    mMdl.setLocalMtx(mWorldMtx);
    mBgW.Move();
    return SUCCEEDED;
}

int dAcOutajimaIsland_c::draw() {
    drawModelType1(&mMdl);
    return SUCCEEDED;
}

void dAcOutajimaIsland_c::initializeState_Wait() {}
void dAcOutajimaIsland_c::executeState_Wait() {
    movePlatforms();
    if (mIsMoving) {
        mOldPosition.y += 1.0f;
        mPlaySound = true;
        sPlatformsSettled = false;
    }

    if (mPlaySound) {
        if (mRingLayer == 1) {
            holdSound(SE_UtaLand_MoveA);
        } else if (mRingLayer == 2) {
            holdSound(SE_UtaLand_MoveB);
        } else if (mRingLayer == 3) {
            holdSound(SE_UtaLand_MoveC);
        }

        if (mOldPosition == mPosition) {
            mPlaySound = false;
            if (mParam2 == 0 && !sPlatformsSettled) {
                startSound(SE_UtaLand_StopOK);
                sPlatformsSettled = true;
            }
        }
    }
}
void dAcOutajimaIsland_c::finalizeState_Wait() {}

void dAcOutajimaIsland_c::movePlatforms() {
    // What is going on here?
    s32 diff = field_0x5BC - field_0x5BE.x;
    if (mAng(diff) > mAng::fromDeg(90.0f)) {
        sLib::chase(&field_0x5BE.y.mVal, 0x7FFF, 1);
    } else {
        sLib::chase(&field_0x5BE.y.mVal, field_0x5BE.z, 1);
    }

    sLib::chaseAngle2(&field_0x5BE.x.mVal, field_0x5BC, field_0x5BE.y);

    mRotation.y = mInitialRot.y - field_0x5BE.x;
    mPosition.z = mInitialPos.z + ((mRingLayer - 1) * 1400.0f + 2500.0f) * mRotation.y.cos();
    mPosition.x = mInitialPos.x + ((mRingLayer - 1) * 1400.0f + 2500.0f) * mRotation.y.sin();
}
