#include "d/d_stage_mgr.h"

#include "common.h"
#include "d/d_base.h"
#include "d/d_bzs_types.h"
#include "d/d_heap.h"
#include "d/d_last.h"
#include "d/d_sc_game.h"
#include "d/d_stage_parse.h"
#include "d/d_sys.h"
#include "d/flag/flag_managers.h"
#include "d/snd/d_snd_state_mgr.h"
#include "f/f_base.h"
#include "f/f_profile_name.h"
#include "m/m_dvd.h"
#include "toBeSorted/arc_managers/current_stage_arc_manager.h"
#include "toBeSorted/arc_managers/layout_arc_manager.h"
#include "toBeSorted/arc_managers/oarc_manager.h"
#include "toBeSorted/d_particle.h"

SPECIAL_BASE_PROFILE(STAGE_MANAGER, dStageMgr_c, fProfile::STAGE_MANAGER, 0X5, 1536);

STATE_DEFINE(dStageMgr_c, ReadStageRes);
STATE_DEFINE(dStageMgr_c, ReadRoomRes);
STATE_DEFINE(dStageMgr_c, ReadObjectRes);
STATE_DEFINE(dStageMgr_c, ReadStageLayerRes);
STATE_DEFINE(dStageMgr_c, ReadLayerObjectRes);
STATE_DEFINE(dStageMgr_c, SoundLoadSceneData);
STATE_DEFINE(dStageMgr_c, CreateObject);
STATE_DEFINE(dStageMgr_c, ReadObjectSound);
STATE_DEFINE(dStageMgr_c, SceneChangeSave);
STATE_DEFINE(dStageMgr_c, RestartSceneWait);
STATE_DEFINE(dStageMgr_c, RestartScene);

dStageMgr_c::dStageMgr_c() : mStateMgr(*this, sStateID::null), mPhase(this, sCallbacks) {
    sInstance = this;
}

dStageMgr_c::~dStageMgr_c() {
    sInstance = nullptr;
}

void dStageMgr_c::initializeState_ReadStageRes() {
    CurrentStageArcManager::GetInstance()->setStage(dScGame_c::currentSpawnInfo.stageName);
}

void dStageMgr_c::executeState_ReadStageRes() {
    if (CurrentStageArcManager::GetInstance()->ensureAllEntriesLoaded() == D_ARC_RESULT_OK) {
        mStateMgr.changeState(StateID_ReadRoomRes);
    }
}

void dStageMgr_c::finalizeState_ReadStageRes() {
    const void *stageBzs = CurrentStageArcManager::GetInstance()->getData("dat/stage.bzs");
    if (stageBzs != nullptr) {
        parseStageBzs(-1, stageBzs);
        parseRoomStageBzs(-1, stageBzs);
    }
    dSndStateMgr_c::GetInstance()->onStageOrLayerUpdate();
}

void dStageMgr_c::initializeState_ReadRoomRes() {}

void dStageMgr_c::executeState_ReadRoomRes() {
    if (CurrentStageArcManager::GetInstance()->ensureAllEntriesLoaded() == D_ARC_RESULT_OK &&
        LayoutArcManager::GetInstance()->ensureAllEntriesLoaded() == D_ARC_RESULT_OK) {
        mStateMgr.changeState(StateID_ReadObjectRes);
    }
}

void dStageMgr_c::finalizeState_ReadRoomRes() {
    if (mpRmpl != nullptr) {
        const RMPL *itRmpl = mpRmpl;
        for (int i = 0; i < mRmplCount; itRmpl++, i++) {
            const void *bzs = CurrentStageArcManager::GetInstance()->loadFromRoomArc(itRmpl->roomId, "dat/room.bzs");
            parseRoomStageBzs(itRmpl->roomId, bzs);
        }
    } else {
        u32 roomId = dScGame_c::currentSpawnInfo.room;
        const void *bzs =
            CurrentStageArcManager::GetInstance()->loadFromRoomArc(dScGame_c::currentSpawnInfo.room, "dat/room.bzs");
        parseRoomStageBzs(roomId, bzs);
    }
}

void dStageMgr_c::initializeState_ReadObjectRes() {
    mStageObjCtrl.doLoad();
}

void dStageMgr_c::executeState_ReadObjectRes() {
    if (mStageObjCtrl.isLoaded()) {
        mStateMgr.changeState(StateID_ReadStageLayerRes);
    }
}

void dStageMgr_c::finalizeState_ReadObjectRes() {}

void dStageMgr_c::initializeState_ReadStageLayerRes() {
    CurrentStageArcManager::GetInstance()->loadFileFromExtraLayerArc(dScGame_c::currentSpawnInfo.layer);
}

void dStageMgr_c::executeState_ReadStageLayerRes() {
    if (CurrentStageArcManager::GetInstance()->ensureAllEntriesLoaded() == D_ARC_RESULT_OK) {
        mStateMgr.changeState(StateID_ReadLayerObjectRes);
    }
}

void dStageMgr_c::finalizeState_ReadStageLayerRes() {}

const char demoName[] = "";
static const char *sSeekerStoneLayoutArcs[] = {
    "SeekerStone",
};

void dStageMgr_c::initializeState_ReadLayerObjectRes() {
    mDemoName = demoName;

    const void *bzs = CurrentStageArcManager::GetInstance()->getData("dat/stage.bzs");
    if (bzs != nullptr) {
        parseBzsStageRoom(-1, bzs);
    }

    if (mpRmpl != nullptr) {
        const RMPL *itRmpl = mpRmpl;
        for (int i = 0; i < mRmplCount; itRmpl++, i++) {
            const void *bzs = CurrentStageArcManager::GetInstance()->loadFromRoomArc(itRmpl->roomId, "dat/room.bzs");
            parseBzsStageRoom(itRmpl->roomId, bzs);
        }
    } else {
        u32 roomId = dScGame_c::currentSpawnInfo.room;
        const void *bzs =
            CurrentStageArcManager::GetInstance()->loadFromRoomArc(dScGame_c::currentSpawnInfo.room, "dat/room.bzs");
        parseBzsStageRoom(roomId, bzs);
    }

    if (dScGame_c::isStateLayerWithSeekerStoneHintMenu()) {
        mLayoutArcCtrl2.set(sSeekerStoneLayoutArcs, ARRAY_LENGTH(sSeekerStoneLayoutArcs));
        mLayoutArcCtrl2.load(dHeap::work2Heap.heap);
        addActorId(fProfile::LYT_SEEKER_STONE);
    }
    mLayerObjCtrl.doLoad();
}

void dStageMgr_c::executeState_ReadLayerObjectRes() {
    if (mLayerObjCtrl.isLoaded() && LayoutArcManager::GetInstance()->ensureAllEntriesLoaded() == D_ARC_RESULT_OK) {
        mStateMgr.changeState(StateID_SoundLoadSceneData);
    }
}

void dStageMgr_c::finalizeState_ReadLayerObjectRes() {
    if (mDemoName.len() != 0) {
        const char *name = mDemoName;
        void *jpc = OarcManager::GetInstance()->getSubEntryData(name, "dat/jparticle.jpc");
        if (jpc != nullptr) {
            void *jpn = OarcManager::GetInstance()->getData(name, "dat/jparticle.jpn");
            dParticle::mgr_c::GetInstance()->createResource(dHeap::work2Heap.heap, 1, jpc, jpn);
        }
    }
}

static void *soundCallback(void *arg) {
    dSndStateMgr_c::GetInstance()->loadStageSound();
    return reinterpret_cast<void *>(true);
}

void dStageMgr_c::initializeState_SoundLoadSceneData() {
    dSndStateMgr_c::GetInstance()->onStageLoad();
    mpDvdCallback = mDvd_callback_c::createOrDie(soundCallback, nullptr);
}

void dStageMgr_c::executeState_SoundLoadSceneData() {
    if (mpDvdCallback != nullptr && mpDvdCallback->isDone()) {
        mStateMgr.changeState(StateID_CreateObject);
    }
}

void dStageMgr_c::finalizeState_SoundLoadSceneData() {
    mpDvdCallback->do_delete();
    mpDvdCallback = nullptr;
}

void dStageMgr_c::initializeState_CreateObject() {
    // too much egg
}

void dStageMgr_c::executeState_CreateObject() {
    if (!checkChildProcessCreateState()) {
        mStateMgr.changeState(StateID_ReadObjectSound);
    }
}

void dStageMgr_c::finalizeState_CreateObject() {}

static void *soundCallback2(void *arg) {
    dSndStateMgr_c::GetInstance()->loadObjectSound();
    return reinterpret_cast<void *>(true);
}

void dStageMgr_c::initializeState_ReadObjectSound() {
    mpDvdCallback2 = mDvd_callback_c::createOrDie(soundCallback2, nullptr);
}

void dStageMgr_c::executeState_ReadObjectSound() {
    if (mpDvdCallback2 == nullptr) {
        return;
    }

    if (!mpDvdCallback2->isDone()) {
        return;
    }

    if (dScGame_c::sInstance != nullptr) {
        if (dScGame_c::GetInstance()->savePromptFlag() == true) {
            mStateMgr.changeState(StateID_SceneChangeSave);
        } else {
            mStateMgr.changeState(StateID_RestartSceneWait);
        }
        return;
    }

    mStateMgr.changeState(StateID_RestartSceneWait);
}

void dStageMgr_c::finalizeState_ReadObjectSound() {
    mpDvdCallback2->do_delete();
    // mpDvdCallback2 = nullptr;
    unsetProcControl(ROOT_DISABLE_EXECUTE);
    unsetProcControl(ROOT_DISABLE_DRAW);
    dLast_c::setExecuteCallback(lastExecuteCallback);
}

extern "C" void *LYT_SAVE_MGR;
extern "C" void fn_80285600(void *, int, int);
void dStageMgr_c::initializeState_SceneChangeSave() {
    dScGame_c::GetInstance()->setSavePromptFlag(false);
    if (LYT_SAVE_MGR != nullptr) {
        fn_80285600(LYT_SAVE_MGR, 3, 0);
    }
    dBase_c::s_NextExecuteControlFlags |= BASE_PROP_0x1;
    dSys_c::setFrameRate(2);
}

void dStageMgr_c::executeState_SceneChangeSave() {
    if (LYT_SAVE_MGR != nullptr) {
        // "isNotSaving???"
        if (((u8 *)LYT_SAVE_MGR)[0x119C] == true) {
            mStateMgr.changeState(StateID_RestartSceneWait);
        }
    } else {
        mStateMgr.changeState(StateID_RestartSceneWait);
    }
}

void dStageMgr_c::finalizeState_SceneChangeSave() {
    dBase_c::s_NextExecuteControlFlags &= ~BASE_PROP_0x1;
    dBase_c::s_DrawControlFlags &= ~BASE_PROP_0x1;
}

void dStageMgr_c::initializeState_RestartSceneWait() {
    dSys_c::setFrameRate(2);
}

void dStageMgr_c::executeState_RestartSceneWait() {
    if (mFader.isSettled()) {
        if (mFader.isStatus(mFaderBase_c::FADED_IN)) {
            if (field_0x88BC) {
                mStateMgr.changeState(StateID_RestartScene);
            }
        } else {
            mFader.fadeIn();
        }
        // TODO
        mFader.calc();
    }
}

void dStageMgr_c::finalizeState_RestartSceneWait() {}

void dStageMgr_c::initializeState_RestartScene() {
    triggerFade(dScGame_c::nextSpawnInfo.transitionType, dScGame_c::nextSpawnInfo.transitionFadeFrames);
    mFader.setFadeInType(dScGame_c::nextSpawnInfo.transitionType);
    dSndStateMgr_c::GetInstance()->onRestartScene(mFader.getFadeOutFrame());
}

void dStageMgr_c::executeState_RestartScene() {
    if (mFader.isStatus(mFaderBase_c::FADED_OUT)) {}
}

void dStageMgr_c::finalizeState_RestartScene() {
    commitAllFlagManagers();
    dSys_c::setFrameRate(1);
}
