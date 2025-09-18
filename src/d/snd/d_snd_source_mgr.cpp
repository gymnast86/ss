#include "d/snd/d_snd_source_mgr.h"

#include "common.h"
#include "d/a/d_a_base.h"
#include "d/snd/d_snd_bgm_mgr.h"
#include "d/snd/d_snd_control_player_mgr.h"
#include "d/snd/d_snd_id_mappers_data.h"
#include "d/snd/d_snd_player_mgr.h"
#include "d/snd/d_snd_small_effect_mgr.h"
#include "d/snd/d_snd_source.h"

// clang-format off
// vtable order - vtables are right but need to split up
// some headers for weak function order
#include "d/snd/d_snd_source_e_spark.h"
#include "d/snd/d_snd_source_enums.h"
#include "d/snd/d_snd_source_equipment.h"
#include "d/snd/d_snd_source_group.h"
#include "d/snd/d_snd_source_py_bird.h"
#include "d/snd/d_snd_source_enemy.h"
#include "d/snd/d_snd_source_obj_clef.h"
#include "d/snd/d_snd_source_obj.h"
#include "d/snd/d_snd_source_npc.h"
#include "d/snd/d_snd_source_harp_related.h"

#include "d/snd/d_snd_source_demo.h"
#include "d/snd/d_snd_source_player.h"
#include "d/snd/d_snd_source_player_head.h"
#include "d/snd/d_snd_source_npc_head.h"
#include "d/snd/d_snd_source_npc_special.h"
#include "d/snd/d_snd_source_tg_sound.h"
// clang-format on

#include "d/snd/d_snd_state_mgr.h"
#include "d/snd/d_snd_util.h"
#include "d/snd/d_snd_wzsound.h"
#include "nw4r/ut/ut_list.h"
#include "sized_string.h"

#include <cmath>

// TODO - weak function order in this file is a problem.
// one particular problem is that all weak functions involving
// dSndAnimSound_c are reversed compared to their natural vtable order,
// and the overridden `SetupSound` function seems to be immune to reordering

bool dSndSourceMgr_c::isAnimSoundSource(s32 sourceType, const char *name) {
    switch (getSourceCategoryForSourceType(sourceType, name)) {
        case SND_SOURCE_CATEGORY_PLAYER:
            switch (sourceType) {
                case SND_SOURCE_PLAYER: return true;
            }
            break;
        case SND_SOURCE_CATEGORY_ENEMY:
            if (sourceType < 0x19) {
                return sourceType != SND_SOURCE_ENEMY_14;
            }
            return false;
        case SND_SOURCE_CATEGORY_NPC:
            return sourceType != SND_SOURCE_NPC_47 && sourceType != SND_SOURCE_NPC_HEAD &&
                   sourceType != SND_SOURCE_INSECT;
        case SND_SOURCE_CATEGORY_OBJECT:
            if (sourceType >= SND_SOURCE_OBJECT_42) {
                return true;
            }
            break;
        case 58:
            // TODO - what is category 58???
            return true;
    }

    return false;
}

bool dSndSourceMgr_c::isMultiSoundSource(s32 sourceType, const char *name) {
    switch (sourceType) {
        case SND_SOURCE_BIGBOSS:
        case SND_SOURCE_BOSS_MG:
        case SND_SOURCE_BOSS_KR:
        case SND_SOURCE_BOSS_NUSI:
        case SND_SOURCE_NPC_NUSI:  return true;
    }
    return false;
}

bool dSndSourceMgr_c::isSwOrEOc(const char *name) {
    if (streq(name, "Sw") || streq(name, "SwWall") || streq(name, "wnut") || streq(name, "EOc")) {
        return true;
    }
    return false;
}

s32 dSndSourceMgr_c::getSourceCategoryForSourceType(s32 sourceType, const char *name) {
    // This might be a full-on switch statement but I don't want to write out
    // all the unknown entries yet and this matches anyway

    if (sourceType >= SND_SOURCE_PLAYER && sourceType <= SND_SOURCE_PLAYER_HEAD) {
        return SND_SOURCE_CATEGORY_PLAYER;
    }

    if (sourceType >= SND_SOURCE_NET && sourceType <= SND_SOURCE_HOOKSHOT) {
        return SND_SOURCE_CATEGORY_EQUIPMENT;
    }

    if (sourceType >= SND_SOURCE_ENEMY_10 && sourceType <= SND_SOURCE_ENEMY_31) {
        return SND_SOURCE_CATEGORY_ENEMY;
    }

    if (sourceType >= SND_SOURCE_OBJECT && sourceType <= SND_SOURCE_OBJECT_42) {
        return SND_SOURCE_CATEGORY_OBJECT;
    }

    if (sourceType >= SND_SOURCE_NPC_43 && sourceType <= SND_SOURCE_NPC_DRAGON) {
        return SND_SOURCE_CATEGORY_NPC;
    }

    if (sourceType == SND_SOURCE_TG_SOUND) {
        return SND_SOURCE_CATEGORY_TG_SOUND;
    }

    if (sourceType >= SND_SOURCE_54 && sourceType <= SND_SOURCE_TG_HARP) {
        return SND_SOURCE_CATEGORY_HARP_RELATED;
    }

    switch (sourceType) {
        case SND_SOURCE_58: return SND_SOURCE_CATEGORY_7;
        case SND_SOURCE_59: return SND_SOURCE_CATEGORY_9;
        default:            return -1;
    }
}

dSoundSourceIf_c *dSndSourceMgr_c::createSource(s32 sourceType, dAcBase_c *actor, const char *name, u8 _subtype) {
    if (actor == nullptr) {
        return nullptr;
    }

    s32 subtype = actor->mSubtype;
    SizedString<64> nameStr;
    nameStr.sprintf("%s", name);

    bool isModified = false;

    if (dSndStateMgr_c::GetInstance()->isInDemo() != nullptr && strneq(name, "$act", 4)) {
        nameStr.sprintf("%s_%s", name + 1, dSndStateMgr_c::GetInstance()->getCurrentStageMusicDemoName());
        isModified = true;
    }

    dSoundSource_c *existingSource = static_cast<dSoundSource_c *>(actor->getSoundSource());
    if (!isModified) {
        bool allowSubtype = true;
        switch (sourceType) {
            case SND_SOURCE_BIGBOSS:
                nameStr = "BBigBos";
                isModified = true;
                break;
            case SND_SOURCE_GIRAHUMU_3:
                nameStr = "BGh3";
                isModified = true;
                break;
            case SND_SOURCE_NPC_HEAD:
                if (existingSource != nullptr) {
                    name = existingSource->getName();
                    nameStr.sprintf("%sHead", name);
                    allowSubtype = false;
                }
                break;
        }

        if (strneq(name, "NpcMole", 7)) {
            if (subtype != 0) {
                nameStr = "NpcMoT";
                subtype = 0;
                isModified = true;
            }
        } else if (strneq(name, "NpcMoT2", 7)) {
            nameStr = "NpcMoT";
            isModified = true;
        } else if (strneq(name, "NpcMoN2", 7)) {
            nameStr = "NpcMoN";
            isModified = true;
        } else if (strneq(name, "NpcMoEN", 7)) {
            nameStr = "NpcMole";
            subtype = 0;
            isModified = true;
        } else if (strneq(name, "NpcMoS", 6)) {
            nameStr = "NpcMoEl";
            isModified = true;
        }

        if (allowSubtype && subtype != 0) {
            nameStr.sprintf("%s_A%d", &nameStr, subtype);
            isModified = true;
        }

        if (sourceType == SND_SOURCE_NPC_51) {
            const ActorBaseNamePair *pair = sActorBaseNamePairs;
            for (int i = 0; i < sNumActorBaseNamePairs; i++) {
                if (streq(nameStr, sActorBaseNamePairs[i].variant)) {
                    nameStr = sActorBaseNamePairs[i].base;
                    isModified = true;
                    break;
                }
            }
        }
    }

    const char *actualName = nameStr;
    s32 category = getSourceCategoryForSourceType(sourceType, actualName);
    dSndSourceGroup_c *group = nullptr;

    if (category != SND_SOURCE_CATEGORY_9) {
        if (isModified) {
            group = GetInstance()->getGroup(sourceType, actor, actualName, name, subtype);
        } else {
            group = GetInstance()->getGroup(sourceType, actor, actualName, nullptr, subtype);
        }
        actualName = group->getName();
    }

    s32 sourceCategory = getSourceCategoryForSourceType(sourceType, actualName);
    dSoundSourceIf_c *newSource = nullptr;
    bool isAnimSource = isAnimSoundSource(sourceType, actualName);
    bool isMultiSource = isMultiSoundSource(sourceType, actualName);
    bool isDemo = dSndPlayerMgr_c::GetInstance()->shouldUseDemoPlayer(sourceType);

    if (isDemo) {
        newSource = new dSndSourceDemo_c(sourceType, actor, actualName, group);
    }

    if (newSource == nullptr) {
        switch (sourceCategory) {
            case SND_SOURCE_CATEGORY_PLAYER:
                if (sourceType == SND_SOURCE_PLAYER_HEAD) {
                    newSource = new dSndSourcePlayerHead_c(sourceType, actor, actualName, group);
                } else {
                    newSource = new dSndSourcePlayer_c(sourceType, actor, actualName, group);
                }
                break;
            case SND_SOURCE_CATEGORY_ENEMY:
                if (sourceType == SND_SOURCE_SPARK) {
                    newSource = new dSndSourceESpark_c(sourceType, actor, actualName, group);
                } else if (isAnimSource) {
                    if (isMultiSource) {
                        newSource = new dSndSourceEnemyMulti_c(sourceType, actor, actualName, group);
                    } else {
                        newSource = new dSndSourceEnemyAnim_c(sourceType, actor, actualName, group);
                    }
                } else {
                    newSource = new dSndSourceEnemy_c(sourceType, actor, actualName, group);
                }
                break;
            case SND_SOURCE_CATEGORY_OBJECT:
                if (sourceType == SND_SOURCE_OBJECT_33) {
                    return nullptr;
                }
                if (sourceType == SND_SOURCE_LIGHT_SHAFT) {
                    newSource = new dSndSourceObjLightShaft_c(sourceType, actor, actualName, group);
                } else if (sourceType == SND_SOURCE_CLEF) {
                    newSource = new dSndSourceObjClef_c(sourceType, actor, actualName, group);
                } else if (isAnimSource) {
                    newSource = new dSndSourceObjAnim_c(sourceType, actor, actualName, group);
                } else {
                    newSource = new dSndSourceObj_c(sourceType, actor, actualName, group);
                }
                break;
            case SND_SOURCE_CATEGORY_EQUIPMENT:
                if (sourceType == SND_SOURCE_WHIP) {
                    newSource = new dSndSourceEquipmentWhip_c(sourceType, actor, actualName, group);
                } else {
                    newSource = new dSndSourceEquipment_c(sourceType, actor, actualName, group);
                }
                break;
            case SND_SOURCE_CATEGORY_NPC:
                if (sourceType == SND_SOURCE_NPC_HEAD) {
                    newSource = new dSndSourceNpcHead_c(sourceType, actor, actualName, group);
                } else if (sourceType == SND_SOURCE_NPC_DRAGON) {
                    newSource = new dSndSourceNpcDr_c(sourceType, actor, actualName, group);
                } else if (sourceType >= SND_SOURCE_NPC_50) {
                    newSource = new dSndSourceNpcSpecial_c(sourceType, actor, actualName, group);
                } else if (sourceType == SND_SOURCE_PLAYER_BIRD) {
                    newSource = new dSndSourcePyBird_c(sourceType, actor, actualName, group);
                } else if (isAnimSource) {
                    newSource = new dSndSourceNpcAnim_c(sourceType, actor, actualName, group);
                } else {
                    newSource = new dSndSourceNpc_c(sourceType, actor, actualName, group);
                }
                break;
            case SND_SOURCE_CATEGORY_TG_SOUND:
                if (dSndStateMgr_c::GetInstance()->isActiveDemoMaybe(subtype)) {
                    return nullptr;
                }
                newSource = new dSndSourceTgSound_c(sourceType, actor, actualName, group);
                break;
            case SND_SOURCE_CATEGORY_HARP_RELATED:
                switch (sourceType) {
                    case SND_SOURCE_OBJECT_WARP:
                        newSource = new dSndSourceHarpObjWarp_c(sourceType, actor, actualName, group);
                        break;
                    case SND_SOURCE_SW_HARP:
                        if (subtype == 4) {
                            newSource = new dSndSourceHarpSwHarp4_c(sourceType, actor, actualName, group);
                        } else {
                            newSource = new dSndSourceHarpSwHarp_c(sourceType, actor, actualName, group);
                        }
                        break;
                    case SND_SOURCE_TG_HARP:
                        newSource = new dSndSourceHarpTg_c(sourceType, actor, actualName, group);
                        break;
                    default: newSource = new dSndSourceHarpRelated_c(sourceType, actor, actualName, group); break;
                }
                break;
            case SND_SOURCE_CATEGORY_7: newSource = new dSndSourceDemo_c(sourceType, actor, actualName, group); break;
            default:
                if (sourceType < SND_SOURCE_59 + 1) {
                    // This part is confusing as heck. Various "tags" have category 9,
                    // but most of them never create their sound source (except for Uground).
                    // On top of that, this called function does weird things where
                    // if no group with the given name is in the "to load list",
                    // it grabs the first group from the "inactive" list but
                    // *doesn't remove it from that list*, then temporarily sets some
                    // variables, adds it to the "to load" list, then clears some other variables.
                    // Maybe there's a very specific reason for it to do things
                    // that way but the effect is that there's a group
                    // in a weird in-between state (inactive but loading)
                    // and I'm really not sure if it has the intended effect.
                    // Then again, I think this is only called from Uground...
                    GetInstance()->fn_803846D0(sourceType, actualName, subtype);
                }
                return nullptr;
        }
    }

    if (newSource == nullptr) {
        return nullptr;
    }

    // setSubtype not emitted by explicit call,
    // so it apparently happens through dSoundSourceIf_c
    newSource->setSubtype(subtype);
    newSource->setup();
    if (!streq(name, actualName)) {
        static_cast<dSoundSource_c *>(newSource)->setOrigName(name);
    }

    if (sourceType == SND_SOURCE_NPC_HEAD) {
        if (existingSource != nullptr && strneq(existingSource->getName(), "NpcMoT", 6)) {
            static_cast<dSndSourceNpcHead_c *>(newSource)->setMainName(existingSource->getName());
        } else if (existingSource != nullptr && streq(name, "NpcGrd")) {
            static_cast<dSndSourceNpcHead_c *>(newSource)->setMainName("NpcGra");
        }
    }

    if (existingSource != nullptr && existingSource != newSource && sourceType != SND_SOURCE_NPC_HEAD &&
        existingSource->isMultiSource()) {
        existingSource->registerAdditionalSource(static_cast<dSoundSource_c *>(newSource));
    }

    return newSource;
}

SND_DISPOSER_DEFINE(dSndSourceMgr_c);

dSndSourceMgr_c::dSndSourceMgr_c()
    : field_0x0010(0),
      field_0x0011(0),
      field_0x0012(0),
      field_0x0013(0),
      field_0x3860(0),
      field_0x3864(0),
      field_0x3868(0),
      field_0x386C(INFINITY),
      mpPlayerSource(nullptr),
      mpKenseiSource(nullptr),
      mpBoomerangSource(nullptr),
      mpTBoatSource(nullptr),
      field_0x3880(nullptr),
      mpMsgSource(nullptr) {
    // TODO offsetof
    nw4r::ut::List_Init(&mGroup1List, 0);
    nw4r::ut::List_Init(&mGroup2List, 0);

    // TODO figure out what these are for
    nw4r::ut::List_Init(&mGroup3List, 8);
    nw4r::ut::List_Init(&mAllSourcesList, 0xE8);
    nw4r::ut::List_Init(&field_0x3848, 0x15C);
    nw4r::ut::List_Init(&mHarpRelatedList, 0x160);

    mpDefaultGroup = new dSndSourceGroup_c(-1, "Default", nullptr, 0);

    for (dSndSourceGroup_c *group = &mGroups[0]; group < &mGroups[NUM_GROUPS]; group++) {
        nw4r::ut::List_Append(&mGroup2List, group);
    }
}

void dSndSourceMgr_c::calcEnemyObjVolume() {
    if (dSndStateMgr_c::GetInstance()->checkEventFlag(dSndStateMgr_c::EVENT_MUTE_ENEMY_FULL)) {
        dSndControlPlayerMgr_c::GetInstance()->setEnemyMuteVolume(0.0f);
    } else if (dSndStateMgr_c::GetInstance()->checkEventFlag(dSndStateMgr_c::EVENT_MUTE_ENEMY_PARTIAL)) {
        dSndControlPlayerMgr_c::GetInstance()->setEnemyMuteVolume(0.3f);
    }

    if (dSndStateMgr_c::GetInstance()->checkEventFlag(dSndStateMgr_c::EVENT_MUTE_OBJ_FULL)) {
        dSndControlPlayerMgr_c::GetInstance()->setObjectMuteVolume(0.0f);
    } else if (dSndStateMgr_c::GetInstance()->checkEventFlag(dSndStateMgr_c::EVENT_MUTE_OBJ_PARTIAL)) {
        dSndControlPlayerMgr_c::GetInstance()->setObjectMuteVolume(0.3f);
    }
}

void dSndSourceMgr_c::setMutedFromFader(bool muteFlag) {
    for (dSoundSource_c *it = getAllSourcesFirst(); it != nullptr; it = getAllSourcesNext(it)) {
        s32 sourceType = it->getSourceType();
        switch (sourceType) {
            case SND_SOURCE_PLAYER:
            case SND_SOURCE_PLAYER_HEAD: break;
            default:                     it->setField0x101(muteFlag); break;
        }
    }
    if (muteFlag) {
        dSndControlPlayerMgr_c::GetInstance()->muteScenePlayers(30);
    } else {
        dSndControlPlayerMgr_c::GetInstance()->unmuteScenePlayers(30);
    }
}

dSndSourceGroup_c *
dSndSourceMgr_c::getGroup(s32 sourceType, dAcBase_c *actor, const char *name, const char *origName, u8 subtype) {
    dSndSourceGroup_c *group = getActiveGroupForName(name);
    if (group != nullptr) {
        return group;
    }

    group = getInactiveGroup();
    if (group != nullptr) {
        group->set(sourceType, name, origName, subtype);
        activateGroup(group);
        addGroupToLoading(group);
        dSndBgmMgr_c::GetInstance()->prepareBossBgm(name);
        return group;
    }

    return nullptr;
}

dSndSourceGroup_c *dSndSourceMgr_c::getActiveGroupForName(const char *name) {
    for (dSndSourceGroup_c *it = getGroup1First(); it != nullptr; it = getGroup1Next(it)) {
        if (streq(it->getName(), name)) {
            return it;
        }
    }
    return nullptr;
}

dSndSourceGroup_c *dSndSourceMgr_c::getInactiveGroup() {
    return getGroup2First();
}

bool dSndSourceMgr_c::addGroupToLoading(dSndSourceGroup_c *source) {
    return false;
}

void dSndSourceMgr_c::registerSource(dSoundSource_c *source) {
    if (source != nullptr) {
        nw4r::ut::List_Append(&mAllSourcesList, source);
        switch (source->getCategory()) {
            case SND_SOURCE_CATEGORY_PLAYER: {
                if (source->getSourceType() == SND_SOURCE_PLAYER && mpPlayerSource == nullptr) {
                    mpPlayerSource = source;
                }
                break;
            }
            case SND_SOURCE_CATEGORY_EQUIPMENT: {
                if (source->getSourceType() == SND_SOURCE_BOOMERANG) {
                    mpBoomerangSource = source;
                }
                break;
            }
            case SND_SOURCE_CATEGORY_ENEMY: {
                if (isCertainEnemyType(source)) {
                    nw4r::ut::List_Append(&field_0x3848, source);
                }
                break;
            }
            case SND_SOURCE_CATEGORY_HARP_RELATED: {
                nw4r::ut::List_Append(&mHarpRelatedList, source);
                break;
            }
            case SND_SOURCE_CATEGORY_OBJECT: {
                if (streq(source->getName(), "TBoat") && mpTBoatSource == nullptr) {
                    mpTBoatSource = source;
                }
                break;
            }
            case SND_SOURCE_CATEGORY_NPC: {
                if (source->getSourceType() == SND_SOURCE_KENSEI) {
                    mpKenseiSource = source;
                }
                break;
            }
        }
    }
}

void dSndSourceMgr_c::unregisterSource(dSoundSource_c *source) {
    if (source != nullptr) {
        removeSourceFromList(source, &mAllSourcesList);
        removeSourceFromList(source, &field_0x3848);
        removeSourceFromList(source, &mHarpRelatedList);
        if (source == mpPlayerSource) {
            mpPlayerSource = nullptr;
        } else if (source == mpKenseiSource) {
            mpKenseiSource = nullptr;
        } else if (source == mpBoomerangSource) {
            mpBoomerangSource = nullptr;
        }

        if (mpTBoatSource == source) {
            mpTBoatSource = nullptr;
        }
    }
}

void dSndSourceMgr_c::removeSourceFromList(dSoundSource_c *source, nw4r::ut::List *list) {
    if (source != nullptr && list != nullptr) {
        // This removal code appears to be needlessly defensive
        dSoundSource_c *sourceIter = static_cast<dSoundSource_c *>(nw4r::ut::List_GetFirst(list));
        while (sourceIter != nullptr) {
            if (field_0x3880 == source) {
                // why in here????
                field_0x3880 = nullptr;
            }

            if (sourceIter == source) {
                nw4r::ut::List_Remove(list, sourceIter);
                sourceIter = nullptr;
            } else {
                sourceIter = static_cast<dSoundSource_c *>(nw4r::ut::List_GetNext(list, sourceIter));
            }
        }
    }
}

void dSndSourceMgr_c::onShutdownSource(dSoundSource_c *source) {
    if (source == nullptr) {
        return;
    }
    removeSourceFromList(source, &field_0x3848);
}

void dSndSourceMgr_c::clearSourceLists() {
    clearSourceList(&mAllSourcesList);
    clearSourceList(&field_0x3848);
    clearSourceList(&mHarpRelatedList);
    mpPlayerSource = nullptr;
}

void dSndSourceMgr_c::activateGroup(dSndSourceGroup_c *group) {
    if (!isActiveGroup(group)) {
        removeGroup2(group);
        appendGroup1(group);
        group->setIsActive(true);
    }
}

bool dSndSourceMgr_c::isActiveGroup(dSndSourceGroup_c *group) const {
    if (group != nullptr) {
        return group->isActive();
    }
    return false;
}

void dSndSourceMgr_c::clearSourceList(nw4r::ut::List *list) {
    if (list != nullptr) {
        dSoundSource_c *sourceIter = static_cast<dSoundSource_c *>(nw4r::ut::List_GetFirst(list));
        while (sourceIter != nullptr) {
            nw4r::ut::List_Remove(list, sourceIter);
            sourceIter = static_cast<dSoundSource_c *>(nw4r::ut::List_GetFirst(list));
        }
    }
}

void dSndSourceMgr_c::onEventStart() {
    // no-op
}

void dSndSourceMgr_c::onEventEnd() {
    // no-op
}

s32 dSndSourceMgr_c::getPlayerSourceRoomId() const {
    if (mpPlayerSource == nullptr) {
        return -1;
    }
    if (getBoomerangSource() != nullptr) {
        return getBoomerangSource()->getRoomId();
    }
    return mpPlayerSource->getRoomId();
}

struct FlowSoundDef {
    const char *groupName;
    u32 soundId;
};

extern "C" const char sLinkHead[] = "LinkHead";

static const FlowSoundDef sFlowSoundDefs[] = {
    {   "Door_A7",      SE_DoorB00_OPEN_SHORT},
    {"TgSound_A4", SE_TgSound_A4_TOILET_WATER},
    {     nullptr,         SE_Door_W_KEY_OPEN},
    {     nullptr,     SE_NpcDskN_BATH_SPLASH},
    {   "NpcSenb",     SE_NpcSenb_OPEN_LETTER},
    {"TgSound_A4",          SE_NV_001_NpcGost},
    {"TgSound_A4",          SE_NV_003_NpcGost},
    {     nullptr,       SE_EVENT_GIRL_SCREAM},
    {     nullptr,          SE_NpcPma_COOKING},
    {     nullptr,            SE_N_GET_LETTER},
    {     nullptr,           SE_NV_021_NpcOim},
    {     nullptr,           SE_NV_020_NpcOim},
    {     nullptr,       SE_F000_L3_RACE_CALL},
    {     nullptr,      SE_F000_L3_RACE_START},
    {     nullptr, SE_S_TALK_CHAR_NpcSlrb_ONE},
    {     nullptr,            SE_LV_DRINK_MSG},
};

void dSndSourceMgr_c::playFlowSound(u32 id) {
    if (id < 100) {
        return;
    }

    id -= 100;
    // @bug should be >=
    if (id > ARRAY_LENGTH(sFlowSoundDefs)) {
        return;
    }

    if (sFlowSoundDefs[id].groupName != nullptr) {
        dSndSourceGroup_c *grp = getActiveGroupForName(sFlowSoundDefs[id].groupName);
        if (grp != nullptr) {
            dSoundSource_c *src = grp->getSourceClosestToListener();
            if (src != nullptr) {
                src->startSound(sFlowSoundDefs[id].soundId);
            }
        }
    } else {
        dSndSmallEffectMgr_c::GetInstance()->playSoundInternalChecked(sFlowSoundDefs[id].soundId, nullptr);
    }
}

struct dSndSourceMgrEmptySinit {
    dSndSourceMgrEmptySinit() {}
};

dSndSourceMgrEmptySinit emptySinit;
